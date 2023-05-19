/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "x_libpd_extra_utils.h"

static int srl_is_valid(t_symbol const* s)
{
    return (s != nullptr && s != gensym(""));
}

#define IEMGUI_MESSAGES hash("send"), hash("receive"), hash("color"), hash("label"), hash("label_pos"), hash("label_font"), hash("vis_size"), hash("init")

extern "C" {
char* pdgui_strnescape(char* dst, size_t dstlen, char const* src, size_t srclen);
}

class IEMHelper {

public:
    IEMHelper(void* ptr, Object* parent, ObjectBase* base)
        : object(parent)
        , gui(base)
        , cnv(parent->cnv)
        , pd(parent->cnv->pd)
        , iemgui(static_cast<t_iemgui*>(ptr))
    {
    }

    void update()
    {
        primaryColour = Colour(getForegroundColour()).toString();
        secondaryColour = Colour(getBackgroundColour()).toString();
        labelColour = Colour(getLabelColour()).toString();

        gui->getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(Label::textWhenEditingColourId));
        gui->getLookAndFeel().setColour(Label::textColourId, Colour::fromString(primaryColour.toString()));

        gui->getLookAndFeel().setColour(TextButton::buttonOnColourId, Colour::fromString(primaryColour.toString()));
        gui->getLookAndFeel().setColour(Slider::thumbColourId, Colour::fromString(primaryColour.toString()));

        gui->getLookAndFeel().setColour(TextEditor::backgroundColourId, Colour::fromString(secondaryColour.toString()));
        gui->getLookAndFeel().setColour(TextButton::buttonColourId, Colour::fromString(secondaryColour.toString()));

        auto sliderBackground = Colour::fromString(secondaryColour.toString());
        sliderBackground = sliderBackground.getBrightness() > 0.5f ? sliderBackground.darker(0.6f) : sliderBackground.brighter(0.6f);

        gui->getLookAndFeel().setColour(Slider::backgroundColourId, sliderBackground);

        labelX = iemgui->x_ldx;
        labelY = iemgui->x_ldy;
        labelHeight = getFontHeight();
        labelText = getExpandedLabelText();

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();

        initialise = getInit();

