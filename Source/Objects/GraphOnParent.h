/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

// Also used by garray
class GraphTicks {
    int xTicksPerBig = 0, yTicksPerBig = 0;
    float xTickPoint = 0, yTickPoint = 0;
    float xTickInc = 0, yTickInc = 0;
    float gl_x1 = 0.f, gl_x2 = 1.f, gl_y1 = 100.f, gl_y2 = 0.f;

public:
    void update(t_glist const* glist)
    {
        xTicksPerBig = glist->gl_xtick.k_lperb;
        yTicksPerBig = glist->gl_ytick.k_lperb;
        xTickPoint = glist->gl_xtick.k_point;
        yTickPoint = glist->gl_ytick.k_point;
        xTickInc = glist->gl_xtick.k_inc;
        yTickInc = glist->gl_ytick.k_inc;
        gl_x1 = glist->gl_x1;
        gl_y1 = glist->gl_y1;
        gl_x2 = glist->gl_x2;
        gl_y2 = glist->gl_y2;
    }

    void render(NVGcontext* nvg, Rectangle<float> const b) const
    {
        if (xTicksPerBig) {
            t_float const y1 = b.getY(), y2 = b.getBottom(), x1 = b.getX(), x2 = b.getRight();

            t_float f = xTickPoint;
            for (int i = 0; f < 0.99f * gl_x2 + 0.01f * gl_x1; i++, f += xTickInc) {
                auto const xpos = jmap<float>(f, gl_x2, gl_x1, x1, x2);
                int const tickpix = i % xTicksPerBig ? 2 : 4;
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, xpos, y2);
                nvgLineTo(nvg, xpos, y2 - tickpix);
                nvgStroke(nvg);

                nvgBeginPath(nvg);
                nvgMoveTo(nvg, xpos, y1);
                nvgLineTo(nvg, xpos, y1 + tickpix);
                nvgStroke(nvg);
            }

            f = xTickPoint - xTickInc;
            for (int i = 1; f > 0.99f * gl_x1 + 0.01f * gl_x2; i++, f -= xTickInc) {
                auto const xpos = jmap<float>(f, gl_x2, gl_x1, x1, x2);
                int const tickpix = i % xTicksPerBig ? 2 : 4;
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, xpos, y2);
                nvgLineTo(nvg, xpos, y2 - tickpix);
                nvgStroke(nvg);

                nvgBeginPath(nvg);
                nvgMoveTo(nvg, xpos, y1);
                nvgLineTo(nvg, xpos, y1 + tickpix);
                nvgStroke(nvg);
            }
        }

        if (yTicksPerBig) {
            t_float const y1 = b.getY(), y2 = b.getBottom(), x1 = b.getX(), x2 = b.getRight();
            t_float f = yTickPoint;
            for (int i = 0; f < 0.99f * gl_y1 + 0.01f * gl_y2; i++, f += yTickInc) {
                auto const ypos = jmap<float>(f, gl_y2, gl_y1, y1, y2);
                int const tickpix = i % yTicksPerBig ? 2 : 4;
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x1, ypos);
                nvgLineTo(nvg, x1 + tickpix, ypos);
                nvgStroke(nvg);

                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x2, ypos);
                nvgLineTo(nvg, x2 - tickpix, ypos);
                nvgStroke(nvg);
            }

            f = yTickPoint - yTickInc;
            for (int i = 1; f > 0.99f * gl_y2 + 0.01f * gl_y1; i++, f -= yTickInc) {
                auto const ypos = jmap<float>(f, gl_y2, gl_y1, y1, y2);
                int const tickpix = i % yTicksPerBig ? 2 : 4;
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x1, ypos);
                nvgLineTo(nvg, x1 + tickpix, ypos);
                nvgStroke(nvg);

                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x2, ypos);
                nvgLineTo(nvg, x2 - tickpix, ypos);
                nvgStroke(nvg);
            }
        }
    }
};

