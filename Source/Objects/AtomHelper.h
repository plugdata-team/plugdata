/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

static t_atom* fake_gatom_getatom(t_fake_gatom const* x)
{
    int const ac = binbuf_getnatom(x->a_text.te_binbuf);
    t_atom const* av = binbuf_getvec(x->a_text.te_binbuf);
    if (x->a_flavor == A_FLOAT && (ac != 1 || av[0].a_type != A_FLOAT)) {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "f", 0.);
    } else if (x->a_flavor == A_SYMBOL && (ac != 1 || av[0].a_type != A_SYMBOL)) {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "s", gensym("symbol"));
    }
    return binbuf_getvec(x->a_text.te_binbuf);
}

class AtomHelper {

    static inline StackArray<int, 8> const atomSizes = { 0, 8, 10, 12, 16, 24, 36 };

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
    Value fontSize = SynchronousValue(5.0f);
    Value labelText = SynchronousValue();
    Value labelPosition = SynchronousValue(0.0f);
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
        objectParameters.addParamString("Text", cLabel, &labelText, "");
        objectParameters.addParamCombo("Position", cLabel, &labelPosition, { "left", "right", "top", "bottom" });
    }

    void update()
    {
        labelText = getLabelText();

        if (auto atom = ptr.get<t_fake_gatom>()) {
            labelPosition = static_cast<int>(atom->a_wherelabel + 1);
        }
        int const idx = atomSizes.index_of(getFontHeight());
        fontSize = idx + 1;

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();
    }

    int getWidthInChars() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_text.te_width;
        }

        return 0;
    }

    void setWidthInChars(int const charWidth) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_text.te_width = charWidth;
        }
    }

    Rectangle<int> getPdBounds(int const textLength) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            auto* patchPtr = cnv->patch.getRawPointer();

            int x, y, w, h;
            pd::Interface::getObjectBounds(patchPtr, atom.cast<t_gobj>(), &x, &y, &w, &h);

            if (atom->a_text.te_width == 0) {
                w = textLength + 10;
            } else {
                w = atom->a_text.te_width * sys_fontwidth(getFontHeight()) + 3;
            }

            return { x, y, w, getAtomHeight() };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            auto* patchPtr = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patchPtr, atom.cast<t_gobj>(), b.getX(), b.getY());

            auto const fontWidth = sys_fontwidth(getFontHeight());
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
                bool const isStretchingTop,
                bool const isStretchingLeft,
                bool const isStretchingBottom,
                bool const isStretchingRight) override
            {

                auto const oldBounds = old.reduced(Object::margin);
                auto const newBounds = bounds.reduced(Object::margin);

                auto const fontWidth = sys_fontwidth(helper->getFontHeight());

                // Calculate the width in text characters for both
                auto const newCharWidth = roundToInt((newBounds.getWidth() - 3) / static_cast<float>(fontWidth));

                // Set new width
                if (auto atom = helper->ptr.get<t_fake_gatom>()) {
                    atom->a_text.te_width = newCharWidth;
                }

                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;

                auto const newHeight = newBounds.getHeight();
                auto const heightIdx = std::clamp<int>(std::ranges::lower_bound(atomSizes, newHeight) - atomSizes.begin(), 2, 7) - 1;

                helper->setFontHeight(atomSizes[heightIdx]);
                object->gui->setParameterExcludingListener(helper->fontSize, heightIdx + 1);

                if (isStretchingTop || isStretchingLeft) {
                    auto const x = oldBounds.getRight() - (bounds.getWidth() - Object::doubleMargin);
                    auto const y = oldBounds.getBottom() - (bounds.getHeight() - Object::doubleMargin);

                    if (auto atom = helper->ptr.get<t_gobj>()) {
                        auto* patch = object->cnv->patch.getRawPointer();

                        pd::Interface::moveObject(patch, atom.get(), x - object->cnv->canvasOrigin.x, y - object->cnv->canvasOrigin.y);
                    }
                    bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
                }
            }
        };

        return std::make_unique<AtomObjectBoundsConstrainer>(object, this);
    }

    int getAtomHeight() const
    {
        int const idx = getValue<int>(fontSize) - 1;
        if (idx == 0 && cnv->patch.getPointer()) {
            return cnv->patch.getPointer()->gl_font + 7;
        }
        return atomSizes[idx] + 7;
    }

    void addAtomParameters(ObjectParameters& objectParams)
    {
        for (auto const& param : objectParameters.getParameters())
            objectParams.addParam(param);
    }

    void valueChanged(Value const& v)
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

    float getMinimum() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_draglo;
        }

        return 0.0f;
    }

    float getMaximum() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_draghi;
        }

        return 0.0f;
    }

    void setMinimum(float const value)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_draglo = value;
        }
    }
    void setMaximum(float const value)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_draghi = value;
        }
    }

    void updateLabel(OwnedArray<ObjectLabel>& labels)
    {
        int const idx = std::clamp<int>(fontSize.getValue(), 1, 7);

        setFontHeight(atomSizes[idx - 1]);

        int const fontHeight = getAtomHeight() - 5;
        String const text = getExpandedLabelText();

        if (text.isNotEmpty()) {
            ObjectLabel* label;
            if (labels.isEmpty()) {
                label = labels.add(new ObjectLabel());
            } else {
                label = labels[0];
            }

            auto const bounds = getLabelBounds();

            label->setBounds(bounds);
            label->setFont(Font(FontOptions(fontHeight)));
            label->setText(text, dontSendNotification);

            auto textColour = PlugDataColours::canvasTextColour;
            if (std::abs(textColour.getBrightness() - PlugDataColours::canvasBackgroundColour.getBrightness()) < 0.3f) {
                textColour = PlugDataColours::canvasBackgroundColour.contrasting();
            }

            label->setColour(Label::textColourId, textColour);
            object->cnv->addAndMakeVisible(label);
        } else {
            labels.clear();
        }
    }

    int getFontHeight() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_fontsize;
        }

        return 0;
    }

    void setFontHeight(float const newSize)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_fontsize = newSize;
        }
    }

    Rectangle<int> getLabelBounds()
    {
        auto const objectBounds = object->getBounds().reduced(Object::margin);
        int const fontHeight = getAtomHeight() - 5;
        int const fontWidth = sys_fontwidth(fontHeight);
        int const labelSpace = fontWidth * (getExpandedLabelText().length() + 1);

        auto const currentHash = hash(getExpandedLabelText());
        int labelLength = lastLabelLength;

        if (lastFontHeight != fontHeight || lastLabelTextHash != currentHash) {
            labelLength = Fonts::getStringWidthInt(getExpandedLabelText(), fontHeight);
            lastFontHeight = fontHeight;
            lastLabelTextHash = currentHash;
            lastLabelLength = labelLength;
        }

        int labelPosition = 0;
        if (auto atom = ptr.get<t_fake_gatom>()) {
            labelPosition = atom->a_wherelabel;
        }
        auto labelBounds = objectBounds.withSizeKeepingCentre(labelLength, fontHeight);
        int const lengthDifference = labelLength - labelSpace; // difference between width in pd-vanilla and plugdata

        if (labelPosition == 0) { // left
            labelBounds.removeFromLeft(lengthDifference);
            return labelBounds.withRightX(objectBounds.getX() - lengthDifference - 2);
        }

        labelBounds.removeFromRight(lengthDifference);

        if (labelPosition == 1) { // right
            return labelBounds.withX(objectBounds.getRight() + 2);
        }

        if (labelPosition == 2) { // top
            return labelBounds.withX(objectBounds.getX()).withBottomY(objectBounds.getY() - 2);
        }

        return labelBounds.withX(objectBounds.getX()).withY(objectBounds.getBottom() + 2);
    }

    String getExpandedLabelText() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {

            if (t_symbol const* sym = canvas_realizedollar(atom->a_glist, atom->a_label)) {
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
            if (t_symbol const* sym = atom->a_label) {
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

    void setLabelPosition(int const wherelabel)
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            atom->a_wherelabel = wherelabel - 1;
        }
    }

    bool hasSendSymbol() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_symto && atom->a_symto != pd->generateSymbol("empty") && atom->a_symto != pd->generateSymbol("");
        }

        return false;
    }

    bool hasReceiveSymbol() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return atom->a_symfrom && atom->a_symfrom != pd->generateSymbol("empty") && atom->a_symfrom != pd->generateSymbol("");
        }

        return false;
    }

    String getSendSymbol() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return String::fromUTF8(atom->a_symto->s_name);
        }

        return {};
    }

    String getReceiveSymbol() const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            return String::fromUTF8(atom->a_symfrom->s_name);
        }

        return {};
    }

    void setSendSymbol(String const& symbol) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            if (symbol.isEmpty() && *atom->a_symto->s_name) {
                outlet_new(&atom->a_text, nullptr);
                cnv->performSynchronise();
            } else if (!symbol.isEmpty() && !*atom->a_symto->s_name && atom->a_text.te_outlet) {
                canvas_deletelinesforio(atom->a_glist, &atom->a_text,
                    nullptr, atom->a_text.te_outlet);
                outlet_free(atom->a_text.te_outlet);
                cnv->performSynchronise();
            }

            atom->a_symto = pd->generateSymbol(symbol);
            atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
        }
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (auto atom = ptr.get<t_fake_gatom>()) {
            if (symbol.isEmpty() && *atom->a_symfrom->s_name) {
                inlet_new(&atom->a_text, &atom->a_text.te_pd, nullptr, nullptr);
                cnv->performSynchronise();
            } else if (!symbol.isEmpty() && !*atom->a_symfrom->s_name && atom->a_text.te_inlet) {
                canvas_deletelinesforio(atom->a_glist, &atom->a_text,
                    atom->a_text.te_inlet, nullptr);
                inlet_free(atom->a_text.te_inlet);
                cnv->performSynchronise();
            }

            if (*atom->a_symfrom->s_name)
                pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
            atom->a_symfrom = pd->generateSymbol(symbol);
            if (*atom->a_symfrom->s_name)
                pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        }
    }
};
