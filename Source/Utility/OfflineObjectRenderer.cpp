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

#include "Objects/ObjectBase.h"
#include "PluginProcessor.h"
#include "Objects/IEMHelper.h"
#include "Objects/CanvasObject.h"

OfflineObjectRenderer::OfflineObjectRenderer(pd::Instance* instance)
    : pd(instance)
{
    pd->setThis();

    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(pd::Instance::defaultPatch);
    String dirname = patchFile.getParentDirectory().getFullPathName().replace("\\", "/");
    auto const* dir = dirname.toRawUTF8();

    String filename = patchFile.getFileName();
    auto const* file = filename.toRawUTF8();

    offlineCnv = static_cast<t_canvas*>(pd::Interface::createCanvas(file, dir));
}

OfflineObjectRenderer::~OfflineObjectRenderer() = default;

OfflineObjectRenderer* OfflineObjectRenderer::findParentOfflineObjectRendererFor(Component* childComponent)
{
    return childComponent != nullptr ? &childComponent->findParentComponentOfClass<PluginEditor>()->offlineRenderer : nullptr;
}

ImageWithOffset OfflineObjectRenderer::patchToMaskedImage(String const& patch, float scale, bool makeInvalidImage)
{
    auto image = patchToTempImage(patch, scale);
    auto width = image.image.getWidth();
    auto height = image.image.getHeight();
    auto output = Image(Image::ARGB, width, height, true);

    Graphics g(output);
    g.reduceClipRegion(image.image, AffineTransform());
    auto backgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);
    g.fillAll(backgroundColour);

    if (makeInvalidImage) {
        AffineTransform rotate;
        rotate = rotate.rotated(MathConstants<float>::pi / 4.0f);
        g.addTransform(rotate);
        float diagonalLength = std::sqrt(width * width + height * height);
        g.setColour(backgroundColour.darker(3.0f));
        auto stripeWidth = 20.0f;
        for (float x = -diagonalLength; x < diagonalLength; x += (stripeWidth * 2)) {
            g.fillRect(x, -diagonalLength, stripeWidth, diagonalLength * 2);
        }
        g.addTransform(rotate.inverted());
    }

    return ImageWithOffset(output, image.offset);
}

