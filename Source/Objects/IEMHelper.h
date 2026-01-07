/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

static int srl_is_valid(t_symbol const* s)
{
    return s != nullptr && s != gensym("");
}

extern "C" {
EXTERN char* pdgui_strnescape(char* dst, size_t dstlen, char const* src, size_t srclen);
}

class IEMHelper {

public:
    IEMHelper(pd::WeakReference iemgui, Object* parent, ObjectBase* base)
        : object(parent)
        , gui(base)
        , ptr(iemgui)
    {
    }

    void update()
    {
        bool colourHasChanged = false;
        if (auto const col = getForegroundColour().toString(); col != primaryColour) {
            primaryColour = col;
            colourHasChanged = true;
        }
        if (auto const col = getBackgroundColour().toString(); col != secondaryColour) {
            secondaryColour = col;
            colourHasChanged = true;
        }
        // we only need the callback that colourHasChanged will trigger for the object ATM.
        labelColour = getLabelColour().toString();

        gui->getLookAndFeel().setColour(Label::textWhenEditingColourId, object->cnv->editor->getLookAndFeel().findColour(Label::textWhenEditingColourId));
        gui->getLookAndFeel().setColour(Label::textColourId, Colour::fromString(primaryColour.toString()));

        gui->getLookAndFeel().setColour(TextButton::buttonOnColourId, Colour::fromString(primaryColour.toString()));
        gui->getLookAndFeel().setColour(Slider::thumbColourId, Colour::fromString(primaryColour.toString()));

        gui->getLookAndFeel().setColour(TextEditor::backgroundColourId, Colour::fromString(secondaryColour.toString()));
        gui->getLookAndFeel().setColour(TextButton::buttonColourId, Colour::fromString(secondaryColour.toString()));

        auto sliderBackground = Colour::fromString(secondaryColour.toString());
        sliderBackground = sliderBackground.getBrightness() > 0.5f ? sliderBackground.darker(0.6f) : sliderBackground.brighter(0.6f);

        gui->getLookAndFeel().setColour(Slider::backgroundColourId, sliderBackground);

        if (auto iemgui = ptr.get<t_iemgui>()) {
            labelPosition = VarArray { var(iemgui->x_ldx), var(iemgui->x_ldy) };
        }

        labelHeight = getFontHeight();
        labelText = getExpandedLabelText();

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();

        initialise = getInit();

        // Let the object know the colour has changed, nbx & background (secondary) colour currently
        if (colourHasChanged)
            iemColourChangedCallback();

        gui->repaint();
    }

    ObjectParameters makeIemParameters(bool const withAppearance = true, bool const withSymbols = true, int labelPosX = 0, int labelPosY = -8, int const labelHeightY = 10)
    {
        ObjectParameters params;

        if (withAppearance) {
            params.addParamColourFG(&primaryColour);
            params.addParamColourBG(&secondaryColour);
        }
        if (withSymbols) {
            params.addParamReceiveSymbol(&receiveSymbol);
            params.addParamSendSymbol(&sendSymbol);
        }
        params.addParamString("Text", cLabel, &labelText, "");
        params.addParamColourLabel(&labelColour);
        params.addParamRange("Position", cLabel, &labelPosition, { labelPosX, labelPosY });
        params.addParamInt("Height", cLabel, &labelHeight, labelHeightY, true, 4);
        params.addParamBool("Initialise", cGeneral, &initialise, { "No", "Yes" }, 0);

        return params;
    }

    /**
     * @brief Add IEM parameters to the object parameters
     * @attention Allows customization for different default settings (PD's default positions are not consistent)
     *
     * @param objectParams the object parameter to add items to
     * @param withAppearance customize the added IEM's to show appearance category
     * @param withSymbols customize the added IEM's to show symbols category
     * @param labelPosX customize the default labels x position
     * @param labelPosY customize the default labels y position
     * @param labelHeightY customize the default labels text height
     */
    void addIemParameters(ObjectParameters& objectParams, bool const withAppearance = true, bool const withSymbols = true, int const labelPosX = 0, int const labelPosY = -8, int const labelHeightY = 10)
    {
        auto IemParams = makeIemParameters(withAppearance, withSymbols, labelPosX, labelPosY, labelHeightY);
        for (auto const& param : IemParams.getParameters())
            objectParams.addParam(param);
    }

