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
        locked = getValue<bool>(object->locked);

        objectParameters.addParamColour("Canvas color", cGeneral, &iemHelper.secondaryColour, PlugDataColour::guiObjectInternalOutlineColour);
        iemHelper.addIemParameters(objectParameters, false, true, 20, 12, 14);
    }

    bool hideInlets() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            IEMGUI_MESSAGES
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        iemHelper.receiveObjectMessage(symbol, atoms);
    }

    void update() override
    {
        iemHelper.update();
    }

    Rectangle<int> getSelectableBounds() override
    {
        auto* cnvObj = reinterpret_cast<t_my_canvas*>(iemHelper.iemgui);
        return { cnvObj->x_gui.x_obj.te_xpix, cnvObj->x_gui.x_obj.te_ypix, cnvObj->x_gui.x_w, cnvObj->x_gui.x_h };
    }

    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked && Rectangle<int>(static_cast<t_iemgui*>(ptr)->x_w, static_cast<t_iemgui*>(ptr)->x_h).contains(x - Object::margin, y - Object::margin);
    }

    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        auto* cnvObj = reinterpret_cast<t_my_canvas*>(iemHelper.iemgui);

        cnvObj->x_gui.x_obj.te_xpix = b.getX();
        cnvObj->x_gui.x_obj.te_ypix = b.getY();
        cnvObj->x_vis_w = b.getWidth() - 1;
        cnvObj->x_vis_h = b.getHeight() - 1;
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto bounds = Rectangle<int>(x, y, static_cast<t_my_canvas*>(ptr)->x_vis_w + 1, static_cast<t_my_canvas*>(ptr)->x_vis_h + 1);

        pd->unlockAudioThread();
        return bounds;
    }

    void paint(Graphics& g) override
    {
        auto bgcolour = Colour::fromString(iemHelper.secondaryColour.toString());

        g.setColour(bgcolour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::objectCornerRadius);

        if (!locked) {
            auto draggableRect = Rectangle<float>(static_cast<t_iemgui*>(ptr)->x_w, static_cast<t_iemgui*>(ptr)->x_h);
            g.setColour(object->isSelected() ? object->findColour(PlugDataColour::objectSelectedOutlineColourId) : object->findColour(PlugDataColour::objectOutlineColourId));
            g.drawRoundedRectangle(draggableRect.reduced(1.0f), Corners::objectCornerRadius, 1.0f);
        }
    }

    void valueChanged(Value& v) override
    {
        iemHelper.valueChanged(v);
    }
};