String OfflineObjectRenderer::patchToSVGFast(String const& patch)
{
    int canvasDepth = -1;

    auto isObject = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens[1] != "f" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isStartingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
    };

    auto isEndingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isGraphCoords = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] == "coords" && tokens[5].containsOnly("-0123456789") && tokens[6].containsOnly("-0123456789");
    };

    StringArray objects;
    Rectangle<int> nextGraphCoords;
    String canvasName;
    bool hasGraphCoords = false;
    for (auto& line : StringArray::fromLines(patch)) {

        line = line.upToLastOccurrenceOf(";", false, false);

        auto tokens = StringArray::fromTokens(line, true);

        if (isStartingCanvas(tokens)) {
            if (tokens.size() > 6)
                canvasName = tokens[6];
            canvasDepth++;
        }

        if (canvasDepth == 0 && isObject(tokens)) {
            objects.add(line);
        }

        if (isGraphCoords(tokens) && tokens.size() > 6) {
            nextGraphCoords = Rectangle<int>(tokens[5].getIntValue(), tokens[6].getIntValue());
            hasGraphCoords = true;
        }

        if (isEndingCanvas(tokens)) {
            if (canvasDepth == 1) {
                if (hasGraphCoords) {
                    objects.add(line + " " + String(nextGraphCoords.getWidth()) + " " + String(nextGraphCoords.getHeight()));
                    hasGraphCoords = false;
                } else {
                    objects.add(line + " " + String(canvasName.length() * 12) + " 24");
                }
            }
            canvasDepth--;
        }
    }

    Array<Rectangle<int>> objectBounds;
    for (auto& object : objects) {
        auto tokens = StringArray::fromTokens(object, true);

        if ((tokens[1] == "floatatom" || tokens[1] == "symbolatom" || tokens[1] == "listatom") && tokens.size() > 11) {
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue() * 8, tokens[11].getIntValue()));
            continue;
        }

        if (tokens[1] == "text") {
            auto fontMetrics = Fonts::getCurrentFont().withHeight(15);
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
                for (auto text : textString) {
                    autoWidth += fontMetrics.getStringWidth(text + " ");
                }
                textAreaWidth = jmin(92 * 8, autoWidth);
            }

            int wordsInLine = 1;
            int lineWidth = 0;
            int wordIdx = 0;
            while (wordIdx < textString.size()) {
                lineWidth += fontMetrics.getStringWidth(textString[wordIdx] + " ");
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
            continue;
        }
        switch (hash(tokens[4])) {
        case hash("restore"): {
            if (tokens.size() < 6)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("bng"):
        case hash("tgl"): {
            if (tokens.size() < 6)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("vradio"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[5].getIntValue() * tokens[6].getIntValue()));
            break;
        }
        case hash("hradio"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue() * tokens[6].getIntValue(), tokens[5].getIntValue()));
            break;
        }
        case hash("cnv"): {
            if (tokens.size() < 8)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[6].getIntValue(), tokens[7].getIntValue()));
            break;
        }
        case hash("vu"):
        case hash("hsl"):
        case hash("vsl"):
        case hash("slider"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue(), tokens[6].getIntValue()));
            break;
        }
        case hash("nbx"): {
            if (tokens.size() < 7)
                break;
            objectBounds.add(Rectangle<int>(tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[5].getIntValue() * 8, tokens[6].getIntValue()));
            break;
        }
        /* // TODO: implement all of these!
        case hash("numbox~"):
        {
            break;
        }
        case hash("pic"):
        case hash("keyboard"):
        case hash("scope~"):
        case hash("function"):
        case hash("knob"):
        case hash("gatom"):
        case hash("button"):
        case hash("bicoeff"):
        case hash("messbox"):
        case hash("pad"):
        {

            break;
        }
        case hash("note"):
        {

            break;
        }
         */
        default: {
            if (tokens.size() < 4)
                break;
            auto x = tokens[2].getIntValue();
            auto y = tokens[3].getIntValue();
            tokens.removeRange(0, 4);
            objectBounds.add(Rectangle<int>(x, y, tokens.joinIntoString(" ").length() * 8, 24));
            break;
        }
        }
    }

    String svgContent;
    auto regionOfInterest = Rectangle<int>();
    for (auto& b : objectBounds) {
        regionOfInterest = regionOfInterest.getUnion(b.reduced(Object::margin));
    }

    for (auto& b : objectBounds) {
        auto rect = b - regionOfInterest.getPosition();
        svgContent += String::formatted(
            "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%.1f\" ry=\"%.1f\" />\n",
            rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), Corners::objectCornerRadius, Corners::objectCornerRadius);
    }

    return "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n" + svgContent + "</svg>";
}

