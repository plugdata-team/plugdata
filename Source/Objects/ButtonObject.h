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
    ButtonObject(pd::WeakReference obj, Object* parent)
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

    void toggleObject(Point<int> position) override
    {
        if (!alreadyTriggered) {

            if(auto button = ptr.get<t_fake_button>()) {
                outlet_float(button->x_obj.ob_outlet, 1);
            }
            state = true;
            repaint();
            alreadyTriggered = true;
        }
    }
    void untoggleObject() override
    {
        if(alreadyTriggered)
        {
            if(auto button = ptr.get<t_fake_button>()) {
                outlet_float(button->x_obj.ob_outlet, 0);
            }
            state = false;
            repaint();
            alreadyTriggered = false;
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

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

            pd::Interface::moveObject(patch, button.cast<t_gobj>(), b.getX(), b.getY());
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
        if (!e.mods.isLeftButtonDown())
            return;

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
    
    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        
        auto foregroundColour = convertColour(Colour::fromString(primaryColour.toString()));
        auto backgroundColour = convertColour(Colour::fromString(secondaryColour.toString()));
        auto selectedOutlineColour = convertColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(object->findColour(PlugDataColour::objectOutlineColourId));
        auto internalLineColour = convertColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);
        
        b = b.reduced(1);
        auto const width = std::max(b.getWidth(), b.getHeight());
        auto const sizeReduction = std::min(1.0f, getWidth() / 20.0f);
        
        float const lineOuter = 80.f * (width * 0.01f);
        float const lineThickness = std::max(width * 0.06f, 1.5f) * sizeReduction;

        auto outerBounds = b.reduced((width - lineOuter) * sizeReduction);
        
        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, outerBounds.getX(),outerBounds.getY(), outerBounds.getWidth(), outerBounds.getHeight(), Corners::objectCornerRadius * sizeReduction);
        nvgStrokeColor(nvg, internalLineColour);
        nvgStrokeWidth(nvg, lineThickness);
        nvgStroke(nvg);
        
        // Fill ellipse if bangState is true
        if (state) {
            auto innerBounds = b.reduced((width - lineOuter + lineThickness) * sizeReduction);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, innerBounds.getX(), innerBounds.getY(), innerBounds.getWidth(), innerBounds.getHeight(), (Corners::objectCornerRadius - 1) * sizeReduction);
            nvgFillColor(nvg, foregroundColour);
            nvgFill(nvg);
        }
    }

    /*
    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat();
        auto const sizeReduction = std::min(1.0f, getWidth() / 20.0f);
        
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(bounds.reduced(0.5f), Corners::objectCornerRadius);

        bool selected = object->isSelected() && !cnv->isGraph;

        g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
        g.drawRoundedRectangle(bounds.reduced(6 * sizeReduction), Corners::objectCornerRadius * sizeReduction, 1.5f);

        if (state) {
            g.setColour(Colour::fromString(primaryColour.toString()));
            g.fillRoundedRectangle(, Corners::objectCornerRadius * sizeReduction);
        }
    } */

    void valueChanged(Value& value) override
    {

        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto size = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_w = size;
                button->x_h = size;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto col = Colour::fromString(primaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_fgcolor[0] = col.getRed();
                button->x_fgcolor[1] = col.getGreen();
                button->x_fgcolor[2] = col.getBlue();
            }
            repaint();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto col = Colour::fromString(secondaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_bgcolor[0] = col.getRed();
                button->x_bgcolor[1] = col.getGreen();
                button->x_bgcolor[2] = col.getBlue();
            }
            repaint();
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
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
