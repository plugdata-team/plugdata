
struct GraphOnParent final : public GUIObject {
    bool isLocked = false;

public:
    // Graph On Parent
    GraphOnParent(void* obj, Object* object)
        : GUIObject(obj, object)
        , subpatch({ ptr, cnv->pd })
    {
        auto* glist = static_cast<t_canvas*>(ptr);
        isGraphChild = true;
        hideNameAndArgs = static_cast<bool>(subpatch.getPointer()->gl_hidetext);
        xRange = Array<var> { var(glist->gl_x1), var(glist->gl_x2) };
        yRange = Array<var> { var(glist->gl_y2), var(glist->gl_y1) };

        isGraphChild.addListener(this);
        hideNameAndArgs.addListener(this);
        xRange.addListener(this);
        yRange.addListener(this);

        updateCanvas();
        resized();
    }

    void resized() override
    {
        updateDrawables();
    }

    // Called by object to make sure clicks on empty parts of the graph are passed on
    bool canReceiveMouseEvent(int x, int y) override
    {
        if (!canvas)
            return true;
        if (!isLocked)
            return true;

        for (auto& obj : canvas->objects) {
            if (!obj->gui)
                continue;

            auto localPoint = obj->getLocalPoint(object, Point<int>(x, y));

            if (obj->hitTest(localPoint.x, localPoint.y)) {
                return true;
            }
        }

        return false;
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(25, maxSize, object->getWidth());
        int h = jlimit(25, maxSize, object->getHeight());

        if (w != object->getWidth() || h != object->getHeight()) {
            object->setSize(w, h);
        }
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    ~GraphOnParent() override
    {
        closeOpenedSubpatchers();
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* graph = static_cast<_glist*>(ptr);
        graph->gl_pixwidth = b.getWidth();
        graph->gl_pixheight = b.getHeight();
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void updateCanvas()
    {
        if (!canvas) {
            canvas = std::make_unique<Canvas>(cnv->main, subpatch, this);

            // Make sure that the graph doesn't become the current canvas
            cnv->patch.setCurrent(true);
            cnv->main.updateCommandStatus();
        }

        auto b = getPatch()->getBounds();
        canvas->setBounds(-b.getX(), -b.getY(), b.getWidth() + b.getX(), b.getHeight() + b.getY());
        canvas->setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
        canvas->locked.referTo(cnv->locked);
    }

    void updateValue() override
    {
        // Change from subpatch to graph
        if (!static_cast<t_canvas*>(ptr)->gl_isgraph) {
            cnv->setSelected(object, false);
            object->cnv->main.sidebar.hideParameters();
            object->setType(getText(), ptr);
            return;
        }

        updateCanvas();

        if (!canvas)
            return;

        for (auto& object : canvas->objects) {
            if (object->gui) {
                object->gui->updateValue();
            }
        }
    }

    void updateDrawables() override
    {
        if (!canvas)
            return;
        for (auto& object : canvas->objects) {
            if (object->gui) {
                object->gui->updateDrawables();
            }
        }
    }

    // override to make transparent
    void paint(Graphics& g) override
    {
        auto outlineColour = object->findColour(cnv->isSelected(object) && !cnv->isGraph ? PlugDataColour::canvasActiveColourId : PlugDataColour::outlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);

        // Strangly, the title goes below the graph content in pd
        auto text = getText();

        if (!static_cast<bool>(hideNameAndArgs.getValue()) && text != "graph") {
            g.setColour(object->findColour(PlugDataColour::canvasTextColourId));
            g.setFont(Font(15));
            auto textArea = getLocalBounds().removeFromTop(20).withTrimmedLeft(5);
            g.drawFittedText(text, textArea, Justification::left, 1, 1.0f);
        }
    }

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    Canvas* getCanvas() override
    {
        return canvas.get();
    }

    ObjectParameters getParameters() override
    {
        return { { "Is graph", tBool, cGeneral, &isGraphChild, { "No", "Yes" } }, { "Hide name and arguments", tBool, cGeneral, &hideNameAndArgs, { "No", "Yes" } }, { "X range", tRange, cGeneral, &xRange, {} },
            { "Y range", tRange, cGeneral, &yRange, {} } };
    };

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(isGraphChild)) {
            subpatch.getPointer()->gl_isgraph = static_cast<bool>(isGraphChild.getValue());
            updateValue();
        } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
            subpatch.getPointer()->gl_hidetext = static_cast<bool>(hideNameAndArgs.getValue());
            repaint();
        } else if (v.refersToSameSourceAs(xRange)) {
            auto* glist = static_cast<t_canvas*>(ptr);
            glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
            glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
            updateDrawables();
        } else if (v.refersToSameSourceAs(yRange)) {
            auto* glist = static_cast<t_canvas*>(ptr);
            glist->gl_y2 = static_cast<float>(yRange.getValue().getArray()->getReference(0));
            glist->gl_y1 = static_cast<float>(yRange.getValue().getArray()->getReference(1));
            updateDrawables();
        }
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

private:
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
    Value xRange, yRange;

    pd::Patch subpatch;
    std::unique_ptr<Canvas> canvas;
};
