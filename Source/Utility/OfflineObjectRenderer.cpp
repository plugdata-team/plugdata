#include "OfflineObjectRenderer.h"
#include "Constants.h"
#include "Utility/HashUtils.h"

#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"

#include "Pd/Patch.h"
#include "Pd/Instance.h"

class OfflineObjectRenderer::OfflineObjectRendererComponent : public Component {
public:
    OfflineObjectRendererComponent()
    {
        auto patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(pd::Instance::defaultPatch);
        String dirname = patchFile.getParentDirectory().getFullPathName().replace("\\", "/");
        auto const* dir = dirname.toRawUTF8();

        String filename = patchFile.getFileName();
        auto const* file = filename.toRawUTF8();

        offlineCnv = static_cast<t_canvas*>(libpd_create_canvas(file, dir));

        // as long as we don't call `canvas_create_editor(offlineCnv)`,
        // it will not be added to the audio graph
        // however we can ask the canvas how large added objects are, how many iolets etc
    }

    ~OfflineObjectRendererComponent() override { }

    bool checkIfPatchIsValid(String const& patch)
    {
        bool isValid = false;
        libpd_paste(offlineCnv, patch.toRawUTF8());

        // if we can create more than 1 valid object, assume the patch is valid
        auto object = offlineCnv->gl_list;
        while (object) {
            isValid = true;

            auto nextObject = object->g_next;
            libpd_removeobj(offlineCnv, object);
            object = nextObject;
        }
        return isValid;
    }

    ImageWithOffset patchToTempImage(String const& patch)
    {
        objectRects.clear();
        totalSize.setBounds(0, 0, 0, 0);
        int obj_x, obj_y, obj_w, obj_h;
        auto rect = Rectangle<int>();
        libpd_paste(offlineCnv, patch.toRawUTF8());

        // traverse the linked list of objects, asking PD the object size each time
        auto object = offlineCnv->gl_list;
        while (object) {
            libpd_get_object_bounds(offlineCnv, object, &obj_x, &obj_y, &obj_w, &obj_h);
            auto objectData = static_cast<t_object*>(static_cast<void*>(object));
            auto maxIolets = jmax<int>(libpd_noutlets(objectData), libpd_ninlets(objectData));
            // ALEX TODO: fix this heuristic, it doesn't work well for everything
            auto maxSize = jmax<int>(maxIolets * 18, obj_w);
            rect.setBounds(obj_x, obj_y, maxSize, obj_h);

            // put the object bounds into the rect list, and also calculate the total size of all objects
            objectRects.add(rect);
            totalSize = totalSize.getUnion(rect);

            // save the pointer to the next object
            auto nextObject = object->g_next;
            // delete the current object from the canvas after we have read its dimensions
            libpd_removeobj(offlineCnv, object);
            // move to the next object in the linked list
            object = nextObject;
        }

        // apply the top left offset to all rects
        for (auto& rect : objectRects) {
            rect.translate(-totalSize.getX(), -totalSize.getY());
        }
        auto size = Point<int>(totalSize.getWidth(), totalSize.getHeight());
        Image image(Image::ARGB, totalSize.getWidth(), totalSize.getHeight(), true);
        Graphics g(image);
        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
        for (auto& rect : objectRects) {
            g.fillRoundedRectangle(rect.toFloat(), 5.0f);
        }
        // ALEX TODO we shouldn't apply alpha here! Do it in the zoomableDragAndDropContainer
        image.multiplyAllAlphas(0.3f);
        return ImageWithOffset(image, size);
    }

private:
    Array<Rectangle<int>> objectRects;
    Rectangle<int> totalSize;
    _glist* offlineCnv = nullptr;
};

OfflineObjectRenderer::OfflineObjectRenderer()
{
    offlineObjectRendererComponent = std::make_unique<OfflineObjectRendererComponent>();
}

OfflineObjectRenderer::~OfflineObjectRenderer() = default;

OfflineObjectRenderer* OfflineObjectRenderer::findParentOfflineObjectRendererFor(Component* childComponent)
{
    return childComponent != nullptr ? childComponent->findParentComponentOfClass<OfflineObjectRenderer>() : nullptr;
}

ImageWithOffset OfflineObjectRenderer::patchToTempImage(String const& patch)
{
    return offlineObjectRendererComponent->patchToTempImage(patch);
}

bool OfflineObjectRenderer::checkIfPatchIsValid(String const& patch)
{
    return offlineObjectRendererComponent->checkIfPatchIsValid(patch);
}
