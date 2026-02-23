/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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
        editor.setLookAndFeel(&object->getLookAndFeel());
        editor.setColour(TextEditor::textColourId, PlugDataColours::canvasTextColour);
        editor.getProperties().set("NoBackground", true);
        editor.getProperties().set("NoOutline", true);
        editor.setColour(ScrollBar::thumbColourId, PlugDataColours::scrollbarThumbColour);
        editor.onFocusLost = [this] {
            needsRepaint = true;
            repaint();
        };

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

        bool const isLocked = getValue<bool>(object->cnv->locked);
        editor.setReadOnly(!isLocked);

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColour("Text", cAppearance, &primaryColour, PlugDataColour::canvasTextColourId);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamInt("Font size", cAppearance, &fontSize, 12, true, 1);
        objectParameters.addParamBool("Bold", cAppearance, &bold, { "No", "Yes" }, 0);
    }

    void update() override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            fontSize = messbox->x_font_size;
            primaryColour = Colour(messbox->x_fg[0], messbox->x_fg[1], messbox->x_fg[2]).toString();
            secondaryColour = Colour(messbox->x_bg[0], messbox->x_bg[1], messbox->x_bg[2]).toString();
            sizeProperty = VarArray { var(messbox->x_width), var(messbox->x_height) };
            bold = pd->generateSymbol("bold") == messbox->x_font_weight;
        }
        
        auto font = getValue<bool>(bold) ? Fonts::getBoldFont() : Fonts::getDefaultFont();
        editor.applyColourToAllText(Colour::fromString(primaryColour.toString()));
        editor.applyFontToAllText(font.withHeight(getValue<int>(fontSize)));

        repaint();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, messbox.cast<t_gobj>(), &x, &y, &w, &h);
            return { x, y, w, h };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto messbox = ptr.get<t_fake_messbox>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, messbox.cast<t_gobj>(), b.getX(), b.getY());

            messbox->x_width = b.getWidth();
            messbox->x_height = b.getHeight();
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto messbox = ptr.get<t_fake_messbox>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(messbox->x_width), var(messbox->x_height) });
        }
    }

    void lock(bool const locked) override
    {
        needsRepaint = true;
        setInterceptsMouseClicks(locked, locked);
        editor.setReadOnly(!locked);
    }

    void render(NVGcontext* nvg) override
    {
        bool const selected = object->isSelected() && !cnv->isGraph;
        auto const outlineColour = selected ? PlugDataColours::objectSelectedOutlineColour : PlugDataColours::objectOutlineColour;
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), nvgColour(Colour::fromString(secondaryColour.toString())), nvgColour(outlineColour), Corners::objectCornerRadius);

        auto const scale = getImageScale();
        if (needsRepaint || isEditorShown() || imageRenderer.needsUpdate(roundToInt(editor.getWidth() * scale), roundToInt(editor.getHeight() * scale))) {
            imageRenderer.renderJUCEComponent(nvg, editor, scale);
            needsRepaint = false;
        } else {
            NVGScopedState state(nvg);
            nvgScale(nvg, 1.0f / scale, 1.0f / scale);
            auto w = roundToInt(scale * static_cast<float>(editor.getWidth()));
            auto h = roundToInt(scale * static_cast<float>(editor.getHeight()));
            imageRenderer.render(nvg, { 0, 0, w, h }, true);
        }
    }

    void paint(Graphics& g) override { }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("append"):
        case hash("set"): {
            updateText();
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
        if(cnv->isShowing()) {
            cnv->grabKeyboardFocus();
            repaint();
        }
    }

    bool isEditorShown() override
    {
        return !editor.isReadOnly() && editor.hasKeyboardFocus(false);
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
        auto const text = ed.getText();
        if (auto messObj = ptr.get<t_fake_messbox>()) {
            binbuf_text(messObj->x_state, text.toRawUTF8(), text.getNumBytesAsUTF8());
        }

        object->updateBounds();
    }

    void updateText()
    {
        SmallArray<pd::Atom> atoms;
        if (auto messObj = ptr.get<t_fake_messbox>()) {
            auto const* av = binbuf_getvec(messObj->x_state);
            auto const ac = binbuf_getnatom(messObj->x_state);
            atoms = pd::Atom::fromAtoms(ac, av);
        }

        StackArray<char, 40> buf;

        auto newText = String();
        for (auto& atom : atoms) {
            if (atom.isFloat()) {
                newText += String(atom.getFloat()) + " ";
            } else {
                auto symbol = atom.toString();
                auto const* sym = symbol.toRawUTF8();
                int pos;
                int j = 0;
                size_t length = 39;
                for (pos = 0; pos < symbol.getNumBytesAsUTF8(); pos++) {
                    auto const c = sym[pos];
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
                    newText += String::fromUTF8(buf.data()) + "\n";
                } else {
                    newText += String::fromUTF8(buf.data()) + " ";
                }
            }
        }

        editor.setText(newText.trimEnd());

        repaint();
        needsRepaint = true;
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        bool const editing = !editor.isReadOnly();

        if (editing && key.getKeyCode() == KeyPress::returnKey && key.getModifiers().isShiftDown()) {

            int const caretPosition = editor.getCaretPosition();
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

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto messbox = ptr.get<t_fake_messbox>()) {
                messbox->x_width = width;
                messbox->x_height = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            needsRepaint = true;
            auto const col = Colour::fromString(primaryColour.toString());
            editor.applyColourToAllText(col);

            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_fg);

            repaint();
        }
        if (value.refersToSameSourceAs(secondaryColour)) {
            needsRepaint = true;
            auto const col = Colour::fromString(secondaryColour.toString());
            if (auto messbox = ptr.get<t_fake_messbox>())
                colourToHexArray(col, messbox->x_bg);
            repaint();
        }
        if (value.refersToSameSourceAs(fontSize)) {
            needsRepaint = true;
            auto const size = getValue<int>(fontSize);
            editor.applyFontToAllText(editor.getFont().withHeight(size));
            if (auto messbox = ptr.get<t_fake_messbox>())
                messbox->x_font_size = size;
        }
        if (value.refersToSameSourceAs(bold)) {
            needsRepaint = true;
            auto const size = getValue<int>(fontSize);
            if (getValue<bool>(bold)) {
                auto const boldFont = Fonts::getBoldFont();
                editor.applyFontToAllText(boldFont.withHeight(size));
                if (auto messbox = ptr.get<t_fake_messbox>())
                    messbox->x_font_weight = pd->generateSymbol("bold");
            } else {
                auto const defaultFont = Fonts::getCurrentFont();
                editor.applyFontToAllText(defaultFont.withHeight(size));
                if (auto messbox = ptr.get<t_fake_messbox>())
                    messbox->x_font_weight = pd->generateSymbol("normal");
            }
        }
    }

    bool hideInGraph() override
    {
        return false;
    }
};
