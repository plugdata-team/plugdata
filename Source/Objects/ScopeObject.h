/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

template<typename S>
class ScopeBase : public ObjectBase
    , public Timer {

    std::vector<float> x_buffer;
    std::vector<float> y_buffer;

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

public:
    ScopeBase(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColour("Grid color", cAppearance, &gridColour, PlugDataColour::guiObjectInternalOutlineColour);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamCombo("Trigger mode", cGeneral, &triggerMode, { "None", "Up", "Down" }, 1);
        objectParameters.addParamFloat("Trigger value", cGeneral, &triggerValue, 0.0f);
        objectParameters.addParamInt("Samples per point", cGeneral, &samplesPerPoint, 256);
        objectParameters.addParamInt("Buffer size", cGeneral, &bufferSize, 128);
        objectParameters.addParamInt("Delay", cGeneral, &delay, 0);
        objectParameters.addParamRange("Signal Range", cGeneral, &signalRange, Array<var> { var(-1.0f), var(1.0f) });

        objectParameters.addParamReceiveSymbol(&receiveSymbol);

        startTimerHz(25);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto scope = ptr.get<S>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(scope->x_width), var(scope->x_height) });
        }
    }

    void update() override
    {
        if (auto scope = ptr.get<S>()) {
            triggerMode = scope->x_trigmode + 1;
            triggerValue = scope->x_triglevel;
            bufferSize = scope->x_bufsize;
            delay = scope->x_delay;
            samplesPerPoint = scope->x_period;
            secondaryColour = colourFromHexArray(scope->x_bg).toString();
            primaryColour = colourFromHexArray(scope->x_fg).toString();
            gridColour = colourFromHexArray(scope->x_gg).toString();
            sizeProperty = Array<var> { var(scope->x_width), var(scope->x_height) };

            auto rcvSym = scope->x_rcv_set ? String::fromUTF8(scope->x_rcv_raw->s_name) : getBinbufSymbol(22);
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            Array<var> arr = { scope->x_min, scope->x_max };
            signalRange = var(arr);
        }
    }

    Colour colourFromHexArray(unsigned char* hex)
    {
        return { hex[0], hex[1], hex[2] };
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto scope = ptr.get<S>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, scope.template cast<t_gobj>(), &x, &y, &w, &h);

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto scope = ptr.get<S>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, scope.template cast<t_gobj>(), b.getX(), b.getY());

            scope->x_width = getWidth() - 1;
            scope->x_height = getHeight() - 1;
        }
    }

    void resized() override
    {

    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        nvgFillColor(nvg, convertColour(Colour::fromString(secondaryColour.toString())));
        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
        nvgFill(nvg);

        auto dx = getWidth() * 0.125f;
        auto dy = getHeight() * 0.25f;



        nvgBeginPath(nvg);
        nvgStrokeColor(nvg, convertColour(Colour::fromString(gridColour.toString())));
        nvgStrokeWidth(nvg, 1.0f);
        auto xx = dx;
        for (int i = 0; i < 7; i++) {
            nvgMoveTo(nvg, xx, 0.0f);
            nvgLineTo(nvg, xx, static_cast<float>(getHeight()));
            xx += dx;
        }

        auto yy = dy;
        for (int i = 0; i < 3; i++) {
            nvgMoveTo(nvg, 0.0f, yy);
            nvgLineTo(nvg, static_cast<float>(getWidth()), yy);
            yy += dy;
        }
        nvgStroke(nvg);

        nvgSave(nvg);
        nvgScissor(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
        if (!(y_buffer.empty() || x_buffer.empty())) {
            nvgBeginPath(nvg);
            nvgStrokeColor(nvg, convertColour(Colour::fromString(primaryColour.toString())));
            nvgStrokeWidth(nvg, 2.0f);
            nvgLineJoin(nvg, NVG_ROUND);
            nvgLineCap(nvg, NVG_ROUND);

            nvgMoveTo(nvg, x_buffer[1], y_buffer[1]);

            for (size_t i = 2; i < y_buffer.size(); i++) {
                nvgLineTo(nvg, x_buffer[i], y_buffer[i]);
            }
            nvgStroke(nvg);
        }
        nvgRestore(nvg);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        auto dx = getWidth() * 0.125f;
        auto dy = getHeight() * 0.25f;

        g.setColour(Colour::fromString(gridColour.toString()));

        auto xx = dx;
        for (int i = 0; i < 7; i++) {
            g.drawLine(xx, 0.0f, xx, static_cast<float>(getHeight()));
            xx += dx;
        }

        auto yy = dy;
        for (int i = 0; i < 3; i++) {
            g.drawLine(0.0f, yy, static_cast<float>(getWidth()), yy);
            yy += dy;
        }

        // skip drawing waveform if buffer is empty
        if (!(y_buffer.empty() || x_buffer.empty())) {
            Point<float> lastPoint = Point<float>(x_buffer[0], y_buffer[0]);
            Point<float> newPoint;

            g.setColour(Colour::fromString(primaryColour.toString()));

            Path p;
            for (size_t i = 1; i < y_buffer.size(); i++) {
                newPoint = Point<float>(x_buffer[i], y_buffer[i]);
                Line segment(lastPoint, newPoint);
                p.addLineSegment(segment, 1.0f);
                lastPoint = newPoint;
            }
            g.fillPath(p);
        }

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void timerCallback() override
    {
        int bufsize = 0, mode = 0;
        float min = 0.0f, max = 1.0f;

        if (object->iolets.size() == 3)
            object->iolets[2]->setVisible(false);

        if (auto scope = ptr.get<S>()) {
            bufsize = scope->x_bufsize;
            min = scope->x_min;
            max = scope->x_max;
            mode = scope->x_xymode;

            if (x_buffer.size() != bufsize) {
                x_buffer.resize(bufsize);
                y_buffer.resize(bufsize);
            }

            std::copy(scope->x_xbuflast, scope->x_xbuflast + bufsize, x_buffer.data());
            std::copy(scope->x_ybuflast, scope->x_ybuflast + bufsize, y_buffer.data());
        }

        if (min > max) {
            auto temp = max;
            max = min;
            min = temp;
        }

        float oldx = 0, oldy = 0;
        float dx = (getWidth() - 2) / (float)bufsize;
        float dy = (getHeight() - 2) / (float)bufsize;

        float waveAreaHeight = getHeight() - 2;
        float waveAreaWidth = getWidth() - 2;

        for (int n = 0; n < bufsize; n++) {
            switch (mode) {
            case 1:
                y_buffer[n] = jmap<float>(x_buffer[n], min, max, waveAreaHeight, 2.f);
                x_buffer[n] = oldx;
                oldx += dx;
                break;
            case 2:
                x_buffer[n] = jmap<float>(y_buffer[n], min, max, 2.f, waveAreaWidth);
                y_buffer[n] = oldy;
                oldy += dy;
                break;
            case 3:
                x_buffer[n] = jmap<float>(x_buffer[n], min, max, 2.f, waveAreaWidth);
                y_buffer[n] = jmap<float>(y_buffer[n], min, max, waveAreaHeight, 2.f);
                break;
            default:
                break;
            }
        }

        repaint();
    }

    void valueChanged(Value& v) override
    {

        if (v.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto scope = ptr.get<S>()) {
                scope->x_width = width;
                scope->x_height = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(primaryColour)) {
            if (auto scope = ptr.get<S>())
                colourToHexArray(Colour::fromString(primaryColour.toString()), scope->x_fg);
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            if (auto scope = ptr.get<S>())
                colourToHexArray(Colour::fromString(secondaryColour.toString()), scope->x_bg);
        } else if (v.refersToSameSourceAs(gridColour)) {
            if (auto scope = ptr.get<S>())
                colourToHexArray(Colour::fromString(gridColour.toString()), scope->x_gg);
        } else if (v.refersToSameSourceAs(bufferSize)) {
            bufferSize = std::clamp<int>(getValue<int>(bufferSize), 0, SCOPE_MAXBUFSIZE * 4);

            if (auto scope = ptr.get<S>()) {
                scope->x_bufsize = bufferSize.getValue();
                scope->x_bufphase = 0;
            }

        } else if (v.refersToSameSourceAs(samplesPerPoint)) {
            if (auto scope = ptr.get<S>()) {
                scope->x_period = limitValueMin(v, 0);
            }
        } else if (v.refersToSameSourceAs(signalRange)) {
            auto min = static_cast<float>(signalRange.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(signalRange.getValue().getArray()->getReference(1));
            if (auto scope = ptr.get<S>()) {
                scope->x_min = min;
                scope->x_max = max;
            }
        } else if (v.refersToSameSourceAs(delay)) {
            if (auto scope = ptr.get<S>())
                scope->x_delay = getValue<int>(delay);
        } else if (v.refersToSameSourceAs(triggerMode)) {
            if (auto scope = ptr.get<S>())
                scope->x_trigmode = getValue<int>(triggerMode) - 1;
        } else if (v.refersToSameSourceAs(triggerValue)) {
            if (auto scope = ptr.get<S>())
                scope->x_triglevel = getValue<int>(triggerValue);
        } else if (v.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            if (auto scope = ptr.get<void>())
                pd->sendDirectMessage(scope.get(), "receive", { pd->generateSymbol(symbol) });
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("receive"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("fgcolor"): {
            if (numAtoms == 3)
                setParameterExcludingListener(primaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        case hash("bgcolor"): {
            if (numAtoms == 3)
                setParameterExcludingListener(secondaryColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        case hash("gridcolor"): {
            if (numAtoms == 3)
                setParameterExcludingListener(gridColour, Colour(atoms[0].getFloat(), atoms[1].getFloat(), atoms[2].getFloat()).toString());
            break;
        }
        default:
            break;
        }
    }
};

// Hilarious use of templates to support both cyclone/scope and else/oscope in the same code
class ScopeObject final : public ScopeBase<t_fake_scope> {
public:
    ScopeObject(pd::WeakReference ptr, Object* object)
        : ScopeBase<t_fake_scope>(ptr, object)
    {
    }
};

class OscopeObject final : public ScopeBase<t_fake_oscope> {
public:
    OscopeObject(pd::WeakReference ptr, Object* object)
        : ScopeBase<t_fake_oscope>(ptr, object)
    {
    }
};
