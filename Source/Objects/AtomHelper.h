/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// False GATOM
typedef struct _gatom {
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
    t_symbol* a_expanded_to;          /* a_symto after $0, $1, ...  expansion */
} t_fake_gatom;

static t_atom* fake_gatom_getatom(t_fake_gatom* x)
{
    int ac = binbuf_getnatom(x->a_text.te_binbuf);
    t_atom* av = binbuf_getvec(x->a_text.te_binbuf);
    if (x->a_flavor == A_FLOAT && (ac != 1 || av[0].a_type != A_FLOAT)) {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "f", 0.);
    } else if (x->a_flavor == A_SYMBOL && (ac != 1 || av[0].a_type != A_SYMBOL)) {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "s", gensym("symbol"));
    }
    return (binbuf_getvec(x->a_text.te_binbuf));
}

class AtomHelper {

    int const atomSizes[8] = { 0, 8, 10, 12, 16, 24, 36};

    Object* object;
    ObjectBase* gui;
    Canvas* cnv;
    PluginProcessor* pd;

    t_fake_gatom* atom;

    inline static int minWidth = 3;

public:
    Value labelColour;
    Value labelPosition = Value(0.0f);
    Value fontSize = Value(18.0f);
    Value labelText;
    Value sendSymbol;
    Value receiveSymbol;

    AtomHelper(void* ptr, Object* parent, ObjectBase* base)
        : object(parent)
        , gui(base)
        , cnv(parent->cnv)
        , pd(parent->cnv->pd)
        , atom(static_cast<t_fake_gatom*>(ptr))
    {
        labelText = getLabelText();
        labelPosition = static_cast<int>(atom->a_wherelabel + 1);

        int h = getFontHeight();

        int idx = static_cast<int>(std::find(atomSizes, atomSizes + 7, h) - atomSizes);
        fontSize = idx + 1;

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();
    }

