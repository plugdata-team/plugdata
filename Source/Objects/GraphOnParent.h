/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class GraphOnParent final : public ObjectBase {

    bool isLocked = false;
    bool isOpenedInSplitView = false;

    Value isGraphChild = SynchronousValue(var(false));
    Value hideNameAndArgs = SynchronousValue(var(false));
    Value xRange = SynchronousValue();
    Value yRange = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    pd::Patch::Ptr subpatch;
    std::unique_ptr<Canvas> canvas;
    ObjectBackground background;

public:
    // Graph On Parent
    GraphOnParent(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
        , subpatch(new pd::Patch(obj, cnv->pd, false))
        , background(object, PlugDataColour::canvasBackgroundColourId)
    {
        resized();

        // We want graph to draw on top of all objects
        // The graph background is managed by ObjectBackground which should always be at the bottom
        setAlwaysOnTop(true);

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" });
        objectParameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" });
        objectParameters.addParamRange("X range", cGeneral, &xRange, { 0, 100 });
        objectParameters.addParamRange("Y range", cGeneral, &yRange, { -1, 1 });

        // There is a possibility that a donecanvasdialog message is sent inbetween the initialisation in pd and the initialisation of the plugdata object, making it possible to miss this message. This especially tends to happen if the messagebox is connected to a loadbang.
        // By running another update call asynchrounously, we can still respond to the new state
        MessageManager::callAsync([_this = SafePointer(this)]() {
            if (_this) {
                _this->update();
                _this->valueChanged(_this->isGraphChild);
            }
        });
    }

    void update() override
    {
        if (auto glist = ptr.get<t_canvas>()) {
            isGraphChild = static_cast<bool>(glist->gl_isgraph);
            hideNameAndArgs = static_cast<bool>(glist->gl_hidetext);
            xRange = Array<var> { var(glist->gl_x1), var(glist->gl_x2) };
            yRange = Array<var> { var(glist->gl_y2), var(glist->gl_y1) };
            sizeProperty = Array<var> { var(glist->gl_pixwidth), var(glist->gl_pixheight) };
        }

        updateCanvas();
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("yticks"):
        case hash("xticks"): {
            repaint();
            break;
        }
        case hash("coords"): {
            Rectangle<int> bounds;
            if (auto gobj = ptr.get<t_gobj>()) {
                auto* patch = cnv->patch.getPointer().get();
                if (!patch)
                    return;

                int x = 0, y = 0, w = 0, h = 0;
                pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
                bounds = Rectangle<int>(x, y, atoms[4].getFloat(), atoms[5].getFloat());
            }
            update();
            object->setObjectBounds(bounds);
            break;
        }
        case hash("donecanvasdialog"): {
            update();
            updateCanvas();
            break;
        }
        default:
            break;
        }
    }

    void resized() override
    {
        updateCanvas();
        updateDrawables();
    }

    // Called by object to make sure clicks on empty parts of the graph are passed on
    bool canReceiveMouseEvent(int x, int y) override
    {
        if (!canvas)
            return true;

        if (ModifierKeys::getCurrentModifiers().isRightButtonDown())
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

    void setPdBounds(Rectangle<int> b) override
    {

        if (auto glist = ptr.get<_glist>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, glist.cast<t_gobj>(), b.getX(), b.getY());
            glist->gl_pixwidth = b.getWidth() - 1;
            glist->gl_pixheight = b.getHeight() - 1;
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

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto glist = ptr.get<t_glist>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(glist->gl_pixwidth), var(glist->gl_pixheight) });
        }
    }

    ~GraphOnParent() override
    {
        closeOpenedSubpatchers();
    }

    void tabChanged() override
    {
        isOpenedInSplitView = false;
        for (auto* split : cnv->editor->splitView.splits) {
            if (auto* cnv = split->getTabComponent()->getCurrentCanvas()) {
                if (cnv->patch == *getPatch()) {
                    isOpenedInSplitView = true;
                }
            }
        }

        updateCanvas();
        repaint();
    }

    void lock(bool locked) override
    {
        setInterceptsMouseClicks(locked, locked);
        isLocked = locked;
    }

    void updateCanvas()
    {
        if (!canvas) {
            canvas = std::make_unique<Canvas>(cnv->editor, subpatch, this);

            // Make sure that the graph doesn't become the current canvas
            cnv->patch.setCurrent();
            cnv->editor->updateCommandStatus();
        }

        auto b = getPatch()->getBounds() + canvas->canvasOrigin;
        canvas->setBounds(-b.getX(), -b.getY(), b.getWidth() + b.getX(), b.getHeight() + b.getY());
        canvas->setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
        canvas->locked.referTo(cnv->locked);

        canvas->performSynchronise();
    }

    void updateDrawables() override
    {
        if (!canvas)
            return;

        canvas->updateDrawables();
    }

    // override to make transparent
    void paint(Graphics& g) override
    {
        // Strangly, the title goes below the graph content in pd
        if (!getValue<bool>(hideNameAndArgs) && getText() != "graph") {
            auto text = getText();

            auto textArea = getLocalBounds().removeFromTop(16).withTrimmedLeft(5);
            Fonts::drawFittedText(g, text, textArea, object->findColour(PlugDataColour::canvasTextColourId));
        }
    }

    static void drawTicksForGraph(Graphics& g, t_glist* x, ObjectBase* parent)
    {
        auto b = parent->getLocalBounds();
        t_float y1 = b.getY(), y2 = b.getBottom(), x1 = b.getX(), x2 = b.getRight();

        g.setColour(parent->cnv->findColour(PlugDataColour::guiObjectInternalOutlineColour));
        if (x->gl_xtick.k_lperb) {
            t_float f = x->gl_xtick.k_point;
            for (int i = 0; f < 0.99f * x->gl_x2 + 0.01f * x->gl_x1; i++, f += x->gl_xtick.k_inc) {
                auto xpos = jmap<float>(f, x->gl_x2, x->gl_x1, x1, x2);
                int tickpix = (i % x->gl_xtick.k_lperb ? 2 : 4);
                g.drawLine((int)xpos, (int)y2, (int)xpos, (int)y2 - tickpix);
                g.drawLine((int)xpos, (int)y1, (int)xpos, (int)y1 + tickpix);
            }

            f = x->gl_xtick.k_point - x->gl_xtick.k_inc;
            for (int i = 1; f > 0.99f * x->gl_x2 + 0.01f * x->gl_x1; i++, f -= x->gl_xtick.k_inc) {
                auto xpos = jmap<float>(f, x->gl_x2, x->gl_x1, x1, x2);
                int tickpix = (i % x->gl_xtick.k_lperb ? 2 : 4);
                g.drawLine(xpos, y2, xpos, y2 - tickpix);
                g.drawLine(xpos, y1, xpos, y1 + tickpix);
            }
        }

        if (x->gl_ytick.k_lperb) {
            t_float f = x->gl_ytick.k_point;
            for (int i = 0; f < 0.99f * x->gl_y2 + 0.01f * x->gl_y1; i++, f += x->gl_ytick.k_inc) {
                auto ypos = jmap<float>(f, x->gl_y2, x->gl_y1, y1, y2);
                int tickpix = (i % x->gl_ytick.k_lperb ? 2 : 4);
                g.drawLine(x1, ypos, x1 + tickpix, ypos);
                g.drawLine(x2, ypos, x2 - tickpix, ypos);
            }

            f = x->gl_ytick.k_point - x->gl_ytick.k_inc;
            for (int i = 1; f > 0.99f * x->gl_y2 + 0.01f * x->gl_y1; i++, f -= x->gl_ytick.k_inc) {
                auto ypos = jmap<float>(f, x->gl_y2, x->gl_y1, y1, y2);
                int tickpix = (i % x->gl_ytick.k_lperb ? 2 : 4);
                g.drawLine(x1, ypos, x1 + tickpix, ypos);
                g.drawLine(x2, ypos, x2 - tickpix, ypos);
            }
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        if (isOpenedInSplitView) {

            g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::objectCornerRadius);

            auto colour = object->findColour(PlugDataColour::commentTextColourId);
            Fonts::drawText(g, "Graph opened in split view", getLocalBounds(), colour, 14, Justification::centred);
        }

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        if (auto graph = ptr.get<t_glist>()) {
            drawTicksForGraph(g, graph.get(), this);
        }
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    Canvas* getCanvas() override
    {
        return canvas.get();
    }

    void valueChanged(Value& v) override
    {

        if (v.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto glist = ptr.get<t_glist>()) {
                glist->gl_pixwidth = width;
                glist->gl_pixheight = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
            int hideText = getValue<bool>(hideNameAndArgs);
            if (auto glist = ptr.get<t_glist>()) {
                canvas_setgraph(glist.get(), glist->gl_isgraph + 2 * hideText, 0);
            }
            repaint();
        } else if (v.refersToSameSourceAs(isGraphChild)) {
            int isGraph = getValue<bool>(isGraphChild);

            if (auto glist = ptr.get<t_glist>()) {
                canvas_setgraph(glist.get(), isGraph + 2 * (isGraph && glist->gl_hidetext), 0);
            }

            if (!isGraph) {
                MessageManager::callAsync([_this = SafePointer(this)]() {
                    if (!_this)
                        return;

                    _this->cnv->setSelected(_this->object, false);
                    _this->object->cnv->editor->sidebar->hideParameters();

                    _this->object->setType(_this->getText(), _this->ptr);
                });
            } else {
                updateCanvas();
                repaint();
            }
        } else if (v.refersToSameSourceAs(xRange)) {
            if (auto glist = ptr.get<t_canvas>()) {
                glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
                glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
            }
            updateDrawables();
        } else if (v.refersToSameSourceAs(yRange)) {
            if (auto glist = ptr.get<t_canvas>()) {
                glist->gl_y2 = static_cast<float>(yRange.getValue().getArray()->getReference(0));
                glist->gl_y1 = static_cast<float>(yRange.getValue().getArray()->getReference(1));
            }
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
};
