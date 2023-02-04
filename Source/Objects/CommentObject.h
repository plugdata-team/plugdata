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

        objectText = getText().trimEnd();
    }

    ~CommentObject()
    {
    }

    void paint(Graphics& g) override
    {
        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());

            auto scale = getWidth() < 50 ? 0.5f : 1.0f;

            PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, scale, 14.0f, Justification::centredLeft);
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

            auto newText = outgoingEditor->getText();

            newText = TextObjectHelper::fixNewlines(newText);

            if (objectText != newText) {
                objectText = newText;
            }

            outgoingEditor.reset();

            updateBounds(); // Recalculate bounds
            applyBounds();  // Send new bounds to Pd

            setSymbol(objectText);
            repaint();
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 14));

            editor->setBorder(border);
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
        auto newNumLines = 0;

        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, ptr, objText, 14, newNumLines).expanded(Object::margin) + cnv->canvasOrigin;

        numLines = newNumLines;

        auto objBounds = object->getBounds();

        // TODO: this is a hack
        // why is there a weird 1px offset, only for comment but not for textobj or message?
        if (objBounds.getPosition().getDistanceFrom(newBounds.getPosition()) > 2) {
            object->setTopLeftPosition(newBounds.getX(), newBounds.getY());
        }
        if (newBounds.getWidth() != objBounds.getWidth() || newBounds.getHeight() != objBounds.getHeight()) {
            object->setSize(newBounds.getWidth(), newBounds.getHeight());
        }

        pd->getCallbackLock()->exit();
    }

    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        auto fontWidth = glist_fontwidth(cnv->patch.getPointer());
        auto* patch = cnv->patch.getPointer();
        TextObjectHelper::checkBounds(patch, ptr, oldBounds, newBounds, resizingOnLeft, fontWidth);
        updateBounds();
        return true;
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        if (TextObjectHelper::getWidthInChars(ptr)) {
            TextObjectHelper::setWidthInChars(ptr, b.getWidth() / glist_fontwidth(cnv->patch.getPointer()));
        }
    }

    void setSymbol(String const& value)
    {
        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), ptr = this->ptr, value]() mutable {
                if (!_this || _this->cnv->patch.objectWasDeleted(_this->ptr))
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

        if (!ed.getHighlightedRegion().isEmpty())
            return;

        if (text[caretPosition - 1] == ';') {
            text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
            caretPosition += 1;
        } else {
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
