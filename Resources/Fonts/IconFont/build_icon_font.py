#!/usr/bin/env python3

# Rebuilds Resources/Fonts/IconFont.ttf, and the Source/Utility/Icons.h that names its
# glyphs, from Resources/Fonts/IconFont/icons.json.
#
# Each glyph has one artwork source: an SVG drawn for plugdata in custom/, or an icon from
# Microsoft's Fluent System Icons, which is not vendored - the build downloads the
# @fluentui/svg-icons npm package and reads it from there, so those track upstream.
#
# Every glyph is placed by the same rule: its square SVG canvas is scaled onto the em, and
# it is one em wide. Nothing is positioned or sized per glyph, so an icon's size on screen
# is decided by its artwork, and the Fluent icons all come from the one _24_ design grid.
#
# Codepoints exist only in the font and in the generated header. They are handed out in
# manifest order from firstCodepoint, in the Private Use Area, so no glyph sits on a
# character that text layout treats specially.
#
# Requires fonttools:  pip install fonttools

import argparse
import json
import os
import re
import sys
import tarfile
import urllib.request

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.recordingPen import RecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.misc.transform import Transform
from fontTools.pens.transformPen import TransformPen
from fontTools.svgLib.path import parse_path
from fontTools.ttLib import newTable

# This script lives with the artwork it builds from, in Resources/Fonts/IconFont.
ICON_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(ICON_DIR)))
OUTPUT = os.path.join(os.path.dirname(ICON_DIR), "IconFont.ttf")
HEADER = os.path.join(PROJECT_ROOT, "Source", "Utility", "Icons.h")
FLUENT_VERSION = "1.1.339"

# Downloaded tarballs land here; .cache/ is already gitignored.
FLUENT_PACKAGE = "@fluentui/svg-icons"
FLUENT_REGISTRY = "https://registry.npmjs.org/%40fluentui%2Fsvg-icons"
FLUENT_CACHE = os.path.join(ICON_DIR, ".cache")

FAMILY = "icon_font"
FULL_NAME = "plugdata_icon_font"
VERSION = "Version 001.000"
COPYRIGHT = "Copyright (c) 2022, Timothy Schoen"

# Vertical metrics, kept identical to the hand-built font so that text layout doesn't shift.
# fsSelection bit 7 (USE_TYPO_METRICS) makes renderers lay out with the sTypo* values.
ASCENT, DESCENT, LINE_GAP = 1700, -264, 184
TYPO_ASCENDER, TYPO_DESCENDER, TYPO_LINE_GAP = 1638, -410, 184
X_HEIGHT, CAP_HEIGHT = 1296, 1548

# Cubics in the artwork are approximated by quadratics to within this many font units.
CURVE_ERROR = 0.5

# Grayscale antialiasing with symmetric smoothing, at every size, and deliberately without
# GASP_GRIDFIT: grid-fitting snaps stems to whole pixels, which visibly distorts the icons on
# low-dpi screens. For the same reason no hinting programs (fpgm/prep/cvt) or glyph
# instructions are emitted, and head.flags keeps the same bits the hand-built font used.
GASP_DOGRAY = 0x0002
GASP_SYMMETRIC_SMOOTHING = 0x0008
HEAD_FLAGS = 0x001F


def load_manifest():
    with open(os.path.join(ICON_DIR, "icons.json"), encoding="utf-8") as f:
        return json.load(f)


def imply_oncurve_points(recording):
    """Merge quadratic segments joined at the midpoint of their control points.

    SVG has to spell out the on-curve point between two consecutive quadratic segments,
    while TrueType leaves it implied whenever it sits exactly halfway between the control
    points. Folding those points back in keeps the outlines - and the point counts - the
    same as the artwork they were drawn from, and keeps the half-unit coordinates SVG
    needs for them out of the rounded glyph.
    """
    out = []
    for op, args in recording:
        if op == "qCurveTo" and out and out[-1][0] == "qCurveTo":
            previous = out[-1][1]
            joint, before, after = previous[-1], previous[-2], args[0]
            if (joint[0] * 2 == before[0] + after[0]) and (joint[1] * 2 == before[1] + after[1]):
                out[-1] = (op, previous[:-1] + args)
                continue
        out.append((op, args))
    return out


