/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct CommentObject final : public TextBase
{
    CommentObject(void* obj, Box* box) : TextBase(obj, box)
    {
        setInterceptsMouseClicks(false, false);

        // Our component doesn't intercept mouse events, so dragging will be okay
        box->addMouseListener(this, false);
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::textColourId));
        g.setFont(font);

        auto textArea = border.subtractedFrom(getLocalBounds());
        g.drawFittedText(currentText, textArea, justification, numLines, minimumHorizontalScale);
    }


    void hideEditor() override
    {
        if (editor != nullptr)
        {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            if (auto* peer = getPeer()) peer->dismissPendingTextInput();

            outgoingEditor->setInputFilter(nullptr, false);

            auto newText = outgoingEditor->getText();

            bool changed;
            if (currentText != newText)
            {
                currentText = newText;
                repaint();
                changed = true;
            }
            else
            {
                changed = false;
            }

            outgoingEditor.reset();

            repaint();

            // update if the name has changed, or if pdobject is unassigned
            if (changed)
            {
                SafePointer<CommentObject> obj;
                cnv->pd->enqueueFunction(
                    [this, obj]() mutable
                    {
                        if(!obj) return;
                        
                        auto* newName = currentText.toRawUTF8();
                        libpd_renameobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), newName, currentText.getNumBytesAsUTF8());

                        MessageManager::callAsync([this, obj]() {
                            if(!obj) return;
                            box->updateBounds(); });
                    });
                
                box->setType(newText);
            }
        }
    }
    
    void showEditor() override
    {
        if (editor == nullptr)
        {
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(font);

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
            editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(false);
            editor->setReturnKeyStartsNewLine(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(justification);

            editor->onFocusLost = [this]()
            {
                // Necessary so the editor doesn't close when clicking on a suggestion
                if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true))
                {
                    hideEditor();
                }
            };

            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(currentText, false);
            editor->addListener(this);

            if (editor == nullptr)  // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, currentText.length()));

            resized();
            repaint();

            editor->grabKeyboardFocus();
        }
    }
    
    bool hideInGraph() override { return true; }

};