class GraphOnParent final : public ObjectBase {

    Value isGraphChild = SynchronousValue(var(false));
    Value hideNameAndArgs = SynchronousValue(var(false));
    Value xRange = SynchronousValue();
    Value yRange = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    pd::Patch::Ptr subpatch;
    std::unique_ptr<Canvas> canvas;

    CachedTextRender textRenderer;

    NVGImage openInGopBackground;
    std::unique_ptr<TextEditor> editor;

    bool isLocked : 1 = false;
    bool isOpenedInSplitView : 1 = false;

    GraphTicks ticks;

public:
    // Graph On Parent
    GraphOnParent(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
        , subpatch(new pd::Patch(obj, cnv->pd, false))
    {
        resized();

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" });
        objectParameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" });
        objectParameters.addParamRange("X range", cGeneral, &xRange, { 0, 100 });
        objectParameters.addParamRange("Y range", cGeneral, &yRange, { -1, 1 });

        // There is a possibility that a donecanvasdialog message is sent inbetween the initialisation in pd and the initialisation of the plugdata object, making it possible to miss this message. This especially tends to happen if the messagebox is connected to a loadbang.
        // By running another update call asynchrounously, we can still respond to the new state
        MessageManager::callAsync([_this = SafePointer(this)] {
            if (_this) {
                _this->update();
                _this->propertyChanged(_this->isGraphChild);
            }
        });
    }

    void update() override
    {
        if (auto glist = ptr.get<t_canvas>()) {
            isGraphChild = static_cast<bool>(glist->gl_isgraph);
            hideNameAndArgs = static_cast<bool>(glist->gl_hidetext);
            xRange = VarArray { var(glist->gl_x1), var(glist->gl_x2) };
            yRange = VarArray { var(glist->gl_y2), var(glist->gl_y1) };
            sizeProperty = VarArray { var(glist->gl_pixwidth), var(glist->gl_pixheight) };
            ticks.update(glist.get());
        }

        updateCanvas();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("yticks"):
        case hash("xticks"): {
            if (auto glist = ptr.get<t_canvas>()) {
                ticks.update(glist.get());
            }
            repaint();
            break;
        }
        case hash("coords"): {
            Rectangle<int> bounds;
            if (auto gobj = ptr.get<t_gobj>()) {
                auto* patch = cnv->patch.getRawPointer();

                int x = 0, y = 0, w = 0, h = 0;
                pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
                bounds = Rectangle<int>(x, y, atoms[4].getFloat() + 1, atoms[5].getFloat() + 1);
            }
            update();
            object->setObjectBounds(bounds);
            break;
        }
        case hash("goprect"): {
            object->updateBounds();
            auto const b = getPatch()->getGraphBounds() + canvas->canvasOrigin;
            canvas->setBounds(-b.getX(), -b.getY(), b.getWidth() + b.getX(), b.getHeight() + b.getY());
            break;
        }
        case hash("donecanvasdialog"): {
            update();
            updateCanvas();
            object->updateBounds();
            break;
        }
        default:
            break;
        }
    }

    void resized() override
    {
        if (editor) {
            editor->setBounds(getLocalBounds().removeFromTop(18));
        }
        
        auto text = getText();
        if(text != "graph" && !text.isNotEmpty()) {
            textRenderer.prepareLayout(getText(), Fonts::getDefaultFont().withHeight(13), cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId), getWidth(), getWidth(), false);
        }
        updateCanvas();
        updateDrawables();