def glyph_from_svg(svg, transform, source):
    """Turn the <path> elements of an SVG into a TrueType glyph, placed by transform."""
    paths = re.findall(r'<path[^>]*\sd="([^"]*)"', svg)
    if not paths:
        raise SystemExit("%s: no <path> to draw - the icon uses shapes this script can't read"
                         % source)
    if "evenodd" in svg:
        print("warning: %s uses fill-rule=evenodd, which TrueType cannot express; its holes "
              "may come out filled" % source)

    recording = RecordingPen()
    # Cu2QuPen leaves quadratics alone and approximates any cubics - which is what drawing
    # programs, and the Fluent artwork, produce - to within half a font unit.
    for d in paths:
        parse_path(d, TransformPen(Cu2QuPen(recording, CURVE_ERROR), transform))

    pen = TTGlyphPen(None)
    recording.value = imply_oncurve_points(recording.value)
    recording.replay(pen)
    return pen.glyph()


def read_glyph(svg, upm, ascender, source):
    """Place any icon on the em by the one rule: its canvas is the em.

    This is what keeps the set even. Both kinds of artwork are drawn on a square canvas -
    2048 units for plugdata's own SVGs, 24 for the Fluent icons - so scaling that canvas
    onto the em leaves every icon at the size its artwork asks for, with nothing to tune
    per glyph and nothing to drift out of step.
    """
    view_box = re.search(r'viewBox="\s*([-\d.]+)[,\s]+([-\d.]+)[,\s]+([-\d.]+)[,\s]+([-\d.]+)', svg)
    if not view_box:
        raise SystemExit("%s: no viewBox" % source)
    min_x, min_y, width, height = (float(v) for v in view_box.groups())
    if width <= 0 or abs(width - height) > 0.01:
        raise SystemExit("%s: canvas is %g by %g; icons have to be drawn square"
                         % (source, width, height))

    # SVG y points down from the typographic ascender.
    scale = upm / width
    transform = Transform(scale, 0, 0, -scale, -min_x * scale, ascender + min_y * scale)
    return glyph_from_svg(svg, transform, source)


class FluentIcons:
    """The @fluentui/svg-icons npm package, downloaded on demand and read from the tarball.

    Icons are drawn y-down on their own square canvas, the same one for both styles of an
    icon, so a glyph's [scale, dx, dy] places either style of it identically.
    """

    def __init__(self, version=FLUENT_VERSION, archive=None):
        self.archive = archive or self._download(version)
        self.tar = tarfile.open(self.archive)
        self.version = re.search(r"svg-icons-(.+)\.tgz$", os.path.basename(self.archive))
        self.version = self.version.group(1) if self.version else os.path.basename(self.archive)

    @staticmethod
    def _download(version):
        os.makedirs(FLUENT_CACHE, exist_ok=True)
        pinned = os.path.join(FLUENT_CACHE, "svg-icons-%s.tgz" % version)
        if version != "latest" and os.path.exists(pinned):
            return pinned

        try:
            with urllib.request.urlopen("%s/%s" % (FLUENT_REGISTRY, version), timeout=30) as f:
                release = json.load(f)
            url = release["dist"]["tarball"]
        except Exception as error:
            # Only "latest" may quietly settle for whatever is around; a pinned version that
            # cannot be had is a real failure.
            cached = [f for f in os.listdir(FLUENT_CACHE) if f.endswith(".tgz")]
            if version != "latest" or not cached:
                raise SystemExit("could not fetch %s %s (%s)%s" % (
                    FLUENT_PACKAGE, version, error,
                    "" if cached else " and nothing is cached in %s - pass --fluent-archive "
                                     "to build offline" % os.path.relpath(FLUENT_CACHE, PROJECT_ROOT)))
            newest = max(cached, key=lambda f: os.path.getmtime(os.path.join(FLUENT_CACHE, f)))
            print("warning: could not reach the npm registry (%s); using cached %s"
                  % (error, newest))
            return os.path.join(FLUENT_CACHE, newest)

        path = os.path.join(FLUENT_CACHE, os.path.basename(url))
        if not os.path.exists(path):
            print("downloading %s %s" % (FLUENT_PACKAGE, release["version"]))
            urllib.request.urlretrieve(url, path)
        return path

    def read(self, name):
        # Named members only - never extractall, the archive is downloaded.
        member = self.tar.extractfile("package/icons/%s.svg" % name)
        if member is None:
            raise SystemExit("%s has no icon called %s" % (FLUENT_PACKAGE, name))
        return member.read().decode("utf-8")

