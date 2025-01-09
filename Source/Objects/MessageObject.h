/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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

        auto const textBounds = getTextSize();

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getRawPointer();
            pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
        }

        return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 5, 20) };
    }

    Rectangle<int> getTextSize()
    {
        auto objText = editor ? editor->getText() : objectText;

        if (editor && cnv->suggestor) {
            cnv->suggestor->updateSuggestions(objText);
            if (cnv->suggestor->getText().isNotEmpty()) {
                objText = cnv->suggestor->getText();
            }
        }

        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<void>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getRawPointer());
        }

        auto const textSize = textRenderer.getTextBounds();

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int const idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 14;

        // We want to adjust the width so ideal text with aligns with fontWidth
        int const offset = idealWidth % fontWidth;

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

        auto const colour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId);
        int const textWidth = getTextSize().getWidth() - 14;
        if (textRenderer.prepareLayout(objText, Fonts::getCurrentFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty), false)) {
            repaint();
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

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

    void lock(bool const locked) override
    {
        isLocked = locked;
    }

    void render(NVGcontext* nvg) override
    {
        auto const bounds = getLocalBounds();
        auto const b = bounds.toFloat();
        auto const sb = b.reduced(0.5f); // reduce size of background to stop AA edges from showing through

        auto const bgCol = isDown ? cnv->outlineCol : cnv->guiObjectBackgroundCol;

        // Draw background
        nvgDrawObjectWithFlag(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(),
            bgCol, bgCol, bgCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagMessage, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());

        auto const flagCol = isDown && ::getValue<bool>(object->locked) ? cnv->selectedOutlineCol : cnv->guiObjectInternalOutlineCol;
        auto const outlineCol = object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        // Draw highlight around inner area when box is clicked
        // We do this by drawing an inner area that is bright, while changing the background colour darker
        if (isDown) {
            auto const dB = bounds.reduced(5);
            nvgDrawRoundedRect(nvg, dB.getX(), dB.getY(), dB.getWidth(), dB.getHeight(), cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol, 0);
        }

        // Draw outline & flag with shader
        nvgDrawObjectWithFlag(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(),
            nvgRGBA(0, 0, 0, 0), outlineCol, flagCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagMessage, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());

        if (editor) {
            imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
        } else {
            auto text = getText();
            textRenderer.renderText(nvg, border.subtractedFrom(getLocalBounds()), getImageScale());
        }
    }

    void receiveObjectMessage(hash32 symbol, SmallArray<pd::Atom> const& atoms) override
    {
        if (symbol == hash("float"))
            return;

        String const v = getSymbol();

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

            editor->onFocusLost = [this] {
                if (cnv->suggestor->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
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
            repaint();

            // startEdition();
            click();
            // stopEdition();
        }
    }

    void click()
    {
        cnv->pd->enqueueFunctionAsync<t_pd>(ptr, [](t_pd* obj) {
            pd_float(obj, 0);
        });
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
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

        auto const result = String::fromUTF8(text, size);
        freebytes(text, size);

        return result.trimEnd();
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const width = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());

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
            auto* canvas = cnv->patch.getRawPointer();
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