    bool receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms)
    {
        auto setColour = [this](Value& targetValue, pd::Atom const& atom) {
            if (atom.isSymbol()) {
                auto const colour = "#FF" + atom.toString().fromFirstOccurrenceOf("#", false, false);
                gui->setParameterExcludingListener(targetValue, colour);
            } else {
                int iemcolor = atom.getFloat();

                if (iemcolor >= 0) {
                    while (iemcolor >= IEM_GUI_MAX_COLOR)
                        iemcolor -= IEM_GUI_MAX_COLOR;
                    while (iemcolor < 0)
                        iemcolor += IEM_GUI_MAX_COLOR;

                    iemcolor = iemgui_color_hex[iemcolor];
                } else
                    iemcolor = (-1 - iemcolor) & 0xffffff;

                auto const colour = convertFromIEMColour(iemcolor);
                gui->setParameterExcludingListener(targetValue, colour.toString());
            }
        };
        switch (symbol) {
        case hash("send"): {
            if (atoms.size() >= 1)
                gui->setParameterExcludingListener(sendSymbol, atoms[0].toString());
            object->updateIolets();
            return true;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                gui->setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            object->updateIolets();
            return true;
        }
        case hash("color"): {
            if (atoms.size() > 0)
                setColour(secondaryColour, atoms[0]);
            if (atoms.size() > 1)
                setColour(primaryColour, atoms[1]);
            if (atoms.size() > 2)
                setColour(labelColour, atoms[2]);

            if (auto* label = gui->getLabel()) {
                label->setColour(Label::textColourId, getLabelColour());
            }

            iemColourChangedCallback();

            gui->repaint();

            return true;
        }
        case hash("label"): {
            if (atoms.size() >= 1) {
                gui->setParameterExcludingListener(labelText, atoms[0].toString());
                gui->updateLabel();
            }
            return true;
        }
        case hash("label_pos"): {
            if (atoms.size() >= 2) {
                gui->setParameterExcludingListener(labelPosition, VarArray { var(atoms[0].getFloat()), var(atoms[1].getFloat()) });
                gui->updateLabel();
            }
            return true;
        }
        case hash("label_font"): {
            if (atoms.size() >= 2) {
                gui->setParameterExcludingListener(labelHeight, static_cast<int>(atoms[1].getFloat()));
                gui->updateLabel();
            }
            return true;
        }
        case hash("vis_size"): {
            if (atoms.size() >= 2) {
                object->updateBounds();
            }
            return true;
        }
        case hash("init"): {
            if (atoms.size() >= 1)
                gui->setParameterExcludingListener(initialise, static_cast<bool>(atoms[0].getFloat()));
            return true;
        }
        default:
            break;
        }

        return false;
    }

    void valueChanged(Value const& v)
    {
        if (v.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (v.refersToSameSourceAs(primaryColour)) {
            auto const colour = Colour::fromString(primaryColour.toString());
            setForegroundColour(colour);
            iemColourChangedCallback();
            gui->repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            auto const colour = Colour::fromString(secondaryColour.toString());
            setBackgroundColour(colour);

            gui->getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
            gui->getLookAndFeel().setColour(TextButton::buttonColourId, colour);

            gui->getLookAndFeel().setColour(Slider::backgroundColourId, colour);

            iemColourChangedCallback();

            gui->repaint();
        } else if (v.refersToSameSourceAs(labelColour)) {
            setLabelColour(Colour::fromString(labelColour.toString()));
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(labelPosition)) {
            setLabelPosition({ labelPosition.getValue().getArray()->getReference(0), labelPosition.getValue().getArray()->getReference(1) });
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(labelHeight)) {
            setFontHeight(getValue<int>(labelHeight));
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(labelText)) {
            setLabelText(labelText.toString());
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(initialise)) {
            setInit(getValue<bool>(initialise));
        }
    }

    void setInit(bool const init)
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_isa.x_loadinit = init;
        }
    }

    bool getInit() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return iemgui->x_isa.x_loadinit;
        }

        return false;
    }

    Rectangle<int> getPdBounds()
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return { iemgui->x_obj.te_xpix, iemgui->x_obj.te_ypix, iemgui->x_w + 1, iemgui->x_h + 1 };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b)
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            pd::Interface::moveObject(iemgui->x_glist, &iemgui->x_obj.te_g, b.getX(), b.getY());

            iemgui->x_w = b.getWidth() - 1;
            iemgui->x_h = b.getHeight() - 1;
        }
    }

    void updateLabel(OwnedArray<ObjectLabel>& labels, Point<int> const offset = { 0, 0 })
    {
        String const text = labelText.toString();

        if (text.isNotEmpty()) {
            ObjectLabel* label;
            if (labels.isEmpty()) {
                label = labels.add(new ObjectLabel());
                object->cnv->addChildComponent(label);
            } else {
                label = labels[0];
            }

            if (text.isNotEmpty()) {
                auto bounds = getLabelBounds();

                bounds.translate(0, bounds.getHeight() / -2.0f);

                label->setFont(Font(bounds.getHeight()));
                label->setBounds(bounds + offset);
                label->setText(text, dontSendNotification);
                label->setVisible(true);
                label->setColour(Label::textColourId, getLabelColour());
            }
        } else {
            labels.clear();
        }
    }

    Rectangle<int> getLabelBounds() const
    {
        auto const objectBounds = object->getBounds().reduced(Object::margin);

        if (auto iemgui = ptr.get<t_iemgui>()) {
            if (iemgui->x_lab) {
                auto const labelText = getExpandedLabelText();
                int const fontHeight = getFontHeight();
                int const fontWidth = sys_fontwidth(fontHeight);
                int const posx = objectBounds.getX() + iemgui->x_ldx;
                int const posy = objectBounds.getY() + iemgui->x_ldy;
                int const textWidth = fontHeight > 55 ? Font(fontHeight).getStringWidth(labelText) : fontWidth * (labelText.length() + 1);
                return { posx, posy, textWidth, fontHeight + 2 };
            }
        }

        return objectBounds;
    }

    String getSendSymbol() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            t_symbol* srlsym[3];
            iemgui_all_sym2dollararg(iemgui.get(), srlsym);

            if (srl_is_valid(srlsym[0])) {
                return String::fromUTF8(iemgui->x_snd_unexpanded->s_name);
            }
        }

        return "";
    }

    String getReceiveSymbol() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            t_symbol* srlsym[3];
            iemgui_all_sym2dollararg(iemgui.get(), srlsym);

            if (srl_is_valid(srlsym[1])) {
                return String::fromUTF8(iemgui->x_rcv_unexpanded->s_name);
            }
        }

        return "";
    }

    bool hasSendSymbol() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return iemgui->x_fsf.x_snd_able;
        }

        return false;
    }

    bool hasReceiveSymbol() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return iemgui->x_fsf.x_rcv_able;
        }

        return false;
    }

    void setSendSymbol(String const& symbol) const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            auto* sym = symbol.isEmpty() ? gui->pd->generateSymbol("empty") : gui->pd->generateSymbol(symbol);
            iemgui_send(iemgui.get(), iemgui.get(), sym);
        }
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            auto* sym = symbol.isEmpty() ? gui->pd->generateSymbol("empty") : gui->pd->generateSymbol(symbol);
            iemgui_receive(iemgui.get(), iemgui.get(), sym);
        }
    }

    Colour getBackgroundColour() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return convertFromIEMColour(iemgui->x_bcol);
        }
        return Colour();
    }

    Colour getForegroundColour() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return convertFromIEMColour(iemgui->x_fcol);
        }
        return Colour();
    }

    Colour getLabelColour() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return convertFromIEMColour(iemgui->x_lcol);
        }
        return Colour();
    }

    void setBackgroundColour(Colour const colour) const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_bcol = convertToIEMColour(colour);
        }
    }

    void setForegroundColour(Colour const colour) const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_fcol = convertToIEMColour(colour);
        }
    }

    void setLabelColour(Colour const colour) const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_lcol = convertToIEMColour(colour);
        }
    }

    int getFontHeight() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return iemgui->x_fontsize;
        }

        return 14;
    }

    void setFontHeight(float const newSize)
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_fontsize = newSize;
        }
    }

    String getExpandedLabelText() const
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            if (t_symbol const* sym = iemgui->x_lab) {
                auto text = String::fromUTF8(sym->s_name);
                if (text.isNotEmpty() && text != "empty") {
                    return text;
                }
            }
        }

        return "";
    }

    static Colour convertFromIEMColour(int const color)
    {
        uint32 const c = static_cast<uint32>(color << 8 | 0xFF);
        return Colour(0xFF << 24 | c >> 24 << 16 | c >> 16 << 8 | c >> 8);
    }

    static uint32 convertToIEMColour(Colour const colour)
    {
        auto const colourString = colour.toString();
        char const* hex = colourString.toRawUTF8() + 2; // Remove alpha channel
        uint32 const col = static_cast<uint32>(strtol(hex, nullptr, 16));
        return col & 0xFFFFFF;
    }

    void setLabelText(String newText)
    {

        if (newText.isEmpty())
            newText = String("empty");

        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui_label(iemgui->x_glist, iemgui.get(), gui->pd->generateSymbol(newText));
        }
    }

    void setLabelPosition(Point<int> const position)
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            iemgui->x_ldx = position.x;
            iemgui->x_ldy = position.y;
        }
    }

    std::function<void()> iemColourChangedCallback = [] { };

    static constexpr int iemgui_color_hex[30] = {
        16579836, 10526880, 4210752, 16572640, 16572608,
        16579784, 14220504, 14220540, 14476540, 16308476,
        14737632, 8158332, 2105376, 16525352, 16559172,
        15263784, 1370132, 2684148, 3952892, 16003312,
        12369084, 6316128, 0, 9177096, 5779456,
        7874580, 2641940, 17488, 5256, 5767248
    };

    Object* object;
    ObjectBase* gui;

    pd::WeakReference ptr;

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value labelColour = SynchronousValue();

    Value labelPosition;
    Value labelHeight = SynchronousValue();
    Value labelText = SynchronousValue();

    Value initialise = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();

    ObjectParameters objectParameters;
};
