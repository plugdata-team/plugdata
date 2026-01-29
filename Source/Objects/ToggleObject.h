/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ToggleObject final : public ObjectBase {
    bool toggleState = false;
    bool alreadyToggled = false;
    Value nonZero = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    float value = 0.0f;

    IEMHelper iemHelper;

public:
    ToggleObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        objectParameters.addParamFloat("Non-zero value", cGeneral, &nonZero, 1.0f);
        objectParameters.addParamSize(&sizeProperty, true);

        iemHelper.addIemParameters(objectParameters, true, true, 17, 7);
    }

    void onConstrainerCreate() override
    {
        constrainer->setFixedAspectRatio(1);
    }
    
    ResizeDirection getAllowedResizeDirections() const override
    {
        return DiagonalOnly;
    }

    bool inletIsSymbol() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(labels);
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        iemHelper.setPdBounds(b);
    }

    void update() override
    {
        if (auto toggle = ptr.get<t_toggle>()) {
            sizeProperty = toggle->x_gui.x_w;
            nonZero = toggle->x_nonzero;
        }

        iemHelper.update();

        value = getValue();
        setToggleStateFromFloat(value);
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();

        auto const bgColour = ::getValue<Colour>(iemHelper.secondaryColour);

        auto const backgroundColour = convertColour(bgColour);
        auto const toggledColour = convertColour(::getValue<Colour>(iemHelper.primaryColour));
        auto const untoggledColour = convertColour(::getValue<Colour>(iemHelper.primaryColour).interpolatedWith(::getValue<Colour>(iemHelper.secondaryColour), 0.8f));
        auto const selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto const outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        auto const sizeReduction = std::min(1.0f, getWidth() / 20.0f);
        float const margin = (getWidth() * 0.08f + 4.5f) * sizeReduction;
        auto const crossBounds = getLocalBounds().toFloat().reduced(margin);

        auto const max = std::max(crossBounds.getWidth(), crossBounds.getHeight());
        auto const strokeWidth = std::max(max * 0.15f, 2.0f) * sizeReduction;

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, crossBounds.getX(), crossBounds.getY());
        nvgLineTo(nvg, crossBounds.getRight(), crossBounds.getBottom());
        nvgMoveTo(nvg, crossBounds.getRight(), crossBounds.getY());
        nvgLineTo(nvg, crossBounds.getX(), crossBounds.getBottom());
        nvgStrokeColor(nvg, toggleState ? toggledColour : untoggledColour);
        nvgStrokeWidth(nvg, strokeWidth);
        nvgStroke(nvg);
    }

    void toggleObject(Point<int> position) override
    {
        ignoreUnused(position);

        if (!alreadyToggled) {
            startEdition();
            auto const newValue = value != 0 ? 0 : ::getValue<float>(nonZero);
            sendToggleValue(newValue);
            setToggleStateFromFloat(newValue);
            stopEdition();
            alreadyToggled = true;
        }
    }

    void sendToggleValue(float const newValue) const
    {
        if (auto iem = ptr.get<t_iemgui>()) {
            t_atom atom;
            SETFLOAT(&atom, newValue);
            pd_typedmess(iem.cast<t_pd>(), pd->generateSymbol("set"), 1, &atom);

            outlet_float(iem->x_obj.ob_outlet, newValue);
            if (iem->x_fsf.x_snd_able && iem->x_snd->s_thing)
                pd_float(iem->x_snd->s_thing, newValue);
        }
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        startEdition();
        auto const newValue = value != 0 ? 0 : ::getValue<float>(nonZero);
        sendToggleValue(newValue);
        setToggleStateFromFloat(newValue);
        stopEdition();

        // Make sure we don't re-toggle with an accidental drag
        alreadyToggled = true;
    }

    void setToggleStateFromFloat(float const newValue)
    {
        value = newValue;
        toggleState = std::abs(newValue) > std::numeric_limits<float>::epsilon();
        repaint();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("bang"): {
            value = !value;
            setToggleStateFromFloat(value);
            break;
        }
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            if (atoms.size() >= 1) {
                value = atoms[0].getFloat();
                setToggleStateFromFloat(value);
            }
            break;
        }
        case hash("nonzero"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(nonZero, atoms[0].getFloat());
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, var(iem->x_w));
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const size = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);

            if (auto tgl = ptr.get<t_toggle>()) {
                tgl->x_gui.x_w = size;
                tgl->x_gui.x_h = size;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(nonZero)) {
            float const val = nonZero.getValue();
            if (auto toggle = ptr.get<t_toggle>()) {
                toggle->x_nonzero = val;
            }
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getValue() const
    {
        if (auto toggle = ptr.get<t_toggle>())
            return toggle->x_on;

        return 0.0f;
    }
};
