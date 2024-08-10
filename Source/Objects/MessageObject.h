/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class MessageObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    Value sizeProperty = SynchronousValue();
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(0, 5, 0, 2);

    String objectText;
    CachedTextRender textRenderer;

    bool isDown = false;
    bool isLocked = false;

    NVGcolor backgroundColour;
    NVGcolor selectedOutlineColour;
    Colour selectedColour;
    NVGcolor outlineColour;
    Colour guiOutlineCol;
    Colour flagCol;

    NVGImage flagImage;

public:
    MessageObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);

        lookAndFeelChanged();
    }

    void update() override
    {
        objectText = getSymbol();

        if (auto obj = ptr.get<t_text>()) {
            sizeProperty = TextObjectHelper::getWidthInChars(obj.get());
        }

        updateTextLayout();
    }

    Rectangle<int> getPdBounds() override
    {
        updateTextLayout(); // make sure layout height is updated

        auto textBounds = getTextSize();

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getPointer().get();
            if (!cnvPtr)
                return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 5, 20) };

            pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
        }

        return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 5, 20) };
    }

    Rectangle<int> getTextSize()
    {
        auto objText = editor ? editor->getText() : objectText;
        
        if (editor && cnv->suggestor) {
            cnv->suggestor->updateSuggestions(objText);
            if(cnv->suggestor->getText().isNotEmpty()) {
                objText = cnv->suggestor->getText();
            }
        }

        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<void>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getPointer().get());
        }

        auto textSize = textRenderer.getTextBounds();

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 14;

        // We want to adjust the width so ideal text with aligns with fontWidth
        int offset = idealWidth % fontWidth;

        int textWidth;
        if (objText.isEmpty()) { // If text is empty, set to minimum width
            textWidth = std::max(charWidth, 6) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            textWidth = std::clamp(idealWidth, TextObjectHelper::minWidth * fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth + offset;
        }

        return { textWidth, textSize.getHeight() };
    }

    void updateTextLayout()
    {
        if (cnv->isGraph)
            return; // Text layouting is expensive, so skip if it's not necessary

        auto objText = editor ? editor->getText() : objectText;
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        }

        auto colour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId);
        int textWidth = getTextSize().getWidth() - 14;
        if (textRenderer.prepareLayout(objText, Fonts::getDefaultFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty))) {
            repaint();
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());

            if (TextObjectHelper::getWidthInChars(gobj.get())) {
                TextObjectHelper::setWidthInChars(gobj.get(), b.getWidth() / glist_fontwidth(patch));
            }
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto text = ptr.get<t_text>()) {
            setParameterExcludingListener(sizeProperty, TextObjectHelper::getWidthInChars(text.get()));
        }
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void render(NVGcontext* nvg) override
    {
        auto bounds = getLocalBounds();
        auto b = bounds.toFloat();
        auto sb = b.reduced(0.5f); // reduce size of background to stop AA edges from showing through

        // Background
        nvgDrawRoundedRect(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), backgroundColour, backgroundColour, Corners::objectCornerRadius);

        auto width = getWidth();
        auto height = getHeight();

        auto pixelScale = cnv->getRenderScale();
        auto zoom = cnv->isZooming ? 2.0f : getValue<float>(cnv->zoomScale);

        auto const flagArea = Point<int>(width * pixelScale * zoom, height * pixelScale * zoom);

        if (flagImage.needsUpdate(flagArea.x, flagArea.y)) {
            flagImage = NVGImage(nvg, flagArea.x, flagArea.y, [this, pixelScale, zoom, sb, width, height](Graphics &g) {

                int d = 6;
                g.addTransform(AffineTransform::scale(pixelScale * zoom, pixelScale * zoom));
                auto b = Rectangle<int>(0, 0, width, height);
                // use the path with a hole in it to exclude the inner rounded rect from painting
                Path outerArea;
                outerArea.addRoundedRectangle(sb, Corners::objectCornerRadius);

                float bRight = b.getRight(); // offset to make it go completely under outline
                float bY = b.getY();
                float bBottom = b.getBottom();

                g.reduceClipRegion(outerArea);

                // draw rectangle when mouse down
                if (isDown) {
                    g.setColour(guiOutlineCol);
                    g.fillRect(b.getX(), b.getY(), b.getWidth(), d);
                    g.fillRect(b.getRight() - d, b.getY(), d, b.getHeight());
                    g.fillRect(b.getX(), b.getBottom() - d, b.getWidth(), d);
                    g.fillRect(b.getX(), b.getY(), d, b.getHeight());
                }

                // draw flag
                Path flag;
                flag.startNewSubPath(bRight, bY);
                flag.lineTo(bRight - d, bY + d);
                flag.lineTo(bRight - d, bBottom - d);
                flag.lineTo(bRight, bBottom);
                flag.closeSubPath();

                g.setColour(isDown && ::getValue<bool>(object->locked) ? selectedColour : flagCol);
                g.fillPath(flag);
            });
        }

        flagImage.render(nvg, getLocalBounds());

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBAf(0, 0, 0, 0), object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        if (editor) {
            imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
        } else {
            auto text = getText();
            textRenderer.renderText(nvg, border.subtractedFrom(getLocalBounds()), getImageScale());
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        String v = getSymbol();

        if (objectText != v) {

            objectText = v;

            repaint();
            object->updateBounds();
        }
    }

    void resized() override
    {
        updateTextLayout();

        if (editor) {
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
        }
    }

    void lookAndFeelChanged() override
    {
        backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        selectedColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId);
        selectedOutlineColour = convertColour(selectedColour);
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        flagCol = cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour);
        guiOutlineCol = cnv->editor->getLookAndFeel().findColour(PlugDataColour::outlineColourId);

        updateTextLayout();
    }

    bool isEditorShown() override
    {
        return editor != nullptr;
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));

            editor->setBorder(border);
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();
            editor->setReturnKeyStartsNewLine(false);

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            cnv->showSuggestions(object, editor.get());

            editor->onFocusLost = [this]() {
                if (reinterpret_cast<Component*>(cnv->suggestor.get())->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
                    editor->grabKeyboardFocus();
                    return;
                }

                hideEditor();
            };

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

            auto newText = outgoingEditor->getText();

            newText = TextObjectHelper::fixNewlines(newText);

            if (objectText != newText) {
                objectText = newText;
                object->updateBounds(); // Recalculate bounds
                setPdBounds(object->getObjectBounds());
                setSymbol(objectText);
                cnv->synchronise();
            }

            outgoingEditor.reset();
            repaint();
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (isLocked) {
            isDown = true;
            flagImage.setDirty();
            repaint();

            // startEdition();
            click();
            // stopEdition();
        }
    }

    void click()
    {
        if (auto message = ptr.get<void>()) {
            cnv->pd->sendDirectMessage(message.get(), 0);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
        flagImage.setDirty();
        repaint();
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        cnv->grabKeyboardFocus();
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        object->updateBounds();
    }

    String getSymbol() const
    {
        char* text;
        int size;

        if (auto messObj = ptr.get<t_text>()) {
            binbuf_gettext(messObj->te_binbuf, &text, &size);
        } else {
            return {};
        }

        auto result = String::fromUTF8(text, size);
        freebytes(text, size);

        return result.trimEnd();
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());

            setParameterExcludingListener(sizeProperty, width);

            if (auto text = ptr.get<t_text>()) {
                TextObjectHelper::setWidthInChars(text.get(), width);
            }

            object->updateBounds();
        }
    }

    void setSymbol(String const& value)
    {
        auto* cstr = value.toRawUTF8();
        if (auto messobj = ptr.get<t_text>()) {
            auto* canvas = cnv->patch.getPointer().get();
            if (!canvas)
                return;

            pd::Interface::renameObject(canvas, messobj.cast<t_gobj>(), cstr, value.getNumBytesAsUTF8());
        }
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        if (key.getKeyCode() == KeyPress::returnKey && editor && key.getModifiers().isShiftDown()) {
            int caretPosition = editor->getCaretPosition();
            auto text = editor->getText();

            if (!editor->getHighlightedRegion().isEmpty())
                return false;
            if (text[caretPosition - 1] == ';') {
                text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
                caretPosition += 1;
            } else {
                text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);
                caretPosition += 2;
            }

            editor->setText(text);
            editor->setCaretPosition(caretPosition);

            return true;
        }

        return false;
    }
    
    bool hideInGraph() override
    {
        return true;
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return TextObjectHelper::createConstrainer(object);
    }
};
