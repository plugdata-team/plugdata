
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
        labelText = getLabelText();
        labelX = static_cast<int>(static_cast<t_fake_gatom*>(ptr)->a_wherelabel + 1);

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
        h = atomSizes[static_cast<int>(labelHeight.getValue())] + Box::doubleMargin;
        box->setObjectBounds({x, y, w, h});
    }

    void applyBounds() override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), box->getX() + Box::margin, box->getY() + Box::margin);

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());

        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        gatom->a_text.te_width = width / fontWidth;
    }

    void resized() override
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int width = std::max(4, getWidth() / fontWidth) * fontWidth;

        if (width != getWidth() || box->getHeight() != Box::height)
        {
            box->setSize(width + Box::doubleMargin, Box::height);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));

        g.fillPath(triangle);
    }

    void initialise() override
    {
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
    }

    void updateLabel() override
    {
        int idx = std::clamp<int>(labelHeight.getValue(), 1, 7);
        setFontHeight(atomSizes[idx - 1]);

        int fontHeight = getFontHeight();

        if (fontHeight == 0)
        {
            fontHeight = glist_getfont(box->cnv->patch.getPointer()) + 2;
        }

        const String text = getLabelText();

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

    float getFontHeight() const noexcept
    {
        return static_cast<t_fake_gatom*>(ptr)->a_fontsize;
    }

    void setFontHeight(float newSize) noexcept
    {
        static_cast<t_fake_gatom*>(ptr)->a_fontsize = newSize;
    }

    Rectangle<int> getLabelBounds() const noexcept
    {
        auto objectBounds = box->getObjectBounds();
        int fontHeight = getFontHeight() + 2;
        if (fontHeight == 0)
        {
            fontHeight = glist_getfont(cnv->patch.getPointer());
        }

        int labelLength = Font(fontHeight).getStringWidth(getLabelText());
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
        if (labelPosition == 3)
        {  // bottom
            return labelBounds.withX(objectBounds.getX()).withY(objectBounds.getBottom());
        }
    }

    String getLabelText() const
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

    void setLabelText(String newText)
    {
        if (newText.isEmpty()) newText = "empty";

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_label = gensym(newText.toRawUTF8());
    }

    void setLabelPosition(int wherelabel) noexcept
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        gatom->a_wherelabel = wherelabel - 1;
    }

    String getSendSymbol() noexcept
    {
        return "";
    }

    String getReceiveSymbol() noexcept
    {
        return "";
    }

    void setSendSymbol(const String& symbol) const noexcept
    {
        if (symbol.isEmpty()) return;

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_symto = gensym(symbol.toRawUTF8());
        atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
    }

    void setReceiveSymbol(const String& symbol) const noexcept
    {
        if (symbol.isEmpty()) return;

        auto* atom = static_cast<t_fake_gatom*>(ptr);
        if (*atom->a_symfrom->s_name) pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        atom->a_symfrom = gensym(symbol.toRawUTF8());
        if (*atom->a_symfrom->s_name) pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
    }
    
    const int atomSizes[7] = {0, 8, 10, 12, 16, 24, 36};
};
