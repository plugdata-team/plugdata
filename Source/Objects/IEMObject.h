#include "x_libpd_extra_utils.h"

extern "C"
{
    char* pdgui_strnescape(char* dst, size_t dstlen, const char* src, size_t srclen);
}

struct IEMObject : public GUIObject
{
    IEMObject(void* ptr, Box* parent) : GUIObject(ptr, parent)
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);

        t_symbol* srlsym[3];
        srlsym[0] = iemgui->x_snd;
        srlsym[1] = iemgui->x_rcv;
        srlsym[2] = iemgui->x_lab;

        iemgui_all_dollar2raute(srlsym);
        iemgui_all_sym2dollararg(iemgui, srlsym);
        String label = String(srlsym[2]->s_name).removeCharacters("\\");
        iemgui->x_lab_unexpanded = gensym(label.toRawUTF8());

        labelText = getLabelText();

        labelX = iemgui->x_ldx;
        labelY = iemgui->x_ldy;
        labelHeight = getFontHeight();

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();
    }

    void paint(Graphics& g) override
    {
        g.setColour(getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        auto outlineColour = box->findColour(cnv->isSelected(box) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
    
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
        for (auto& [name, type, cat, value, list] : params)
        {
            value->addListener(this);

            // Push current parameters to pd
            valueChanged(*value);
        }

        repaint();
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters params = defineParameters();

        params.push_back({"Foreground", tColour, cAppearance, &primaryColour, {}});
        params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Colour", tColour, cLabel, &labelColour, {}});
        params.push_back({"Label X", tInt, cLabel, &labelX, {}});
        params.push_back({"Label Y", tInt, cLabel, &labelY, {}});
        params.push_back({"Label Height", tInt, cLabel, &labelHeight, {}});

        return params;
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sendSymbol))
        {
            setSendSymbol(sendSymbol.toString());
        }
        else if (v.refersToSameSourceAs(receiveSymbol))
        {
            setReceiveSymbol(receiveSymbol.toString());
        }
        else if (v.refersToSameSourceAs(primaryColour))
        {
            auto colour = Colour::fromString(primaryColour.toString());
            setForegroundColour(colour);

            getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
            getLookAndFeel().setColour(Slider::thumbColourId, colour);
            getLookAndFeel().setColour(Slider::trackColourId, colour);

            getLookAndFeel().setColour(Label::textColourId, colour);
            getLookAndFeel().setColour(Label::textWhenEditingColourId, colour);
            getLookAndFeel().setColour(TextEditor::textColourId, colour);

            repaint();
        }
        else if (v.refersToSameSourceAs(secondaryColour))
        {
            auto colour = Colour::fromString(secondaryColour.toString());
            setBackgroundColour(colour);

            getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
            getLookAndFeel().setColour(TextButton::buttonColourId, colour);

            getLookAndFeel().setColour(Slider::backgroundColourId, colour);

            repaint();
        }

        else if (v.refersToSameSourceAs(labelColour))
        {
            setLabelColour(Colour::fromString(labelColour.toString()));
            updateLabel();
        }
        else if (v.refersToSameSourceAs(labelX))
        {
            setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
            updateLabel();
        }
        if (v.refersToSameSourceAs(labelY))
        {
            setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
            updateLabel();
        }
        else if (v.refersToSameSourceAs(labelHeight))
        {
            setFontHeight(static_cast<int>(labelHeight.getValue()));
            updateLabel();
        }
        else if (v.refersToSameSourceAs(labelText))
        {
            setLabelText(labelText.toString());
            updateLabel();
        }
    }

    void updateBounds() override
    {
        box->cnv->pd->enqueueFunction([this, _this = SafePointer(this)]() {
            if(!_this) return;
            
            auto* iemgui = static_cast<t_iemgui*>(ptr);
            auto bounds = Rectangle<int>(iemgui->x_obj.te_xpix, iemgui->x_obj.te_ypix, iemgui->x_w, iemgui->x_h);
            
            MessageManager::callAsync([this, _this = SafePointer(this), bounds]() mutable {
                if(!_this) return;
                box->setObjectBounds(bounds);
            });
        });
    }

    void updateLabel() override
    {
        int fontHeight = getFontHeight();

        if (fontHeight == 0)
        {
            // fontHeight = glist_getfont(box->cnv->patch.getPointer());
        }

        const String text = getExpandedLabelText();

        if (text.isNotEmpty())
        {
            if (!label)
            {
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

            box->cnv->addAndMakeVisible(label.get());
        }
    }

    Rectangle<int> getLabelBounds() const 
    {
        auto objectBounds = box->getBounds().reduced(Box::margin);

        t_symbol const* sym = canvas_realizedollar(static_cast<t_iemgui*>(ptr)->x_glist, static_cast<t_iemgui*>(ptr)->x_lab);
        if (sym)
        {
            int fontHeight = getFontHeight();
            int labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());

            auto const* iemgui = static_cast<t_iemgui*>(ptr);
            int const posx = objectBounds.getX() + iemgui->x_ldx;
            int const posy = objectBounds.getY() + iemgui->x_ldy;

            return {posx, posy, labelLength, fontHeight};
        }
        
        return objectBounds;
    }

    String getSendSymbol() 
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);
        String name = iemgui->x_snd_unexpanded->s_name;
        if (name == "empty") return "";

        return name;
    }

    String getReceiveSymbol() 
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);

        String name = iemgui->x_rcv_unexpanded->s_name;

        if (name == "empty") return "";

        return name;
    }

    void setSendSymbol(const String& symbol) const 
    {
        if (symbol.isEmpty()) return;

        auto* iemgui = static_cast<t_iemgui*>(ptr);

        if (symbol == "empty")
        {
            iemgui->x_fsf.x_snd_able = false;
        }
        else
        {
            iemgui->x_snd_unexpanded = gensym(symbol.toRawUTF8());
            iemgui->x_snd = canvas_realizedollar(iemgui->x_glist, iemgui->x_snd_unexpanded);

            iemgui->x_fsf.x_snd_able = true;
            iemgui_verify_snd_ne_rcv(iemgui);
        }
    }

    void setReceiveSymbol(const String& symbol) const 
    {
        if (symbol.isEmpty()) return;

        auto* iemgui = static_cast<t_iemgui*>(ptr);

        bool rcvable = true;

        if (symbol == "empty")
        {
            rcvable = false;
        }

        // iemgui_all_raute2dollar(srl);
        // iemgui_all_dollararg2sym(iemgui, srl);

        if (rcvable)
        {
            if (strcmp(symbol.toRawUTF8(), iemgui->x_rcv_unexpanded->s_name))
            {
                if (iemgui->x_fsf.x_rcv_able) pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
                iemgui->x_rcv_unexpanded = gensym(symbol.toRawUTF8());
                iemgui->x_rcv = canvas_realizedollar(iemgui->x_glist, iemgui->x_rcv_unexpanded);
                pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            }
        }
        else if (iemgui->x_fsf.x_rcv_able)
        {
            pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv_unexpanded = gensym(symbol.toRawUTF8());
            iemgui->x_rcv = canvas_realizedollar(iemgui->x_glist, iemgui->x_rcv_unexpanded);
        }

        iemgui->x_fsf.x_rcv_able = rcvable;
        iemgui_verify_snd_ne_rcv(iemgui);
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
        if (sym)
        {
            auto const text = String(sym->s_name);
            if (text.isNotEmpty() && text != "empty")
            {
                return text;
            }
        }

        return "";
    }

    String getLabelText() const
    {
        t_symbol const* sym = static_cast<t_iemgui*>(ptr)->x_lab_unexpanded;
        if (sym)
        {
            auto const text = String(sym->s_name);
            if (text.isNotEmpty() && text != "empty")
            {
                return text;
            }
        }

        return "";
    }

    void setLabelText(String newText)
    {
        if (newText.isEmpty()) newText = "empty";

        auto* iemgui = static_cast<t_iemgui*>(ptr);
        if (newText != "empty")
        {
            iemgui->x_lab_unexpanded = gensym(newText.toRawUTF8());
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
