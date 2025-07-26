/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

// ELSE mousepad
class MousePadObject final : public ObjectBase {
    bool isPressed = false;

    Point<int> lastPosition;
    Value sizeProperty = SynchronousValue();
    NVGcolor fillColour;
    
public:
    MousePadObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , mouseListener(this)
    {
        mouseListener.globalMouseDown = [this](MouseEvent const& e) {
            auto const relativeEvent = e.getEventRelativeTo(this);

            if (!getLocalBounds().contains(relativeEvent.getPosition()) || !isInsideGraphBounds(e) || !isLocked() || !cnv->isShowing() || isPressed)
                return;

            StackArray<t_atom, 3> at;
            SETFLOAT(&at[0], 1.0f);

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                pad->x_x = relativeEvent.getPosition().x;
                pad->x_y = getHeight() - relativeEvent.getPosition().y;

                outlet_anything(pad->x_obj.ob_outlet, pd->generateSymbol("click"), 1, at.data());
            }

            isPressed = true;
        };
        mouseListener.globalMouseUp = [this](MouseEvent const& e) {
            if (!getScreenBounds().contains(e.getMouseDownScreenPosition()) || !isInsideGraphBounds(e) || !isPressed || !isLocked() || !cnv->isShowing())
                return;

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                StackArray<t_atom, 1> at;
                SETFLOAT(&at[0], 0);
                outlet_anything(pad->x_obj.ob_outlet, pd->generateSymbol("click"), 1, at.data());
            }

            isPressed = false;
        };

        mouseListener.globalMouseMove = [this](MouseEvent const& e) {
            if ((!getScreenBounds().contains(e.getMouseDownScreenPosition()) && !isPressed) || !isInsideGraphBounds(e) || !isLocked() || !cnv->isShowing())
                return;

            auto const relativeEvent = e.getEventRelativeTo(this);

            // Don't repeat values
            if (relativeEvent.getPosition() == lastPosition)
                return;

            int const xPos = relativeEvent.getPosition().x;
            int const yPos = getHeight() - relativeEvent.getPosition().y;

            lastPosition = relativeEvent.getPosition();

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                pad->x_x = xPos;
                pad->x_y = yPos;

                StackArray<t_atom, 3> at;
                SETFLOAT(&at[0], xPos);
                SETFLOAT(&at[1], yPos);

                outlet_anything(pad->x_obj.ob_outlet, gensym("list"), 2, at.data());
            }
        };

        mouseListener.globalMouseDrag = [this](MouseEvent const& e) {
            mouseListener.globalMouseMove(e);
        };

        setInterceptsMouseClicks(false, false);

        objectParameters.addParamSize(&sizeProperty);
    }

    ~MousePadObject() override = default;

    bool isInsideGraphBounds(MouseEvent const& e) const
    {
        auto const* topLevel = cnv;
        while (auto const* nextCanvas = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCanvas;
            if(auto* graph = dynamic_cast<GraphOnParent*>(topLevel->getParentComponent())) {
                auto const pos = e.getEventRelativeTo(graph).getPosition();
                if (!graph->getLocalBounds().contains(pos)) {
                    return false;
                }
            }
        }

        return true;
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto outlineColour = object->isSelected() && !cnv->isGraph ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), fillColour, outlineColour, Corners::objectCornerRadius);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto pad = ptr.get<t_fake_pad>()) {
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, pad.cast<t_gobj>(), b.getX(), b.getY());
            pad->x_w = b.getWidth() - 1;
            pad->x_h = b.getHeight() - 1;
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void update() override
    {
        if (auto pad = ptr.get<t_fake_pad>()) {
            sizeProperty = VarArray { var(pad->x_w), var(pad->x_h) };
            fillColour = NVGComponent::convertColour(Colour(pad->x_color[0], pad->x_color[1], pad->x_color[2]));
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto pad = ptr.get<t_fake_pad>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(pad->x_w), var(pad->x_h) });
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto pad = ptr.get<t_fake_pad>()) {
                pad->x_w = width;
                pad->x_h = height;
            }

            object->updateBounds();
        }
    }

    // Check if top-level canvas is locked to determine if we should respond to mouse events
    bool isLocked() const
    {
        // Find top-level canvas
        auto const* topLevel = cnv;
        while (auto const* nextCanvas = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCanvas;
        }

        return getValue<bool>(topLevel->locked) || getValue<bool>(topLevel->commandLocked) || topLevel->isGraph;
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("color"): {
            if (auto pad = ptr.get<t_fake_pad>()) {
                fillColour = NVGComponent::convertColour(Colour(pad->x_color[0], pad->x_color[1], pad->x_color[2]));
            }
            repaint();
            break;
        }
        default:
            break;
        }
    }

    GlobalMouseListener mouseListener;
};
