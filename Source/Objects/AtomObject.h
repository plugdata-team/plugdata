

// False GATOM
typedef struct _fake_gatom
{
    t_text a_text;
    int a_flavor;          /* A_FLOAT, A_SYMBOL, or A_LIST */
    t_glist* a_glist;      /* owning glist */
    t_float a_toggle;      /* value to toggle to */
    t_float a_draghi;      /* high end of drag range */
    t_float a_draglo;      /* low end of drag range */
    t_symbol* a_label;     /* symbol to show as label next to box */
    t_symbol* a_symfrom;   /* "receive" name -- bind ourselves to this */
    t_symbol* a_symto;     /* "send" name -- send to this on output */
    t_binbuf* a_revertbuf; /* binbuf to revert to if typing canceled */
    int a_dragindex;       /* index of atom being dragged */
    int a_fontsize;
    unsigned int a_shift : 1;         /* was shift key down when drag started? */
    unsigned int a_wherelabel : 2;    /* 0-3 for left, right, above, below */
    unsigned int a_grabbed : 1;       /* 1 if we've grabbed keyboard */
    unsigned int a_doubleclicked : 1; /* 1 if dragging from a double click */
    t_symbol* a_expanded_to;
} t_fake_gatom;

static t_atom* fake_gatom_getatom(t_fake_gatom* x)
{
    int ac = binbuf_getnatom(x->a_text.te_binbuf);
    t_atom* av = binbuf_getvec(x->a_text.te_binbuf);
    if (x->a_flavor == A_FLOAT && (ac != 1 || av[0].a_type != A_FLOAT))
    {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "f", 0.);
    }
    else if (x->a_flavor == A_SYMBOL && (ac != 1 || av[0].a_type != A_SYMBOL))
    {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "s", &s_);
    }
    return (binbuf_getvec(x->a_text.te_binbuf));
}

struct AtomObject : public GUIObject
{
    AtomObject(void* ptr, Box* parent) : GUIObject(ptr, parent)
    {
        auto* atom = static_cast<t_fake_gatom*>(ptr);

        labelText = getLabelText();
        labelX = static_cast<int>(atom->a_wherelabel + 1);

        int h = getFontHeight();

        int idx = static_cast<int>(std::find(atomSizes, atomSizes + 7, h) - atomSizes);
        labelHeight = idx + 1;
    }

    void updateBounds() override
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        int x, y, w, h;

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        w = std::max<int>(4, gatom->a_text.te_width) * glist_fontwidth(cnv->patch.getPointer());

