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

class MessageObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);

    String objectText;

    int numLines = 1;
    bool isDown = false;
    bool isLocked = false;

public:
    MessageObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        objectText = getSymbol();

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

        auto* cnvPtr = cnv->patch.getPointer();
        auto objText = editor ? editor->getText() : objectText;
        auto* textObj = static_cast<t_text*>(ptr);
        auto newNumLines = 0;
        
        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, textObj, objText, 15, newNumLines);
        
        numLines = newNumLines;
        
        // Create extra space for drawing the message box flag
        newBounds.setWidth(newBounds.getWidth() + 5);
        
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
        
    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void paint(Graphics& g) override
    {
        // Draw background
        g.setColour(object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);
        
        // Draw text
        if(!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds().withTrimmedRight(5));
            PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, 0.8f);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(1);

        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());

        if(isDown) {
            g.setColour(object->findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(b.reduced(1).toFloat(), PlugDataLook::objectCornerRadius, 3.0f);
            
            g.setColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.fillPath(flagPath);
        }
        else {
            g.setColour(object->findColour(PlugDataColour::outlineColourId));
            g.fillPath(flagPath);
        }
        
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        // TODO: select messages
        String v = getSymbol();

        if (objectText != v && !v.startsWith("click")) {

            objectText = v;

            repaint();
            updateBounds();
        }
    }

    void resized() override
    {
        if (editor) {
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));
            
            editor->setBorder(border);
            editor->setJustification(Justification::centredLeft);
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
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

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            auto newText = outgoingEditor->getText().trimEnd();

            if (objectText != newText) {
                objectText = newText;
            }
            
            outgoingEditor.reset();
            
            updateBounds(); // Recalculate bounds
            applyBounds();  // Send new bounds to Pd

            setSymbol(objectText);
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

    bool hideInGraph() override
    {
        return true;
    }
};