def notdef_glyph(upm, ascender, advance):
    """A hollow box, drawn like FontForge's default .notdef, so a missing icon is visible."""
    left, right = round(advance * 0.09), round(advance * 0.91)
    top, bottom = ascender, round(ascender - upm * 0.73)
    stroke = round(upm * 0.05)
    pen = TTGlyphPen(None)
    for box, clockwise in (((left, bottom, right, top), True),
                           ((left + stroke, bottom + stroke, right - stroke, top - stroke), False)):
        x0, y0, x1, y1 = box
        corners = [(x0, y0), (x0, y1), (x1, y1), (x1, y0)]
        pen.moveTo(corners[0])
        for point in (corners[1:] if clockwise else corners[:0:-1]):
            pen.lineTo(point)
        pen.closePath()
    return pen.glyph()


def glyph_name(entry, codepoint, taken):
    """Name glyphs after the Icons:: constant that selects them, for readable font dumps."""
    for candidate in entry["names"] + ["uni%04X" % codepoint]:
        name = re.sub(r"[^A-Za-z0-9._]", "_", candidate)[:63]
        if name and name[0].isdigit():
            name = "_" + name[:62]
        if name and name not in taken:
            return name
    raise SystemExit("could not name the glyph for %s" % entry["names"])


HEADER_PREAMBLE = """/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Generated by Resources/Fonts/IconFont/build_icon_font.py from
// Resources/Fonts/IconFont/icons.json, alongside Resources/Fonts/IconFont.ttf.
// Do not edit: add the icon to icons.json and rebuild, or the name and the glyph
// it selects will disagree.

#pragma once

#include "Utility/Config.h"

struct Icons {
"""


def write_header(path, sections, codepoints):
    """Write the Icons:: constants that select each glyph from the font just built."""
    lines = [HEADER_PREAMBLE]
    for item in sections:
        if isinstance(item, str):
            if len(lines) > 1:
                lines.append("\n")
            lines.append("    // %s\n" % item)
            continue
        character = chr(codepoints[id(item)])
        literal = 'CharPointer_UTF8("%s")' % "".join(
            "\\x%02x" % byte for byte in character.encode("utf-8"))
        for name in item["names"]:
            lines.append("    static inline String const %s = %s;\n" % (name, literal))
    lines.append("};\n")
    with open(path, "w", encoding="utf-8") as f:
        f.write("".join(lines))
    print("wrote %s: %d names" % (os.path.relpath(path, PROJECT_ROOT),
                                  sum(len(i["names"]) for i in sections if not isinstance(i, str))))


