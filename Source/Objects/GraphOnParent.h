/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class GraphOnParent final : public ObjectBase {

    bool isLocked = false;
    bool isOpenedInSplitView = false;

    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
    Value xRange, yRange;

    pd::Patch::Ptr subpatch;
    std::unique_ptr<Canvas> canvas;

public:
    // Graph On Parent
    GraphOnParent(void* obj, Object* object)
        : ObjectBase(obj, object)
        , subpatch(new pd::Patch(obj, cnv->pd, false))
    {
        resized();

        MessageManager::callAsync([_this = SafePointer(this)]() {
            if (_this)
                _this->checkGraphState();
        });

        objectParameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" });
        objectParameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" });
        objectParameters.addParamRange("X range", cGeneral, &xRange);
        objectParameters.addParamRange("Y range", cGeneral, &yRange);
    }

    void update() override
    {
        if(auto glist = ptr.get<t_canvas>())
        {
            isGraphChild = static_cast<bool>(glist->gl_isgraph);
            hideNameAndArgs = static_cast<bool>(glist->gl_hidetext);
            xRange = Array<var> { var(glist->gl_x1), var(glist->gl_x2) };
            yRange = Array<var> { var(glist->gl_y2), var(glist->gl_y1) };
        }

        updateCanvas();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("coords"),
            hash("donecanvasdialog")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("coords"): {
            if (atoms.size() >= 8) {

                // x_range: 0 1
                // y_range: 1 -1
                // w: $4 h: 22
                // hidetext: 2
                // margin: 100 100
                // isgraph: 1

                Rectangle<int> bounds;
                if(auto gobj = ptr.get<t_gobj>())
                {
                    auto* patch = cnv->patch.getPointer().get();
                    if(!patch) return;
                    
                    int x = 0, y = 0, w = 0, h = 0;
                    libpd_get_object_bounds(patch, gobj.get(), &x, &y, &w, &h);
                    bounds = Rectangle<int>(x, y, atoms[4].getFloat(), atoms[5].getFloat());
                }

                object->setObjectBounds(bounds);
            }
            break;
        }
        case hash("donecanvasdialog"): {
            if (atoms.size() >= 11) {

                updateCanvas();
                updateDrawables();
            }
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
        
        if(auto glist = ptr.get<_glist>())
        {
            auto* patch = cnv->patch.getPointer().get();
            if(!patch) return;
            
            libpd_moveobj(patch, glist.cast<t_gobj>(), b.getX(), b.getY());
            glist->gl_pixwidth = b.getWidth() - 1;
            glist->gl_pixheight = b.getHeight() - 1;
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if(auto gobj = ptr.get<t_gobj>())
        {
            auto* patch = cnv->patch.getPointer().get();
            if(!patch) return;
            
            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(patch, gobj.get(), &x, &y, &w, &h);
            return {x, y, w + 1, h + 1};
        }

        return {};
    }

    ~GraphOnParent() override
    {
        closeOpenedSubpatchers();
    }

    void tabChanged() override
    {
        auto* leftTabbar = cnv->editor->splitView.getLeftTabbar();
        auto* rightTabbar = cnv->editor->splitView.getRightTabbar();

        auto* otherTabbar = cnv->getTabbar() == leftTabbar ? rightTabbar : leftTabbar;

        if (auto* otherCnv = otherTabbar->getCurrentCanvas()) {
            isOpenedInSplitView = otherCnv->patch == *getPatch();
        } else {
            isOpenedInSplitView = false;
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
        for (auto& object : canvas->objects) {
            if (object->gui) {
                object->gui->updateDrawables();
            }
        }
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
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    Canvas* getCanvas() override
    {
        return canvas.get();
    }

    void checkGraphState()
    {
        pd->setThis();

        int isGraph = getValue<bool>(isGraphChild);
        int hideText = isGraph && getValue<bool>(hideNameAndArgs);
        
        if(auto glist = ptr.get<t_glist>())
        {
            canvas_setgraph(glist.get(), isGraph + 2 * hideText, 0);
        }
        
        repaint();

        MessageManager::callAsync([this, _this = SafePointer(this)]() {
            if (!_this)
                return;

            // Change from graph to subpatch
            if (auto glist = ptr.get<t_canvas>()) {
                if(!glist->gl_isgraph) {
                    cnv->setSelected(object, false);
                    object->cnv->editor->sidebar->hideParameters();
                    
                    object->setType(getText(), glist.get());
                    return; // Make sure we don't run updateCanvas because class might be deleted!
                }
            }

            updateCanvas();
        });
    }

    void valueChanged(Value& v) override
    {
        

        if (v.refersToSameSourceAs(isGraphChild) || v.refersToSameSourceAs(hideNameAndArgs)) {
            checkGraphState();
        } if (v.refersToSameSourceAs(xRange)) {
            if(auto glist = ptr.get<t_canvas>()) {
                glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
                glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
            }
            updateDrawables();
        } else if (v.refersToSameSourceAs(yRange)) {
            if(auto glist = ptr.get<t_canvas>()) {
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
