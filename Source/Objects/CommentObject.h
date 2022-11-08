/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct CommentObject final : public TextBase
    , public KeyListener {
    CommentObject(void* obj, Object* object)
        : TextBase(obj, object)
    {
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        if (!editor) {
            TextLayout textLayout;
            auto textArea = border.subtractedFrom(getLocalBounds());
            AttributedString attributedObjectText(objectText);
            attributedObjectText.setColour(findColour(PlugDataColour::canvasTextColourId));
            attributedObjectText.setFont(font);
            attributedObjectText.setJustification(justification);
            textLayout.createLayout(attributedObjectText, textArea.getWidth());
            textLayout.draw(g, textArea.toFloat());

            auto selected = cnv->isSelected(object);
            if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
                g.setColour(object->findColour(selected ?  PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

                g.drawRect(getLocalBounds().toFloat(), 0.5f);
            }
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
                                _this->object->updateBounds();
                            });
                    });
            }
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(font);

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
            editor->setColour(Label::backgroundWhenEditingColourId, Colours::transparentWhite);
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));
            editor->setColour(TextEditor::backgroundColourId, Colours::transparentWhite);

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(false);
            editor->setReturnKeyStartsNewLine(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(justification);
            
            

            editor->onFocusLost = [this]() {
                // Necessary so the editor doesn't close when clicking on a suggestion
                if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true)) {
                    hideEditor();
                }
            };

            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(objectText, false);
            objectText = "";
            
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

    bool hideInGraph() override
    {
        return false;
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
};
