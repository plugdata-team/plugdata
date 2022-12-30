/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "x_libpd_extra_utils.h"

static int srl_is_valid(t_symbol const* s)
{
    return (!!s && s != &s_);
}

extern "C" {
char* pdgui_strnescape(char* dst, size_t dstlen, char const* src, size_t srclen);
}

struct IEMObject : public GUIObject {

    Value initialise;

    IEMObject(void* ptr, Object* parent)
        : GUIObject(ptr, parent)
    {
        /*

        t_symbol* srlsym[3];
        srlsym[0] = iemgui->x_snd;
        srlsym[1] = iemgui->x_rcv;
        srlsym[2] = iemgui->x_lab;

        char label_chars[MAXPDSTRING];

        for(int i = 0; i < 3; i++) {
            if(srlsym[i])
                srlsym[i] = pd->generateSymbol(pdgui_strnescape(label_chars, sizeof(label_chars), srlsym[i]->s_name, strlen(srlsym[i]->s_name)));
        }

        String label = String::fromUTF8(pd->generateSymbol[2]->s_name).removeCharacters("\\");
        iemgui->x_lab_unexpanded = gensym(label)); */

        labelText = getLabelText();

        auto* iemgui = static_cast<t_iemgui*>(ptr);

        labelX = iemgui->x_ldx;
        labelY = iemgui->x_ldy;
        labelHeight = getFontHeight();

        initialise = static_cast<bool>(iemgui->x_isa.x_loadinit);
        initialise.addListener(this);

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();
    }

    void paint(Graphics& g) override
    {
        g.setColour(getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Constants::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Constants::objectCornerRadius, 1.0f);
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();

        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui->x_obj.te_xpix = b.getX();
        iemgui->x_obj.te_ypix = b.getY();
        iemgui->x_w = b.getWidth();
        iemgui->x_h = b.getHeight();
    }

