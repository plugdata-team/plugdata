/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class NoteObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

    TextEditor noteEditor;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value font = SynchronousValue();
    Value fontSize = SynchronousValue();
    Value bold = SynchronousValue();
    Value italic = SynchronousValue();
    Value underline = SynchronousValue();
    Value fillBackground = SynchronousValue();
    Value justification = SynchronousValue();
    Value outline = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value width = SynchronousValue();

    bool locked;
    bool wasSelectedOnMouseDown = false;
    bool needsRepaint = false;

public:
    NoteObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        locked = getValue<bool>(object->locked);

        if (auto note = ptr.get<t_pd>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            (*(note.get()))->c_wb->w_visfn(note.cast<t_gobj>(), patch, 1);
        }

        addAndMakeVisible(noteEditor);

        noteEditor.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        noteEditor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        noteEditor.setColour(ScrollBar::thumbColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::scrollbarThumbColourId));

        noteEditor.setAlwaysOnTop(true);
        noteEditor.setMultiLine(true);
        noteEditor.setReturnKeyStartsNewLine(true);
        noteEditor.setScrollbarsShown(false);
        noteEditor.setIndents(0, 2);
        noteEditor.setScrollToShowCursor(true);

        noteEditor.setBorder(border);
        noteEditor.addMouseListener(this, true);
        noteEditor.setReadOnly(true);

        noteEditor.onFocusLost = [this]() {
            noteEditor.setText(noteEditor.getText().trim());
            noteEditor.setReadOnly(true);
        };

        noteEditor.onTextChange = [this, object]() {
            std::vector<t_atom> atoms;

            auto words = StringArray::fromTokens(noteEditor.getText(), " ", "\"");
            for (const auto& word : words) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
            }
            if (noteEditor.getText().endsWith(" ")) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(" "));
            }

            if (auto note = ptr.get<t_fake_note>()) {
                binbuf_clear(note->x_binbuf);
                binbuf_restore(note->x_binbuf, atoms.size(), atoms.data());
                binbuf_gettext(note->x_binbuf, &note->x_buf, &note->x_bufsize);
            }

            object->updateBounds();
            needsRepaint = true;
        };

        objectParameters.addParamInt("Width", cDimensions, &width);
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

    bool isTransparent() override
    {
        return true;
    }

    bool inletIsSymbol() override
    {
        // we want to hide the note inlet regardless if it's symbol or not in locked mode
        auto receiveSym = receiveSymbol.toString();
        if (receiveSym.isEmpty() || receiveSym == "empty")
            return locked;

        return true;
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

    void update() override
    {
        auto oldFont = getFont();

        if (auto note = ptr.get<t_fake_note>()) {
            textColour = Colour(note->x_red, note->x_green, note->x_blue);
            noteEditor.setText(getNote());

            primaryColour = Colour(note->x_red, note->x_green, note->x_blue).toString();
            secondaryColour = Colour(note->x_bg[0], note->x_bg[1], note->x_bg[2]).toString();
            fontSize = note->x_fontsize;

            bold = note->x_bold;
            italic = note->x_italic;
            underline = note->x_underline;
            fillBackground = note->x_bg_flag;
            justification = note->x_textjust + 1;
            outline = note->x_outline;
            width = note->x_max_pixwidth;

            if (note->x_fontname && String::fromUTF8(note->x_fontname->s_name).isNotEmpty()) {
                font = String::fromUTF8(note->x_fontname->s_name);
            } else {
                font = "Inter Variable";
            }

            auto receiveSym = String::fromUTF8(note->x_rcv_raw->s_name);
            receiveSymbol = receiveSym == "empty" ? "" : note->x_rcv_raw->s_name;
        }

        auto newFont = getFont();

        auto justificationType = getValue<int>(justification);
        if (justificationType == 1) {
            noteEditor.setJustification(Justification::topLeft);
        } else if (justificationType == 2) {
            noteEditor.setJustification(Justification::centredTop);
        } else if (justificationType == 3) {
            noteEditor.setJustification(Justification::topRight);
        }

        noteEditor.setColour(TextEditor::textColourId, Colour::fromString(primaryColour.toString()));

        if (oldFont != newFont) {
            updateFont();
        }

        getLookAndFeel().setColour(Label::textWhenEditingColourId, cnv->editor->getLookAndFeel().findColour(Label::textWhenEditingColourId));
        getLookAndFeel().setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(Label::textColourId));
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto note = ptr.get<t_fake_note>()) {
            setParameterExcludingListener(width, var(note->x_max_pixwidth));
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

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
        needsRepaint = true;

        noteEditor.setInterceptsMouseClicks(!isLocked, !isLocked);
        object->updateIolets(); // TODO: why?
    }

    void resized() override
    {
        noteEditor.setBounds(getLocalBounds());
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
            auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

            g.setColour(outlineColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        needsRepaint = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        needsRepaint = true;
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
        auto height = noteEditor.getTextHeight();

        if (auto note = ptr.get<t_fake_note>()) {
            int width = note->x_resized ? note->x_max_pixwidth : CachedFontStringWidth::get()->calculateStringWidth(getFont(), getNote()) + 12;

            return { note->x_obj.te_xpix, note->x_obj.te_ypix, width, height + 4 };
        }

        return {};
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class NoteObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            NoteObject* noteObject;
            Object* object;

            NoteObjectBoundsConstrainer(Object* obj, NoteObject* parent)
                : noteObject(parent)
                , object(obj)
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
                auto* note = reinterpret_cast<t_fake_note*>(object->getPointer());
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
        if (auto note = ptr.get<t_fake_note>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            note->x_max_pixwidth = b.getWidth();
            note->x_height = b.getHeight();
            pd::Interface::moveObject(patch, note.cast<t_gobj>(), b.getX(), b.getY());
        }
    }

    String getNote()
    {
        if (auto note = ptr.get<t_fake_note>()) {
            // Get string and unescape characters
            return String::fromUTF8(note->x_buf, note->x_bufsize).trim().replace("\\,", ",").replace("\\;", ";");
        }

        return {};
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(width)) {
            auto* constrainer = getConstrainer();
            auto newWidth = std::max(getValue<int>(width), constrainer->getMinimumWidth());

            setParameterExcludingListener(width, var(newWidth));

            if (auto note = ptr.get<t_fake_note>()) {
                note->x_max_pixwidth = newWidth;
                note->x_resized = 1;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            noteEditor.applyColourToAllText(colour);
            if (auto note = ptr.get<t_fake_note>())
                colourToHexArray(colour, &note->x_red);
            needsRepaint = true;
            repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            if (auto note = ptr.get<t_fake_note>())
                colourToHexArray(Colour::fromString(secondaryColour.toString()), note->x_bg);
            needsRepaint = true;
            repaint();
        } else if (v.refersToSameSourceAs(fontSize)) {
            if (auto note = ptr.get<t_fake_note>())
                note->x_fontsize = getValue<int>(fontSize);
            updateFont();
        } else if (v.refersToSameSourceAs(bold)) {
            if (auto note = ptr.get<t_fake_note>()) {
                note->x_bold = getValue<int>(bold);
                note->x_fontface = note->x_bold + 2 * note->x_italic + 4 * note->x_outline;
            }
            updateFont();
        } else if (v.refersToSameSourceAs(italic)) {
            if (auto note = ptr.get<t_fake_note>()) {
                note->x_italic = getValue<int>(italic);
                note->x_fontface = note->x_bold + 2 * note->x_italic + 4 * note->x_outline;
            }
            updateFont();
        } else if (v.refersToSameSourceAs(underline)) {
            if (auto note = ptr.get<t_fake_note>()) {
                note->x_underline = getValue<int>(underline);
                note->x_fontface = note->x_bold + 2 * note->x_italic + 4 * note->x_outline;
            }
            updateFont();
        } else if (v.refersToSameSourceAs(fillBackground)) {
            if (auto note = ptr.get<t_fake_note>())
                note->x_bg_flag = getValue<int>(fillBackground);
            needsRepaint = true;
            repaint();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            auto receive = receiveSymbol.toString();
            if (auto note = ptr.get<t_fake_note>()) {
                pd->sendDirectMessage(note.get(), "receive", { pd->generateSymbol(receive) });
            }
        } else if (v.refersToSameSourceAs(justification)) {
            auto justificationType = getValue<int>(justification);
            if (auto note = ptr.get<t_fake_note>())
                note->x_textjust = justificationType - 1;
            if (justificationType == 1) {
                noteEditor.setJustification(Justification::topLeft);
            } else if (justificationType == 2) {
                noteEditor.setJustification(Justification::centredTop);
            } else if (justificationType == 3) {
                noteEditor.setJustification(Justification::topRight);
            }
        } else if (v.refersToSameSourceAs(outline)) {
            if (auto note = ptr.get<t_fake_note>())
                note->x_outline = getValue<int>(outline);
            needsRepaint = true;
            repaint();
        } else if (v.refersToSameSourceAs(font)) {
            auto fontName = font.toString();
            if (auto note = ptr.get<t_fake_note>())
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
        needsRepaint = true;
        repaint();
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("font"): {
            if (auto note = ptr.get<t_fake_note>()) {
                font = String::fromUTF8(note->x_fontname->s_name);
            }

            updateFont();
            break;
        }
        case hash("italic"): {
            if (auto note = ptr.get<t_fake_note>()) {
                italic = note->x_italic;
            }
            updateFont();
            break;
        }
        case hash("size"): {
            if (auto note = ptr.get<t_fake_note>()) {
                fontSize = note->x_fontsize;
            }
            updateFont();
            break;
        }
        case hash("underline"): {
            if (auto note = ptr.get<t_fake_note>()) {
                underline = note->x_underline;
            }
            updateFont();
            break;
        }
        case hash("bold"): {
            if (auto note = ptr.get<t_fake_note>()) {
                bold = note->x_bold;
            }
            updateFont();
            break;
        }
        case hash("prepend"):
        case hash("append"):
        case hash("set"): {
            noteEditor.setText(getNote());
            object->updateBounds();
            needsRepaint = true;
            break;
        }
        case hash("color"): {
            if (auto note = ptr.get<t_fake_note>()) {
                primaryColour = Colour(note->x_red, note->x_green, note->x_blue).toString();
            }
            break;
        }
        case hash("bgcolor"): {
            if (auto note = ptr.get<t_fake_note>()) {
                secondaryColour = Colour(note->x_bg[0], note->x_bg[1], note->x_bg[2]).toString();
            }
            break;
        }
        case hash("just"): {
            if (auto note = ptr.get<t_fake_note>()) {
                justification = note->x_textjust;
            }
            break;
        }
        case hash("width"): {
            object->updateBounds();
            break;
        }
        case hash("outline"): {
            if (auto note = ptr.get<t_fake_note>()) {
                outline = note->x_outline;
            }
        }
        case hash("receive"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("bg"): {
            if (numAtoms > 0 && atoms[0].isFloat())
                fillBackground = atoms[0].getFloat();
            break;
        }
        default:
            break;
        }
    }
};
