/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

typedef struct _messresponder {
    t_pd mr_pd;
    t_outlet* mr_outlet;
} t_messresponder;

typedef struct _message {
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist* m_glist;
    t_clock* m_clock;
} t_message;

struct MessageObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {
    bool isDown = false;
    bool isLocked = false;

    MessageObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        objectText = getText();

        // To get enter/exit messages
        addMouseListener(object, false);
    }

    ~MessageObject()
    {
        removeMouseListener(object);
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;

        // If it's a text object, we need to handle the resizable width, which pd saves in amount of text characters
        auto* textObj = static_cast<t_text*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        w = std::max(35, textObj->te_width * glist_fontwidth(cnv->patch.getPointer()));

        if (textObj->te_width == 0 && !getText().isEmpty()) {
            w = getBestTextWidth(getText());
        }

        pd->getCallbackLock()->exit();

        object->setObjectBounds({ x, y, w, h });
    }

    void checkBounds() override
    {
        int numLines = StringUtils::getNumLines(getText(), object->getWidth() - Object::doubleMargin - 5);
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int newHeight = (numLines * 19) + Object::doubleMargin + 2;
        int newWidth = getWidth() / fontWidth;

        static_cast<t_text*>(ptr)->te_width = newWidth;
        newWidth = std::max((newWidth * fontWidth), 35) + Object::doubleMargin;

        if (getParentComponent() && (object->getHeight() != newHeight || newWidth != object->getWidth())) {
            object->setSize(newWidth, newHeight);
        }
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* textObj = static_cast<t_text*>(ptr);
        textObj->te_width = b.getWidth() / glist_fontwidth(cnv->patch.getPointer());
    }

    void paint(Graphics& g) override
    {
        BorderSize<int> border { 1, 6, 1, 4 };

        g.setColour(object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        g.setColour(object->findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        auto textArea = border.subtractedFrom(getLocalBounds());
        g.drawFittedText(objectText, textArea, justification, numLines, minimumHorizontalScale);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        if (!isValid) {
            outlineColour = selected ? Colours::red.brighter(1.5) : Colours::red;
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(1);

        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());

        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        g.fillPath(flagPath);

        if (isDown) {
            g.drawRoundedRectangle(b.reduced(1).toFloat(), PlugDataLook::objectCornerRadius, 3.0f);
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

    void updateValue()
    {
        String v = getSymbol();

        if (objectText != v && !v.startsWith("click")) {

            objectText = v;

            repaint();
        }
    }

    void receiveMessage(String const& symbol, int argc, t_atom* argv) override
    {
        MessageManager::callAsync([_this = SafePointer(this)]() {
            if (_this)
                _this->updateValue();
        });
    }

    void resized() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        textObjectWidth = (getWidth() - textWidthOffset) / fontWidth;

        int width = textObjectWidth * fontWidth + textWidthOffset;
        width = std::max(width, std::max({ 1, object->numInputs, object->numOutputs }) * 18);

        numLines = StringUtils::getNumLines(objectText, width);
        int height = numLines * 19 + 2;

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            BorderSize<int> border { 1, 6, 1, 4 };

            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(font);

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(TextEditor::backgroundColourId, object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(true);
            editor->setReturnKeyStartsNewLine(false);
            editor->setScrollbarsShown(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(justification);

            editor->setSize(10, 10);

            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);

            addAndMakeVisible(editor.get());

            editor->onFocusLost = [this]() {
                hideEditor();
            };

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, objectText.length()));

            resized();
            repaint();

            if (isShowing()) {
                editor->grabKeyboardFocus();
            }
        }
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            auto newText = outgoingEditor->getText();

            if (objectText != newText) {
                objectText = newText;
                repaint();
            } else {
            }

            outgoingEditor.reset();

            repaint();

            // Calculate size of new text
            auto lines = StringArray::fromTokens(newText, ";\n", "");
            auto maxWidth = 32;

            for (auto& line : lines) {
                maxWidth = std::max<int>(font.getStringWidthFloat(line) + 19, maxWidth);
            }

            int newHeight = (lines.size() * 19) + Object::doubleMargin;
            int newWidth = maxWidth + Object::doubleMargin + 4;

            auto newBounds = Rectangle<int>(object->getX(), object->getY(), newWidth, newHeight);
            object->setObjectBounds(newBounds.reduced(Object::margin));

            applyBounds();

            object->setType(newText);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isLocked) {
            isDown = true;
            repaint();

            // startEdition();
            click();
            // stopEdition();
        }
    }

    void click()
    {
        cnv->pd->enqueueDirectMessages(ptr, 0);
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
        repaint();
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

    String getSymbol() const
    {
        cnv->pd->setThis();

        char* text;
        int size;

        binbuf_gettext(static_cast<t_message*>(ptr)->m_text.te_binbuf, &text, &size);

        auto result = String::fromUTF8(text, size);
        freebytes(text, size);

        return result;
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
        return true;
    }

    Justification justification = Justification::centredLeft;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);
    float minimumHorizontalScale = 0.8f;

    String objectText;
    Font font = Font(15.0f);

    int textObjectWidth = 0;
    int textWidthOffset = 0;
    int numLines = 1;

    bool wasSelected = false;
    bool isValid = true;
};