        // Update parent canvas directly (needed if open in splitview)
        canvas->synchroniseAllCanvases();
    }

    void lookAndFeelChanged() override
    {
        auto text = getText();
        if(text != "graph" && !text.isNotEmpty()) {
            textRenderer.prepareLayout(getText(), Fonts::getDefaultFont().withHeight(13), cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId), getWidth(), getWidth(), false);
        }
    }

    void showEditor() override
    {
        if (!getValue<bool>(hideNameAndArgs) && editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 13));

            editor->setLookAndFeel(&object->getLookAndFeel());
            editor->setBorder(BorderSize<int>(2, 5, 2, 1));
            editor->setBounds(getLocalBounds().removeFromTop(18));
            editor->setText(getText(), false);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onReturnKey = [this] {
                cnv->grabKeyboardFocus();
            };
            editor->onFocusLost = [this] {
                if (cnv->suggestor->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
                    editor->grabKeyboardFocus();
                    return;
                }
                hideEditor();
            };

            cnv->showSuggestions(object, editor.get());

            object->updateBounds();
            repaint();
        }
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            cnv->hideSuggestions();

            auto const oldText = getText();
            auto newText = outgoingEditor->getText();

            newText = TextObjectHelper::fixNewlines(newText);

            outgoingEditor.reset();

            if (oldText != newText) {
                object->setType(newText);
            }
        }
    }

    // Called by object to make sure clicks on empty parts of the graph are passed on
    bool canReceiveMouseEvent(int const x, int const y) override
    {
        if (!canvas)
            return true;

        if (ModifierKeys::getCurrentModifiers().isRightButtonDown())
            return true;

        if (!isLocked)
            return true;

        for (auto const& obj : canvas->objects) {
            if (!obj->gui)
                continue;

            auto const localPoint = obj->getLocalPoint(object, Point<int>(x, y));

            if (obj->hitTest(localPoint.x, localPoint.y)) {
                return true;
            }
        }

        return false;
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto glist = ptr.get<_glist>()) {
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, glist.cast<t_gobj>(), b.getX(), b.getY());
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

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto glist = ptr.get<t_glist>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(glist->gl_pixwidth), var(glist->gl_pixheight) });
        }
    }

    ~GraphOnParent() override
    {
        if (getValue<bool>(isGraphChild)) {
            closeOpenedSubpatchers();
        }
    }

    void tabChanged() override
    {
        auto setIsOpenedInSplitView = [this](bool const shouldBeOpen) {
            if (isOpenedInSplitView != shouldBeOpen) {
                isOpenedInSplitView = shouldBeOpen;
                repaint();
            }
        };

        setIsOpenedInSplitView(false);
        for (auto* editor : cnv->pd->getEditors()) {
            for (auto const* visibleCanvas : editor->getTabComponent().getVisibleCanvases()) {
                if (visibleCanvas->patch == *getPatch()) {
                    setIsOpenedInSplitView(true);
                    break;
                }
            }
        }

        updateCanvas();
        repaint();
    }

    void lock(bool const locked) override
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

        auto const b = getPatch()->getGraphBounds() + canvas->canvasOrigin;
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

    void render(NVGcontext* nvg) override
    {
        // Strangly, the title goes below the graph content in pd
        if (!getValue<bool>(hideNameAndArgs)) {
            if (editor && editor->isVisible()) {
                imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
            } else {
                auto const text = getText();
                if (text != "graph" && text.isNotEmpty()) {
                    textRenderer.renderText(nvg, Rectangle<float>(5, 0, getWidth() - 5, 16), getImageScale());
                }
            }
        }

        auto const b = getLocalBounds().toFloat();
        if (canvas) {
            auto invalidArea = cnv->currentRenderArea;

            invalidArea = invalidArea.getIntersection(cnv->getLocalArea(this, getLocalBounds()));

            if (invalidArea.isEmpty())
                return;

            invalidArea = canvas->getLocalArea(cnv, invalidArea).expanded(1);

            NVGScopedState scopedState(nvg);
            nvgIntersectRoundedScissor(nvg, b.getX() + 0.75f, b.getY() + 0.75f, b.getWidth() - 1.5f, b.getHeight() - 1.5f, Corners::objectCornerRadius);
            nvgTranslate(nvg, canvas->getX(), canvas->getY());
            canvas->performRender(nvg, invalidArea);
        }

        if (isOpenedInSplitView) {
            auto width = getWidth();
            auto height = getHeight();

            if (openInGopBackground.needsUpdate(width, height)) {
                auto bgColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId);

                openInGopBackground = NVGImage(nvg, width, height, [width, height, bgColour](Graphics& g) {
                    AffineTransform rotate;
                    rotate = rotate.rotated(MathConstants<float>::pi / 4.0f);
                    g.fillAll(bgColour);
                    g.addTransform(rotate);
                    float const diagonalLength = std::sqrt(width * width + height * height);
                    g.setColour(bgColour.contrasting(0.5f).withAlpha(0.12f));
                    auto constexpr stripeWidth = 20.0f;
                    for (float x = -diagonalLength; x < diagonalLength; x += stripeWidth * 2) {
                        g.fillRect(x, -diagonalLength, stripeWidth, diagonalLength * 2);
                    }
                    g.addTransform(rotate.inverted());
                });
            }
            auto const imagePaint = nvgImagePattern(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), 0.0f, openInGopBackground.getImageId(), 1.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
            nvgFillPaint(nvg, imagePaint);
            nvgFill(nvg);
            
            auto const errorText = String("Graph open in split view");

            auto const stringLength = Fonts::getStringWidth(errorText, 12);
            if (stringLength < getWidth() - Object::doubleMargin - 20 /* 20 is a hack for now */ && getHeight() > 12) {
                nvgBeginPath(nvg);
                nvgFontFace(nvg, "Inter-Regular");
                nvgFontSize(nvg, 12.0f);
                nvgFillColor(nvg, cnv->commentTextCol); // why comment colour?
                nvgTextAlign(nvg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
                nvgText(nvg, b.getCentreX(), b.getCentreY(), errorText.toRawUTF8(), nullptr);
            }
        }

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);

        nvgStrokeColor(nvg, cnv->guiObjectInternalOutlineCol);
        ticks.render(nvg, b);
    }
    
    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        // Custom constrainer because a regular ComponentBoundsConstrainer will mess up the aspect ratio
        class GraphBoundsConstrainer : public ComponentBoundsConstrainer {
            
        public:
            explicit GraphBoundsConstrainer()
            {
            }

            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool const isStretchingTop,
                bool const isStretchingLeft,
                bool const isStretchingBottom,
                bool const isStretchingRight) override
            {
                bounds = old; // Don't allow resizing graph from the outside
            }
        };

        return std::make_unique<GraphBoundsConstrainer>();
    }
    
    ResizeDirection getAllowedResizeDirections() const override
    {
        return ResizeDirection::None;
    }

    
    void onConstrainerCreate() override
    {
        constrainer->setFixedAspectRatio(1);
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto glist = ptr.get<t_glist>()) {
                glist->gl_pixwidth = width;
                glist->gl_pixheight = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
            int const hideText = getValue<bool>(hideNameAndArgs);
            if (auto glist = ptr.get<t_glist>()) {
                canvas_setgraph(glist.get(), glist->gl_isgraph + 2 * hideText, 0);
            }
            repaint();
        } else if (v.refersToSameSourceAs(isGraphChild)) {
            int const isGraph = getValue<bool>(isGraphChild);

            if (auto glist = ptr.get<t_glist>()) {
                canvas_setgraph(glist.get(), isGraph + 2 * (isGraph && glist->gl_hidetext), 0);
            }

            if (!isGraph) {
                MessageManager::callAsync([_this = SafePointer(this)] {
                    if (!_this)
                        return;

                    _this->cnv->setSelected(_this->object, false);
                    _this->object->editor->sidebar->hideParameters();

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

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open", [_this = SafePointer(this)] { if(_this) _this->openSubpatch(); });
    }
    
    bool checkHvccCompatibility() override
    {
        return recurseHvccCompatibility(getText(), subpatch.get());
    }
};
