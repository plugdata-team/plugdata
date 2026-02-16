/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "OfflineObjectRenderer.h"
#include <juce_cryptography/juce_cryptography.h>
#include "Constants.h"
#include "PluginEditor.h"

#include "Pd/Interface.h"
#include "Pd/Patch.h"
#include "Objects/AllGuis.h"
#include <g_all_guis.h>

#include "Utility/Fonts.h"
#include "Utility/CachedStringWidth.h"
#include "Objects/ObjectBase.h"
#include "PluginProcessor.h"
#include "Objects/IEMHelper.h"
#include "Objects/CanvasObject.h"


ImageWithOffset OfflineObjectRenderer::patchToMaskedImage(String const& patch, float const scale, bool const makeInvalidImage)
{
    auto const image = patchToTempImage(patch, scale);
    auto const width = image.image.getWidth();
    auto const height = image.image.getHeight();
    auto const output = Image(Image::ARGB, width, height, true);

    Graphics g(output);
    g.reduceClipRegion(image.image, AffineTransform());
    auto const backgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);
    g.fillAll(backgroundColour);

    if (makeInvalidImage) {
        AffineTransform rotate;
        rotate = rotate.rotated(MathConstants<float>::pi / 4.0f);
        g.addTransform(rotate);
        float const diagonalLength = std::sqrt(width * width + height * height);
        g.setColour(backgroundColour.darker(3.0f));
        constexpr auto stripeWidth = 20.0f;
        for (float x = -diagonalLength; x < diagonalLength; x += stripeWidth * 2) {
            g.fillRect(x, -diagonalLength, stripeWidth, diagonalLength * 2);
        }
        g.addTransform(rotate.inverted());
    }

    return ImageWithOffset(output, image.offset);
}

bool OfflineObjectRenderer::parseGraphSize(String const& objectText, Rectangle<int>& bounds)
{
    auto const patchName = objectText.upToFirstOccurrenceOf(" ", false, false).upToFirstOccurrenceOf(";", false, false).upToFirstOccurrenceOf("\\", false, false);
    if (patchName.isEmpty())
        return false;

    auto const patchFile = pd::Library::findPatch(patchName);
    if (!patchFile.existsAsFile())
        return false;

    auto const patchAsString = patchFile.loadFileAsString();
    auto const lines = StringArray::fromLines(patchAsString.trim());
    if (lines.size()) {
        auto const graphCoordsLine = lines[lines.size() - 1];
        auto const tokens = StringArray::fromTokens(graphCoordsLine, true);

        if (tokens[0] == "#X" && tokens[1] == "coords" && tokens.size() >= 8 && tokens[6].containsOnly("-0123456789") && tokens[7].containsOnly("-0123456789")) {
            bounds = bounds.withSize(tokens[6].getIntValue(), tokens[7].getIntValue());
            return true;
        }
    }

    return false;
}

void OfflineObjectRenderer::parsePatch(String const& patch, std::function<void(PatchItemType, int, String const&)> callback)
{
    int canvasDepth = patch.startsWith("#N canvas") ? -1 : 0;

    auto isComment = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "text" && tokens.size() >= 4 && tokens[1] != "f" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };
    auto isMessage = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "msg" && tokens.size() >= 4 && tokens[1] != "f" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };
    auto isObject = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens.size() >= 4 && tokens[1] != "f" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isConnection = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "connect" && tokens[2].containsOnly("0123456789") && tokens[3].containsOnly("0123456789") && tokens[4].containsOnly("0123456789") && tokens[5].containsOnly("0123456789");
    };

    auto isStartingCanvas = [](StringArray const& tokens) {
        return tokens[0] == "#N" && tokens[1] == "canvas" && tokens.size() >= 6 && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
    };

    auto isEndingCanvas = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "restore" && tokens.size() >= 4 && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isGraphCoords = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "coords" && tokens.size() >= 7 && tokens[5].containsOnly("-0123456789") && tokens[6].containsOnly("-0123456789");
    };

    Rectangle<int> nextGraphCoords;
    String canvasName;
    bool hasGraphCoords = false;
    for (auto& line : StringArray::fromLines(patch)) {

        line = line.upToLastOccurrenceOf(";", false, false);

        auto tokens = StringArray::fromTokens(line, true);

        if (isStartingCanvas(tokens)) {
            if (tokens.size() > 6)
                canvasName = tokens[6];

            callback(CanvasStart, canvasDepth, "");
            canvasDepth++;
        }

        if (isComment(tokens)) {
            callback(Comment, canvasDepth, line);
        } else if (isMessage(tokens)) {
            callback(Message, canvasDepth, line);
        } else if (isObject(tokens)) {
            callback(Object, canvasDepth, line);
        } else if (isConnection(tokens)) {
            callback(Connection, canvasDepth, line);
        }

        if (isGraphCoords(tokens)) {
            callback(GraphCoords, canvasDepth, "");
            nextGraphCoords = Rectangle<int>(tokens[6].getIntValue(), tokens[7].getIntValue());
            hasGraphCoords = true;
        }

        if (isEndingCanvas(tokens)) {
            callback(CanvasEnd, canvasDepth, "");
            if (hasGraphCoords) {
                callback(Object, canvasDepth, line + " " + String(nextGraphCoords.getWidth()) + " " + String(nextGraphCoords.getHeight()));
                hasGraphCoords = false;
            } else {
                callback(Object, canvasDepth, line + " " + String(canvasName.length() * 12) + " 24");
            }
            canvasDepth--;
        }
    }
}

