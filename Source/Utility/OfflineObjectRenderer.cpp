/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "OfflineObjectRenderer.h"
#include "Constants.h"
#include "PluginEditor.h"

#include "Pd/Interface.h"
#include "Pd/Patch.h"

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

ImageWithOffset OfflineObjectRenderer::patchToTempImage(String const& patch, float scale)
{
    pd->setThis();

    sys_lock();
    pd->muteConsole(true);

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
        auto maxIolets = jmax<int>(pd::Interface::numOutlets(objectPtr), pd::Interface::numInlets(objectPtr));
        // ALEX TODO: fix this heuristic, it doesn't work well for everything
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
    g.setColour(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
    for (auto& rect : objectRects) {
        g.fillRoundedRectangle(rect.toFloat(), 5.0f);
    }
    // ALEX TODO we shouldn't apply alpha here! Do it in the zoomableDragAndDropContainer
    image.multiplyAllAlphas(0.3f);

    return ImageWithOffset(image, size);
}

bool OfflineObjectRenderer::checkIfPatchIsValid(String const& patch)
{
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

    return std::make_pair(inlets, outlets);
}
