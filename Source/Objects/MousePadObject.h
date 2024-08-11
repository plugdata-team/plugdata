/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// ELSE mousepad
class MousePadObject final : public ObjectBase {
    bool isPressed = false;

    Point<int> lastPosition;
    Value sizeProperty = SynchronousValue();

public:
    MousePadObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , mouseListener(this)
    {
        mouseListener.globalMouseDown = [this](MouseEvent const& e) {
            auto relativeEvent = e.getEventRelativeTo(this);

            if (!getLocalBounds().contains(relativeEvent.getPosition()) || !isInsideGraphBounds(e) || !isLocked() || !cnv->isShowing() || isPressed)
                return;

            t_atom at[3];
            SETFLOAT(at, 1.0f);

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                pad->x_x = relativeEvent.getPosition().x;
                pad->x_y = getHeight() - relativeEvent.getPosition().y;

                outlet_anything(pad->x_obj.ob_outlet, pd->generateSymbol("click"), 1, at);
            }

            isPressed = true;
        };
        mouseListener.globalMouseUp = [this](MouseEvent const& e) {
            if (!getScreenBounds().contains(e.getMouseDownScreenPosition()) || !isInsideGraphBounds(e) || !isPressed || !isLocked() || !cnv->isShowing())
                return;

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                t_atom at[1];
                SETFLOAT(at, 0);
                outlet_anything(pad->x_obj.ob_outlet, pd->generateSymbol("click"), 1, at);
            }

            isPressed = false;
        };

        mouseListener.globalMouseMove = [this](MouseEvent const& e) {
            if ((!getScreenBounds().contains(e.getMouseDownScreenPosition()) && !isPressed) || !isInsideGraphBounds(e) || !isLocked() || !cnv->isShowing())
                return;

            auto relativeEvent = e.getEventRelativeTo(this);

            // Don't repeat values
            if (relativeEvent.getPosition() == lastPosition)
                return;

            int xPos = relativeEvent.getPosition().x;
            int yPos = getHeight() - relativeEvent.getPosition().y;

            lastPosition = relativeEvent.getPosition();

            if (auto pad = this->ptr.get<t_fake_pad>()) {
                pad->x_x = xPos;
                pad->x_y = yPos;

                t_atom at[3];
                SETFLOAT(at, xPos);
                SETFLOAT(at + 1, yPos);

                outlet_anything(pad->x_obj.ob_outlet, gensym("list"), 2, at);
            }
        };

        mouseListener.globalMouseDrag = [this](MouseEvent const& e) {
            mouseListener.globalMouseMove(e);
        };

        setInterceptsMouseClicks(false, false);

        objectParameters.addParamSize(&sizeProperty);
    }

    ~MousePadObject() override = default;
    
    bool isInsideGraphBounds(const MouseEvent& e)
    {
        auto* graph = findParentComponentOfClass<GraphOnParent>();
        while(graph)
        {
            auto pos = e.getEventRelativeTo(graph).getPosition();
            if(!graph->getLocalBounds().contains(pos))
            {
                return false;
            }
            
            graph = graph->findParentComponentOfClass<GraphOnParent>();
        }
        
        return true;
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        Colour fillColour, outlineColour;
        if(auto x = ptr.get<t_fake_pad>()) {
            fillColour = Colour(x->x_color[0], x->x_color[1], x->x_color[2]);
            outlineColour = cnv->editor->getLookAndFeel().findColour(object->isSelected() && !cnv->isGraph ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::outlineColourId);
        }
            

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(fillColour), convertColour(outlineColour), Corners::objectCornerRadius);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto pad = ptr.get<t_fake_pad>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, pad.cast<t_gobj>(), b.getX(), b.getY());
            pad->x_w = b.getWidth() - 1;
            pad->x_h = b.getHeight() - 1;
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

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void update() override
    {
        if (auto pad = ptr.get<t_fake_pad>()) {
            sizeProperty = Array<var> { var(pad->x_w), var(pad->x_h) };
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto pad = ptr.get<t_fake_pad>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(pad->x_w), var(pad->x_h) });
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto pad = ptr.get<t_fake_pad>()) {
                pad->x_w = width;
                pad->x_h = height;
            }

            object->updateBounds();
        }
    }

    // Check if top-level canvas is locked to determine if we should respond to mouse events
    bool isLocked()
    {
        // Find top-level canvas
        auto* topLevel = findParentComponentOfClass<Canvas>();
        while (auto* nextCanvas = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCanvas;
        }

        return static_cast<bool>(topLevel->locked.getValue() || topLevel->commandLocked.getValue()) || topLevel->isGraph;
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("color"): {
            repaint();
            break;
        }
        default:
            break;
        }
    }

    GlobalMouseListener mouseListener;
};
