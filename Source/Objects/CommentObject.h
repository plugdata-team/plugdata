/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CommentObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    bool locked;

    Value sizeProperty = SynchronousValue();

    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 3, 0, 0);
    String objectText;

    CachedTextRender textRenderer;

public:
    CommentObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
        locked = getValue<bool>(object->locked);

        updateTextLayout();
    }

    bool isTransparent() override
    {
        return true;
    }

    void update() override
    {
        objectText = getText().trimEnd();

        if (auto obj = ptr.get<t_text>()) {
            sizeProperty = TextObjectHelper::getWidthInChars(obj.get());
        }

        updateTextLayout();
    }

    void render(NVGcontext* nvg) override
    {
        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());
            textRenderer.renderText(nvg, textArea, getImageScale());
        } else {
            imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto selected = object->isSelected();
        if (!locked && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        }
    }

    void mouseEnter(MouseEvent const&) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const&) override
    {
        repaint();
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

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

    bool isEditorShown() override
    {
        return editor != nullptr;
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));
            editor->setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::commentTextColourId));
            
            editor->setBorder(border.addedTo(BorderSize<int>(1, 0, 0, 0)));
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();
            editor->setJustification(Justification::topLeft);
            
  
            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onFocusLost = [this]() {
                hideEditor();
            };

            object->updateBounds();
            repaint();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        updateTextLayout(); // make sure layout height is updated

        auto textBounds = getTextSize();

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getPointer().get();
            if (!cnvPtr)
                return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 4, 19) };

            pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
        }

        return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 4, 19) };
    }

    Rectangle<int> getTextSize()
    {
        auto objText = editor ? editor->getText() : objectText;
        
        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<void>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getPointer().get());
        }

        auto textSize = textRenderer.getTextBounds();

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 8;

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

    void lookAndFeelChanged() override
    {
        updateTextLayout();
    }

    void updateTextLayout()
    {
        auto objText = editor ? editor->getText() : objectText;

        auto colour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::commentTextColourId);
        int textWidth = getTextSize().getWidth() - 8;
        if (textRenderer.prepareLayout(objText, Fonts::getDefaultFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty))) {
            repaint();
        }
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return TextObjectHelper::createConstrainer(object);
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
        if (auto comment = ptr.get<t_text>()) {
            auto* cstr = value.toRawUTF8();
            auto* canvas = cnv->patch.getPointer().get();
            if (!canvas)
                return;

            pd::Interface::renameObject(canvas, comment.cast<t_gobj>(), cstr, value.getNumBytesAsUTF8());
        }
    }

    bool hideInGraph() override
    {
        return false;
    }

    void lock(bool isLocked) override
    {
        locked = isLocked;
        repaint();
    }

    bool canReceiveMouseEvent(int, int) override
    {
        return !locked;
    }

    bool keyPressed(KeyPress const& key, Component*) override
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

    void resized() override
    {
        updateTextLayout();

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    void textEditorReturnKeyPressed(TextEditor&) override
    {
        cnv->grabKeyboardFocus();
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor&) override
    {
        object->updateBounds();
    }
};