ImageWithOffset OfflineObjectRenderer::patchToTempImage(String const& patch, float scale)
{
    static std::unordered_map<String, ImageWithOffset> patchImageCache;

    auto const patchSHA256 = SHA256(patch.getCharPointer()).toHexString();
    if (patchImageCache.contains(patchSHA256)) {
        return patchImageCache[patchSHA256];
    }

    pd->setThis();

    sys_lock();
    pd->muteConsole(true);

    canvas_create_editor(offlineCnv);

    objectRects.clear();
    totalSize.setBounds(0, 0, 0, 0);
    int obj_x, obj_y, obj_w, obj_h;
    auto rect = Rectangle<int>();
    pd::Interface::paste(offlineCnv, stripConnections(patch).toRawUTF8());

    // traverse the linked list of objects, asking PD the object size each time
    auto object = offlineCnv->gl_list;
    while (object) {
        pd::Interface::getObjectBounds(offlineCnv, object, &obj_x, &obj_y, &obj_w, &obj_h);

        obj_w = obj_w + 1;
        obj_h = obj_h + 1;

        auto const padding = 5;

        auto* objectPtr = pd::Interface::checkObject(object);

        char* objectText;
        int len;
        pd::Interface::getObjectText(objectPtr, &objectText, &len);
        auto const objectTextString = String::fromUTF8(objectText, len);

        String type = String::fromUTF8(pd::Interface::getObjectClassName(&object->g_pd));
        auto const typeHash = hash(type);

        switch (typeHash) {
        case hash("bng"):
        case hash("hsl"):
        case hash("vsl"):
        case hash("slider"):
        case hash("tgl"):
        case hash("nbx"):
        case hash("numbox~"):
        case hash("vradio"):
        case hash("hradio"):
        case hash("vu"):
        case hash("pic"):
        case hash("keyboard"):
        case hash("scope~"):
        case hash("function"):
        case hash("knob"):
        case hash("gatom"):
        case hash("button"):
        case hash("graph"):
        case hash("bicoeff"):
        case hash("messbox"):
        case hash("pad"): {
            // use the bounds obtained from pd::Interface::getObjectBounds()
            break;
        }
        case hash("note"): {
            auto noteObject = (t_fake_note*)object;

            // if note object isn't init, initialize because text buf will be empty
            if (noteObject->x_init == 0) {
                auto notePtr = (t_pd*)object;
                (*notePtr)->c_wb->w_visfn((t_gobj*)noteObject, offlineCnv, 1);
            }
            obj_w = noteObject->x_max_pixwidth;
            auto const noteText = String::fromUTF8(noteObject->x_buf, noteObject->x_bufsize).trim().replace("\\,", ",").replace("\\;", ";");
            auto const fontName = String::fromUTF8(noteObject->x_fontname->s_name);
            auto const fontSize = noteObject->x_fontsize;

            Font fontMetrics;

            if (fontName.isEmpty() || fontName == "Inter") {
                fontMetrics = Fonts::getVariableFont().withStyle(Font::plain).withHeight(fontSize);
            } else {
                fontMetrics = Font(fontName, fontSize, Font::plain);
            }

            int lineLength = 0;
            int lineChars = 0;
            int lineCount = 1;
            int charIndex = 0;

            auto const objReducedPadding = obj_w - 3;

            while (charIndex < noteText.length()) {
                auto charLength = fontMetrics.getStringWidth(noteText.substring(charIndex, charIndex + 1));
                lineLength += charLength;
                lineChars++;
                if (lineLength > objReducedPadding) {
                    if (lineChars == 1) {
                        charIndex++;
                    }
                    lineCount++;
                    lineLength = 0;
                    lineChars = 0;
                } else {
                    charIndex++;
                }
            }
            obj_h = lineCount * fontMetrics.getHeight();
            break;
        }
        case hash("canvas"): {
            if (((t_canvas*)object)->gl_isgraph == 1) {
                break;
            }
            StringArray lines;
            lines.addTokens(objectTextString, ";", "");
            auto charWidth = objectPtr->te_width;
            obj_w = jmax<int>(Font(16).getStringWidth(objectTextString), charWidth * 7);
            obj_h = jmax<int>(20, 20 * lines.size());
            break;
        }
        case hash("cnv"): {
            // example of how we could get the size from the objects themselves via static function
            // in which case we would get the whole bounds
            // this way all object dimension calculations would be in the same place
            auto const cnvBounds = CanvasObject::getPDSize((t_my_canvas*)object);
            obj_w = cnvBounds.getWidth();
            obj_h = cnvBounds.getHeight();
            break;
        }
        case hash("message"):
        case hash("comment"):
        case hash("text"): {
            StringArray lines;
            lines.addTokens(objectTextString, ";", "");
            auto charWidth = objectPtr->te_width;
            // charWidth of 0 == auto width, which means we need to calculate the width manually
            obj_w = jmax<int>(Font(15).getStringWidth(objectTextString), charWidth * 7) + padding;
            obj_h = 20 * jmax<int>(1, lines.size());

            if ((typeHash == hash("message")) || (((t_fake_text_define*)object)->x_textbuf.b_ob.te_type == T_OBJECT)) {
                if (objectPtr->te_width == 0) {
                    obj_w = jmax<int>(42, Font(15).getStringWidth(objectTextString) + padding);
                } else {
                    obj_w = (objectPtr->te_width * 7) + padding;
                }
            }
            break;
        }
        default: {
            obj_h = 20;
            if (objectPtr->te_width == 0) {
                obj_w = Font(15).getStringWidth(objectTextString) + padding;
            } else {
                obj_w = (objectPtr->te_width * 7);
            }
            auto maxIolets = jmax<int>(pd::Interface::numOutlets(objectPtr), pd::Interface::numInlets(objectPtr));
            obj_w = jmax<int>((maxIolets * 18) + padding, obj_w);
            break;
        }
        }

        rect.setBounds(obj_x, obj_y, obj_w, obj_h);

        // put the object bounds into the rect list, and also calculate the total size of all objects
        objectRects.add(rect);
        totalSize = totalSize.getUnion(rect);

        // save the pointer to the next object
        auto nextObject = object->g_next;
        // delete the current object from the canvas after we have read its dimensions
        pd::Interface::removeObjects(offlineCnv, { object });
        // move to the next object in the linked list
        object = nextObject;
    }

    pd->muteConsole(false);
    sys_unlock();

    // apply the top left offset to all rects
    for (auto& rect : objectRects) {
        rect.translate(-totalSize.getX(), -totalSize.getY());
    }
    auto size = Point<int>(totalSize.getWidth(), totalSize.getHeight());
    Image image(Image::ARGB, totalSize.getWidth() * scale, totalSize.getHeight() * scale, true);
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
    static std::unordered_map<String, bool> patchValidCache;

    auto const patchSHA256 = SHA256(patch.getCharPointer()).toHexString();
    if (patchValidCache.contains(patchSHA256)) {
        return patchValidCache[patchSHA256];
    }

    pd->setThis();

    sys_lock();
    pd->muteConsole(true);

    bool isValid = false;
    pd::Interface::paste(offlineCnv, stripConnections(patch).toRawUTF8());

    // if we can create more than 1 valid object, assume the patch is valid
    auto object = offlineCnv->gl_list;
    while (object) {
        isValid = true;

        auto nextObject = object->g_next;
        pd::Interface::removeObjects(offlineCnv, { object });
        object = nextObject;
    }

    pd->muteConsole(false);
    sys_unlock();

    patchValidCache.emplace(patchSHA256, isValid);
    return isValid;
}

