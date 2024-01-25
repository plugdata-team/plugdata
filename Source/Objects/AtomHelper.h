/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

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

    static inline int const atomSizes[8] = { 0, 8, 10, 12, 16, 24, 36 };

    Object* object;
    ObjectBase* gui;
    Canvas* cnv;
    PluginProcessor* pd;

    pd::WeakReference ptr;
    
    int lastFontHeight = 10;
    hash32 lastLabelTextHash = 0;
    int lastLabelLength = 0;

public:
    Value labelColour = SynchronousValue();
    Value labelPosition = SynchronousValue(0.0f);
    Value fontSize = SynchronousValue(5.0f);
    Value labelText = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();

    ObjectParameters objectParameters;

    AtomHelper(pd::WeakReference pointer, Object* parent, ObjectBase* base)
        : object(parent)
        , gui(base)
        , cnv(parent->cnv)
        , pd(parent->cnv->pd)
        , ptr(pointer)
    {
        objectParameters.addParamCombo("Font height", cDimensions, &fontSize, { "auto", "8", "10", "12", "16", "24", "36" });
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
        objectParameters.addParamString("Label", cLabel, &labelText, "");
        objectParameters.addParamCombo("Label Position", cLabel, &labelPosition, { "left", "right", "top", "bottom" });
    }

    void update()
    {
        labelText = getLabelText();

        if (auto atom = ptr.get<t_fake_gatom>()) {
            labelPosition = static_cast<int>(atom->a_wherelabel + 1);
        }

        int h = getFontHeight();

        int idx = static_cast<int>(std::find(atomSizes, atomSizes + 7, h) - atomSizes);
        fontSize = idx + 1;

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();

        gui->getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(Label::textWhenEditingColourId));
        gui->getLookAndFeel().setColour(Label::textColourId, object->findColour(Label::textColourId));
    }

    int getWidthInChars()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_text.te_width;
        }

        return 0;
    }

    void setWidthInChars(int charWidth)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_text.te_width = charWidth;
        }
    }

    Rectangle<int> getPdBounds(int textLength)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            auto* patchPtr = cnv->patch.getPointer().get();
            if (!patchPtr)
                return {};

            int x, y, w, h;
            pd::Interface::getObjectBounds(patchPtr, atom.cast<t_gobj>(), &x, &y, &w, &h);

            if (atom->a_text.te_width == 0) {
                w = textLength + 10;
            } else {
                w = (atom->a_text.te_width * glist_fontwidth(patchPtr)) + 3;
            }

            return { x, y, w, getAtomHeight() };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            auto* patchPtr = cnv->patch.getPointer().get();
            if (!patchPtr)
                return;

            pd::Interface::moveObject(patchPtr, atom.cast<t_gobj>(), b.getX(), b.getY());

            auto fontWidth = glist_fontwidth(patchPtr);
            if (atom->a_text.te_width != 0) {
                atom->a_text.te_width = (b.getWidth() - 3) / fontWidth;
            }
        }
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer(Object* object)
    {
        class AtomObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            Object* object;
            AtomHelper* helper;

            AtomObjectBoundsConstrainer(Object* parent, AtomHelper* atomHelper)
                : object(parent)
                , helper(atomHelper)
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

                auto oldBounds = old.reduced(Object::margin);
                auto newBounds = bounds.reduced(Object::margin);

                auto* atom = reinterpret_cast<t_fake_gatom*>(object->getPointer());
                auto* patch = object->cnv->patch.getPointer().get();

                if (!atom || !patch)
                    return;

                auto fontWidth = glist_fontwidth(patch);

                // Calculate the width in text characters for both
                auto oldCharWidth = (oldBounds.getWidth() - 3) / fontWidth;
                auto newCharWidth = (newBounds.getWidth() - 3) / fontWidth;

                // If we're resizing the left edge, move the object left
                if (isStretchingLeft) {
                    auto widthDiff = (newCharWidth - oldCharWidth) * fontWidth;
                    auto x = oldBounds.getX() - widthDiff;
                    auto y = oldBounds.getY();

                    if (auto atom = helper->ptr.get<t_gobj>()) {
                        pd::Interface::moveObject(static_cast<t_glist*>(patch), atom.get(), x - object->cnv->canvasOrigin.x, y - object->cnv->canvasOrigin.y);
                    }
                }

                // Set new width
                if (auto atom = helper->ptr.get<t_fake_gatom>()) {
                    atom->a_text.te_width = newCharWidth;
                }

                auto newHeight = newBounds.getHeight();
                auto heightIdx = std::clamp<int>(std::lower_bound(atomSizes, atomSizes + 7, newHeight) - atomSizes, 2, 7) - 1;

                helper->setFontHeight(atomSizes[heightIdx]);
                object->gui->setParameterExcludingListener(helper->fontSize, heightIdx + 1);

                bounds = helper->getPdBounds(0).expanded(Object::margin) + object->cnv->canvasOrigin;
            }
        };

        return std::make_unique<AtomObjectBoundsConstrainer>(object, this);
    }

    int getAtomHeight() const
    {
        int idx = getValue<int>(fontSize) - 1;
        if (idx == 0 && cnv->patch.getPointer()) {
            return cnv->patch.getPointer()->gl_font + 7;
        } else {
            return atomSizes[idx] + 7;
        }
    }

    void addAtomParameters(ObjectParameters& objectParams)
    {
        for (auto const& param : objectParameters.getParameters())
            objectParams.addParam(param);
    }

    void valueChanged(Value& v)
    {
        if (v.refersToSameSourceAs(labelPosition)) {
            setLabelPosition(getValue<int>(labelPosition));
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

    float getMinimum()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_draglo;
        }

        return 0.0f;
    }

    float getMaximum()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_draghi;
        }

        return 0.0f;
    }

    void setMinimum(float value)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_draglo = value;
        }
    }
    void setMaximum(float value)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_draghi = value;
        }
    }

    void updateLabel(std::unique_ptr<ObjectLabel>& label)
    {
        int idx = std::clamp<int>(fontSize.getValue(), 1, 7);

        setFontHeight(atomSizes[idx - 1]);

        int fontHeight = getAtomHeight() - 6;
        String const text = getExpandedLabelText();

        if (text.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<ObjectLabel>();
            }

            auto bounds = getLabelBounds();

            label->setBounds(bounds);
            label->setFont(Font(fontHeight));
            label->setText(text, dontSendNotification);

            auto textColour = object->findColour(PlugDataColour::canvasTextColourId);
            if (std::abs(textColour.getBrightness() - object->findColour(PlugDataColour::canvasBackgroundColourId).getBrightness()) < 0.3f) {
                textColour = object->findColour(PlugDataColour::canvasBackgroundColourId).contrasting();
            }

            label->setColour(Label::textColourId, textColour);

            object->cnv->addAndMakeVisible(label.get());
        } else {
            label.reset(nullptr);
        }
    }

    float getFontHeight() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_fontsize;
        }

        return 0;
    }

    void setFontHeight(float newSize)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_fontsize = newSize;
        }
    }

    Rectangle<int> getLabelBounds() const
    {
        auto objectBounds = object->getBounds().reduced(Object::margin);
        int fontHeight = getAtomHeight() - 6;

        auto currentHash = hash(getExpandedLabelText());
        int labelLength = lastLabelLength;
        if(lastFontHeight != fontHeight || lastLabelTextHash != currentHash)
        {
            labelLength = Font(fontHeight).getStringWidth(getExpandedLabelText());
            lastFontHeight = fontHeight;
            lastLabelTextHash = currentHash;
            lastLabelLength = labelLength;
        }
        
        int labelPosition = 0;
        if (auto atom = ptr.get<t_fake_gatom>()) {
            labelPosition = atom->a_wherelabel;
        }
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
        if (auto atom = ptr.get<t_fake_gatom>()) {
            t_symbol* const sym = canvas_realizedollar(atom->a_glist, atom->a_label);

            if (sym) {
                auto text = String::fromUTF8(sym->s_name);
                if (text.isNotEmpty() && text != "empty") {
                    return text;
                }
            }
        }

        return "";
    }

    String getLabelText() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            t_symbol const* sym = atom->a_label;
            if (sym) {
                auto const text = String::fromUTF8(sym->s_name);
                if (text.isNotEmpty() && text != "empty") {
                    return text;
                }
            }
        }

        return "";
    }

    void setLabelText(String const& newText)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_label = pd->generateSymbol(newText);
        }
    }

    void setLabelPosition(int wherelabel)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_wherelabel = wherelabel - 1;
        }
    }

    bool hasSendSymbol()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_symto && atom->a_symto != pd->generateSymbol("empty") && atom->a_symto != pd->generateSymbol("");
        }

        return false;
    }

    bool hasReceiveSymbol()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_symfrom && atom->a_symfrom != pd->generateSymbol("empty") && atom->a_symfrom != pd->generateSymbol("");
        }

        return false;
    }

    String getSendSymbol()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return String::fromUTF8(atom->a_symto->s_name);
        }

        return {};
    }

    String getReceiveSymbol()
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return String::fromUTF8(atom->a_symfrom->s_name);
        }

        return {};
    }

    void setSendSymbol(String const& symbol) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {

            atom->a_symto = pd->generateSymbol(symbol);
            atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
        }
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            if (*atom->a_symfrom->s_name)
                pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
            atom->a_symfrom = pd->generateSymbol(symbol);
            if (*atom->a_symfrom->s_name)
                pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        }
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