    void updateParameters() override
    {
        primaryColour = Colour(getForegroundColour()).toString();
        secondaryColour = Colour(getBackgroundColour()).toString();
        labelColour = Colour(getLabelColour()).toString();

        getLookAndFeel().setColour(TextButton::buttonOnColourId, Colour::fromString(primaryColour.toString()));
        getLookAndFeel().setColour(Slider::thumbColourId, Colour::fromString(primaryColour.toString()));

        getLookAndFeel().setColour(TextEditor::backgroundColourId, Colour::fromString(secondaryColour.toString()));
        getLookAndFeel().setColour(TextButton::buttonColourId, Colour::fromString(secondaryColour.toString()));

        auto sliderBackground = Colour::fromString(secondaryColour.toString());
        sliderBackground = sliderBackground.getBrightness() > 0.5f ? sliderBackground.darker(0.6f) : sliderBackground.brighter(0.6f);

        getLookAndFeel().setColour(Slider::backgroundColourId, sliderBackground);

        auto params = getParameters();
        for (auto& [name, type, cat, value, list] : params) {
            value->addListener(this);

            // Push current parameters to pd
            valueChanged(*value);
        }

        repaint();
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters params = defineParameters();

        params.push_back({ "Foreground", tColour, cAppearance, &primaryColour, {} });
        params.push_back({ "Background", tColour, cAppearance, &secondaryColour, {} });
        params.push_back({ "Send Symbol", tString, cGeneral, &sendSymbol, {} });
        params.push_back({ "Receive Symbol", tString, cGeneral, &receiveSymbol, {} });
        params.push_back({ "Label", tString, cLabel, &labelText, {} });
        params.push_back({ "Label Colour", tColour, cLabel, &labelColour, {} });
        params.push_back({ "Label X", tInt, cLabel, &labelX, {} });
        params.push_back({ "Label Y", tInt, cLabel, &labelY, {} });
        params.push_back({ "Label Height", tInt, cLabel, &labelHeight, {} });
        params.push_back({ "Initialise", tBool, cGeneral, &initialise, { "No", "Yes" } });
        return params;
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {

        if (symbol == "color") {

            if (atoms.size() > 0)
                setParameterExcludingListener(secondaryColour, atoms[0].getSymbol());
            if (atoms.size() > 1)
                setParameterExcludingListener(primaryColour, atoms[1].getSymbol());
            if (atoms.size() > 2)
                setParameterExcludingListener(labelColour, atoms[2].getSymbol());

            repaint();
        } else if (symbol == "label" && atoms.size() >= 1) {
            setParameterExcludingListener(labelText, atoms[0].getSymbol());
            updateLabel();
        } else if (symbol == "label_pos" && atoms.size() >= 2) {
            setParameterExcludingListener(labelX, static_cast<int>(atoms[0].getFloat()));
            setParameterExcludingListener(labelY, static_cast<int>(atoms[1].getFloat()));
            updateLabel();
        } else if (symbol == "label_font" && atoms.size() >= 2) {
            setParameterExcludingListener(labelHeight, static_cast<int>(atoms[1].getFloat()));
            updateLabel();
        } else if (symbol == "init" && atoms.size() >= 1) {
            setParameterExcludingListener(initialise, static_cast<bool>(atoms[0].getFloat()));
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
        } else if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            setForegroundColour(colour);

            // TODO: move this!
            getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
            getLookAndFeel().setColour(Slider::thumbColourId, colour);
            getLookAndFeel().setColour(Slider::trackColourId, colour);

            getLookAndFeel().setColour(Label::textColourId, colour);
            getLookAndFeel().setColour(Label::textWhenEditingColourId, colour);
            getLookAndFeel().setColour(TextEditor::textColourId, colour);

            repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            auto colour = Colour::fromString(secondaryColour.toString());
            setBackgroundColour(colour);

            getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
            getLookAndFeel().setColour(TextButton::buttonColourId, colour);

            getLookAndFeel().setColour(Slider::backgroundColourId, colour);

            repaint();
        }

        else if (v.refersToSameSourceAs(labelColour)) {
            setLabelColour(Colour::fromString(labelColour.toString()));
            updateLabel();
        } else if (v.refersToSameSourceAs(labelX)) {
            setLabelPosition({ static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue()) });
            updateLabel();
        }
        if (v.refersToSameSourceAs(labelY)) {
            setLabelPosition({ static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue()) });
            updateLabel();
        } else if (v.refersToSameSourceAs(labelHeight)) {
            setFontHeight(static_cast<int>(labelHeight.getValue()));
            updateLabel();
        } else if (v.refersToSameSourceAs(labelText)) {
            setLabelText(labelText.toString());
            updateLabel();
        } else if (v.refersToSameSourceAs(initialise)) {
            auto* nbx = static_cast<t_my_numbox*>(ptr);
            nbx->x_gui.x_isa.x_loadinit = static_cast<bool>(initialise.getValue());
        }
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        auto bounds = Rectangle<int>(iemgui->x_obj.te_xpix, iemgui->x_obj.te_ypix, iemgui->x_w, iemgui->x_h);
        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    void updateLabel() override
    {
        int fontHeight = getFontHeight();

        const String text = getExpandedLabelText();

        if (text.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<Label>();
            }

            auto bounds = getLabelBounds();

            bounds.translate(0, fontHeight / -2.0f);

            label->setFont(Font(fontHeight));
            label->setJustificationType(Justification::centredLeft);
            label->setBounds(bounds);
            label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
            label->setMinimumHorizontalScale(1.f);
            label->setText(text, dontSendNotification);
            label->setEditable(false, false);
            label->setInterceptsMouseClicks(false, false);

            label->setColour(Label::textColourId, getLabelColour());

            object->cnv->addAndMakeVisible(label.get());
        }
    }

    Rectangle<int> getLabelBounds() const
    {
        auto objectBounds = object->getBounds().reduced(Object::margin);

        t_symbol const* sym = canvas_realizedollar(static_cast<t_iemgui*>(ptr)->x_glist, static_cast<t_iemgui*>(ptr)->x_lab);
        if (sym) {
            int fontHeight = getFontHeight();
            int labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());

            auto const* iemgui = static_cast<t_iemgui*>(ptr);
            int const posx = objectBounds.getX() + iemgui->x_ldx;
            int const posy = objectBounds.getY() + iemgui->x_ldy;

            return { posx, posy, labelLength, fontHeight };
        }

        return objectBounds;
    }

    String getSendSymbol()
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);

        if (srl_is_valid(srlsym[0])) {
            return String(iemgui->x_snd_unexpanded->s_name);
        }

        return "";
    }

    String getReceiveSymbol()
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);

        if (srl_is_valid(srlsym[1])) {
            return String(iemgui->x_rcv_unexpanded->s_name);
        }

        return "";
    }

    void setSendSymbol(String const& symbol) const
    {
        auto* sym = symbol.isEmpty() ? nullptr : pd->generateSymbol(symbol);
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_send(ptr, iemgui, sym);
    }

    void setReceiveSymbol(String const& symbol) const
    {

        auto* sym = symbol.isEmpty() ? nullptr : pd->generateSymbol(symbol);
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_receive(ptr, iemgui, sym);
    }

    static unsigned int fromIemColours(int const color)
    {
        auto const c = static_cast<unsigned int>(color << 8 | 0xFF);
        return ((0xFF << 24) | ((c >> 24) << 16) | ((c >> 16) << 8) | (c >> 8));
    }

    Colour getBackgroundColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_background_color(ptr)));
    }

    Colour getForegroundColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_foreground_color(ptr)));
    }

    Colour getLabelColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_label_color(ptr)));
    }

    void setBackgroundColour(Colour colour)
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_background_color(ptr, colourStr.toRawUTF8());
    }

    void setForegroundColour(Colour colour)
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_foreground_color(ptr, colourStr.toRawUTF8());
    }

    void setLabelColour(Colour colour)
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_label_color(ptr, colourStr.toRawUTF8());
    }

    int getFontHeight() const
    {
        return static_cast<t_iemgui*>(ptr)->x_fontsize;
    }

    void setFontHeight(float newSize)
    {
        static_cast<t_iemgui*>(ptr)->x_fontsize = newSize;
    }

    String getExpandedLabelText() const
    {
        t_symbol const* sym = static_cast<t_iemgui*>(ptr)->x_lab;
        if (sym) {
            auto const text = String::fromUTF8(sym->s_name);
            if (text.isNotEmpty() && text != "empty") {
                return text;
            }
        }

        return "";
    }

    String getLabelText() const
    {
        t_symbol const* sym = static_cast<t_iemgui*>(ptr)->x_lab_unexpanded;
        if (sym) {
            auto const text = String::fromUTF8(sym->s_name);
            if (text.isNotEmpty() && text != "empty") {
                return text;
            }
        }

        return "";
    }

    void setLabelText(String newText)
    {
        if (newText.isEmpty())
            newText = "empty";

        auto* iemgui = static_cast<t_iemgui*>(ptr);
        if (newText != "empty") {
            iemgui->x_lab_unexpanded = pd->generateSymbol(newText);
            iemgui->x_lab = canvas_realizedollar(iemgui->x_glist, iemgui->x_lab_unexpanded);
        }
    }

    void setLabelPosition(Point<int> position)
    {
        auto* iem = static_cast<t_iemgui*>(ptr);
        iem->x_ldx = position.x;
        iem->x_ldy = position.y;
    }
};
