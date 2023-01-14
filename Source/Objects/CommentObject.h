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
        justification = Justification::topLeft;
        font = font.withHeight(13.5f);

        locked = static_cast<bool>(object->locked.getValue());
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        if (!editor) {
            TextLayout textLayout;
            auto textArea = getLocalBounds().reduced(4, 2);
            AttributedString attributedObjectText(objectText);
            attributedObjectText.setColour(findColour(PlugDataColour::canvasTextColourId));
            attributedObjectText.setFont(font);
            attributedObjectText.setJustification(justification);
            textLayout.createLayout(attributedObjectText, textArea.getWidth());
            textLayout.draw(g, textArea.toFloat());

            auto selected = cnv->isSelected(object);
            if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
                g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

                g.drawRect(getLocalBounds().toFloat(), 0.5f);
            }
        }
    }

    int getBestTextWidth(String const& text) override
    {
        auto lines = StringArray::fromLines(text);
        auto maxWidth = 32;

        for (auto& line : lines) {
            maxWidth = std::max<int>(font.getStringWidthFloat(line) + 19, maxWidth);
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

                                auto lines = StringArray::fromTokens(_this->objectText, ";\n", "");
                                auto maxWidth = 32;

                                for (auto& line : lines) {
                                    maxWidth = std::max<int>(_this->font.getStringWidthFloat(line) + 19, maxWidth);
                                }

                                int newHeight = (lines.size() * 20) + Object::doubleMargin;
                                int newWidth = maxWidth + +Object::doubleMargin + 4;

                                auto newBounds = Rectangle<int>(_this->object->getX(), _this->object->getY(), newWidth, newHeight);
                                _this->object->setObjectBounds(newBounds.reduced(Object::margin));

                                _this->applyBounds();

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
            editor->setColour(Label::backgroundWhenEditingColourId, Colours::transparentBlack);
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));
            editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(true);
            editor->setReturnKeyStartsNewLine(false);
            editor->setScrollbarsShown(false);
            editor->setBorder(BorderSize<int> { 1, 4, 0, 0 });
            editor->setIndents(0, 0);
            editor->setJustification(justification);

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
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        textObjectWidth = (getWidth() - textWidthOffset) / fontWidth;

        int width = textObjectWidth * fontWidth + textWidthOffset;

        numLines = StringUtils::getNumLines(objectText, width);
        int height = numLines * 20;

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    bool locked;
};
