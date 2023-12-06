/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// This object is a dumb version of [cyclone/comment] that only serves to make cyclone's documentation readable

/*
class CycloneCommentObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

public:
    CycloneCommentObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
    }

    void update() override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            textColour = Colour(comment->x_red, comment->x_green, comment->x_blue);
        }

        repaint();
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());
        }
    }

    void resized() override
    {
        int width = std::max(getBestTextWidth(getText()) * 8, 25);
        int height = 23;

        if (auto comment = ptr.get<t_fake_comment>()) {
            height = comment->x_fontsize + 18;
        }

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }
    }

    void paint(Graphics& g) override
    {
        // TODO: we don't want to lock here!
        int fontsize = 14;
        if (auto comment = ptr.get<t_fake_comment>()) {
            fontsize = comment->x_fontsize + 18;
        }

        auto textArea = border.subtractedFrom(getLocalBounds());
        Fonts::drawFittedText(g, getText(), textArea, textColour, fontsize);

        auto selected = object->isSelected();
        if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(selected ? object->findColour(PlugDataColour::objectSelectedOutlineColourId) : object->findColour(PlugDataColour::objectOutlineColourId));

            g.drawRect(getLocalBounds().toFloat(), 0.5f);
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

    Rectangle<int> getPdBounds() override
    {
        int width = getBestTextWidth(getText()) * 8;
        Rectangle<int> bounds;

        if (auto comment = ptr.get<t_fake_comment>()) {
            bounds = Rectangle<int>(comment->x_obj.te_xpix, comment->x_obj.te_ypix, width, comment->x_fontsize + 18);
        }

        return bounds;
    }

    String getText() override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            return String::fromUTF8(comment->x_buf, comment->x_bufsize);
        }

        return {};
    }

    int getBestTextWidth(String const& text)
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            return std::max<float>(round(Font(comment->x_fontsize + 18).getStringWidthFloat(text) + 14.0f), 32);
        }

        return 32;
    }
}; */

class CycloneCommentObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

    TextEditor commentEditor;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value font = SynchronousValue();
    Value fontSize = SynchronousValue();
    Value bold = SynchronousValue();
    Value italic = SynchronousValue();
    Value underline = SynchronousValue();
    Value fillBackground = SynchronousValue();
    Value justification = SynchronousValue();
    Value receiveSymbol = SynchronousValue();

    bool locked;
    bool wasSelectedOnMouseDown = false;

