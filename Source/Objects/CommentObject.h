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
        g.setColour(object->findColour(PlugDataColour::canvasTextColourId));

        if (!editor) {

            auto textArea = getLocalBounds().reduced(4, 2);

            PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, 0.9f, 14.0f, Justification::topLeft);
            
            auto selected = cnv->isSelected(object);
            if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
                g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

                g.drawRect(getLocalBounds().toFloat(), 0.5f);
            }
        }
    }

    int getBestTextWidth(String const& text)
    {
        auto lines = StringArray::fromLines(text);
        auto maxWidth = 32;

        for (auto& line : lines) {
            maxWidth = std::max<int>(Font(14.f).getStringWidthFloat(line) + 19, maxWidth);
        }

        return maxWidth;
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

            auto newText = outgoingEditor->getText().trimEnd();

            bool changed;
            if (objectText != newText) {
                objectText = newText;
                repaint();
                changed = true;
            } else {
                changed = false;
            }

            outgoingEditor.reset();

            repaint();

            // update if the name has changed, or if pdobject is unassigned
            if (changed) {
                cnv->pd->enqueueFunction(
                    [this, _this = SafePointer<CommentObject>(this)]() mutable {
                        if (!_this)
                            return;

                        auto* newName = objectText.toRawUTF8();
                        libpd_renameobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), newName, objectText.getNumBytesAsUTF8());

                        MessageManager::callAsync(
                            [_this]() {
                                if (!_this)
                                    return;

                                _this->updateBounds();
                                _this->applyBounds();

        
                            });
                    });
            }
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(Font(14.f));

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(Label::backgroundWhenEditingColourId, Colours::transparentBlack);
            editor->setColour(Label::outlineWhenEditingColourId, object->findColour(TextEditor::focusedOutlineColourId));
            editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(true);
            editor->setReturnKeyStartsNewLine(false);
            editor->setScrollbarsShown(false);
            editor->setBorder(BorderSize<int> { 1, 4, 0, 0 });
            editor->setIndents(0, 0);
            editor->setJustification(Justification::topLeft);
            editor->setScrollToShowCursor(false);

            editor->onFocusLost = [this]() {
                hideEditor();
            };

            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(objectText, false);

            editor->addListener(this);
            editor->addKeyListener(this);

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->selectAll();

            resized();
            repaint();

            editor->grabKeyboardFocus();
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
        
        if(newNumLines > 0) {
            numLines = newNumLines;
        }
        
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
        text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);

        ed.setText(text);
        ed.setCaretPosition(caretPosition + 2);
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        updateBounds();
    }
};
