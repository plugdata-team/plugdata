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
        locked = getValue<bool>(object->locked);
    }

    void update() override
    {
        objectText = getText().trimEnd();
    }

    void paint(Graphics& g) override
    {
        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());

            auto scale = getWidth() < 50 ? 0.5f : 1.0f;

            Fonts::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::commentTextColourId), numLines, scale, 14.0f, Justification::centredLeft);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto selected = object->isSelected();
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
            editor.reset(TextObjectHelper::createTextEditor(object, 14));

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
        pd->lockAudioThread();

        auto* cnvPtr = cnv->patch.getPointer();
        auto objText = editor ? editor->getText() : objectText;
        auto newNumLines = 0;

        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, ptr, objText, 14, newNumLines);

        numLines = newNumLines;

        pd->unlockAudioThread();

        return newBounds.withTrimmedBottom(4);
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return TextObjectHelper::createConstrainer(object);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        pd->lockAudioThread();

        libpd_moveobj(cnv->patch.getPointer(), ptr.get<t_gobj>(), b.getX(), b.getY());

        if (TextObjectHelper::getWidthInChars(ptr)) {
            TextObjectHelper::setWidthInChars(ptr, b.getWidth() / glist_fontwidth(cnv->patch.getPointer()));
        }

        pd->unlockAudioThread();
    }

    void setSymbol(String const& value)
    {
        auto* cstr = value.toRawUTF8();
        auto* commentObj = ptr.get<t_text>();
        auto* canvas = cnv->patch.getPointer();

        pd->lockAudioThread();
        libpd_renameobj(canvas, &commentObj->te_g, cstr, value.getNumBytesAsUTF8());
        pd->unlockAudioThread();
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
        object->updateBounds();
    }
};