def build(fluent_version=FLUENT_VERSION, fluent_archive=None, output=OUTPUT, header=HEADER):
    manifest = load_manifest()
    upm = manifest["unitsPerEm"]
    ascender = manifest["ascender"]
    advance = manifest["advance"]

    items = manifest["glyphs"]
    entries = [e for e in items if not isinstance(e, str)]
    icons = (FluentIcons(fluent_version, fluent_archive)
             if any(e.get("fluent") for e in entries) else None)

    glyphs = {".notdef": notdef_glyph(upm, ascender, round(advance * 0.36))}
    metrics = {".notdef": (round(advance * 0.36), round(advance * 0.09))}
    order = [".notdef"]
    char_map = {}
    codepoints = {}
    cache = {}
    next_codepoint = int(manifest["firstCodepoint"][2:], 16)

    for entry in entries:
        if entry.get("fluent"):
            key = entry["fluent"]
            if not key.split("_")[-2] == "24":
                raise SystemExit("%s: Fluent icons have to come from the _24_ grid, so that "
                                 "one rule places them all" % key)
            if key not in cache:
                cache[key] = read_glyph(icons.read(key), upm, ascender, key)
        else:
            key = entry["file"]
            path = os.path.join(ICON_DIR, key.replace("/", os.sep))
            if not os.path.exists(path):
                raise SystemExit("missing artwork for %s: %s" % (entry["names"][0], key))
            if key not in cache:
                with open(path, encoding="utf-8") as f:
                    cache[key] = read_glyph(f.read(), upm, ascender, key)

        codepoint = next_codepoint
        next_codepoint += 1
        name = glyph_name(entry, codepoint, glyphs)
        glyphs[name] = cache[key]
        metrics[name] = (advance, 0)
        order.append(name)
        char_map[codepoint] = name
        codepoints[id(entry)] = codepoint

    builder = FontBuilder(unitsPerEm=upm, isTTF=True)
    builder.setupGlyphOrder(order)
    builder.setupCharacterMap(char_map)
    builder.setupGlyf(glyphs)
    # The left side bearing has to agree with the outline's xMin, or the glyph is drawn
    # shifted by the difference.
    glyf = builder.font["glyf"]
    for name, (advance, _) in metrics.items():
        glyph = glyf[name]
        glyph.recalcBounds(glyf)
        metrics[name] = (advance, glyph.xMin if glyph.numberOfContours else 0)
    builder.setupHorizontalMetrics(metrics)
    builder.setupHorizontalHeader(ascent=ASCENT, descent=DESCENT, lineGap=LINE_GAP)
    builder.setupNameTable({
        "copyright": COPYRIGHT,
        "familyName": FAMILY,
        "styleName": "Regular",
        "uniqueFontIdentifier": "%s : %s" % (FAMILY, FULL_NAME),
        "fullName": FULL_NAME,
        "version": VERSION,
        "psName": FULL_NAME,
    })
    builder.setupOS2(
        version=4,
        usWeightClass=400,
        usWidthClass=5,
        fsType=0,
        fsSelection=0x0080,  # USE_TYPO_METRICS
        achVendID="PfEd",
        sTypoAscender=TYPO_ASCENDER,
        sTypoDescender=TYPO_DESCENDER,
        sTypoLineGap=TYPO_LINE_GAP,
        usWinAscent=ASCENT,
        usWinDescent=-DESCENT,
        sxHeight=X_HEIGHT,
        sCapHeight=CAP_HEIGHT,
        ySubscriptXSize=1331,
        ySubscriptYSize=1434,
        ySubscriptYOffset=287,
        ySuperscriptXSize=1331,
        ySuperscriptYSize=1434,
        ySuperscriptYOffset=983,
        yStrikeoutSize=100,
        yStrikeoutPosition=528,
        ulUnicodeRange1=0x00000001,
        ulCodePageRange1=0x00000001,
        usDefaultChar=0,
        usBreakChar=32,
        usMaxContext=1,
    )
    builder.setupPost(italicAngle=0.0, underlinePosition=-153, underlineThickness=102)

    gasp = newTable("gasp")
    gasp.version = 1
    gasp.gaspRange = {0xFFFF: GASP_DOGRAY | GASP_SYMMETRIC_SMOOTHING}
    builder.font["gasp"] = gasp

    builder.font["head"].flags = HEAD_FLAGS
    builder.font["head"].lowestRecPPEM = 8
    builder.font["head"].fontRevision = 1.0
    # Fixed timestamps, so that rebuilding without changing any artwork is a no-op in git.
    builder.font["head"].created = builder.font["head"].modified = 3723840000

    # An SVG nothing points at is dead weight, and easy to leave behind after an icon is
    # switched to a Fluent reference.
    on_disk = set()
    for folder in ("gui", "objects"):
        directory = os.path.join(ICON_DIR, "custom", folder)
        on_disk.update("custom/%s/%s" % (folder, f) for f in os.listdir(directory)
                       if f.endswith(".svg"))
    unused = sorted(on_disk - {e["file"] for e in entries if e.get("file")})
    if unused:
        print("warning: %d SVG(s) no icon refers to any more: %s"
              % (len(unused), ", ".join(unused)))

    builder.save(output)
    print("wrote %s: %d glyphs%s" % (
        os.path.relpath(output, PROJECT_ROOT), len(char_map),
        ", Fluent icons %s" % icons.version if icons else ""))
    if header:
        write_header(header, items, codepoints)
    if icons:
        icons.tar.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--output", default=OUTPUT, help="path of the .ttf to write")
    parser.add_argument("--header", default=HEADER, metavar="PATH",
                        help="path of the Icons:: header to write (default: Source/Utility/Icons.h)")
    parser.add_argument("--no-header", dest="header", action="store_const", const=None,
                        help="build the font only, leaving the header alone")
    parser.add_argument("--fluent-version", default=FLUENT_VERSION,
                        help="npm version of %s (default: %s)" % (FLUENT_PACKAGE, FLUENT_VERSION))
    parser.add_argument("--fluent-archive",
                        help="build from an already downloaded %s tarball" % FLUENT_PACKAGE)
    args = parser.parse_args()
    build(fluent_version=args.fluent_version, fluent_archive=args.fluent_archive,
          output=args.output, header=args.header)


if __name__ == "__main__":
    sys.exit(main())
