/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


class MessboxObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(5, 7, 1, 2);

    String currentText;

    int numLines = 1;
    bool isLocked = false;
        
    Value primaryColour;
    Value secondaryColour;
    Value fontSize;
    Value bold;

public:
    MessboxObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        isLocked = static_cast<bool>(object->cnv->locked.getValue());
    }
        
    void initialiseParameters() override
    {
        auto* messbox = static_cast<t_fake_messbox*>(ptr);

        primaryColour = Colour(messbox->x_fg[0], messbox->x_fg[1], messbox->x_fg[2]).toString();
        secondaryColour = Colour(messbox->x_bg[0], messbox->x_bg[1], messbox->x_bg[2]).toString();

        auto params = getParameters();
        for (auto& [name, type, cat, value, list] : params) {
            value->addListener(this);

            // Push current parameters to pd
            valueChanged(*value);
        }

        repaint();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        pd->getCallbackLock()->exit();

        object->setObjectBounds({ x, y, w, h });
    }

    void applyBounds() override
    {
        auto* messbox = static_cast<t_fake_messbox*>(ptr);
        
        auto b = object->getObjectBounds();
        
        messbox->x_width = b.getWidth();
        messbox->x_height = b.getHeight();
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        // Draw background
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(bounds.toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        // Draw text
        if (!editor) {
            auto textColour = Colour::fromString(primaryColour.toString());
            auto textArea = border.subtractedFrom(bounds.withTrimmedRight(5));
            auto scale = bounds.getWidth() < 50 ? 0.5f : 1.0f;

            
            PlugDataLook::drawFittedText(g, currentText, textArea, textColour, numLines, scale, 15, Justification::topLeft);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(1);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch(objectMessageMapped[symbol])
        {
            case msg_set: {
                currentText = "";
                getSymbols(atoms);
                break;
            }
            case msg_append: {
                getSymbols(atoms);
                break;
            }
            case msg_bang: {
                setSymbols(currentText);
                break;
            }
            default: break;
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
        if(!isLocked) return;
        
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));
            
            editor->setJustification(Justification::topLeft);
            editor->setBorder(border);
            editor->setBounds(getLocalBounds().withTrimmedRight(5));
            editor->setText(currentText, false);
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

            auto newText = outgoingEditor->getText();

            // newText = TextObjectHelper::fixNewlines(newText);

            if (currentText != newText) {
                currentText = newText;
            }

            outgoingEditor.reset();

            updateBounds(); // Recalculate bounds
            applyBounds();  // Send new bounds to Pd

            repaint();
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        showEditor(); // TODO: Do we even need to?
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        setSymbols(ed.getText());
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        updateBounds();
    }

    void setSymbols(const String& symbols) {
        
        std::vector<t_atom> atoms;
        auto words = StringArray::fromTokens(symbols.trim(), true);
        for (int j = 0; j < words.size(); j++) {
            atoms.emplace_back();
            if (words[j].containsOnly("0123456789e.-+e") && words[j] != "-") {
                SETFLOAT(&atoms.back(), words[j].getFloatValue());
            } else {
                SETSYMBOL(&atoms.back(), pd->generateSymbol(words[j]));
            }
        }
        
        auto* sym = atom_getsymbol(&atoms[0]);
        atoms.erase(atoms.begin());
        
        outlet_anything(static_cast<t_object*>(ptr)->ob_outlet, sym, atoms.size(), atoms.data());
    }

    void getSymbols(std::vector<pd::Atom> atoms)
    {
        char buf[40];
        size_t length;
        
        auto newText = String();
        for(auto& atom : atoms)
        {
            
            if(atom.isFloat())
                newText += String(atom.getFloat()) + " ";
            else
            {
                auto symbol = atom.getSymbol();
                const auto* sym = symbol.toRawUTF8();
                int pos;
                int j = 0;
                length = 39;
                for(pos = 0; pos < symbol.getNumBytesAsUTF8(); pos++) {
                    auto c = sym[pos];
                    if(c == '\\' || c == '[' || c == '$' || c == ';') {
                        length--;
                        if(length <= 0) break;
                        buf[j++] = '\\';
                    }
                    length--;
                    if(length <= 0) break;
                    buf[j++] = c;
                }
                buf[j] = '\0';
                if(sym[pos-1] == ';') {
                    //sys_vgui("%s insert end %s\\n\n", x->text_id, buf);
                    newText += String::fromUTF8(buf) + "\n";
                }
                else {
                    //sys_vgui("%s insert end \"%s \"\n", x->text_id, buf);
                    newText += String::fromUTF8(buf) + " ";
                }
            }
        }
        
        currentText = newText.trimEnd();
        repaint();
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        if(editor && key.getKeyCode() == KeyPress::returnKey && key.getModifiers().isShiftDown()) {
            
            int caretPosition = editor->getCaretPosition();
            auto text = editor->getText();
            
            if (!editor->getHighlightedRegion().isEmpty())
                return false;
            
            text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
            editor->setText(text);
            
            currentText = text;
            editor->setCaretPosition(caretPosition + 1);
           
            return true;
        }
        
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
        
    ObjectParameters getParameters() override
    {
        return {
            { "Text colour", tColour, cAppearance, &primaryColour, {} },
            { "Background colour", tColour, cAppearance, &secondaryColour, {} },
            { "Font size", tInt, cAppearance, &fontSize, {} },
            { "Bold", tBool, cAppearance, &bold, {"no", "yes"} }
        };
    }
    
    void valueChanged(Value& value) override
    {
        auto* messbox = static_cast<t_fake_messbox*>(ptr);
        if (value.refersToSameSourceAs(primaryColour)) {
            auto col = Colour::fromString(primaryColour.toString());
            messbox->x_fg[0] = col.getRed();
            messbox->x_fg[1] = col.getGreen();
            messbox->x_fg[2] = col.getBlue();
            repaint();
        }
        if (value.refersToSameSourceAs(secondaryColour)) {
            auto col = Colour::fromString(secondaryColour.toString());
            messbox->x_bg[0] = col.getRed();
            messbox->x_bg[1] = col.getGreen();
            messbox->x_bg[2] = col.getBlue();
            repaint();
        }
        if (value.refersToSameSourceAs(fontSize)) {
            // TODO
        }
        if (value.refersToSameSourceAs(bold)) {
            // TODO 
        }
    }

    bool hideInGraph() override
    {
        return false;
    }
        
    struct t_fake_messbox
    {
        t_object         x_obj;
        t_canvas        *x_canvas;
        t_glist         *x_glist;
        t_symbol        *x_bind_sym;
        void            *x_proxy;
        t_symbol        *x_dollzero;
        int              x_flag;
        int              x_height;
        int              x_width;
        int              x_resizing;
        int              x_active;
        int              x_selected;
        char             x_fgcolor[8];
        unsigned int     x_fg[3];    // fg RGB color
        char             x_bgcolor[8];
        unsigned int     x_bg[3];    // bg RGB color
        int              x_font_size;
        int              x_zoom;
        t_symbol        *x_font_weight;
        char            *tcl_namespace;
        char            *x_cv_id;
        char            *frame_id;
        char            *text_id;
    //    char            *handle_id;
        char            *window_tag;
        char            *all_tag;
    };
};
