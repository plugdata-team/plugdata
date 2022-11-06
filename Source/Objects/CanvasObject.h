/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct CanvasObject final : public IEMObject {
    
    bool locked;
    
    CanvasObject(void* ptr, Object* object)
        : IEMObject(ptr, object)
    {
        object->setColour(PlugDataColour::outlineColourId, Colours::transparentBlack);
        locked = static_cast<bool>(object->locked.getValue());
    }
    
    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked;
    }
    
    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto bounds = Rectangle<int>(x, y, static_cast<t_my_canvas*>(ptr)->x_vis_w, static_cast<t_my_canvas*>(ptr)->x_vis_h);

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    void resized() override
    {
        static_cast<t_my_canvas*>(ptr)->x_vis_w = getWidth();
        static_cast<t_my_canvas*>(ptr)->x_vis_h = getHeight();
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(20, maxSize, object->getWidth());
        int h = jlimit(20, maxSize, object->getHeight());

        if (w != object->getWidth() || h != object->getHeight()) {
            object->setSize(w, h);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
    }

    void updateValue() override {};

    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        params.push_back({ "Background", tColour, cAppearance, &secondaryColour, {} });
        params.push_back({ "Send Symbol", tString, cGeneral, &sendSymbol, {} });
        params.push_back({ "Receive Symbol", tString, cGeneral, &receiveSymbol, {} });
        params.push_back({ "Label", tString, cLabel, &labelText, {} });
        params.push_back({ "Label Colour", tColour, cLabel, &labelColour, {} });
        params.push_back({ "Label X", tInt, cLabel, &labelX, {} });
        params.push_back({ "Label Y", tInt, cLabel, &labelY, {} });
        params.push_back({ "Label Height", tInt, cLabel, &labelHeight, {} });
        return params;
    }
};
