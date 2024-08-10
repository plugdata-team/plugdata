/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ButtonObject final : public ObjectBase {

    bool state = false;
    bool alreadyTriggered = false;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    enum Mode {
        Latch,
        Toggle,
        Bang
    };

    Mode mode;

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
            if (button->x_mode == 0) {
                mode = Latch;
            } else if (button->x_mode == 1) {
                mode = Toggle;
            } else {
                mode = Bang;
            }
        }

        repaint();
    }

    void toggleObject(Point<int> position) override
    {
        if (!alreadyTriggered) {

            if (mode == Latch) {
                state = true;
            } else if (mode == Toggle) {
                state = !state;
            }

            if (mode == Bang) {
                state = true;
                if (auto button = ptr.get<t_fake_button>()) {
                    outlet_bang(button->x_obj.ob_outlet);
                }
                Timer::callAfterDelay(250,
                    [_this = SafePointer(this)]() mutable {
                        // First check if this object still exists
                        if (!_this)
                            return;

                        if (_this->state) {
                            _this->state = false;
                            _this->repaint();
                        }
                    });
            } else {
                if (auto button = ptr.get<t_fake_button>()) {
                    outlet_float(button->x_obj.ob_outlet, state);
                }
            }
            repaint();
            alreadyTriggered = true;
        }
    }
    void untoggleObject() override
    {
        if (alreadyTriggered) {
            if (mode == Latch) {
                state = false;
                if (auto button = ptr.get<t_fake_button>()) {
                    outlet_float(button->x_obj.ob_outlet, 0);
                }
            }
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

        if (mode == Latch) {
            state = true;
        } else if (mode == Toggle) {
            state = !state;
        }

        if (mode == Bang) {
            state = true;
            if (auto button = ptr.get<t_fake_button>()) {
                outlet_bang(button->x_obj.ob_outlet);
            }
            Timer::callAfterDelay(250,
                [_this = SafePointer(this)]() mutable {
                    // First check if this object still exists
                    if (!_this)
                        return;

                    if (_this->state) {
                        _this->state = false;
                        _this->repaint();
                    }
                });
        } else {
            if (auto button = ptr.get<t_fake_button>()) {
                outlet_float(button->x_obj.ob_outlet, state);
            }
        }

        // Make sure we don't re-click with an accidental drag
        alreadyTriggered = true;

        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        alreadyTriggered = false;
        if (mode == Latch) {
            state = false;
            if (auto button = ptr.get<t_fake_button>()) {
                outlet_float(button->x_obj.ob_outlet, 0);
            }
        }

        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();

        auto foregroundColour = convertColour(Colour::fromString(primaryColour.toString()));
        auto bgColour = Colour::fromString(secondaryColour.toString());
        
        auto backgroundColour = convertColour(bgColour);
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        auto internalLineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        b = b.reduced(1);
        auto const width = std::max(b.getWidth(), b.getHeight());
        auto const sizeReduction = std::min(1.0f, getWidth() / 20.0f);

        float const lineOuter = 80.f * (width * 0.01f);
        float const lineThickness = std::max(width * 0.06f, 1.5f) * sizeReduction;

        auto outerBounds = b.reduced((width - lineOuter) * sizeReduction);

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, outerBounds.getX(), outerBounds.getY(), outerBounds.getWidth(), outerBounds.getHeight(), Corners::objectCornerRadius * sizeReduction);
        nvgStrokeColor(nvg, internalLineColour);
        nvgStrokeWidth(nvg, lineThickness);
        nvgStroke(nvg);

        // Fill ellipse if bangState is true
        if (state) {
            auto innerBounds = b.reduced((width - lineOuter + lineThickness) * sizeReduction);
            nvgFillColor(nvg, foregroundColour);
            nvgFillRoundedRect(nvg, innerBounds.getX(), innerBounds.getY(), innerBounds.getWidth(), innerBounds.getHeight(), (Corners::objectCornerRadius - 1) * sizeReduction);
        }
    }

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
        case hash("float"): {
            state = !approximatelyEqual(atoms[0].getFloat(), 0.0f);
            repaint();
            break;
        }
        case hash("latch"):
        case hash("bang"):
        case hash("toggle"): {
            update();
            break;
        }
        default:
            break;
        }
    }
};
