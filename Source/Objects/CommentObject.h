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
        
    TextLayout textLayout;
    hash32 layoutTextHash;
        
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);
    String objectText;

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

    void paint(Graphics& g) override
    {
        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());
            textLayout.draw(g, textArea.toFloat());
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto selected = object->isSelected();
        if (!locked && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

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
            }

            outgoingEditor.reset();

            object->updateBounds(); // Recalculate bounds
            setPdBounds(object->getObjectBounds());

            setSymbol(objectText);
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

            editor->setBorder(border);
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->setColour(TextEditor::textColourId, object->findColour(PlugDataColour::commentTextColourId));

            editor->onFocusLost = [this]() {
                hideEditor();
            };

            resized();
            repaint();
        }
    }

        Rectangle<int> getPdBounds() override
        {
            updateTextLayout(); // make sure layout height is updated

            int x = 0, y = 0, w, h;
            if (auto obj = ptr.get<t_gobj>()) {
                auto* cnvPtr = cnv->patch.getPointer().get();
                if (!cnvPtr) return {x, y, getTextObjectWidth(), std::max<int>(textLayout.getHeight() + 6, 21)};
        
                pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
            }

            return {x, y, getTextObjectWidth(), std::max<int>(textLayout.getHeight() + 6, 21)};
        }
            
        int getTextObjectWidth()
        {
            auto objText = editor ? editor->getText() : objectText;

            int fontWidth = 7;
            int charWidth = 0;
            if (auto obj = ptr.get<void>()) {
                charWidth = TextObjectHelper::getWidthInChars(obj.get());
                fontWidth = glist_fontwidth(cnv->patch.getPointer().get());
            }
            
            // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
            int idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 12;
            
            // We want to adjust the width so ideal text with aligns with fontWidth
            int offset = idealWidth % fontWidth;
            
            int textWidth;
            if (objText.isEmpty()) { // If text is empty, set to minimum width
                textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth;
            } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
                textWidth = std::clamp(idealWidth, TextObjectHelper::minWidth * fontWidth, fontWidth * 60);
            } else { // If width was set manually, calculate what the width is
                textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth + offset;
            }
            
            return textWidth;
        }
            
        void updateTextLayout()
        {
            auto objText = editor ? editor->getText() : objectText;
            
            int textWidth = getTextObjectWidth() - 12; // Reserve a bit of extra space for the text margin
            auto currentLayoutHash = hash(objText) ^ textWidth;
            if(layoutTextHash != currentLayoutHash)
            {
                auto attributedText = AttributedString(objText);
                attributedText.setColour(object->findColour(PlugDataColour::canvasTextColourId));
                attributedText.setJustification(Justification::centredLeft);
                attributedText.setFont(Font(15));
                
                textLayout = TextLayout();
                textLayout.createLayout(attributedText, textWidth);
                layoutTextHash = currentLayoutHash;
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
        if (key == KeyPress::rightKey && editor && !editor->getHighlightedRegion().isEmpty()) {
            editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
            return true;
        }
        if (key == KeyPress::leftKey && editor && !editor->getHighlightedRegion().isEmpty()) {
            editor->setCaretPosition(editor->getHighlightedRegion().getStart());
            return true;
        }
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
        if (editor) {
            editor->setBounds(getLocalBounds());
        }
        
        updateTextLayout();
    }

    void textEditorReturnKeyPressed(TextEditor&) override
    {
        cnv->grabKeyboardFocus();
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor&) override
    {
        updateTextLayout();
        object->updateBounds();
    }
};