        gui->repaint();
    }

    ObjectParameters makeIemParameters(bool withAppearance = true, bool withSymbols = true, int labelPosX = 0, int labelPosY = -8, int labelHeightY = 10)
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
        params.addParamString("Label", cLabel, &labelText, "");
        params.addParamColourLabel(&labelColour);
        params.addParamInt("Label X", cLabel, &labelX, labelPosX);
        params.addParamInt("Label Y", cLabel, &labelY, labelPosY);
        params.addParamInt("Label Height", cLabel, &labelHeight, labelHeightY);
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
    void addIemParameters(ObjectParameters& objectParams, bool withAppearance = true, bool withSymbols = true, int labelPosX = 0, int labelPosY = -8, int labelHeightY = 10)
    {
        auto IemParams = makeIemParameters(withAppearance, withSymbols, labelPosX, labelPosY, labelHeightY);
        for (auto const& param : IemParams.getParameters())
            objectParams.addParam(param);
    }

    bool receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms)
    {
        auto setColour = [this](Value& targetValue, pd::Atom& atom) {
            if (atom.isSymbol()) {
                auto colour = "#FF" + atom.getSymbol().fromFirstOccurrenceOf("#", false, false);
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
                    iemcolor = ((-1 - iemcolor) & 0xffffff);

                auto colour = Colour(static_cast<uint32>(convert_from_iem_color(iemcolor)));

                gui->setParameterExcludingListener(targetValue, colour.toString());
            }
        };
        switch (hash(symbol)) {
        case hash("send"): {
            if (atoms.size() >= 1)
                gui->setParameterExcludingListener(sendSymbol, atoms[0].getSymbol());
            return true;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                gui->setParameterExcludingListener(receiveSymbol, atoms[0].getSymbol());
            return true;
        }
        case hash("color"): {
            if (atoms.size() > 0)
                setColour(secondaryColour, atoms[0]);
            if (atoms.size() > 1)
                setColour(primaryColour, atoms[1]);
            if (atoms.size() > 2)
                setColour(labelColour, atoms[2]);
            gui->repaint();
            gui->updateLabel();
            return true;
        }
        case hash("label"): {
            if (atoms.size() >= 1) {
                gui->setParameterExcludingListener(labelText, atoms[0].getSymbol());
                gui->updateLabel();
            }
            return true;
        }
        case hash("label_pos"): {
            if (atoms.size() >= 2) {
                gui->setParameterExcludingListener(labelX, static_cast<int>(atoms[0].getFloat()));
                gui->setParameterExcludingListener(labelY, static_cast<int>(atoms[1].getFloat()));
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

    void valueChanged(Value& v)
    {
        if (v.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            setForegroundColour(colour);

            // TODO: move this!
            gui->getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
            gui->getLookAndFeel().setColour(Slider::thumbColourId, colour);
            gui->getLookAndFeel().setColour(Slider::trackColourId, colour);

            gui->getLookAndFeel().setColour(Label::textColourId, colour);
            gui->getLookAndFeel().setColour(Label::textWhenEditingColourId, colour);
            gui->getLookAndFeel().setColour(TextEditor::textColourId, colour);

            gui->repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            auto colour = Colour::fromString(secondaryColour.toString());
            setBackgroundColour(colour);

            gui->getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
            gui->getLookAndFeel().setColour(TextButton::buttonColourId, colour);

            gui->getLookAndFeel().setColour(Slider::backgroundColourId, colour);

            gui->repaint();
        } else if (v.refersToSameSourceAs(labelColour)) {
            setLabelColour(Colour::fromString(labelColour.toString()));
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(labelX) || v.refersToSameSourceAs(labelY)) {
            setLabelPosition({ getValue<int>(labelX), getValue<int>(labelY) });
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

    void setInit(bool init)
    {
        pd->lockAudioThread();
        iemgui->x_isa.x_loadinit = init;
        pd->unlockAudioThread();
    }

    bool getInit()
    {
        return iemgui->x_isa.x_loadinit;
    }

    Rectangle<int> getPdBounds()
    {
        pd->lockAudioThread();
        auto bounds = Rectangle<int>(iemgui->x_obj.te_xpix, iemgui->x_obj.te_ypix, iemgui->x_w, iemgui->x_h);
        pd->unlockAudioThread();

        return bounds.withTrimmedRight(-1).withTrimmedBottom(-1);
    }

    void setPdBounds(Rectangle<int> const b)
    {
        iemgui->x_obj.te_xpix = b.getX();
        iemgui->x_obj.te_ypix = b.getY();

        iemgui->x_w = b.getWidth() - 1;
        iemgui->x_h = b.getHeight() - 1;
    }

    void updateLabel(std::unique_ptr<ObjectLabel>& label)
    {
        int fontHeight = getFontHeight();

        const String text = getExpandedLabelText();

        if (text.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<ObjectLabel>(object);
            }

            auto bounds = getLabelBounds();

            bounds.translate(0, fontHeight / -2.0f);

            label->setFont(Font(fontHeight));
            label->setBounds(bounds);
            label->setText(text, dontSendNotification);

            label->setColour(Label::textColourId, getLabelColour());

            object->cnv->addAndMakeVisible(label.get());
        } else {
            label.reset(nullptr);
        }
    }

    Rectangle<int> getLabelBounds() const
    {
        auto objectBounds = object->getBounds().reduced(Object::margin);

        t_symbol const* sym = canvas_realizedollar(iemgui->x_glist, iemgui->x_lab);
        if (sym) {
            int fontHeight = getFontHeight();
            int labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());

            int const posx = objectBounds.getX() + iemgui->x_ldx + 4;
            int const posy = objectBounds.getY() + iemgui->x_ldy;

            return { posx, posy, labelLength, fontHeight };
        }

        return objectBounds;
    }

    String getSendSymbol() const
    {
        pd->setThis();

        t_symbol* srlsym[3];
        iemgui_all_sym2dollararg(iemgui, srlsym);

        if (srl_is_valid(srlsym[0])) {
            return String::fromUTF8(iemgui->x_snd_unexpanded->s_name);
        }

        return "";
    }

    String getReceiveSymbol() const
    {
        pd->setThis();

        t_symbol* srlsym[3];
        iemgui_all_sym2dollararg(iemgui, srlsym);

        if (srl_is_valid(srlsym[1])) {
            return String::fromUTF8(iemgui->x_rcv_unexpanded->s_name);
        }

        return "";
    }

    bool hasSendSymbol() const
    {
        return iemgui->x_fsf.x_snd_able;
    }

    bool hasReceiveSymbol() const
    {
        return iemgui->x_fsf.x_rcv_able;
    }

    void setSendSymbol(String const& symbol) const
    {
        auto* sym = symbol.isEmpty() ? pd->generateSymbol("empty") : pd->generateSymbol(symbol);
        iemgui_send(iemgui, iemgui, sym);
    }

    void setReceiveSymbol(String const& symbol) const
    {
        auto* sym = symbol.isEmpty() ? pd->generateSymbol("empty") : pd->generateSymbol(symbol);
        iemgui_receive(iemgui, iemgui, sym);
    }

    Colour getBackgroundColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_background_color(iemgui)));
    }

    Colour getForegroundColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_foreground_color(iemgui)));
    }

    Colour getLabelColour() const
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_label_color(iemgui)));
    }

    void setBackgroundColour(Colour colour) const
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_background_color(iemgui, colourStr.toRawUTF8());
    }

    void setForegroundColour(Colour colour) const
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_foreground_color(iemgui, colourStr.toRawUTF8());
    }

    void setLabelColour(Colour colour) const
    {
        String colourStr = colour.toString();
        libpd_iemgui_set_label_color(iemgui, colourStr.toRawUTF8());
    }

    int getFontHeight() const
    {
        return iemgui->x_fontsize;
    }

    void setFontHeight(float newSize)
    {
        iemgui->x_fontsize = newSize;
    }

    String getExpandedLabelText() const
    {
        t_symbol const* sym = iemgui->x_lab;
        if (sym) {
            auto text = String::fromUTF8(sym->s_name);
            if (text.isNotEmpty() && text != "empty") {
                return text;
            }
        }

        return "";
    }

    String getLabelText() const
    {
        t_symbol const* sym = iemgui->x_lab_unexpanded;
        if (sym) {
            auto text = String::fromUTF8(sym->s_name);
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

        iemgui->x_lab_unexpanded = pd->generateSymbol(newText);
        iemgui->x_lab = canvas_realizedollar(iemgui->x_glist, iemgui->x_lab_unexpanded);
    }

    void setLabelPosition(Point<int> position)
    {
        iemgui->x_ldx = position.x;
        iemgui->x_ldy = position.y;
    }

    int iemgui_color_hex[30] = {
        16579836, 10526880, 4210752, 16572640, 16572608,
        16579784, 14220504, 14220540, 14476540, 16308476,
        14737632, 8158332, 2105376, 16525352, 16559172,
        15263784, 1370132, 2684148, 3952892, 16003312,
        12369084, 6316128, 0, 9177096, 5779456,
        7874580, 2641940, 17488, 5256, 5767248
    };

    Object* object;
    ObjectBase* gui;
    Canvas* cnv;
    PluginProcessor* pd;

    t_iemgui* iemgui;

    Value primaryColour;
    Value secondaryColour;
    Value labelColour;

    Value labelX = Value(0.0f);
    Value labelY = Value(0.0f);
    Value labelHeight = Value(18.0f);
    Value labelText;

    Value initialise;
    Value sendSymbol;
    Value receiveSymbol;

    ObjectParameters objectParameters;
};
