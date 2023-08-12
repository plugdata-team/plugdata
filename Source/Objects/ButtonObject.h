/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ButtonObject : public ObjectBase {

    bool state = false;
    bool alreadyTriggered = false;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sizeProperty = SynchronousValue();

public:
    ButtonObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1);
        };

        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);
    }

    void update() override
    {
        if (auto button = ptr.get<t_fake_button>()) {
            primaryColour = Colour(button->x_fgcolor[0], button->x_fgcolor[1], button->x_fgcolor[2]).toString();
            secondaryColour = Colour(button->x_bgcolor[0], button->x_bgcolor[1], button->x_bgcolor[2]).toString();
            sizeProperty = button->x_w;
        }

        repaint();
    }

    /* TODO: finish this!
    void toggleObject(Point<int> position) override
    {

        if (!alreadyBanged) {

            auto* button = ptr.get<t_fake_button>();
            outlet_float(button->x_obj.ob_outlet, 1);
            update();
            alreadyBanged = true;
        }
    }
    void untoggleObject() override
    {

        if(alreadyBanged)
        {
            auto* button = ptr.get<t_fake_button>();
            outlet_float(button->x_obj.ob_outlet, 0);
            update();
        }
        alreadyBanged = false;
    }*/

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(patch, gobj.get(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, w + 1, h + 1);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto button = ptr.get<t_fake_button>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            libpd_moveobj(patch, button.cast<t_gobj>(), b.getX(), b.getY());
            button->x_w = b.getWidth() - 1;
            button->x_h = b.getHeight() - 1;
        }
    }
    
    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        
        if (auto button = ptr.get<t_fake_button>()) {
            setParameterExcludingListener(sizeProperty, var(button->x_w));
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (auto button = ptr.get<t_fake_button>()) {
            outlet_float(button->x_obj.ob_outlet, 1);
        }
        state = true;

        // Make sure we don't re-click with an accidental drag
        alreadyTriggered = true;

        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        alreadyTriggered = false;
        state = false;
        if (auto button = ptr.get<t_fake_button>()) {
            outlet_float(button->x_obj.ob_outlet, 0);
        }

        repaint();
    }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat();

        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(bounds.reduced(0.5f), Corners::objectCornerRadius);

        bool selected = object->isSelected() && !cnv->isGraph;

        g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), Corners::objectCornerRadius, selected ? 2.0f : 1.0f);

        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
        g.drawRoundedRectangle(bounds.reduced(6), Corners::objectCornerRadius, 1.5f);

        if (state) {
            g.setColour(Colour::fromString(primaryColour.toString()));
            g.fillRoundedRectangle(bounds.reduced(6), Corners::objectCornerRadius);
        }
    }

    void valueChanged(Value& value) override
    {

        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto size = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);
            if (auto button = ptr.get<t_fake_button>())
            {
                button->x_w = size;
                button->x_h = size;
            }
            object->updateBounds();
        }
        else if (value.refersToSameSourceAs(primaryColour)) {
            auto col = Colour::fromString(primaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_fgcolor[0] = col.getRed();
                button->x_fgcolor[1] = col.getGreen();
                button->x_fgcolor[2] = col.getBlue();
            }
            repaint();
        }
        else if (value.refersToSameSourceAs(secondaryColour)) {
            auto col = Colour::fromString(secondaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_bgcolor[0] = col.getRed();
                button->x_bgcolor[1] = col.getGreen();
                button->x_bgcolor[2] = col.getBlue();
            }
            repaint();
        }
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("bgcolor"),
            hash("fgcolor")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("bgcolor"): {
            setParameterExcludingListener(secondaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            repaint();
            break;
        }
        case hash("fgcolor"): {
            setParameterExcludingListener(primaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            repaint();
            break;
        }
        default:
            break;
        }
    }
};