SmallArray<Rectangle<int>> OfflineObjectRenderer::getObjectBoundsForPatch(String const& patch)
{
    SmallArray<Rectangle<int>> objectBounds;

    parsePatch(patch, [&objectBounds](PatchItemType const type, int const depth, String const& text) {
        if ((type != PatchItemType::Object && type != PatchItemType::Message && type != PatchItemType::Comment) || depth != 0)
            return;

        auto tokens = StringArray::fromTokens(text, true);

        if ((tokens[1] == "floatatom" || tokens[1] == "symbolatom" || tokens[1] == "listatom") && tokens.size() > 11) {
            auto const height = tokens[11].getIntValue();
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue() * sys_fontwidth(height) + 3, (height == 0 ? 12 : height) + 7));
            return;
        }

        if (tokens[1] == "text") {
            StringArray textString;
            textString.addArray(tokens, 4, tokens.size() - 2 - 4);

            int textAreaWidth = 0;
            int lines = 1;

            // calcuate the length of the text string:
            // if char number is specified, then use that
            // if it's not, then it's auto sizing, which is max of 92 chars, or min of the text length
            if (tokens[tokens.size() - 2] == "f") {
                textAreaWidth = tokens[tokens.size() - 1].getIntValue() * 8;
            } else {
                int autoWidth = 0;
                for (auto const& text : textString) {
                    autoWidth += CachedStringWidth<15>::calculateStringWidth(text + " ");
                }
                textAreaWidth = jmin(92 * 8, autoWidth);
            }

            int wordsInLine = 1;
            int lineWidth = 0;
            int wordIdx = 0;
            while (wordIdx < textString.size()) {
                lineWidth += CachedStringWidth<15>::calculateStringWidth(textString[wordIdx] + " ");
                if (lineWidth > textAreaWidth) {
                    if (wordsInLine == 1) {
                        break;
                    }
                    lines++;
                }
                wordIdx++;
                wordsInLine++;
            }
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), textAreaWidth, lines * 12));
            return;
        }
        switch (hash(tokens[4])) {
        case hash("restore"): {
            if (tokens.size() < 6)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("bng"):
        case hash("tgl"):
        case hash("knob"): {
            if (tokens.size() < 6)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("vradio"): {
            if (tokens.size() < 9)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[5].getIntValue() * tokens[8].getIntValue()));
            break;
        }
        case hash("hradio"): {
            if (tokens.size() < 9)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue() * tokens[8].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("numbox~"):
        case hash("cnv"): {
            if (tokens.size() < 8)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[6].getIntValue(), tokens[7].getIntValue()));
            break;
        }
        case hash("graph"):
        case hash("vu"):
        case hash("hsl"):
        case hash("vsl"):
        case hash("scope~"):
        case hash("function"):
        case hash("button"):
        case hash("bicoeff"):
        case hash("messbox"):
        case hash("pad"):
        case hash("slider"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[6].getIntValue()));
            break;
        }
        case hash("nbx"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue() * 12, tokens[6].getIntValue()));
            break;
        }
        case hash("keyboard"): {
            if (tokens.size() < 8)
                break;

            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue() * (tokens[7].getIntValue() * 7), tokens[6].getIntValue()));
            break;
        }
        case hash("pic"):
        case hash("note"): {
            // TODO: implement these
            break;
        }
        default: {
            if (tokens.size() < 4)
                break;

            auto bounds = Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), 0, 23);

            tokens.removeRange(0, 4);
            auto const text = tokens.joinIntoString(" ");
            auto const wasGraph = parseGraphSize(text, bounds);

            if (!wasGraph) {
                if (text.contains(", f")) {
                    bounds = bounds.withWidth(text.fromFirstOccurrenceOf("f", false, false).getIntValue() * 8 + 11);
                } else {
                    bounds = bounds.withWidth(CachedStringWidth<15>::calculateStringWidth(text) + 11);
                }
            }

            objectBounds.add(bounds);
            break;
        }
        }
    });

    return objectBounds;
}

