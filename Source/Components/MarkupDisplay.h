/*
 ==============================================================================

 BarelyML.h
 Created: 5 Oct 2023
 Author:  Fritz Menzer
 Version: 0.2.1

 ==============================================================================
 Copyright (C) 2023 Fritz Menzer

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of BarelyML and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 ==============================================================================

 BarelyML.h and BarelyML.cpp implement the BarelyML markup language, which
 supports the following syntax:

 Headings

 # Level 1 Heading
 ## Level 2 Heading
 ### Level 3 Heading
 #### Level 4 Heading
 ##### Level 5 Heading


 Bold and Italic

 *Bold Text*
 _Italic Text_


 Unordered lists with hyphens

 - Item 1
 - Item 2


 Ordered lists with numbers

 1. Item 1
 2. Item 2


 Tables

 ^ Header 1      ^ Header 2     ^
 | Cell 1-1      | Cell 1-2     |
 ^ Also a header | Not a header |


 Font Colour

 <c:red>Red Text</c>
 <c#FFFFFF>White Text</c>

 Colour names supported by default (CGA 16-colour palette with some extensions):
 black,blue,green,cyan,red,magenta,brown,lightgray,
 darkgray,lightblue,lightgreen,lightcyan,lightred,lightmagenta,yellow,white,
 orange, pink, darkyellow, purple, gray
 (the idea is that there will be the option to provide a custom colour definition object)


 Images

 {{image-filename.jpg?200}}

 The number after the "?" is the maximum width (optional).


 Links

 [[https://mnsp.ch|My Website]]


 Admonitions

 INFO: This is an info paragraph (blue tab).
 HINT: This is a hint paragraph (green tab).
 IMPORTANT: This is an important paragraph (red tab).
 CAUTION: This is a caution paragraph (yellow tab).
 WARNING: This is a warning paragraph (orange tab).

 TODO: Links in tables
 TODO: Images in tables
 TODO: Icons for admonitions

 NOTE: The conversion methods FROM OTHER FORMATS TO BarelyML are incomplete,
 but work for most simple documents. If you have a use case that doesn't
 work yet, please let me know via GitHub or the JUCE forum and I'll try
 to make it work.

 The conversion methods FROM BarelyML TO OTHER FORMATS on the other hand
 are extremely minimal and only used in the demo application to keep the
 UI from doing weird stuff when switching the markdown language. For now
 I don't see any other use, so don't count on this becoming a feature.

 ==============================================================================
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Fonts.h"
#include "BouncingViewport.h"

namespace MarkupDisplay {

class FileSource {
public:
    virtual ~FileSource() {};
    virtual Image getImageForFilename(String filename) = 0;
};

class Block : public Component {
public:
    Block()
    {
        colours = nullptr;
        defaultColour = findColour(PlugDataColour::canvasTextColourId);
    }
    // static utility methods
    static Colour parseHexColourStatic(String s, Colour defaultColour)
    {
        if (s.startsWith("#")) {
            s = s.substring(1);
            // if we have 3 or 4 characters, expand by duplicating characters
            if (s.length() == 3 || s.length() == 4) {
                String expanded;
                for (int i = 0; i < s.length(); i++) {
                    expanded += s[i];
                    expanded += s[i];
                }
                s = expanded;
            }
            if (s.length() == 6) { // also applies to duplicated 3 char string
                s = String("FF") + s;
            }
        }
        if (s.isEmpty()) {
            return defaultColour;
        } else {
            return Colour::fromString(s);
        }
    }

    static bool containsLink(String line)
    {
        return line.contains("[[") && line.fromFirstOccurrenceOf("[[", false, false).contains("]]");
    }

    // Common functionalities for all blocks
    String consumeLink(String line)
    {
        int idx1, idx2;
        while ((idx1 = line.indexOf("[[")) >= 0 && (idx2 = line.indexOf(idx1, "]]")) > idx1) {
            auto link = line.substring(idx1 + 2, idx2);
            if (link.contains("|")) {
                String altText = link.fromFirstOccurrenceOf("|", false, false);
                link = link.upToFirstOccurrenceOf("|", false, false);
                line = line.substring(0, idx1) + "<l:" + link + ">*" + altText + "*</l>" + line.substring(idx2 + 2);
            } else {
                line = line.substring(0, idx1) + "<l:" + link + ">*" + link + "*</l>" + line.substring(idx2 + 2);
            }
        }

        return line;
    }

    virtual void parseMarkup(StringArray const& lines, Font font) {};
    virtual float getHeightRequired(float width) = 0;
    void setColours(StringPairArray* c)
    {
        colours = c;
        defaultColour = parseHexColour((*colours)["default"]);
    };
    virtual bool canExtendBeyondMargin() { return false; }; // for tables

    void mouseMove(MouseEvent const& event) override
    {
        bool isHoveringLink = false;

        for (auto& [link, bounds] : linkBounds) {
            if (bounds.contains(event.x, event.y)) {
                isHoveringLink = true;
                break;
            }
        }

        setMouseCursor(isHoveringLink ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }

    void mouseUp(MouseEvent const& event) override
    {
        for (auto& [link, bounds] : linkBounds) {
            if (bounds.contains(event.x, event.y)) {
                URL(link).launchInDefaultBrowser();
                break;
            }
        }
    }

protected:
    AttributedString parsePureText(StringArray const& lines, Font font, bool addNewline = true)
    {
        AttributedString attributedString;

        String currentLine;
        currentColour = defaultColour;

        bool bold = false;
        bool italic = false;

        for (auto line : lines) {
            if (line.startsWith("##### ")) {
                attributedString.append(parsePureText(line.substring(6), Fonts::getBoldFont().withHeight(font.getHeight() * 1.1f), false));
            } else if (line.startsWith("#### ")) {
                attributedString.append(parsePureText(line.substring(5), Fonts::getBoldFont().withHeight(font.getHeight() * 1.25f), false));
            } else if (line.startsWith("### ")) {
                attributedString.append(parsePureText(line.substring(4), Fonts::getBoldFont().withHeight(font.getHeight() * 1.42f), false));
            } else if (line.startsWith("## ")) {
                attributedString.append(parsePureText(line.substring(3), Fonts::getBoldFont().withHeight(font.getHeight() * 1.7f), false));
            } else if (line.startsWith("# ")) {
                attributedString.append(parsePureText(line.substring(2), Fonts::getBoldFont().withHeight(font.getHeight() * 2.1f), false));
            } else {
                auto parseLink = [this, &attributedString](String& link, String& linkText) {
                    if (link.isNotEmpty()) {
#if JUCE_MAC
                        auto start = attributedString.getText().length();
                        auto end = start + linkText.length();
#else
                        // macOS version of TextLayout considers whitespace as a character, but the Windows/Linux versions don't!
                        auto start = attributedString.getText().replace(" ", "").replace("\n", "").replace("\r", "").replace("\t", "").length();
                        auto end = start + linkText.replace(" ", "").replace("\n", "").replace("\r", "").replace("\t", "").length();
#endif
                        links.add({ link, start, end });
                        link = "";
                    }
                };

                String currentLink;
                while (line.isNotEmpty()) {
                    bool needsNewFont = false;
                    // find first token to interpret
                    int bidx = line.indexOf("*");
                    int iidx = line.indexOf("_");
                    int tidx = line.indexOf("<");
                    Colour nextColour = currentColour;
                    String nextLink;
                    if (bidx > -1 && ((bidx < iidx) | (iidx == -1)) && ((bidx < tidx) | (tidx == -1))) {
                        // if the next token is toggling the bold state...
                        // ...first add everything up to the token...
                        auto linkText = line.substring(0, bidx);
                        if (bold)
                            parseLink(currentLink, linkText);
                        attributedString.append(linkText, font, currentColour);
                        line = line.substring(bidx + 1); // ...then drop up to and including the token...
                        bold = !bold;                    // ...toggle the bold status...
                        needsNewFont = true;             // ...and request new font.
                    } else if (iidx > -1 && ((iidx < tidx) | (tidx == -1))) {
                        // if the next token is toggling the italic state...
                        // ...first add everything up to the token...
                        auto linkText = line.substring(0, iidx);
                        if (italic)
                            parseLink(currentLink, linkText);
                        attributedString.append(linkText, font, currentColour);
                        line = line.substring(iidx + 1); // ...then drop up to and including the token...
                        italic = !italic;                // ...toggle the italic status...
                        needsNewFont = true;             // ...and request new font.
                    } else if (tidx > -1) {
                        // if the next token is a tag, first figure out if it is a recognized tag...
                        String tag;
                        bool tagRecognized = false;
                        // find tag end
                        int tidx2 = line.indexOf(tidx, ">");
                        if (tidx2 > tidx) {
                            tag = line.substring(tidx + 1, tidx2);
                        }
                        if (tag.startsWith("c#")) {
                            // hex colour tag
                            nextColour = parseHexColour(tag.substring(1));
                            tagRecognized = true;
                        } else if (tag.startsWith("c:")) {
                            // named colour tag
                            String name = tag.substring(2);
                            if (colours != nullptr && colours->containsKey(name)) {
                                nextColour = parseHexColour((*colours)[name]);
                            }
                            tagRecognized = true;
                        } else if (tag.startsWith("/c")) {
                            // end of colour tag
                            nextColour = defaultColour;
                            tagRecognized = true;
                        } else if (tag.startsWith("l:")) {
                            currentLink = tag.substring(2);
                            nextColour = findColour(PlugDataColour::dataColourId); // link colour is just data colour for now
                            tagRecognized = true;
                        } else if (tag.startsWith("/l")) {
                            currentLink = "";
                            nextColour = defaultColour;
                            tagRecognized = true;
                        }
                        if (tagRecognized) {
                            // ...first add everything up to the tag...
                            attributedString.append(line.substring(0, tidx), font, currentColour);
                            // ...then drop up to and including the tag.
                            line = line.substring(tidx2 + 1);
                        } else {
                            // ...first add everything up to and including the token...
                            attributedString.append(line.substring(0, tidx + 1), font, currentColour);
                            // ...then drop it.
                            line = line.substring(tidx + 1);
                        }
                    } else {
                        parseLink(currentLink, line);
                        // if no token was found -> add the remaining text...
                        attributedString.append(line, font, currentColour);
                        // ...and clear the line.
                        line.clear();
                    }

                    currentColour = nextColour;
                    if (needsNewFont) {
                        font = Fonts::getDefaultFont().withHeight(15);
                        if (bold) {
                            font = Fonts::getBoldFont().withHeight(15);
                        }
                        if (italic) {
                            // italic only seems to work on macOS...
#if JUCE_MAC
                            font = Fonts::getVariableFont().italicised().withHeight(15);
#else
                            font = Fonts::getDefaultFont().withHeight(15);
#endif
                        }
                    }
                }
            }

            if (addNewline) {
                attributedString.append(" \n", font, defaultColour);
            }
        }
        return attributedString;
    }

    void updateLinkBounds(TextLayout& layout)
    {
        linkBounds.clear();

        // Look for clickable links
        for (auto& [link, start, end] : links) {
            int offset = 0;
            auto currentLinkBounds = Rectangle<float>();
            for (auto& line : layout) {
                for (auto* run : line.runs) {
                    for (int i = start - offset; i < end - offset; i++) {
                        if (i < 0 || i >= run->glyphs.size())
                            continue;

                        auto& glyph = run->glyphs.getReference(i);
                        auto lineBounds = Rectangle<float>(glyph.width, 14).withPosition((glyph.anchor + line.lineOrigin));
                        currentLinkBounds = linkBounds.isEmpty() ? lineBounds : currentLinkBounds.getUnion(lineBounds);
                    }

                    linkBounds.add({ link, currentLinkBounds.translated(0, -11) });
                    offset += run->glyphs.size();
                }
            }
        }
    }

    Colour defaultColour;
    Colour currentColour;
    StringPairArray* colours;

    Colour parseHexColour(String s)
    {
        return parseHexColourStatic(s, defaultColour);
    }

    AttributedString attributedString;

private:
    Array<std::pair<String, Rectangle<float>>> linkBounds;
    Array<std::tuple<String, int, int>> links;
};

class TextBlock : public Block {
public:
    void parseMarkup(StringArray const& lines, Font font) override
    {
        attributedString = parsePureText(lines, font);
    }

    float getHeightRequired(float width) override
    {
        TextLayout layout;
        layout.createLayout(attributedString, width);
        return layout.getHeight();
    }

    void paint(Graphics& g) override
    {
        TextLayout layout;
        layout.createLayout(attributedString, getWidth());
        layout.draw(g, getLocalBounds().toFloat());
    }

    void resized() override
    {
        TextLayout layout;
        layout.createLayout(attributedString, getWidth());
        updateLinkBounds(layout);
    }

private:
};

class AdmonitionBlock : public Block {
public:
    static bool isAdmonitionLine(String const& line)
    {
        return line.startsWith("INFO: ") || line.startsWith("HINT: ") || line.startsWith("IMPORTANT: ") || line.startsWith("CAUTION: ") || line.startsWith("WARNING: ") || line.startsWith(">");
    }

    void parseAdmonitionMarkup(String const& line, Font font, int iconsize, int margin, int linewidth)
    {
        if (line.startsWith("INFO: ")) {
            type = info;
        } else if (line.startsWith("HINT: ")) {
            type = hint;
        } else if (line.startsWith("IMPORTANT: ")) {
            type = important;
        } else if (line.startsWith("CAUTION: ")) {
            type = caution;
        } else if (line.startsWith("WARNING: ")) {
            type = warning;
        }
        attributedString = parsePureText(line.fromFirstOccurrenceOf(": ", false, false), font);
        this->iconsize = iconsize;
        this->margin = margin;
        this->linewidth = linewidth;
    }

    float getHeightRequired(float width) override
    {
        TextLayout layout;
        layout.createLayout(attributedString, width - iconsize - 2 * (margin + linewidth));
        return jmax(layout.getHeight(), (float)iconsize);
    }

    void paint(Graphics& g) override
    {
        // select colour
        switch (type) {
        case info:
            g.setColour(parseHexColour((*colours)["blue"]));
            break;

        case hint:
            g.setColour(parseHexColour((*colours)["green"]));
            break;

        case important:
            g.setColour(parseHexColour((*colours)["red"]));
            break;

        case caution:
            g.setColour(parseHexColour((*colours)["yellow"]));
            break;

        case warning:
            g.setColour(parseHexColour((*colours)["orange"]));
            break;
        }
        // draw tab
        g.fillRect(Rectangle<int>(0, 0, iconsize, iconsize));
        // draw lines left and right
        g.fillRect(Rectangle<int>(iconsize, 0, linewidth, getHeight()));
        g.fillRect(Rectangle<int>(getWidth() - linewidth, 0, linewidth, getHeight()));
        attributedString.draw(g, Rectangle<float>(iconsize + margin + linewidth, 0, getWidth() - iconsize - 2 * (margin + linewidth), getHeight()));
    }

private:
    enum ParagraphType { info,
        hint,
        important,
        caution,
        warning };
    ParagraphType type;
    int iconsize, margin, linewidth;
};

class TableBlock : public Block {
public:
    TableBlock()
    {
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&table, false);             // we manage the content component
        viewport.setScrollBarsShown(false, false, false, true); // scroll only horizontally
        viewport.setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
    }

    static bool isTableLine(String const& line)
    {
        return line.startsWith("^") || line.startsWith("|");
    }

    void parseMarkup(StringArray const& lines, Font font) override
    {
        // read cells
        table.cells.clear();
        for (auto line : lines) {
            // find all cells in this line
            OwnedArray<Cell>* row = new OwnedArray<Cell>();
            while (line.containsAnyOf("^|")) {
                bool isHeader = line.startsWith("^");
                line = line.substring(1);                    // remove left delimiter
                int nextDelimiter = line.indexOfAnyOf("^|"); // find right delimiter
                if (nextDelimiter >= 0) {                    // no delimiter found -> we're done with this line
                    String rawString = line.substring(0, nextDelimiter);
                    line = line.substring(nextDelimiter); // drop everything up to right delimiter
                    // TODO: use the number of whitespace characters on either side of rawString to determine justification
                    // TODO: implement || -> previous cell spans two columns
                    AttributedString attributedString = parsePureText(rawString.trim(), isHeader ? font.boldened() : font);
                    TextLayout layout;
                    layout.createLayout(attributedString, 1.0e7f);
                    updateLinkBounds(layout);
                    row->add(new Cell { attributedString, isHeader, layout.getWidth(), layout.getHeight() });
                }
            }
            table.cells.add(row);
        }
        // compute column widths
        table.columnwidths.clear();
        for (int i = 0; i < table.cells.size(); i++) {
            OwnedArray<Cell>* row = table.cells[i];
            for (int j = 0; j < row->size(); j++) {
                if (j < table.columnwidths.size()) {
                    table.columnwidths.set(j, jmax(table.columnwidths[j], (*row)[j]->width));
                } else {
                    table.columnwidths.set(j, (*row)[j]->width);
                }
            }
        }
        // compute row heights
        table.rowheights.clear();
        for (int i = 0; i < table.cells.size(); i++) {
            OwnedArray<Cell>* row = table.cells[i];
            float rowheight = 0;
            for (int j = 0; j < row->size(); j++) {
                rowheight = jmax(rowheight, (*row)[j]->height);
            }
            table.rowheights.set(i, rowheight);
        }
        table.setBounds(0, 0, getWidthRequired() + table.leftmargin + table.cellgap, getHeightRequired(0.f));
    }

    float getWidthRequired()
    {
        float width = 0;
        for (int i = 0; i < table.columnwidths.size(); i++) {
            width += table.columnwidths[i] + 2 * table.cellmargin + table.cellgap;
        }
        return width - table.cellgap;
    }

    float getHeightRequired(float width) override
    {
        // NOTE: We're ignoring width - the idea is that tables can be scrolled horizontally if necessary
        float height = 0;
        for (int i = 0; i < table.rowheights.size(); i++) {
            height += table.rowheights[i] + 2 * table.cellmargin + table.cellgap;
        }
        return height - table.cellgap;
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds());
    }
    void setBGColours(Colour bg, Colour bgHeader)
    {
        table.bg = bg;
        table.bgHeader = bgHeader;
    }
    void setMargins(int margin, int gap, int leftmargin)
    {
        table.cellmargin = margin;
        table.cellgap = gap;
        table.leftmargin = leftmargin;
    }
    bool canExtendBeyondMargin() override { return true; };

private:
    typedef struct {
        AttributedString s;
        bool isHeader;
        float width;
        float height;
    } Cell;
    class InnerViewport : public Viewport {
    public:
        // Override the mouse event methods to forward them to the parent Viewport
        void mouseDown(MouseEvent const& e) override
        {
            if (Viewport* parent = findParentComponentOfClass<Viewport>()) {
                MouseEvent ep = MouseEvent(e.source, e.position, e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, parent, e.originalComponent, e.eventTime, e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
                parent->mouseDown(ep);
            }
            Viewport::mouseDown(e);
        }
        void mouseUp(MouseEvent const& e) override
        {
            if (Viewport* parent = findParentComponentOfClass<Viewport>()) {
                MouseEvent ep = MouseEvent(e.source, e.position, e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, parent, e.originalComponent, e.eventTime, e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
                parent->mouseUp(ep);
            }
            Viewport::mouseUp(e);
        }
        void mouseDrag(MouseEvent const& e) override
        {
            if (Viewport* parent = findParentComponentOfClass<Viewport>()) {
                MouseEvent ep = MouseEvent(e.source, e.position, e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, parent, e.originalComponent, e.eventTime, e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
                parent->mouseDrag(ep);
            }
            Viewport::mouseDrag(e);
        }
        // Override mouseWheelMove to forward events to the parent Viewport
        void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
        {
            Viewport* parent = findParentComponentOfClass<Viewport>();
            if (parent != nullptr) {
                MouseEvent ep = MouseEvent(e.source, e.position, e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, parent, e.originalComponent, e.eventTime, e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
                parent->mouseWheelMove(ep, wheel);
            }
            Viewport::mouseWheelMove(e, wheel);
        }
    };
    class Table : public Component {
    public:
        void paint(Graphics& g) override
        {
            float y = 0.f; // Y coordinate of cell's top left corner
            for (int i = 0; i < cells.size(); i++) {
                float x = leftmargin;             // X coordinate of cell's top left corner
                OwnedArray<Cell>* row = cells[i]; // get current row
                for (int j = 0; j < row->size(); j++) {
                    Cell c = *((*row)[j]);     // get current cell
                    if (c.isHeader) {          // if it's a header cell...
                        g.setColour(bgHeader); // ...set header background colour
                    } else {                   // otherwise...
                        g.setColour(bg);       // ...set regular background colour
                    }
                    // fill background
                    g.fillRect(x, y, columnwidths[j] + 2 * cellmargin, rowheights[i] + 2 * cellmargin);
                    // draw cell text
                    c.s.draw(g, Rectangle<float>(x + cellmargin, y + cellmargin, columnwidths[j], rowheights[i]));
                    // move one cell to the right
                    x += columnwidths[j] + 2 * cellmargin + cellgap;
                }
                // move to next row (note: x will be reset at next loop iteration)
                y += rowheights[i] + 2 * cellmargin + cellgap;
            }
        }

        OwnedArray<OwnedArray<Cell>> cells;
        Array<float> columnwidths;
        Array<float> rowheights;
        Colour bg, bgHeader;
        int cellmargin, cellgap, leftmargin;
    };
    InnerViewport viewport;
    Table table;
};

class ImageBlock : public Block {
public:
    static bool isImageLine(String const& line)
    {
        return (line.startsWith("{{") && line.trim().endsWith("}}")) || // either just an image...
            (line.startsWith("[[") && line.trim().endsWith("]]") &&     // ...or a link around...
                line.contains("{{") && line.contains("}}"));            // ...an image.
    }

    static bool isHTMLImageLine(String const& line)
    {
        // Check if the line contains an <img> tag
        return line.contains("<img") && line.contains("src=") && line.contains(">");
    }

    void parseImageMarkup(String const& line, FileSource* fileSource)
    {
        String filename = line.fromFirstOccurrenceOf("{{", false, false).upToFirstOccurrenceOf("}}", false, false);
        if (filename.contains("?")) {
            maxWidth = filename.fromFirstOccurrenceOf("?", false, false).getIntValue();
            filename = filename.upToFirstOccurrenceOf("?", false, false);
        } else {
            maxWidth = -1;
        }
        if (fileSource) {
            image = fileSource->getImageForFilename(filename);
        } else {
            imageMissingMessage.append("no file source. ", Font(14), defaultColour);
            image = Image();
        }
        if (!image.isValid()) {
            imageMissingMessage.append(filename + " not found.", Font(14), defaultColour);
        }
    }

    void parseHTMLImageMarkup(String const& html, FileSource* fileSource)
    {
        // Assuming the HTML-like format has an <img> tag with a 'src' attribute
        String imgTag = html.fromFirstOccurrenceOf("<img", false, false).upToFirstOccurrenceOf(">", false, false);
        String srcAttribute = imgTag.fromFirstOccurrenceOf("src=\"", false, false).upToFirstOccurrenceOf("\"", false, false);

        if (imgTag.contains("width=\"")) {
            maxWidth = imgTag.fromFirstOccurrenceOf("width=\"", false, false).upToFirstOccurrenceOf("\"", false, false).getIntValue();
        } else {
            maxWidth = -1;
        }

        if (fileSource) {
            image = fileSource->getImageForFilename(srcAttribute);
        } else {
            imageMissingMessage.append("no file source. ", Font(14), defaultColour);
            image = Image();
        }

        if (!image.isValid()) {
            imageMissingMessage.append(srcAttribute + " not found.", Font(14), defaultColour);
        }
    }

    float getHeightRequired(float width) override
    {
        if (image.isValid() && image.getWidth() > 0) {
            if (maxWidth > 0) {
                return jmin((float)maxWidth, width) * (float)image.getHeight() / (float)image.getWidth();
            } else {
                return width * (float)image.getHeight() / (float)image.getWidth();
            }
        } else {
            return 20.f;
        }
    }

    void paint(Graphics& g) override
    {
        if (image.isValid()) {
            float w = getWidth();
            if (maxWidth > 0) {
                w = jmin((float)maxWidth, w);
            }
            g.drawImage(image, Rectangle<float>(0, 0, w, getHeight()), RectanglePlacement::centred);
        } else {
            g.setColour(defaultColour);
            g.drawRect(getLocalBounds());
            g.drawLine(0, 0, getWidth(), getHeight());
            g.drawLine(getWidth(), 0, 0, getHeight());
            imageMissingMessage.draw(g, getLocalBounds().reduced(5, 5).toFloat());
        }
    }

    void resized() override { }

private:
    AttributedString imageMissingMessage;
    Image image;
    int maxWidth;
};

class ListItem : public Block {
public:
    static bool isListItem(String const& line)
    {
        return (line.indexOf(". ") > 0 && line.substring(0, line.indexOf(". ")).trim().containsOnly("0123456789")) || (line.indexOf("- ") >= 0 && !line.substring(0, line.indexOf("- ")).containsNonWhitespaceChars());
    }

    void parseItemMarkup(String const& line, Font font, int indentPerSpace, int gap)
    {
        this->gap = gap;
        label.clear();

        int dotidx = line.indexOf(". ");                    // find dot+space in line
        String beforedot = line.substring(0, dotidx);       // find out if before the dot...
        String lbl = beforedot.trimStart();                 // ...there's only whitespace...
        if (dotidx > 0 && lbl.containsOnly("0123456789")) { // ...and at least one number.
            label.append(lbl + ".", font, defaultColour);   // create label
            // parse item text (everything after the dot)
            attributedString = parsePureText(line.substring(dotidx + 2).trimStart(), font);
            // use number of whitespace characters to determine indent
            indent = indentPerSpace * (beforedot.length() - lbl.length());
        } else {                                                // otherwise try unordered list:
            int hyphenidx = line.indexOf("- ");                 // find hyphen+space in line
            String beforehyphen = line.substring(0, hyphenidx); // find out if before the hyphen...
            if (!beforehyphen.containsNonWhitespaceChars()) {   // ...there's only whitespace.
                // parse item text (everything after the hyphen)
                attributedString = parsePureText(line.substring(hyphenidx + 2).trimStart(), font);
                // use number of whitespace characters to determine indent
                indent = indentPerSpace * beforehyphen.length();
                // create label TODO: have bullet character depend on indent
                label.append(CharPointer_UTF8("•"), font, defaultColour);
            } else { // if everything fails, interpret as regular text without label
                indent = 0;
                attributedString = parsePureText(line, font);
            }
        }
    }

    float getHeightRequired(float width) override
    {
        TextLayout layout;
        layout.createLayout(attributedString, width - indent - gap);
        return layout.getHeight();
    }

    void paint(Graphics& g) override
    {
        label.draw(g, getLocalBounds().withTrimmedLeft(indent).toFloat());
        attributedString.draw(g, getLocalBounds().withTrimmedLeft(indent + gap).toFloat());
    }

    void resized() override
    {
        TextLayout layout;
        layout.createLayout(attributedString, getWidth() - indent - gap);
        updateLinkBounds(layout);
    }

private:
    AttributedString label;
    int indent;
    int gap;
};

class MarkupDisplayComponent : public Component {
public:
    MarkupDisplayComponent()
    {
        // default colour palette (CGA 16 colours with some extensions)
        colours.set("black", "#000");
        colours.set("blue", "#00A");
        colours.set("green", "#0A0");
        colours.set("cyan", "#0AA");
        colours.set("red", "#A00");
        colours.set("magenta", "#A0A");
        colours.set("brown", "#A50");
        colours.set("lightgray", "#AAA");
        colours.set("darkgray", "#555");
        colours.set("lightblue", "#55F");
        colours.set("lightgreen", "#5F5");
        colours.set("lightcyan", "#5FF");
        colours.set("lightred", "#F55");
        colours.set("lightmagenta", "#F5F");
        colours.set("yellow", "#FF5");
        colours.set("white", "#FFF");
        colours.set("orange", "#FA5");
        colours.set("pink", "#F5F");
        colours.set("darkyellow", "#AA0");
        colours.set("purple", "#A0F");
        colours.set("gray", "#777");

        // default font
        font = Fonts::getDefaultFont().withHeight(15);

        // default table backgrounds
        tableBGHeader = Block::parseHexColourStatic(colours["lightcyan"], Colours::black);
        tableBG = Block::parseHexColourStatic(colours["lightgray"], Colours::black);

        // default table margins
        tableMargin = 10;
        tableGap = 2;

        // default list indents
        indentPerSpace = 10;
        labelGap = 10;

        // default content margin
        margin = 20;

        // default admonition margin and sizes
        iconsize = 20;
        admargin = 10;
        adlinewidth = 2;

        // default file source (none)
        fileSource = nullptr;

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&content, false); // we manage the content component
        viewport.setScrollBarsShown(true, false, true, false);
        viewport.setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);
    }

    // clear the background
    void resized() override
    {
        // let's keep the relative vertical position
        double relativeScrollPosition = static_cast<double>(viewport.getViewPositionY()) / content.getHeight();
        // compute content height
        int h = margin;
        for (int i = 0; i < blocks.size(); i++) {
            int bh;
            bh = blocks[i]->getHeightRequired(getWidth() - 2 * margin) + 5; // just to be on the safe side
            if (blocks[i]->canExtendBeyondMargin()) {
                blocks[i]->setBounds(0, h, getWidth(), bh);
            } else {
                blocks[i]->setBounds(margin, h, getWidth() - 2 * margin, bh + 10);
            }
            h += bh;
        }
        // set new bounds
        viewport.setBounds(getLocalBounds());
        content.setBounds(0, 0, getWidth(), h + margin);
        // set vertical scroll position
        int newScrollY = static_cast<int>(relativeScrollPosition * content.getHeight());
        viewport.setViewPosition(0, newScrollY);
    }

    void setFont(Font font) { this->font = font; };
    void setMargin(int m) { margin = m; };
    void setColours(StringPairArray c) { colours = c; };
    void setTableColours(Colour bg, Colour bgHeader)
    {
        tableBG = bg;
        tableBGHeader = bgHeader;
    };
    void setTableMargins(int margin, int gap)
    {
        tableMargin = margin;
        tableGap = gap;
    };
    void setListIndents(int indentPerSpace, int labelGap)
    {
        this->indentPerSpace = indentPerSpace;
        this->labelGap = labelGap;
    };
    void setAdmonitionSizes(int iconsize, int admargin, int adlinewidth)
    {
        this->iconsize = iconsize;
        this->admargin = admargin;
        this->adlinewidth = adlinewidth;
    };

    static String convertFromMarkdown(String md)
    {
        StringArray lines;
        lines.addLines(md);
        String bml;
        bool lastLineWasTable = false;
        for (int li = 0; li < lines.size(); li++) {
            String line = lines[li];
            // replace unspported unordered list markers
            if (line.trimStart().startsWith("* ")) {
                int idx = line.indexOf("* ");
                line = line.substring(0, idx) + "- " + line.substring(idx + 2);
            }
            if (line.trimStart().startsWith("+ ")) {
                int idx = line.indexOf("+ ");
                line = line.substring(0, idx) + "- " + line.substring(idx + 2);
            }
            // replace images
            while (line.contains("![") && line.fromFirstOccurrenceOf("![", false, false).contains("](") && line.fromLastOccurrenceOf("](", false, false).contains(")")) {
                // replace images
                int idx1 = line.indexOf("![");
                int idx2 = line.indexOf(idx1 + 2, "](");
                int idx3 = line.indexOf(idx2 + 2, ")");
                String address = line.substring(idx2 + 2, idx3);
                line = line.substring(0, idx1) + "{{" + address + "}}" + line.substring(idx3 + 2);
            }
            // replace links with labels
            while (line.contains("[") && line.fromFirstOccurrenceOf("[", false, false).contains("](") && line.fromLastOccurrenceOf("](", false, false).contains(")")) {
                // replace links
                int idx1 = line.upToFirstOccurrenceOf("](", false, false).lastIndexOf("[");
                int idx2 = line.indexOf(idx1 + 1, "](");
                int idx3 = line.indexOf(idx2 + 2, ")");
                String text = line.substring(idx1 + 1, idx2);
                String address = line.substring(idx2 + 2, idx3);
                line = line.substring(0, idx1) + "[[" + address + "|" + text + "]]" + line.substring(idx3 + 1);
            }
            // replace links without labels
            while (line.contains("<") && line.fromFirstOccurrenceOf("<", false, false).contains(">") && (line.fromFirstOccurrenceOf("<", false, false).startsWith("http://") || line.fromFirstOccurrenceOf("<", false, false).startsWith("https://") || line.fromFirstOccurrenceOf("<", false, false).startsWith("mailto:"))) {
                // replace links
                int idx1 = line.indexOf("<");
                int idx2 = line.indexOf(idx1 + 1, ">");
                String address = line.substring(idx1 + 1, idx2);
                line = line.substring(0, idx1) + "[[" + address + "]]" + line.substring(idx2 + 1);
            }
            // when in a table, skip lines which look like this : | --- | --- |
            if (!lastLineWasTable || !(line.containsOnly("| -\t") && line.isNotEmpty())) {
                // if we found a table...
                if (line.trim().startsWith("|")) {
                    // ...and this is the first line and the next line is a header separator...
                    if (!lastLineWasTable && li + 1 < lines.size() && lines[li + 1].isNotEmpty() && lines[li + 1].containsOnly("| -\t") && lines[li + 1].contains("-")) {
                        lastLineWasTable = true;       // ...keep track of it...
                        line = line.replace("|", "^"); // ...and make its cells header cells.
                    }
                } else {
                    lastLineWasTable = false; // ...otherwise, keep also track.
                }
                bml += line + (li < lines.size() - 1 ? "\n" : "");
            }
        }
        // replace bold and italic markers
        String tmpBoldMarker = "%%%BarelyML%%%Bold%%%";
        bml = bml.replace("**", tmpBoldMarker);
        bml = bml.replace("__", tmpBoldMarker);
        bml = bml.replace("*", "_");           // replace italic marker
        bml = bml.replace("<sub>", "_");       // replace italic marker
        bml = bml.replace("</sub>", "_");      // replace italic marker
        bml = bml.replace(tmpBoldMarker, "*"); // replace temporary bold marker
        return bml;
    }

    void setMarkupString(String s)
    {
        blocks.clear();

        StringArray lines;
        lines.addLines(s);

        int li = 0; // line index
        while (li < lines.size()) {
            String line = lines[li];
            if (ListItem::isListItem(line)) {    // if we find a list item...
                ListItem* b = new ListItem;      // ...create a new object...
                b->setColours(&colours);         // ...set its colour palette...
                if (Block::containsLink(line)) { // ...and, if there's a link...
                    line = b->consumeLink(line); // ...preprocess line...
                }
                b->parseItemMarkup(line, font, indentPerSpace, labelGap); // ...parse it...
                content.addAndMakeVisible(b);                             // ...add the object to content component...
                blocks.add(b);                                            // ...and the block list...
                li++;                                                     // ...and go to next line.
            } else if (AdmonitionBlock::isAdmonitionLine(line)) {         // if we find an admonition...
                AdmonitionBlock* b = new AdmonitionBlock;                 // ...create a new object...
                b->setColours(&colours);                                  // ...set its colour palette...
                if (Block::containsLink(line)) {                          // ...and, if there's a link...
                    line = b->consumeLink(line);                          // ...preprocess line...
                }
                b->parseAdmonitionMarkup(line, font, iconsize, admargin, adlinewidth); // ...parse it...
                content.addAndMakeVisible(b);                                          // ...add the object to content component...
                blocks.add(b);                                                         // ...and the block list...
                li++;                                                                  // ...and go to next line.
            } else if (ImageBlock::isImageLine(line)) {                                // if we find an image...
                ImageBlock* b = new ImageBlock;                                        // ...create a new object...
                if (Block::containsLink(line)) {                                       // ...and, if there's a link...
                    line = b->consumeLink(line);                                       // ...preprocess line...
                }
                b->parseImageMarkup(line, fileSource);      // ...parse it...
                content.addAndMakeVisible(b);               // ...add the object to content component...
                blocks.add(b);                              // ...and the block list...
                li++;                                       // ...and go to next line.
            } else if (ImageBlock::isHTMLImageLine(line)) { // if we find an image...
                ImageBlock* b = new ImageBlock;             // ...create a new object...
                if (Block::containsLink(line)) {            // ...and, if there's a link...
                    line = b->consumeLink(line);            // ...preprocess line...
                }
                b->parseHTMLImageMarkup(line, fileSource);    // ...parse it...
                content.addAndMakeVisible(b);                 // ...add the object to content component...
                blocks.add(b);                                // ...and the block list...
                li++;                                         // ...and go to next line.
            } else if (TableBlock::isTableLine(line)) {       // if we find a table...
                TableBlock* b = new TableBlock;               // ...create a new object...
                b->setColours(&colours);                      // ...set its colour palette...
                b->setBGColours(tableBG, tableBGHeader);      // ...its background colours...
                b->setMargins(tableMargin, tableGap, margin); // ...and its margins.
                StringArray tlines;                           // set up table lines and while...
                while (TableBlock::isTableLine(line)) {       // ...current line belongs to table...
                    tlines.add(line);                         // ...add it to table lines...
                    line = lines[++li];                       // ...and read next line.
                }
                b->parseMarkup(tlines, font);       // ...parse the collected lines...
                content.addAndMakeVisible(b);       // ...add the object to content component...
                blocks.add(b);                      // ...and the block list.
            } else if (Block::containsLink(line)) { // ...if we got here and there's a link...
                TextBlock* b = new TextBlock();     // ...set up a new text block object...
                b->setColours(&colours);            // ...set its colours...
                line = b->consumeLink(line);        // ...preprocess line...
                b->parseMarkup(line, font);         // ...parse markup...
                content.addAndMakeVisible(b);       // ...add the object to content component...
                blocks.add(b);                      // ...and the block list...
                li++;                               // ...and go to next line.
            } else {                                // otherwise we assume that we have a text block
                StringArray blines;                 // set up text block lines
                bool blockEnd = false;
                while (!ListItem::isListItem(line) &&           // while line is not part of a list...
                    !TableBlock::isTableLine(line) &&           // ...nor a table...
                    !AdmonitionBlock::isAdmonitionLine(line) && // ...nor an admonition...
                    !ImageBlock::isImageLine(line) &&           // ...nor an image...
                    !Block::containsLink(line) &&               // ...and doesn't contain a link...
                    li < lines.size() && !blockEnd) {           // ...and we're not done yet...
                    blines.add(line);                           // ...add line to text block lines...
                    blockEnd = line.isEmpty();                  // ...and set up shouldEndBlock...
                    line = lines[++li];                         // ...read next line...
                    blockEnd &= line.isNotEmpty();              // ...and finish shouldEndBloc...
                }
                TextBlock* b = new TextBlock(); // set up a new text block object...
                b->setColours(&colours);        // ...set its colours...
                b->parseMarkup(blines, font);   // ...parse markup...
                content.addAndMakeVisible(b);   // ...add the object to content component...
                blocks.add(b);                  // ...and the block list.
            }
        }

        resized();
    }
    void setMarkdownString(String md) { setMarkupString(convertFromMarkdown(md)); }

    void setFileSource(FileSource* fs) { fileSource = fs; }

private:
    StringPairArray colours;       // colour palette
    Colour bg;                     // background colour
    Colour tableBG, tableBGHeader; // table background colours
    int tableMargin, tableGap;     // table margins
    int indentPerSpace, labelGap;  // list item indents
    BouncingViewport viewport;     // a viewport to scroll the content
    Component content;             // a component with the content
    OwnedArray<Block> blocks;      // representation of the document as blocks
    int margin;                    // content margin in pixels
    int iconsize;                  // admonition icon size in pixels
    int admargin;                  // admonition margin in pixels
    int adlinewidth;               // admonition line width in pixels
    FileSource* fileSource;        // data source for image files, etc.
    Font font;                     // default font for regular text

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkupDisplayComponent)
};

}
