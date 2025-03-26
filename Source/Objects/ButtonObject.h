/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ButtonObject final : public ObjectBase {

    bool state = false;
    bool alreadyTriggered = false;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    NVGcolor fgCol;
    NVGcolor bgCol;

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
        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);

        updateColours();
    }

    void onConstrainerCreate() override
    {
        constrainer->setFixedAspectRatio(1);
        constrainer->setMinimumHeight(9);
        constrainer->setMinimumWidth(9);
    }

    void updateColours()
    {
        bgCol = convertColour(Colour::fromString(secondaryColour.toString()));
        fgCol = convertColour(Colour::fromString(primaryColour.toString()));
        repaint();
    }

    void update() override
    {
        if (auto button = ptr.get<t_fake_button>()) {
            primaryColour = Colour(button->x_fgcolor[0], button->x_fgcolor[1], button->x_fgcolor[2]).toString();
            secondaryColour = Colour(button->x_bgcolor[0], button->x_bgcolor[1], button->x_bgcolor[2]).toString();
            sizeProperty = VarArray(button->x_w, button->x_h);
            if (button->x_mode == 0) {
                mode = Latch;
            } else if (button->x_mode == 1) {
                mode = Toggle;
            } else {
                mode = Bang;
            }
        }

        updateColours();
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
            auto* patch = cnv->patch.getRawPointer();
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, w + 1, h + 1);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto button = ptr.get<t_fake_button>()) {
            auto* patch = cnv->patch.getRawPointer();
            pd::Interface::moveObject(patch, button.cast<t_gobj>(), b.getX(), b.getY());
            button->x_w = b.getWidth() - 1;
            button->x_h = b.getHeight() - 1;
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto button = ptr.get<t_fake_button>()) {
            setParameterExcludingListener(sizeProperty, VarArray(var(button->x_w), var(button->x_h)));
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
        auto const b = getLocalBounds().toFloat();

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), bgCol, object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);

        auto const guiBounds = b.reduced(1);
        auto const outerBounds = guiBounds.reduced(5);
        auto const innerRectBounds = outerBounds.reduced(2.5);
        bool spaceToShowRect = false;

        if (b.getWidth() >= 25 && b.getHeight() >= 25) {
            spaceToShowRect = true;
            nvgDrawRoundedRect(nvg, outerBounds.getX(), outerBounds.getY(), outerBounds.getWidth(), outerBounds.getHeight(), cnv->guiObjectInternalOutlineCol, cnv->guiObjectInternalOutlineCol, Corners::objectCornerRadius);
            nvgDrawRoundedRect(nvg, innerRectBounds.getX(), innerRectBounds.getY(), innerRectBounds.getWidth(), innerRectBounds.getHeight(), bgCol, bgCol, Corners::objectCornerRadius - 1.0f);
        }

        // Fill ellipse if bangState is true
        if (state) {
            auto const innerBounds = spaceToShowRect ? innerRectBounds.reduced(1) : guiBounds;
            auto const cornerRadius = spaceToShowRect ? Corners::objectCornerRadius - 1.5f : Corners::objectCornerRadius - 1;
            nvgDrawRoundedRect(nvg, innerBounds.getX(), innerBounds.getY(), innerBounds.getWidth(), innerBounds.getHeight(), fgCol, fgCol, cornerRadius);
        }
    }

    bool canEdgeOverrideAspectRatio() override
    {
        return true;
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();

            auto const& arr = *sizeProperty.getValue().getArray();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            constrainer->setFixedAspectRatio(static_cast<float>(width) / height);

            setParameterExcludingListener(sizeProperty, VarArray(width, height));
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_w = width;
                button->x_h = height;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto const col = Colour::fromString(primaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_fgcolor[0] = col.getRed();
                button->x_fgcolor[1] = col.getGreen();
                button->x_fgcolor[2] = col.getBlue();
            }
            updateColours();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto const col = Colour::fromString(secondaryColour.toString());
            if (auto button = ptr.get<t_fake_button>()) {
                button->x_bgcolor[0] = col.getRed();
                button->x_bgcolor[1] = col.getGreen();
                button->x_bgcolor[2] = col.getBlue();
            }
            updateColours();
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("bgcolor"): {
            if (atoms.size() >= 3) {
                setParameterExcludingListener(secondaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
                updateColours();
            }
            break;
        }
        case hash("fgcolor"): {
            if (atoms.size() >= 3) {
                setParameterExcludingListener(primaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
                updateColours();
            }
            break;
        }
        case hash("float"): {
            if (atoms.size()) {
                state = !approximatelyEqual(atoms[0].getFloat(), 0.0f);
            }
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
