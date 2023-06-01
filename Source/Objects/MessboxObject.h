/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class MessboxObject final : public ObjectBase
    , public KeyListener
    , public TextEditor::Listener {

    TextEditor editor;
    BorderSize<int> border = BorderSize<int>(5, 7, 1, 2);

    int numLines = 1;

    Value primaryColour;
    Value secondaryColour;
    Value fontSize;
    Value bold;

public:
    MessboxObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        editor.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        editor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        editor.setColour(ScrollBar::thumbColourId, object->findColour(PlugDataColour::scrollbarThumbColourId));

        editor.setAlwaysOnTop(true);
        editor.setMultiLine(true);
        editor.setReturnKeyStartsNewLine(false);
        editor.setScrollbarsShown(true);
        editor.setIndents(0, 0);
        editor.setScrollToShowCursor(true);
        editor.setJustification(Justification::topLeft);
        editor.setBorder(border);
        editor.setBounds(getLocalBounds().withTrimmedRight(5));
        editor.addListener(this);
        editor.addKeyListener(this);
        editor.selectAll();

        addAndMakeVisible(editor);

        resized();
        repaint();

        bool isLocked = getValue<bool>(object->cnv->locked);
        editor.setReadOnly(!isLocked);

        objectParameters.addParamColour("Text color", cAppearance, &primaryColour, PlugDataColour::canvasTextColourId);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamInt("Font size", cAppearance, &fontSize, 12);
        objectParameters.addParamBool("Bold", cAppearance, &bold, { "No", "Yes" }, 0);
    }

    void update() override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            fontSize = messbox->x_font_size;

            primaryColour = Colour(messbox->x_fg[0], messbox->x_fg[1], messbox->x_fg[2]).toString();
            secondaryColour = Colour(messbox->x_bg[0], messbox->x_bg[1], messbox->x_bg[2]).toString();
        }

        repaint();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            auto* patch = object->cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(patch, messbox.get(), &x, &y, &w, &h);
            return { x, y, w, h };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            auto* patch = object->cnv->patch.getPointer().get();
            if (!patch)
                return;

            libpd_moveobj(patch, messbox.cast<t_gobj>(), b.getX(), b.getY());

            messbox->x_width = b.getWidth();
            messbox->x_height = b.getHeight();
        }
    }

    void lock(bool locked) override
    {
        setInterceptsMouseClicks(locked, locked);
        editor.setReadOnly(!locked);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        // Draw background
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(bounds.toFloat().reduced(0.5f), Corners::objectCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("set"),
            hash("append"),
            hash("fgcolor"),
            hash("bgcolor"),
            hash("bang"),
            hash("fontsize"),
            hash("bold")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("set"): {
            editor.setText("");
            getSymbols(atoms);
            break;
        }
        case hash("append"): {
            getSymbols(atoms);
            break;
        }
        case hash("bang"): {
            setSymbols(editor.getText());
            break;
        }
        case hash("bold"): {
            if (atoms.size() >= 1 && atoms[0].isFloat())
                bold = atoms[0].getFloat();
            break;
        }
        case hash("fontsize"):
        case hash("fgcolor"):
        case hash("bgcolor"): {
            update();
            break;
        }
        default:
            break;
        }
    }

    void resized() override
    {
        editor.setBounds(getLocalBounds().withTrimmedRight(5));
    }

    void showEditor() override
    {
        editor.grabKeyboardFocus();
    }

    void hideEditor() override
    {
        editor.setReadOnly(true);
        repaint();
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
        object->updateBounds();
    }

    void setSymbols(String const& symbols)
    {

        std::vector<t_atom> atoms;
        auto words = StringArray::fromTokens(symbols.trim(), true);
        for (auto const& word : words) {
            atoms.emplace_back();
            // check if string is a valid number
            auto charptr = word.getCharPointer();
            auto ptr = charptr;
            auto value = CharacterFunctions::readDoubleValue(ptr);
            if (ptr - charptr == word.getNumBytesAsUTF8()) {
                SETFLOAT(&atoms.back(), word.getFloatValue());
            } else {
                SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
            }
        }

        if (atoms.size()) {
            if (auto messObj = ptr.get<t_object>()) {
                outlet_anything(messObj->ob_outlet, pd->generateSymbol("list"), atoms.size(), atoms.data());
            }
        }
    }

    void getSymbols(std::vector<pd::Atom> const& atoms)
    {
        char buf[40];
        size_t length;

        auto newText = String();
        for (auto& atom : atoms) {

            if (atom.isFloat())
                newText += String(atom.getFloat()) + " ";
            else {
                auto symbol = atom.getSymbol();
                auto const* sym = symbol.toRawUTF8();
                int pos;
                int j = 0;
                length = 39;
                for (pos = 0; pos < symbol.getNumBytesAsUTF8(); pos++) {
                    auto c = sym[pos];
                    if (c == '\\' || c == '[' || c == '$' || c == ';') {
                        length--;
                        if (length <= 0)
                            break;
                        buf[j++] = '\\';
                    }
                    length--;
                    if (length <= 0)
                        break;
                    buf[j++] = c;
                }
                buf[j] = '\0';
                if (sym[pos - 1] == ';') {
                    // sys_vgui("%s insert end %s\\n\n", x->text_id, buf);
                    newText += String::fromUTF8(buf) + "\n";
                } else {
                    // sys_vgui("%s insert end \"%s \"\n", x->text_id, buf);
                    newText += String::fromUTF8(buf) + " ";
                }
            }
        }

        editor.setText(newText.trimEnd());

        repaint();
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        bool editing = !editor.isReadOnly();

        if (editing && key.getKeyCode() == KeyPress::returnKey && key.getModifiers().isShiftDown()) {

            int caretPosition = editor.getCaretPosition();
            auto text = editor.getText();

            if (!editor.getHighlightedRegion().isEmpty())
                return false;

            text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
            editor.setText(text);

            editor.setCaretPosition(caretPosition + 1);

            return true;
        }

        if (editing && key == KeyPress::rightKey && !editor.getHighlightedRegion().isEmpty()) {
            editor.setCaretPosition(editor.getHighlightedRegion().getEnd());
            return true;
        }
        if (editing && key == KeyPress::leftKey && !editor.getHighlightedRegion().isEmpty()) {
            editor.setCaretPosition(editor.getHighlightedRegion().getStart());
            return true;
        }
        return false;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(primaryColour)) {

            auto col = Colour::fromString(primaryColour.toString());
            editor.applyColourToAllText(col);

            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_fg);
            repaint();
        }
        if (value.refersToSameSourceAs(secondaryColour)) {
            auto col = Colour::fromString(secondaryColour.toString());
            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_bg);
            repaint();
        }
        if (value.refersToSameSourceAs(fontSize)) {
            auto size = getValue<int>(fontSize);
            editor.applyFontToAllText(editor.getFont().withHeight(size));
            if (auto messbox = ptr.get<t_fake_messbox>())
                messbox->x_font_size = size;
        }
        if (value.refersToSameSourceAs(bold)) {
            auto size = getValue<int>(fontSize);
            if (getValue<bool>(bold)) {
                auto boldFont = Fonts::getBoldFont();
                editor.applyFontToAllText(boldFont.withHeight(size));
                if (auto messbox = ptr.get<t_fake_messbox>())
                    messbox->x_font_weight = pd->generateSymbol("normal");
            } else {
                auto defaultFont = Fonts::getCurrentFont();
                editor.applyFontToAllText(defaultFont.withHeight(size));
                if (auto messbox = ptr.get<t_fake_messbox>())
                    messbox->x_font_weight = pd->generateSymbol("bold");
            }
        }
    }

    bool hideInGraph() override
    {
        return false;
    }
};
