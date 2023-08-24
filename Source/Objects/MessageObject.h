/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class MessageObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    Value sizeProperty = SynchronousValue();
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
        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
    }

    void update() override
    {
        objectText = getSymbol();
        
        if (auto obj = ptr.get<t_text>()) {
            sizeProperty = TextObjectHelper::getWidthInChars(obj.get());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        auto objText = editor ? editor->getText() : objectText;
        auto newNumLines = 0;

        if (auto message = ptr.get<t_text>()) {
            auto* cnvPtr = cnv->patch.getPointer().get();
            if (!cnvPtr)
                return {};

            auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, message.get(), objText, 15, newNumLines);

            numLines = newNumLines;

            // Create extra space for drawing the message box flag
            newBounds.setWidth(newBounds.getWidth() + 5);
            return newBounds;
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            libpd_moveobj(patch, gobj.get(), b.getX(), b.getY());

            if (TextObjectHelper::getWidthInChars(gobj.get())) {
                TextObjectHelper::setWidthInChars(gobj.get(), b.getWidth() / glist_fontwidth(patch));
            }
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        
        if (auto text = ptr.get<t_text>()) {
            setParameterExcludingListener(sizeProperty, TextObjectHelper::getWidthInChars(text.get()));
        }
    }
        
    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void paint(Graphics& g) override
    {
        const int d = 6;
        auto reducedBounds = getLocalBounds().toFloat().reduced(0.5f);

        // Draw background
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        Path roundEdgeClipping;
        roundEdgeClipping.addRoundedRectangle(reducedBounds, Corners::objectCornerRadius);

        g.saveState();
        g.reduceClipRegion(roundEdgeClipping);

        if (isDown) {
            g.setColour(object->findColour(PlugDataColour::outlineColourId));
            g.drawRect(getLocalBounds(), d);
        }

        g.restoreState();

        // Draw text
        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds().withTrimmedRight(5));
            auto scale = getWidth() < 50 ? 0.5f : 1.0f;

            Fonts::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, scale);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto b = getLocalBounds();
        auto reducedBounds = b.toFloat().reduced(0.5f);

        const int d = 6;

        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - d, b.getY() + d, b.getRight() - d, b.getBottom() - d, b.getRight(), b.getBottom());

        Path roundEdgeClipping;
        roundEdgeClipping.addRoundedRectangle(reducedBounds, Corners::objectCornerRadius);

        g.saveState();
        g.reduceClipRegion(roundEdgeClipping);

        g.setColour(object->findColour(PlugDataColour::objectOutlineColourId));
        g.fillPath(flagPath);

        g.restoreState();

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(reducedBounds, Corners::objectCornerRadius, 1.0f);
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("set"),
            hash("add"),
            hash("add2"),
            hash("addcomma"),
            hash("addsemi"),
            hash("adddollar"),
            hash("adddollsym")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        String v = getSymbol();

        if (objectText != v) {

            objectText = v;

            repaint();
            object->updateBounds();
        }
    }

    void resized() override
    {
        if (editor) {
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
        }
    }

    bool isEditorShown() override
    {
        return editor != nullptr;
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));

            editor->setBorder(border);
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();
            editor->setReturnKeyStartsNewLine(false);

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

    void mouseDown(MouseEvent const& e) override
    {
        if(!e.mods.isLeftButtonDown()) return;
        
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
        if (auto message = ptr.get<void>()) {
            cnv->pd->sendDirectMessage(message.get(), 0);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
        repaint();
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        cnv->grabKeyboardFocus();
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        object->updateBounds();
    }

    String getSymbol() const
    {
        char* text;
        int size;

        if (auto messObj = ptr.get<t_text>()) {
            binbuf_gettext(messObj->te_binbuf, &text, &size);
        } else {
            return {};
        }

        auto result = String::fromUTF8(text, size);
        freebytes(text, size);

        return result.trimEnd();
    }
        
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            
            setParameterExcludingListener(sizeProperty, width);
            
            if (auto text = ptr.get<t_text>())
            {
                TextObjectHelper::setWidthInChars(text.get(), width);
            }
            
            object->updateBounds();
        }
    }

    void setSymbol(String const& value)
    {
        auto* cstr = value.toRawUTF8();
        if (auto messobj = ptr.get<t_text>()) {
            auto* canvas = cnv->patch.getPointer().get();
            if (!canvas)
                return;

            libpd_renameobj(canvas, messobj.cast<t_gobj>(), cstr, value.getNumBytesAsUTF8());
        }
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
        if(key.getKeyCode() == KeyPress::returnKey && editor && key.getModifiers().isShiftDown())
        {
            int caretPosition = editor->getCaretPosition();
            auto text = editor->getText();

            if (!editor->getHighlightedRegion().isEmpty())
                return false;
            if (text[caretPosition - 1] == ';') {
                text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
                caretPosition += 1;
            } else {
                text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);
                caretPosition += 2;
            }
            
            editor->setText(text);
            editor->setCaretPosition(caretPosition);
            
            return true;
        }
        
        return false;
    }

    bool hideInGraph() override
    {
        return true;
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return TextObjectHelper::createConstrainer(object);
    }
};
