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

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        iemHelper.receiveObjectMessage(symbol, atoms);
    }

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b.removeFromRight(1).removeFromBottom(1));
    }

    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked;
    }

    void lock(bool isLocked) override
    {
        locked = isLocked;
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
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);
    }

    void valueChanged(Value& v) override
    {
        iemHelper.valueChanged(v);
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Background", tColour, cAppearance, &iemHelper.secondaryColour, {} },
            { "Receive Symbol", tString, cGeneral, &iemHelper.receiveSymbol, {} },
            { "Send Symbol", tString, cGeneral, &iemHelper.sendSymbol, {} },
            { "Label", tString, cLabel, &iemHelper.labelText, {} },
            { "Label Colour", tColour, cLabel, &iemHelper.labelColour, {} },
            { "Label X", tInt, cLabel, &iemHelper.labelX, {} },
            { "Label Y", tInt, cLabel, &iemHelper.labelY, {} },
            { "Label Height", tInt, cLabel, &iemHelper.labelHeight, {} },
        };
    }
};