        box->setObjectBounds({x, y, w, getAtomHeight()});
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());

        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        gatom->a_text.te_width = b.getWidth() / fontWidth;
    }

    void resized() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int width = jlimit(30, maxSize, (getWidth() / fontWidth) * fontWidth);
        int height = jlimit(18, maxSize, getHeight());
        if (getWidth() != width || getHeight() != height)
        {
            box->setSize(width + Box::doubleMargin, height + Box::doubleMargin);
        }
    }

    int getAtomHeight() const
    {
        int idx = static_cast<int>(labelHeight.getValue()) - 1;
        if (idx == 0)
        {
            return glist_fontheight(cnv->patch.getPointer()) + 8;
        }
        else
        {
            return atomSizes[idx] + 8;
        }
    }

    void paint(Graphics& g) override
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(PlugDataColour::textColourId));
        getLookAndFeel().setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));
        getLookAndFeel().setColour(TextEditor::textColourId, box->findColour(PlugDataColour::textColourId));

        g.setColour(box->findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangle = triangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(triangle);

        auto outlineColour = box->findColour(cnv->isSelected(box) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters params = defineParameters();

        params.push_back({"Height", tCombo, cGeneral, &labelHeight, {"auto", "8", "10", "12", "16", "24", "36"}});

        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});

        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Position", tCombo, cLabel, &labelX, {"left", "right", "top", "bottom"}});

        return params;
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(labelX))
        {
            setLabelPosition(static_cast<int>(labelX.getValue()));
            updateLabel();
        }
        else if (v.refersToSameSourceAs(labelHeight))
        {
            updateLabel();
            updateBounds();
        }
        else if (v.refersToSameSourceAs(labelText))
        {
            setLabelText(labelText.toString());
            updateLabel();
        }
        else if (v.refersToSameSourceAs(sendSymbol))
        {
            setSendSymbol(sendSymbol.toString());
        }
        else if (v.refersToSameSourceAs(receiveSymbol))
        {
            setReceiveSymbol(receiveSymbol.toString());
        }
    }

    void updateLabel() override
    {
        int idx = std::clamp<int>(labelHeight.getValue(), 1, 7);
        setFontHeight(atomSizes[idx - 1]);

        int fontHeight = getAtomHeight() - 6;
        const String text = getExpandedLabelText();

        if (text.isNotEmpty())
        {
            if (!label)
            {
                label = std::make_unique<Label>();
            }

            auto bounds = getLabelBounds();
            label->setBounds(bounds);

            label->setFont(Font(fontHeight));
            label->setJustificationType(Justification::centredLeft);

            label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
            label->setMinimumHorizontalScale(1.f);
            label->setText(text, dontSendNotification);
            label->setEditable(false, false);
            label->setInterceptsMouseClicks(false, false);

            label->setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));

            box->cnv->addAndMakeVisible(label.get());
        }
    }

    float getFontHeight() const
    {
        return static_cast<t_fake_gatom*>(ptr)->a_fontsize;
    }

    void setFontHeight(float newSize)
    {
        static_cast<t_fake_gatom*>(ptr)->a_fontsize = newSize;
    }

    Rectangle<int> getLabelBounds() const
    {
        auto objectBounds = box->getBounds().reduced(Box::margin);
        int fontHeight = getAtomHeight() - 6;

        int labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());
        int labelPosition = static_cast<t_fake_gatom*>(ptr)->a_wherelabel;
        auto labelBounds = objectBounds.withSizeKeepingCentre(labelLength, fontHeight);

        if (labelPosition == 0)
        {  // left
            return labelBounds.withRightX(objectBounds.getX() - 4);
        }
        if (labelPosition == 1)
        {  // right
            return labelBounds.withX(objectBounds.getRight() + 4);
        }
        if (labelPosition == 2)
        {  // top
            return labelBounds.withX(objectBounds.getX()).withBottomY(objectBounds.getY());
        }

        return labelBounds.withX(objectBounds.getX()).withY(objectBounds.getBottom());
    }

    String getExpandedLabelText() const
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        t_symbol const* sym = canvas_realizedollar(gatom->a_glist, gatom->a_label);
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
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        t_symbol const* sym = gatom->a_label;
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

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_label = gensym(newText.toRawUTF8());
    }

    void setLabelPosition(int wherelabel)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        gatom->a_wherelabel = wherelabel - 1;
    }

    String getSendSymbol()
    {
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        return String(atom->a_symfrom->s_name);
    }

    String getReceiveSymbol()
    {
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        return String(atom->a_symto->s_name);
    }

    void setSendSymbol(const String& symbol) const
    {
        if (symbol.isEmpty()) return;

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_symto = gensym(symbol.toRawUTF8());
        atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
    }

    void setReceiveSymbol(const String& symbol) const
    {
        if (symbol.isEmpty()) return;

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        if (*atom->a_symfrom->s_name) pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        atom->a_symfrom = gensym(symbol.toRawUTF8());
        if (*atom->a_symfrom->s_name) pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
    }

    /* prepend "-" as necessary to avoid empty strings, so we can
    use them in Pd messages. */
    static t_symbol* gatom_escapit(t_symbol* s)
    {
        if (!*s->s_name)
            return (gensym("-"));
        else if (*s->s_name == '-')
        {
            char shmo[100];
            shmo[0] = '-';
            strncpy(shmo + 1, s->s_name, 99);
            shmo[99] = 0;
            return (gensym(shmo));
        }
        else
            return (s);
    }

    /* undo previous operation: strip leading "-" if found.  This is used
    both to restore send, etc., names when loading from a file, and to
    set them from the properties dialog.  In the former case, since before
    version 0.52 '$" was aliases to "#", we also bash any "#" characters
    to "$".  This is unnecessary when reading files saved from 0.52 or later,
    and really we should test for that and only bash when necessary, just
    in case someone wants to have a "#" in a name. */
    static t_symbol* gatom_unescapit(t_symbol* s)
    {
        if (*s->s_name == '-')
            return (gensym(s->s_name + 1));
        else
            return (iemgui_raute2dollar(s));
    }

    const int atomSizes[7] = {0, 8, 10, 12, 16, 24, 36};
};
