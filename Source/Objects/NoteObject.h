/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class NoteObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

    TextEditor noteEditor;

    Value primaryColour, secondaryColour, font, fontSize, bold, italic, underline, fillBackground, justification, outline, receiveSymbol;

    bool locked;
    bool wasSelectedOnMouseDown = false;

public:
    NoteObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        locked = getValue<bool>(object->locked);

        // Lock around it, to make sure this gets called synchronously
        // Unfortunately note needs to receive the vis message to make it initialise
        pd->lockAudioThread();
        (*static_cast<t_pd*>(ptr))->c_wb->w_visfn(static_cast<t_gobj*>(ptr), object->cnv->patch.getPointer(), 1);
        pd->unlockAudioThread();

        addAndMakeVisible(noteEditor);

        noteEditor.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        noteEditor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        noteEditor.setColour(ScrollBar::thumbColourId, object->findColour(PlugDataColour::scrollbarThumbColourId));

        noteEditor.setAlwaysOnTop(true);
        noteEditor.setMultiLine(true);
        noteEditor.setReturnKeyStartsNewLine(true);
        noteEditor.setScrollbarsShown(false);
        noteEditor.setIndents(0, 2);
        noteEditor.setScrollToShowCursor(true);
        noteEditor.setJustification(Justification::topLeft);
        noteEditor.setBorder(border);
        noteEditor.addMouseListener(this, true);
        noteEditor.setReadOnly(true);

        noteEditor.onFocusLost = [this]() {
            noteEditor.setText(noteEditor.getText().trim());
            noteEditor.setReadOnly(true);
        };

        noteEditor.onTextChange = [this, object]() {
            auto* x = static_cast<t_fake_note*>(ptr);

            std::vector<t_atom> atoms;

            auto words = StringArray::fromTokens(noteEditor.getText(), " ", "\"");
            for (const auto& word : words) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
            }

            pd->lockAudioThread();
            binbuf_clear(x->x_binbuf);
            binbuf_restore(x->x_binbuf, atoms.size(), atoms.data());
            binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
            pd->unlockAudioThread();

            object->updateBounds();
        };

        objectParameters.addParamColour("Text color", cAppearance, &primaryColour, PlugDataColour::canvasTextColourId);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamFont("Font", cAppearance, &font, "Inter");
        objectParameters.addParamInt("Font size", cAppearance, &fontSize, 14);
        objectParameters.addParamBool("Outline", cAppearance, &outline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Bold", cAppearance, &bold, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Italic", cAppearance, &italic, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Underline", cAppearance, &underline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Fill background", cAppearance, &fillBackground, { "No", "Yes" }, 0);
        objectParameters.addParamCombo("Justification", cAppearance, &justification, { "Left", "Centered", "Right" }, 1);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
    }

    void update() override
    {
        auto* note = static_cast<t_fake_note*>(ptr);

        textColour = Colour(note->x_red, note->x_green, note->x_blue);
        noteEditor.setText(getNote());

        primaryColour = Colour(note->x_red, note->x_green, note->x_blue).toString();
        secondaryColour = Colour(note->x_bg[0], note->x_bg[1], note->x_bg[2]).toString();
        fontSize = note->x_fontsize;

        noteEditor.setColour(TextEditor::textColourId, Colour::fromString(primaryColour.toString()));

        bold = note->x_bold;
        italic = note->x_italic;
        underline = note->x_underline;
        fillBackground = note->x_bg_flag;
        justification = note->x_textjust + 1;
        outline = note->x_outline;

        if (note->x_fontname && String::fromUTF8(note->x_fontname->s_name).isNotEmpty()) {
            font = String::fromUTF8(note->x_fontname->s_name);
        } else {
            font = "Inter Variable";
        }

        auto receiveSym = String::fromUTF8(note->x_rcv_raw->s_name);
        receiveSymbol = receiveSym == "empty" ? "" : note->x_rcv_raw->s_name;

        repaint();

        updateFont();

        getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(Label::textWhenEditingColourId));
        getLookAndFeel().setColour(Label::textColourId, object->findColour(Label::textColourId));
    }

    void mouseDown(MouseEvent const& e) override
    {
        wasSelectedOnMouseDown = object->isSelected();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!locked && wasSelectedOnMouseDown && !e.mouseWasDraggedSinceMouseDown()) {
            noteEditor.setReadOnly(false);
            noteEditor.grabKeyboardFocus();
        }
    }

    void lock(bool isLocked) override
    {
        locked = isLocked;
        repaint();

        object->updateIolets();
    }

    void resized() override
    {
        noteEditor.setBounds(getLocalBounds());
    }

    bool hideInlets() override
    {
        return locked;
    }

    void paint(Graphics& g) override
    {
        if (getValue<bool>(fillBackground)) {
            auto bounds = getLocalBounds();
            // Draw background
            g.setColour(Colour::fromString(secondaryColour.toString()));
            g.fillRoundedRectangle(bounds.toFloat().reduced(0.5f), Corners::objectCornerRadius);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        if (getValue<bool>(outline)) {
            bool selected = object->isSelected() && !cnv->isGraph;
            auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

            g.setColour(outlineColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    bool hideInGraph() override
    {
        return false;
    }

    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked;
    }

    bool isEditorShown() override
    {
        return !noteEditor.isReadOnly();
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        auto* note = static_cast<t_fake_note*>(ptr);
        int width = note->x_resized ? note->x_max_pixwidth : StringUtils::getPreciseStringWidth(getNote(), getFont()) + 12;
        auto height = noteEditor.getTextHeight();

        auto bounds = Rectangle<int>(note->x_obj.te_xpix, note->x_obj.te_ypix, width, height + 4);
        pd->unlockAudioThread();

        return bounds;
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class NoteObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            NoteObject* noteObject;
            Object* object;

            NoteObjectBoundsConstrainer(Object* obj, NoteObject* parent)
                : object(obj)
                , noteObject(parent)
            {
            }
            /*
             * Custom version of checkBounds that takes into consideration
             * the padding around plugdata node objects when resizing
             * to allow the aspect ratio to be interpreted correctly.
             * Otherwise resizing objects with an aspect ratio will
             * resize the object size **including** margins, and not follow the
             * actual size of the visible object
             */
            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool isStretchingTop,
                bool isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                auto* note = static_cast<t_fake_note*>(object->getPointer());
                note->x_resized = 1;
                note->x_max_pixwidth = bounds.getWidth() - Object::doubleMargin;

                // Set editor size first, so getTextHeight will return a correct result
                noteObject->noteEditor.setSize(note->x_max_pixwidth, noteObject->noteEditor.getHeight());
                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
            }
        };

        return std::make_unique<NoteObjectBoundsConstrainer>(object, this);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        note->x_max_pixwidth = b.getWidth();
        note->x_height = b.getHeight();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
    }

    String getNote()
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        return String::fromUTF8(note->x_buf, note->x_bufsize).trim();
    }

    void valueChanged(Value& v) override
    {
        auto* note = static_cast<t_fake_note*>(ptr);

        if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            noteEditor.applyColourToAllText(colour);
            colourToHexArray(colour, &note->x_red); // this should be illegal, but it works
            repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            colourToHexArray(Colour::fromString(secondaryColour.toString()), note->x_bg);
            repaint();
        } else if (v.refersToSameSourceAs(fontSize)) {
            note->x_fontsize = getValue<int>(fontSize);
            updateFont();
        } else if (v.refersToSameSourceAs(bold)) {
            note->x_bold = getValue<int>(bold);
            updateFont();
        } else if (v.refersToSameSourceAs(italic)) {
            note->x_italic = getValue<int>(italic);
            updateFont();
        } else if (v.refersToSameSourceAs(underline)) {
            note->x_underline = getValue<int>(underline);
            updateFont();
        } else if (v.refersToSameSourceAs(fillBackground)) {
            note->x_bg_flag = getValue<int>(fillBackground);
            repaint();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            auto receive = receiveSymbol.toString();
            note->x_rcv_raw = pd->generateSymbol(receive);
            note->x_rcv_set = receive.isNotEmpty();
            repaint();
        } else if (v.refersToSameSourceAs(justification)) {
            auto justificationType = getValue<int>(justification);
            note->x_textjust = justificationType - 1;
            if (justificationType == 1) {
                noteEditor.setJustification(Justification::topLeft);
            } else if (justificationType == 2) {
                noteEditor.setJustification(Justification::centredTop);
            } else if (justificationType == 3) {
                noteEditor.setJustification(Justification::topRight);
            }
        } else if (v.refersToSameSourceAs(outline)) {
            note->x_outline = getValue<int>(outline);
            repaint();
        } else if (v.refersToSameSourceAs(font)) {
            auto fontName = font.toString();
            note->x_fontname = gensym(fontName.toRawUTF8());
            updateFont();
        }
    }

    Font getFont()
    {
        auto isBold = getValue<bool>(bold);
        auto isItalic = getValue<bool>(italic);
        auto isUnderlined = getValue<bool>(underline);
        auto fontHeight = getValue<int>(fontSize);

        auto style = (isBold * Font::bold) | (isItalic * Font::italic) | (isUnderlined * Font::underlined);
        auto typefaceName = font.toString();

        if (typefaceName.isEmpty() || typefaceName == "Inter") {
            return Fonts::getVariableFont().withStyle(style).withHeight(fontHeight);
        }

        return { typefaceName, static_cast<float>(fontHeight), style };
    }

    void updateFont()
    {
        noteEditor.applyFontToAllText(getFont());
        object->updateBounds();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("font"),
            hash("italic"),
            hash("size"),
            hash("underline"),
            hash("bold"),
            hash("prepend"),
            hash("append"),
            hash("set"),
            hash("color"),
            hash("bgcolor"),
            hash("justification"),
            hash("width"),
            hash("outline"),
            hash("receive"),
            hash("bg")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        auto* note = static_cast<t_fake_note*>(ptr);

        switch (hash(symbol)) {
        case hash("font"): {
            font = String::fromUTF8(note->x_fontname->s_name);
            updateFont();
            break;
        }
        case hash("italic"): {
            italic = note->x_italic;
            updateFont();
            break;
        }
        case hash("size"): {
            fontSize = note->x_fontsize;
            updateFont();
            break;
        }
        case hash("underline"): {
            underline = note->x_underline;
            updateFont();
            break;
        }
        case hash("bold"): {
            bold = note->x_bold;
            updateFont();
            break;
        }
        case hash("prepend"):
        case hash("append"):
        case hash("set"): {
            noteEditor.setText(getNote());
            object->updateBounds();
            break;
        }
        case hash("color"): {
            primaryColour = Colour(note->x_red, note->x_green, note->x_blue).toString();
            break;
        }
        case hash("bgcolor"): {
            secondaryColour = Colour(note->x_bg[0], note->x_bg[1], note->x_bg[2]).toString();
            break;
        }
        case hash("justification"): {
            justification = note->x_textjust;
            break;
        }
        case hash("width"): {
            object->updateBounds();
            break;
        }
        case hash("outline"): {
            outline = note->x_outline;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("bg"): {
            if (atoms.size() > 0 && atoms[0].isFloat())
                fillBackground = atoms[0].getFloat();
            break;
        }
        default:
            break;
        }
    }
};
