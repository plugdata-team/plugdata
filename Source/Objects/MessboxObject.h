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

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value fontSize = SynchronousValue();
    Value bold = SynchronousValue();
    Value sizeProperty = SynchronousValue();
        
    bool needsRepaint = true;

public:
    MessboxObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        editor.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        editor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        editor.setColour(ScrollBar::thumbColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::scrollbarThumbColourId));

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

        objectParameters.addParamSize(&sizeProperty);
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
            sizeProperty = Array<var> { var(messbox->x_width), var(messbox->x_height) };
        }

        editor.applyColourToAllText(Colour::fromString(primaryColour.toString()));
        editor.applyFontToAllText(editor.getFont().withHeight(getValue<int>(fontSize)));

        repaint();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            auto* patch = object->cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, messbox.cast<t_gobj>(), &x, &y, &w, &h);
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

            pd::Interface::moveObject(patch, messbox.cast<t_gobj>(), b.getX(), b.getY());

            messbox->x_width = b.getWidth();
            messbox->x_height = b.getHeight();
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto messbox = ptr.get<t_fake_messbox>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(messbox->x_width), var(messbox->x_height) });
        }
    }

    void lock(bool locked) override
    {
        needsRepaint = true;
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
        
    void render(NVGcontext* nvg) override
    {
        auto scale = getImageScale();
        if (needsRepaint || isEditorShown() || imageRenderer.needsUpdate(roundToInt(getWidth() * scale), roundToInt(getHeight() * scale))) {
            imageRenderer.renderJUCEComponent(nvg, *this, scale);
            needsRepaint = false;
        } else {
            imageRenderer.render(nvg, getLocalBounds());
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("set"): {
            updateText();
            break;
        }
        case hash("append"): {
            updateText();
            break;
        }
        case hash("bold"): {
            if (numAtoms >= 1 && atoms[0].isFloat())
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
        if (!e.mods.isLeftButtonDown())
            return;

        showEditor(); // TODO: Do we even need to?
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (auto messObj = ptr.get<t_fake_messbox>()) {
            pd_list(messObj.cast<t_pd>(), pd->generateSymbol(""), 0, nullptr);
        }
    }

    // For resize-while-typing behaviour
    void textEditorTextChanged(TextEditor& ed) override
    {
        auto text = ed.getText();
        if (auto messObj = ptr.get<t_fake_messbox>()) {
            binbuf_text(messObj->x_state, text.toRawUTF8(), text.getNumBytesAsUTF8());
        }

        object->updateBounds();
    }

    void updateText()
    {
        std::vector<pd::Atom> atoms;
        if (auto messObj = ptr.get<t_fake_messbox>()) {
            auto* av = binbuf_getvec(messObj->x_state);
            auto ac = binbuf_getnatom(messObj->x_state);
            atoms = pd::Atom::fromAtoms(ac, av); // TODO: malloc inside lock, not good!
        }

        char buf[40];
        size_t length;

        auto newText = String();
        for (auto& atom : atoms) {
            if (atom.isFloat())
                newText += String(atom.getFloat()) + " ";
            else {
                auto symbol = atom.toString();
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
        needsRepaint = true;
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
        return false;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto messbox = ptr.get<t_fake_messbox>()) {
                messbox->x_width = width;
                messbox->x_height = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            needsRepaint = true;
            auto col = Colour::fromString(primaryColour.toString());
            editor.applyColourToAllText(col);

            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_fg);

            repaint();
        }
        if (value.refersToSameSourceAs(secondaryColour)) {
            needsRepaint = true;
            auto col = Colour::fromString(secondaryColour.toString());
            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_bg);
            repaint();
        }
        if (value.refersToSameSourceAs(fontSize)) {
            needsRepaint = true;
            auto size = getValue<int>(fontSize);
            editor.applyFontToAllText(editor.getFont().withHeight(size));
            if (auto messbox = ptr.get<t_fake_messbox>())
                messbox->x_font_size = size;
        }
        if (value.refersToSameSourceAs(bold)) {
            needsRepaint = true;
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
