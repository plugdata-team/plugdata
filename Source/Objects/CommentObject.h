/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CommentObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    bool locked;

    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);
    String objectText;
    int numLines = 1;

public:
    CommentObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        locked = static_cast<bool>(object->locked.getValue());

        objectText = getText();

        // To get enter/exit messages
        addMouseListener(object, false);
    }

    ~CommentObject()
    {
        removeMouseListener(object);
    }

    void paint(Graphics& g) override
    {
        if (!editor) {
            auto textArea = getLocalBounds().reduced(4, 2);

            PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, 0.8f, 14.0f, Justification::centredLeft);
        }
    }
        
    void paintOverChildren(Graphics& g) override
    {
        auto selected = cnv->isSelected(object);
        if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

            g.drawRect(getLocalBounds().toFloat(), 0.5f);
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            auto newText = outgoingEditor->getText();

            newText = TextObjectHelper::fixNewlines(newText);
            
            bool changed;
            if (objectText != newText) {
                objectText = newText;
                repaint();
                changed = true;
            } else {
                changed = false;
            }

            outgoingEditor.reset();

            updateBounds(); // Recalculate bounds
            applyBounds();  // Send new bounds to Pd

            // update if the name has changed, or if pdobject is unassigned
            if (changed) {
                setSymbol(objectText);
            }
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 14));
            
            editor->setBorder(BorderSize<int> { 1, 4, 0, 0 });
            editor->setJustification(Justification::centredLeft);
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();
            
            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();
            
            editor->onFocusLost = [this]() {
                hideEditor();
            };

            resized();
            repaint();
        }
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        auto* cnvPtr = cnv->patch.getPointer();
        auto objText = editor ? editor->getText() : objectText;
        auto* textObj = static_cast<t_text*>(ptr);
        auto newNumLines = 0;
        
        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, textObj, objText, 14, newNumLines);
        
        numLines = newNumLines;
        
        if(newBounds != object->getObjectBounds()) {
            object->setObjectBounds(newBounds);
        }
        
        pd->getCallbackLock()->exit();
    }

    void checkBounds() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        TextObjectHelper::setWidthInChars(ptr, getWidth() / fontWidth);
        updateBounds();
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        if(TextObjectHelper::getWidthInChars(ptr)) {
            TextObjectHelper::setWidthInChars(ptr,  b.getWidth() / glist_fontwidth(cnv->patch.getPointer()));
        }
    }
        
    void setSymbol(String const& value)
    {
        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), ptr = this->ptr, value]() mutable {
                if (!_this)
                    return;

                auto* cstr = value.toRawUTF8();
                auto* messobj = static_cast<t_message*>(ptr);
                auto* canvas = _this->cnv->patch.getPointer();

                libpd_renameobj(canvas, &messobj->m_text.te_g, cstr, value.getNumBytesAsUTF8());
            });
    }
        
    bool hideInGraph() override
    {
        return false;
    }

    // Override to cancel default behaviour
    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked;
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        if (key == KeyPress::rightKey && editor && !editor->getHighlightedRegion().isEmpty()) {
            editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
            return true;
        }
        if (key == KeyPress::leftKey && editor && !editor->getHighlightedRegion().isEmpty()) {
            editor->setCaretPosition(editor->getHighlightedRegion().getStart());
            return true;
        }
        return false;
    }

    void resized() override
    {
        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        int caretPosition = ed.getCaretPosition();
        auto text = ed.getText();
        
        if(text[caretPosition - 1] == ';') {
            text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
            caretPosition += 1;
        }
        else {
            text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);
            caretPosition += 2;
        }
        
        ed.setText(text);
        ed.setCaretPosition(caretPosition);
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        updateBounds();
    }
};