public:
    CycloneCommentObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        locked = getValue<bool>(object->locked);

        if (auto comment = ptr.get<t_pd>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            (*(comment.get()))->c_wb->w_visfn(comment.cast<t_gobj>(), patch, 1);
        }

        addAndMakeVisible(commentEditor);

        commentEditor.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        commentEditor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        commentEditor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        commentEditor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        commentEditor.setColour(ScrollBar::thumbColourId, object->findColour(PlugDataColour::scrollbarThumbColourId));

        commentEditor.setAlwaysOnTop(true);
        commentEditor.setMultiLine(true);
        commentEditor.setReturnKeyStartsNewLine(true);
        commentEditor.setScrollbarsShown(false);
        commentEditor.setIndents(0, 2);
        commentEditor.setScrollToShowCursor(true);
        commentEditor.setJustification(Justification::topLeft);
        commentEditor.setBorder(border);
        commentEditor.addMouseListener(this, true);
        commentEditor.setReadOnly(true);

        commentEditor.onFocusLost = [this]() {
            commentEditor.setText(commentEditor.getText().trim());
            commentEditor.setReadOnly(true);
        };

        commentEditor.onTextChange = [this, object]() {
            std::vector<t_atom> atoms;

            auto words = StringArray::fromTokens(commentEditor.getText(), " ", "\"");
            for (const auto& word : words) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
            }

            if (auto comment = ptr.get<t_fake_comment>()) {
                binbuf_clear(comment->x_binbuf);
                binbuf_restore(comment->x_binbuf, atoms.size(), atoms.data());
                binbuf_gettext(comment->x_binbuf, &comment->x_buf, &comment->x_bufsize);
            }

            object->updateBounds();
        };

        objectParameters.addParamColour("Text color", cAppearance, &primaryColour, PlugDataColour::canvasTextColourId);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamInt("Font size", cAppearance, &fontSize, 14);
        objectParameters.addParamBool("Bold", cAppearance, &bold, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Italic", cAppearance, &italic, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Underline", cAppearance, &underline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Fill background", cAppearance, &fillBackground, { "No", "Yes" }, 0);
        objectParameters.addParamCombo("Justification", cAppearance, &justification, { "Left", "Centered", "Right" }, 1);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
    }

    void update() override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            textColour = Colour(comment->x_red, comment->x_green, comment->x_blue);
            commentEditor.setText(getComment());

            primaryColour = Colour(comment->x_red, comment->x_green, comment->x_blue).toString();
            secondaryColour = Colour(comment->x_bg[0], comment->x_bg[1], comment->x_bg[2]).toString();
            fontSize = comment->x_fontsize;

            bold = comment->x_bold;
            italic = comment->x_italic;
            underline = comment->x_underline;
            fillBackground = comment->x_bg_flag;
            justification = comment->x_textjust + 1;

            if (comment->x_fontname && String::fromUTF8(comment->x_fontname->s_name).isNotEmpty()) {
                font = String::fromUTF8(comment->x_fontname->s_name);
            } else {
                font = "Inter Variable";
            }

            auto receiveSym = String::fromUTF8(comment->x_rcv_raw->s_name);
            receiveSymbol = receiveSym == "empty" ? "" : comment->x_rcv_raw->s_name;
        }

        commentEditor.setColour(TextEditor::textColourId, Colour::fromString(primaryColour.toString()));

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
            commentEditor.setReadOnly(false);
            commentEditor.grabKeyboardFocus();
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
        commentEditor.setBounds(getLocalBounds());
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
        return !commentEditor.isReadOnly();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            int width = StringUtils::getPreciseStringWidth(getComment(), getFont()) + 12;

            return { comment->x_obj.te_xpix, comment->x_obj.te_ypix, width, comment->x_fontsize + 18 };
        }

        return {};
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class CycloneCommentObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            CycloneCommentObject* commentObject;
            Object* object;

            CycloneCommentObjectBoundsConstrainer(Object* obj, CycloneCommentObject* parent)
                : commentObject(parent)
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
                auto* comment = reinterpret_cast<t_fake_comment*>(object->getPointer());
                comment->x_max_pixwidth = bounds.getWidth() - Object::doubleMargin;

                // Set editor size first, so getTextHeight will return a correct result
                commentObject->commentEditor.setSize(comment->x_max_pixwidth, commentObject->commentEditor.getHeight());
                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
            }
        };

        return std::make_unique<CycloneCommentObjectBoundsConstrainer>(object, this);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            comment->x_max_pixwidth = b.getWidth();
            pd::Interface::moveObject(patch, comment.cast<t_gobj>(), b.getX(), b.getY());
        }
    }

    String getComment()
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            return String::fromUTF8(comment->x_buf, comment->x_bufsize).trim();
        }

        return {};
    }

    void valueChanged(Value& v) override
    {

        if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            commentEditor.applyColourToAllText(colour);
            if (auto comment = ptr.get<t_fake_comment>())
                colourToHexArray(colour, &comment->x_red); // this should be illegal, but it works
            repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            if (auto comment = ptr.get<t_fake_comment>())
                colourToHexArray(Colour::fromString(secondaryColour.toString()), comment->x_bg);
            repaint();
        } else if (v.refersToSameSourceAs(bold)) {
            if (auto comment = ptr.get<t_fake_comment>())
                comment->x_bold = getValue<int>(bold);
            updateFont();
        } else if (v.refersToSameSourceAs(italic)) {
            if (auto comment = ptr.get<t_fake_comment>())
                comment->x_italic = getValue<int>(italic);
            updateFont();
        } else if (v.refersToSameSourceAs(underline)) {
            if (auto comment = ptr.get<t_fake_comment>())
                comment->x_underline = getValue<int>(underline);
            updateFont();
        } else if (v.refersToSameSourceAs(fillBackground)) {
            if (auto comment = ptr.get<t_fake_comment>())
                comment->x_bg_flag = getValue<int>(fillBackground);
            repaint();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            auto receive = receiveSymbol.toString();
            if (auto comment = ptr.get<t_fake_comment>()) {
                comment->x_rcv_raw = pd->generateSymbol(receive);
                comment->x_rcv_set = receive.isNotEmpty();
            }

            repaint();
        } else if (v.refersToSameSourceAs(justification)) {
            auto justificationType = getValue<int>(justification);
            if (auto comment = ptr.get<t_fake_comment>())
                comment->x_textjust = justificationType - 1;
            if (justificationType == 1) {
                commentEditor.setJustification(Justification::topLeft);
            } else if (justificationType == 2) {
                commentEditor.setJustification(Justification::centredTop);
            } else if (justificationType == 3) {
                commentEditor.setJustification(Justification::topRight);
            }
        }
    }

    Font getFont()
    {
        auto isBold = getValue<bool>(bold);
        auto isItalic = getValue<bool>(italic);
        auto isUnderlined = getValue<bool>(underline);
        auto fontHeight = getValue<int>(fontSize);

        if (fontHeight < 1) {
            if (auto glist = cnv->patch.getPointer()) {
                fontHeight = glist_getfont(glist.get()) + 12;
            } else {
                fontHeight = 18;
            }
        }

        auto style = (isBold * Font::bold) | (isItalic * Font::italic) | (isUnderlined * Font::underlined);
        auto typefaceName = font.toString();

        if (typefaceName.isEmpty() || typefaceName == "Inter") {
            return Fonts::getVariableFont().withStyle(style).withHeight(fontHeight);
        }

        return { typefaceName, static_cast<float>(fontHeight), style };
    }

    void updateFont()
    {
        commentEditor.applyFontToAllText(getFont());
        object->updateBounds();
    }

    void receiveObjectMessage(hash32 symbol, const pd::Atom atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("italic"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                italic = comment->x_italic;
            }
            updateFont();
            break;
        }
        case hash("underline"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                underline = comment->x_underline;
            }
            updateFont();
            break;
        }
        case hash("bold"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                bold = comment->x_bold;
            }
            updateFont();
            break;
        }
        case hash("prepend"):
        case hash("append"):
        case hash("set"): {
            commentEditor.setText(getComment());
            object->updateBounds();
            break;
        }
        case hash("color"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                primaryColour = Colour(comment->x_red, comment->x_green, comment->x_blue).toString();
            }
            break;
        }
        case hash("bgcolor"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                secondaryColour = Colour(comment->x_bg[0], comment->x_bg[1], comment->x_bg[2]).toString();
            }
            break;
        }
        case hash("justification"): {
            if (auto comment = ptr.get<t_fake_comment>()) {
                justification = comment->x_textjust;
            }
            break;
        }
        case hash("width"): {
            object->updateBounds();
            break;
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