    Rectangle<int> getPdBounds()
    {
        pd->lockAudioThread();

        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), atom, &x, &y, &w, &h);

        w = std::max<int>(minWidth, atom->a_text.te_width) * glist_fontwidth(cnv->patch.getPointer());

        auto bounds = Rectangle<int>(x, y, w, getAtomHeight());

        pd->unlockAudioThread();

        return bounds;
    }

    void setPdBounds(Rectangle<int> b)
    {
        libpd_moveobj(cnv->patch.getPointer(), reinterpret_cast<t_gobj*>(atom), b.getX(), b.getY());

        auto fontWidth = glist_fontwidth(cnv->patch.getPointer());
        atom->a_text.te_width = b.getWidth() / fontWidth;
    }

    void checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft)
    {
        auto* patch = cnv->patch.getPointer();

        auto fontWidth = glist_fontwidth(patch);

        // Remove margin
        newBounds = newBounds.reduced(Object::margin);
        oldBounds = oldBounds.reduced(Object::margin);

        // Calculate the width in text characters for both
        auto oldCharWidth = oldBounds.getWidth() / fontWidth;
        auto newCharWidth = std::max(minWidth, newBounds.getWidth() / fontWidth);

        // If we're resizing the left edge, move the object left
        if (resizingOnLeft) {
            auto widthDiff = (newCharWidth - oldCharWidth) * fontWidth;
            auto x = oldBounds.getX() - widthDiff;
            auto y = oldBounds.getY();

            libpd_moveobj(patch, reinterpret_cast<t_gobj*>(atom), x, y);
        }

        // Set new width
        atom->a_text.te_width = newCharWidth;

        auto newHeight = newBounds.getHeight();
        auto heightIdx = std::clamp<int>(std::upper_bound(atomSizes, atomSizes + 7, newHeight) - atomSizes, 2, 7) - 1;
        
        setFontHeight(atomSizes[heightIdx]);
        gui->setParameterExcludingListener(fontSize, heightIdx + 1);
    }

    int getAtomHeight() const
    {
        int idx = static_cast<int>(fontSize.getValue()) - 1;
        if (idx == 0) {
            return cnv->patch.getPointer()->gl_font + 7;
        } else {
            return atomSizes[idx] + 7;
        }
    }

    ObjectParameters getParameters()
    {
        return {
            { "Font size", tCombo, cGeneral, &fontSize, { "auto", "8", "10", "12", "16", "24", "36" } },
            { "Receive Symbol", tString, cGeneral, &receiveSymbol, {} },
            { "Send Symbol", tString, cGeneral, &sendSymbol, {} },
            { "Label", tString, cLabel, &labelText, {} },
            { "Label Position", tCombo, cLabel, &labelPosition, { "left", "right", "top", "bottom" } }
        };
    }

    void valueChanged(Value& v)
    {
        if (v.refersToSameSourceAs(labelPosition)) {
            setLabelPosition(static_cast<int>(labelPosition.getValue()));
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(fontSize)) {
            gui->updateLabel();
            object->updateBounds();
        } else if (v.refersToSameSourceAs(labelText)) {
            setLabelText(labelText.toString());
            gui->updateLabel();
        } else if (v.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        }
    }

    void updateLabel(std::unique_ptr<ObjectLabel>& label)
    {
        int idx = std::clamp<int>(fontSize.getValue(), 1, 7);

        // TODO: fix data race
        setFontHeight(atomSizes[idx - 1]);

        int fontHeight = getAtomHeight() - 6;
        const String text = getExpandedLabelText();

        if (text.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<ObjectLabel>(object);
            }

            auto bounds = getLabelBounds();

            label->setBounds(bounds);
            label->setFont(Font(fontHeight));
            label->setText(text, dontSendNotification);

            label->setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));

            object->cnv->addAndMakeVisible(label.get());
        } else {
            label.reset(nullptr);
        }
    }

    float getFontHeight() const
    {
        return atom->a_fontsize;
    }

    void setFontHeight(float newSize)
    {
        pd->enqueueFunctionAsync([obj = atom, patch = &cnv->patch, newSize](){
            
            if(patch->objectWasDeleted(obj)) return;
            
            obj->a_fontsize = newSize;
        });
        
    }

    Rectangle<int> getLabelBounds() const
    {
        auto objectBounds = object->getBounds().reduced(Object::margin);
        int fontHeight = getAtomHeight() - 6;

        int labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());
        int labelPosition = atom->a_wherelabel;
        auto labelBounds = objectBounds.withSizeKeepingCentre(labelLength, fontHeight);

        if (labelPosition == 0) { // left
            return labelBounds.withRightX(objectBounds.getX() - 4);
        }
        if (labelPosition == 1) { // right
            return labelBounds.withX(objectBounds.getRight() + 4);
        }
        if (labelPosition == 2) { // top
            return labelBounds.withX(objectBounds.getX()).withBottomY(objectBounds.getY());
        }

        return labelBounds.withX(objectBounds.getX()).withY(objectBounds.getBottom());
    }

    String getExpandedLabelText() const
    {
        t_symbol const* sym = canvas_realizedollar(atom->a_glist, atom->a_label);
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
        t_symbol const* sym = atom->a_label;
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
        atom->a_label = pd->generateSymbol(newText);
    }

    void setLabelPosition(int wherelabel)
    {
        atom->a_wherelabel = wherelabel - 1;
    }

    bool hasSendSymbol()
    {
        return atom->a_symto && atom->a_symto != pd->generateSymbol("empty") && atom->a_symto != pd->generateSymbol("");
    }

    bool hasReceiveSymbol()
    {
        return atom->a_symfrom && atom->a_symfrom != pd->generateSymbol("empty") && atom->a_symfrom != pd->generateSymbol("");
    }

    String getSendSymbol()
    {
        return String::fromUTF8(atom->a_symto->s_name);
    }

    String getReceiveSymbol()
    {
        return String::fromUTF8(atom->a_symfrom->s_name);
    }

    void setSendSymbol(String const& symbol) const
    {
        atom->a_symto = pd->generateSymbol(symbol);
        atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (*atom->a_symfrom->s_name)
            pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        atom->a_symfrom = pd->generateSymbol(symbol);
        if (*atom->a_symfrom->s_name)
            pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
    }

    /* prepend "-" as necessary to avoid empty strings, so we can
     use them in Pd messages. */
    t_symbol* gatom_escapit(t_symbol* s)
    {
        if (!*s->s_name)
            return (pd->generateSymbol("-"));
        else if (*s->s_name == '-') {
            char shmo[100];
            shmo[0] = '-';
            strncpy(shmo + 1, s->s_name, 99);
            shmo[99] = 0;
            return (pd->generateSymbol(shmo));
        } else
            return (s);
    }

    /* undo previous operation: strip leading "-" if found.  This is used
     both to restore send, etc., names when loading from a file, and to
     set them from the properties dialog.  In the former case, since before
     version 0.52 '$" was aliases to "#", we also bash any "#" characters
     to "$".  This is unnecessary when reading files saved from 0.52 or later,
     and really we should test for that and only bash when necessary, just
     in case someone wants to have a "#" in a name. */
    t_symbol* gatom_unescapit(t_symbol* s)
    {
        if (*s->s_name == '-')
            return (pd->generateSymbol(String::fromUTF8(s->s_name + 1)));
        else
            return (iemgui_raute2dollar(s));
    }
};
