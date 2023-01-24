/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CanvasObject final : public ObjectBase {

    bool locked;

    IEMHelper iemHelper;

public:
    CanvasObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        object->setColour(PlugDataColour::outlineColourId, Colours::transparentBlack);
        locked = static_cast<bool>(object->locked.getValue());
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        iemHelper.receiveObjectMessage(symbol, atoms);
    }

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
    }

    void applyBounds() override
    {
        iemHelper.applyBounds();
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
        g.fillAll(Colour::fromString(iemHelper.secondaryColour.toString()));
    }

    void valueChanged(Value& v) override
    {
        iemHelper.valueChanged(v);
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Background", tColour, cAppearance, &iemHelper.secondaryColour, {} },
            { "Send Symbol", tString, cGeneral, &iemHelper.sendSymbol, {} },
            { "Receive Symbol", tString, cGeneral, &iemHelper.receiveSymbol, {} },
            { "Label", tString, cLabel, &iemHelper.labelText, {} },
            { "Label Colour", tColour, cLabel, &iemHelper.labelColour, {} },
            { "Label X", tInt, cLabel, &iemHelper.labelX, {} },
            { "Label Y", tInt, cLabel, &iemHelper.labelY, {} },
            { "Label Height", tInt, cLabel, &iemHelper.labelHeight, {} },
        };
    }
};