String OfflineObjectRenderer::patchToSVG(String const& patch)
{
    auto objectRects = getObjectBoundsForPatch(patch);

    String svgContent;
    auto regionOfInterest = Rectangle<int>();
    for (auto& b : objectRects) {
        regionOfInterest = regionOfInterest.getUnion(b.reduced(Object::margin));
    }

    for (auto& b : objectRects) {
        auto rect = b - regionOfInterest.getPosition();
        svgContent += String::formatted(
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%.1f\" ry=\"%.1f\" />\n",
            rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), Corners::objectCornerRadius, Corners::objectCornerRadius);
    }

    return "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n" + svgContent + "</svg>";
}

ImageWithOffset OfflineObjectRenderer::patchToTempImage(String const& patch, float const scale)
{
    static UnorderedMap<String, ImageWithOffset> patchImageCache;

    auto const patchSHA256 = SHA256(patch.getCharPointer()).toHexString();
    if (patchImageCache.contains(patchSHA256)) {
        return patchImageCache[patchSHA256];
    }

    auto objectRects = getObjectBoundsForPatch(patch);
    Rectangle<int> totalSize;

    for (auto& rect : objectRects) {
        totalSize = rect.getUnion(totalSize);
    }

    // apply the top left offset to all rects
    for (auto& rect : objectRects) {
        rect.translate(-totalSize.getX(), -totalSize.getY());
    }
    auto const size = Point<int>(totalSize.getWidth(), totalSize.getHeight());
    Image const image(Image::ARGB, totalSize.getWidth() * scale, totalSize.getHeight() * scale, true);
    Graphics g(image);
    g.addTransform(AffineTransform::scale(scale));
    g.setColour(Colours::white);
    for (auto& rect : objectRects) {
        if (ProjectInfo::canUseSemiTransparentWindows()) {
            g.fillRoundedRectangle(rect.toFloat(), 5.0f);
        } else {
            g.fillRect(rect);
        }
    }

    auto output = ImageWithOffset(image, size);
    patchImageCache.emplace(patchSHA256, output);
    return output;
}

bool OfflineObjectRenderer::checkIfPatchIsValid(String const& patch)
{
    // Split the patch into lines
    auto lines = StringArray::fromLines(patch);

    for (auto const& line : lines) {
        if (line.startsWith("#X") || line.startsWith("#N") || line.startsWith("#A") || !line.containsNonWhitespaceChars())
            continue;

        return false;
    }

    return true;
}

std::pair<SmallArray<bool>, SmallArray<bool>> OfflineObjectRenderer::countIolets(String const& patch)
{
    static UnorderedMap<String, std::pair<SmallArray<bool>, SmallArray<bool>>> patchIoletCache;

    auto const patchSHA256 = SHA256(patch.getCharPointer()).toHexString();
    if (patchIoletCache.contains(patchSHA256)) {
        return patchIoletCache[patchSHA256];
    }

    auto const trimmedPatch = patch.trim();
    bool const onlyOneObject = StringArray::fromLines(trimmedPatch).size() == 1;

    SmallArray<bool> inlets, outlets;

    if (onlyOneObject) {
        auto const tokens = StringArray::fromTokens(trimmedPatch, true);
        if (tokens.size() >= 5) {
            auto const objectText = tokens[4].trimCharactersAtEnd(";");
            auto const patchFile = pd::Library::findPatch(objectText);
            if (!patchFile.existsAsFile())
                return { { false }, { false } };

            auto const patchAsString = patchFile.loadFileAsString();
            parsePatch(patchAsString, [&inlets, &outlets](PatchItemType const type, int const depth, String const& text) {
                if (type == Object && depth == 0) {
                    auto tokens = StringArray::fromTokens(text.trim(), true);
                    if (tokens.size() >= 5) {
                        auto const& name = tokens.getReference(4);
                        if (name.startsWith("inlet~"))
                            inlets.add(true);
                        else if (name.startsWith("inlet"))
                            inlets.add(false);
                        else if (name.startsWith("outlet~"))
                            outlets.add(true);
                        else if (name.startsWith("outlet"))
                            outlets.add(false);
                    }
                }
            });
        }
    } else {
        parsePatch(trimmedPatch, [&inlets, &outlets](PatchItemType const type, int const depth, String const& text) {
            if (type == Object && depth == 1) {
                auto tokens = StringArray::fromTokens(text.trim(), true);
                if (tokens.size() >= 5) {
                    auto const& objectText = tokens.getReference(4);
                    if (objectText.startsWith("inlet~"))
                        inlets.add(true);
                    else if (objectText.startsWith("inlet"))
                        inlets.add(false);
                    else if (objectText.startsWith("outlet~"))
                        outlets.add(true);
                    else if (objectText.startsWith("outlet"))
                        outlets.add(false);
                }
            }
        });
    }

    auto result = std::pair<SmallArray<bool>, SmallArray<bool>> { inlets, outlets };
    patchIoletCache.emplace(patchSHA256, result);

    return result;
}
