/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CommentObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    bool locked;

    Justification justification = Justification::centredLeft;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);
    float minimumHorizontalScale = 0.8f;

    String objectText;
    Font font = Font(15.0f);

    int textObjectWidth = 0;
    int numLines = 1;

    bool wasSelected = false;
    bool isValid = true;

public:
    CommentObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        justification = Justification::topLeft;
        font = font.withHeight(13.5f);

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
        g.setFont(font);

        if (!editor) {
            TextLayout textLayout;
            auto textArea = getLocalBounds().reduced(4, 2);
            AttributedString attributedObjectText(objectText);
            attributedObjectText.setColour(object->findColour(PlugDataColour::canvasTextColourId));
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

    int getBestTextWidth(String const& text)
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

                                int newHeight = (lines.size() * 17 + 3) + Object::doubleMargin;
                                int newWidth = maxWidth + Object::doubleMargin + 4;
                                
                                auto newBounds = Rectangle<int>(_this->object->getX(), _this->object->getY(), newWidth + Object::doubleMargin, newHeight + Object::doubleMargin);
                                _this->object->setBounds(newBounds);

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

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x, y, w, h;

        auto* textObj = static_cast<t_text*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        Rectangle<int> bounds = { x, y, textObj->te_width, h };
        
        auto objText = editor ? editor->getText() : objectText;

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int textWidth = getBestTextWidth(objectText);

        pd->getCallbackLock()->exit();

        // We need to handle the resizable width, which pd saves in amount of text characters
        textObjectWidth = bounds.getWidth();

        int width;
        if (textObjectWidth == 0) {
            width = std::min(textWidth / fontWidth, 60) * fontWidth;
        }
        else {
            width = textObjectWidth * fontWidth;
        }

        numLines = StringUtils::getNumLines(objText, width);
        int height = numLines * 17 + 3;

        bounds.setWidth(width);
        bounds.setHeight(height);

        object->setObjectBounds(bounds);
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* textObj = static_cast<t_text*>(ptr);
        textObj->te_width = textObjectWidth;
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

        int width = (getWidth() / fontWidth) * fontWidth;
        
        auto objText = editor ? editor->getText() : objectText;

        numLines = StringUtils::getNumLines(objText, width);
        auto height = numLines * 17 + 3;

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        ed.setText(ed.getText() + ";\n");
        ed.moveCaretToEnd();

        applyBounds();
        object->updateBounds();
    }

    void textEditorTextChanged(TextEditor& ed) override
    {
        auto text = ed.getText();
        
        // For resize-while-typing behaviour
        auto width = getBestTextWidth(text);

        width = std::max(width, getWidth());
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        width = std::min(width / fontWidth, 60) * fontWidth;
        
        numLines = StringUtils::getNumLines(text, width);
        auto height = numLines * 17 + 3;
        
        height = std::max(height, getHeight());
        
        if (width != getWidth() || height != getHeight()) {
            auto newBounds = Rectangle<int>(object->getX(), object->getY(), width + Object::doubleMargin, height + Object::doubleMargin);
            object->setBounds(newBounds);
        }
    }
};