// Remove all connections from the PD patch, so that it can't activate loadbangs etc
String OfflineObjectRenderer::stripConnections(String const& patch)
{
    StringArray lines;
    lines.addTokens(patch, "\n", StringRef());
    for (int i = lines.size() - 1; i >= 0; --i) {
        if (lines[i].startsWith("#X connect"))
            lines.remove(i);
    }

    String strippedPatch;

    for (auto const& line : lines) {
        strippedPatch += line + "\n";
    }

    return strippedPatch;
}

std::pair<std::vector<bool>, std::vector<bool>> OfflineObjectRenderer::countIolets(String const& patch)
{
    static std::unordered_map<String, std::pair<std::vector<bool>, std::vector<bool>>> patchIoletCache;

    auto const patchSHA256 = SHA256(patch.getCharPointer()).toHexString();
    if (patchIoletCache.contains(patchSHA256)) {
        return patchIoletCache[patchSHA256];
    }

    std::vector<bool> inlets;
    std::vector<bool> outlets;
    pd->setThis();

    sys_lock();
    pd->muteConsole(true);
    pd::Interface::paste(offlineCnv, stripConnections(patch).toRawUTF8());

    if (auto* object = reinterpret_cast<t_object*>(offlineCnv->gl_list)) {
        int numIn = pd::Interface::numInlets(object);
        int numOut = pd::Interface::numOutlets(object);
        for (int i = 0; i < numIn; i++) {
            inlets.push_back(pd::Interface::isSignalInlet(object, i));
        }
        for (int i = 0; i < numOut; i++) {
            outlets.push_back(pd::Interface::isSignalOutlet(object, i));
        }
    }

    glist_clear(offlineCnv);

    pd->muteConsole(false);
    sys_unlock();

    auto output = std::make_pair(inlets, outlets);
    patchIoletCache.emplace(patchSHA256, output);
    return output;
}
