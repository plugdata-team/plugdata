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

        auto* objectPtr = pd::Interface::checkObject(object);

        char *objectText;
        int len;
        pd::Interface::getObjectText(objectPtr, &objectText, &len);
        const auto objectTextString = String::fromUTF8(objectText, len);

        String type = String::fromUTF8(object->g_pd->c_name->s_name);
        const auto objectNameHash = hash(type);

        std::cout << "object type: " << type << std::endl;

        switch(objectNameHash){
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
            case hash("canvas"):
            case hash("pic"):
            case hash("keyboard"):
            case hash("scope~"):
            case hash("function"):
            case hash("note"):
            case hash("knob"):
            case hash("gatom"):
            case hash("button"):
            case hash("graph"):
            case hash("bicoeff"):
            case hash("messbox"):
            case hash("pad"):
            {
                break;
            }
            case hash("cnv"): {
                auto cnvObject = ((t_my_canvas*)object);
                obj_w = cnvObject->x_vis_w;
                obj_h = cnvObject->x_vis_h;
                break;
            }
            case hash("message"):
            case hash("comment"):
            case hash("text"): {
                StringArray lines;
                lines.addTokens(objectTextString, ";", "");
                auto charWidth = objectPtr->te_width;
                // charWidth of 0 == auto width, which means we need to calculate the width manually :facepalm:
                obj_w = jmax(Font(16).getStringWidth(objectTextString), charWidth * 7);
                obj_h = jmax(20, 20 * lines.size());

                if ((objectNameHash == hash("message")) || (((t_fake_text_define *) object)->x_textbuf.b_ob.te_type == T_OBJECT)) {
                    obj_w = 45;
                }
                break;
            }
            default:
            {
                obj_h = 20;
                if (objectPtr->te_width == 0) {
                    std::cout << "object text is: " << objectTextString << std::endl;
                    obj_w = Font(16).getStringWidth(objectTextString);
                }
            }
            break;
        }

        auto maxIolets = jmax<int>(pd::Interface::numOutlets(objectPtr), pd::Interface::numInlets(objectPtr));
        auto maxSize = jmax<int>(maxIolets * 18, obj_w);
        rect.setBounds(obj_x, obj_y, maxSize, obj_h);

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
        if(ProjectInfo::canUseSemiTransparentWindows()) {
            g.fillRoundedRectangle(rect.toFloat(), 5.0f);
        }
        else {
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
