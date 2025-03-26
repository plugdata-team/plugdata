/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ScopeObject final : public ObjectBase
    , public Timer {

    HeapArray<float> x_buffer;
    HeapArray<float> y_buffer;

    Value gridColour = SynchronousValue();
    Value triggerMode = SynchronousValue();
    Value triggerValue = SynchronousValue();
    Value samplesPerPoint = SynchronousValue();
    Value bufferSize = SynchronousValue();
    Value delay = SynchronousValue();
    Value signalRange = SynchronousValue();
    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    bool freezeScope = false;

public:
    ScopeObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColour("Grid", cAppearance, &gridColour, PlugDataColour::guiObjectInternalOutlineColour);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamCombo("Trigger mode", cGeneral, &triggerMode, { "None", "Up", "Down" }, 1);
        objectParameters.addParamFloat("Trigger value", cGeneral, &triggerValue, 0.0f);
        objectParameters.addParamInt("Samples per point", cGeneral, &samplesPerPoint, 256, true, 2, 8192);
        objectParameters.addParamInt("Buffer size", cGeneral, &bufferSize, 128, true, 0, SCOPE_MAXBUFSIZE * 4);
        objectParameters.addParamInt("Delay", cGeneral, &delay, 0, true, 0);
        objectParameters.addParamRange("Signal Range", cGeneral, &signalRange, VarArray { var(-1.0f), var(1.0f) });

        objectParameters.addParamReceiveSymbol(&receiveSymbol);

        startTimerHz(25);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto scope = ptr.get<t_fake_scope>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(scope->x_width), var(scope->x_height) });
        }
    }

    void update() override
    {
        if (auto scope = ptr.get<t_fake_scope>()) {
            triggerMode = scope->x_trigmode + 1;
            triggerValue = scope->x_triglevel;
            bufferSize = scope->x_bufsize;
            delay = scope->x_delay;
            samplesPerPoint = scope->x_period;
            secondaryColour = colourFromHexArray(scope->x_bg).toString();
            primaryColour = colourFromHexArray(scope->x_fg).toString();
            gridColour = colourFromHexArray(scope->x_gg).toString();
            sizeProperty = VarArray { var(scope->x_width), var(scope->x_height) };

            auto rcvSym = scope->x_rcv_set ? String::fromUTF8(scope->x_rcv_raw->s_name) : getBinbufSymbol(22);
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            VarArray const arr = { scope->x_min, scope->x_max };
            signalRange = var(arr);
        }
    }

    static Colour colourFromHexArray(unsigned char* hex)
    {
        return { hex[0], hex[1], hex[2] };
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto scope = ptr.get<t_fake_scope>()) {
            auto* patch = cnv->patch.getRawPointer();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, scope.cast<t_gobj>(), &x, &y, &w, &h);

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto scope = ptr.get<t_fake_scope>()) {
            auto* patch = cnv->patch.getRawPointer();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, scope.cast<t_gobj>(), b.getX(), b.getY());

            scope->x_width = getWidth() - 1;
            scope->x_height = getHeight() - 1;
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();

        auto const outlineColour = object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(Colour::fromString(secondaryColour.toString())), outlineColour, Corners::objectCornerRadius);

        auto const dx = getWidth() * 0.125f;
        auto const dy = getHeight() * 0.25f;

        nvgBeginPath(nvg);
        nvgStrokeColor(nvg, convertColour(Colour::fromString(gridColour.toString())));
        nvgStrokeWidth(nvg, 1.0f);

        auto xx = dx;
        for (int i = 0; i < 7; i++) {
            nvgMoveTo(nvg, xx, 1.0f);
            nvgLineTo(nvg, xx, getHeight() - 1.0f);
            xx += dx;
        }

        auto yy = dy;
        for (int i = 0; i < 3; i++) {
            nvgMoveTo(nvg, 1.0f, yy);
            nvgLineTo(nvg, getWidth() - 1.0f, yy);
            yy += dy;
        }

        nvgStroke(nvg);

        NVGScopedState scopedState(nvg);
        nvgIntersectScissor(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
        if (!(y_buffer.empty() || x_buffer.empty())) {
            nvgBeginPath(nvg);
            nvgStrokeColor(nvg, convertColour(Colour::fromString(primaryColour.toString())));
            nvgStrokeWidth(nvg, 2.0f);
            nvgLineJoin(nvg, NVG_ROUND);
            nvgLineCap(nvg, NVG_ROUND);

            constexpr float offset = 2.0f;

            float const w = getWidth() - 4;
            float const h = getHeight() - 4;

            nvgMoveTo(nvg, x_buffer[0] * w + offset, y_buffer[0] * h + offset);

            for (size_t i = 1; i < y_buffer.size(); i++) {
                nvgLineTo(nvg, x_buffer[i] * w + offset, y_buffer[i] * h + offset);
            }
            nvgStroke(nvg);
        }
    }

    void timerCallback() override
    {
        if (freezeScope)
            return;

        int mode = 0;
        int bufsize = 0;
        float min = 0.0f;
        float max = 1.0f;

        if (object->iolets.size() == 3)
            object->iolets[2]->setVisible(false);

        if (auto scope = ptr.get<t_fake_scope>()) {
            bufsize = scope->x_bufsize;
            min = scope->x_min;
            max = scope->x_max;
            mode = scope->x_xymode;

            if (x_buffer.size() != bufsize) {
                x_buffer.resize(bufsize);
                y_buffer.resize(bufsize);
            }

            std::copy_n(scope->x_xbuflast, bufsize, x_buffer.data());
            std::copy_n(scope->x_ybuflast, bufsize, y_buffer.data());
        }

        // Normalise the buffers
        if (min > max) {
            std::swap(min, max);
        }

        float const dx = 1.0f / static_cast<float>(bufsize); // Normalized step size

        float const range = max - min;
        float const scale = 1.0f / range;

        switch (mode) {
        case 1: {
            for (int n = 0; n < bufsize; n++) {
                y_buffer[n] = 1.0f - (x_buffer[n] - min) * scale;
                x_buffer[n] = dx * n;
            }
        } break;
        case 2: {
            for (int n = 0; n < bufsize; n++) {
                x_buffer[n] = (y_buffer[n] - min) * scale;
                y_buffer[n] = 1.0f - dx * n;
            }
        } break;
        case 3: {
            for (int n = 0; n < bufsize; n++) {
                x_buffer[n] = (x_buffer[n] - min) * scale;
                y_buffer[n] = 1.0f - (y_buffer[n] - min) * scale;
            }
        } break;
        default:
            break;
        }

        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        freezeScope = true;
    }

    void mouseUp(MouseEvent const& e) override
    {
        freezeScope = false;
    }

    void propertyChanged(Value& v) override
    {

        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto scope = ptr.get<t_fake_scope>()) {
                scope->x_width = width;
                scope->x_height = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(primaryColour)) {
            if (auto scope = ptr.get<t_fake_scope>())
                colourToHexArray(Colour::fromString(primaryColour.toString()), scope->x_fg);
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            if (auto scope = ptr.get<t_fake_scope>())
                colourToHexArray(Colour::fromString(secondaryColour.toString()), scope->x_bg);
        } else if (v.refersToSameSourceAs(gridColour)) {
            if (auto scope = ptr.get<t_fake_scope>())
                colourToHexArray(Colour::fromString(gridColour.toString()), scope->x_gg);
        } else if (v.refersToSameSourceAs(bufferSize)) {
            bufferSize = std::clamp<int>(getValue<int>(bufferSize), 0, SCOPE_MAXBUFSIZE * 4);

            if (auto scope = ptr.get<t_fake_scope>()) {
                scope->x_bufsize = bufferSize.getValue();
                scope->x_bufphase = 0;
            }

        } else if (v.refersToSameSourceAs(samplesPerPoint)) {
            if (auto scope = ptr.get<t_fake_scope>()) {
                scope->x_period = getValue<int>(v);
            }
        } else if (v.refersToSameSourceAs(signalRange)) {
            auto const min = static_cast<float>(signalRange.getValue().getArray()->getReference(0));
            auto const max = static_cast<float>(signalRange.getValue().getArray()->getReference(1));
            if (auto scope = ptr.get<t_fake_scope>()) {
                scope->x_min = min;
                scope->x_max = max;
            }
        } else if (v.refersToSameSourceAs(delay)) {
            if (auto scope = ptr.get<t_fake_scope>())
                scope->x_delay = getValue<int>(delay);
        } else if (v.refersToSameSourceAs(triggerMode)) {
            if (auto scope = ptr.get<t_fake_scope>())
                scope->x_trigmode = getValue<int>(triggerMode) - 1;
        } else if (v.refersToSameSourceAs(triggerValue)) {
            if (auto scope = ptr.get<t_fake_scope>())
                scope->x_triglevel = getValue<int>(triggerValue);
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            auto const symbol = receiveSymbol.toString();
            if (auto scope = ptr.get<void>())
                pd->sendDirectMessage(scope.get(), "receive", { pd->generateSymbol(symbol) });
        }
    }

    bool inletIsSymbol() override
    {
        auto const rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && rSymbol != "empty";
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("fgcolor"): {
            if (atoms.size() == 3)
                setParameterExcludingListener(primaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        case hash("bgcolor"): {
            if (atoms.size() == 3)
                setParameterExcludingListener(secondaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        case hash("gridcolor"): {
            if (atoms.size() == 3)
                setParameterExcludingListener(gridColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        default:
            break;
        }
    }
};
