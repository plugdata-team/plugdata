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

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        iemHelper.receiveObjectMessage(symbol, atoms);
    }

    void updateParameters() override
    {
        iemHelper.updateParameters();
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
        // TODO: simplify this
        ObjectParameters params;
        params.push_back({ "Background", tColour, cAppearance, &iemHelper.secondaryColour, {} });
        params.push_back({ "Send Symbol", tString, cGeneral, &iemHelper.sendSymbol, {} });
        params.push_back({ "Receive Symbol", tString, cGeneral, &iemHelper.receiveSymbol, {} });
        params.push_back({ "Label", tString, cLabel, &iemHelper.labelText, {} });
        params.push_back({ "Label Colour", tColour, cLabel, &iemHelper.labelColour, {} });
        params.push_back({ "Label X", tInt, cLabel, &iemHelper.labelX, {} });
        params.push_back({ "Label Y", tInt, cLabel, &iemHelper.labelY, {} });
        params.push_back({ "Label Height", tInt, cLabel, &iemHelper.labelHeight, {} });
        return params;
    }
};
