// Text base class that text objects with special implementation details can derive from
struct TextBase : public ObjectBase
    , public TextEditor::Listener {
    TextBase(void* obj, Object* parent, bool valid = true)
        : ObjectBase(obj, parent)
        , isValid(valid)
    {
        currentText = getText();

        // To get enter/exit messages
        addMouseListener(object, false);
    }

    ~TextBase()
    {
        removeMouseListener(object);
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* textObj = static_cast<t_text*>(ptr);
        textObj->te_width = textObjectWidth;
    }

    void resized() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        textObjectWidth = (getWidth() - textWidthOffset) / fontWidth;

        int width = textObjectWidth * fontWidth + textWidthOffset;
        width = std::max(width, std::max({ 1, object->numInputs, object->numOutputs }) * 18);

        numLines = getNumLines(currentText, width);
        int height = numLines * 15 + 6;

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        TextLayout textLayout;
        auto textArea = border.subtractedFrom(getLocalBounds());
        AttributedString attributedCurrentText(currentText);
        attributedCurrentText.setColour(object->findColour(PlugDataColour::canvasTextColourId));
        attributedCurrentText.setFont(font);
        attributedCurrentText.setJustification(justification);
        textLayout.createLayout(attributedCurrentText, textArea.getWidth());
        textLayout.draw(g, textArea.toFloat());

        bool selected = cnv->isSelected(object);

        Colour outlineColour;

        if(cnv->isSelected(object) && !cnv->isGraph) {
            outlineColour = object->findColour(PlugDataColour::canvasActiveColourId);
        }
        else {
            outlineColour = object->findColour(static_cast<bool>(object->locked.getValue()) ? PlugDataColour::canvasLockedOutlineColourId : PlugDataColour::canvasUnlockedOutlineColourId);
        }
        
        if (!isValid) {
            outlineColour = selected && !cnv->isGraph ? Colours::red.brighter(1.5) : Colours::red;
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    void updateValue() override {};

    // Override to cancel default behaviour
    void lock(bool isLocked) override
    {
    }

    virtual int getBestTextWidth(String const& text)
    {
        return std::max<float>(round(font.getStringWidthFloat(text) + 14.0f), 32);
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (editor != nullptr) {
            editor->giveAwayKeyboardFocus();
        }
    }

    void textEditorTextChanged(TextEditor& ed) override
    {
        // For resize-while-typing behaviour
        auto width = getBestTextWidth(ed.getText());

        if (width > getWidth()) {
            setSize(width, getHeight());
        }
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x, y, w, h;

        auto* textObj = static_cast<t_text*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        Rectangle<int> bounds = { x, y, textObj->te_width, h };

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int textWidth = getBestTextWidth(currentText);

        pd->getCallbackLock()->exit();

        // We need to handle the resizable width, which pd saves in amount of text characters
        textWidthOffset = textWidth % fontWidth;
        textObjectWidth = bounds.getWidth();

        if (textObjectWidth == 0) {
            textObjectWidth = (textWidth - textWidthOffset) / fontWidth;
        }

        int width = textObjectWidth * fontWidth + textWidthOffset;
        width = std::max(width, std::max({ 1, object->numInputs, object->numOutputs }) * 18);

        numLines = getNumLines(currentText, width);
        int height = numLines * 15 + 6;

        bounds.setWidth(width);
        bounds.setHeight(height);

        object->setObjectBounds(bounds);
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            cnv->hideSuggestions();

            auto newText = outgoingEditor->getText();

            bool changed;
            if (currentText != newText) {
                currentText = newText;
                repaint();
                changed = true;
            } else {
                changed = false;
            }

            outgoingEditor.reset();

            repaint();

            // update if the name has changed, or if pdobject is unassigned
            if (changed) {
                object->setType(newText);
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
            editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

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

            cnv->showSuggestions(object, editor.get());

            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(currentText, false);
            editor->addListener(this);

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, currentText.length()));

            resized();
            repaint();

            if (isShowing()) {
                editor->grabKeyboardFocus();
            }
        }
    }

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    TextEditor* getCurrentTextEditor() const
    {
        return editor.get();
    }

    bool hideInGraph() override
    {
        return true;
    }

protected:
    Justification justification = Justification::centredLeft;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border { 1, 7, 1, 2 };
    float minimumHorizontalScale = 0.8f;

    String currentText;
    Font font { 15.0f };

    int textObjectWidth = 0;
    int textWidthOffset = 0;
    int numLines = 1;

    bool wasSelected = false;
    bool isValid = true;
};

// Actual text object, marked final for optimisation
struct TextObject final : public TextBase {
    TextObject(void* obj, Object* parent, bool isValid = true)
        : TextBase(obj, parent, isValid)
    {
    }
};
