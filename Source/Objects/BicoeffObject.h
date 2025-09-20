/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class BicoeffGraph final : public Component
    , public NVGComponent {

    float a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0;

    float filterGain = 0.5f;

    float filterWidth, filterCentre;
    float filterX1, filterX2;
    float lastX1, lastX2, lastGain;

    Object* object;

    Path magnitudePath;

public:
    std::function<void(float, float, float, float, float)> graphChangeCallback = [](float, float, float, float, float) { };

    enum FilterType {
        Allpass,
        Lowpass,
        Highpass,
        Bandpass,
        Resonant,
        Bandstop,
        EQ,
        Lowshelf,
        Highshelf
    };

    StringArray filterTypeNames = { "allpass", "lowpass", "highpass", "bandpass", "resonant", "bandstop", "eq", "lowshelf", "highshelf" };

    FilterType filterType = EQ;

    explicit BicoeffGraph(Object* parent)
        : NVGComponent(this)
        , object(parent)
    {
        filterWidth = 0.2f;
        filterCentre = 0.5f;
        filterX1 = filterCentre - filterWidth / 2.0f;
        filterX2 = filterCentre + filterWidth / 2.0f;

        setSize(300, 300);
        update();
    }

    void setFilterType(FilterType const type)
    {
        filterType = type;
        update();
        saveProperties();
    }

    // Hack to make it remember the bounds and filter type...
    void saveProperties()
    {
        auto const b = object->getObjectBounds();
        auto const dim = String(" -dim ") + String(b.getWidth()) + " " + String(b.getHeight());
        auto const type = String(" -type ") + String(filterTypeNames[static_cast<int>(filterType)]);
        auto const buftext = String("bicoeff") + dim + type;
        auto const* ptr = pd::Interface::checkObject(object->getPointer());
        binbuf_text(ptr->te_binbuf, buftext.toRawUTF8(), buftext.getNumBytesAsUTF8());
    }

    bool canResizefilterAmplitude() const
    {
        return filterType == Highshelf || filterType == Lowshelf || filterType == EQ;
    }
    
    auto calcMagnitudePhase(float const f, float const a1, float const a2, float const b0, float const b1, float const b2) const
    {
        struct MagnitudeAndPhase
        {
            float magnitude, phase;
        };
        
        float const x1 = cos(-1.0 * f);
        float const x2 = cos(-2.0 * f);
        float const y1 = sin(-1.0 * f);
        float const y2 = sin(-2.0 * f);

        float const a = b0 + b1 * x1 + b2 * x2;
        float const b = b1 * y1 + b2 * y2;
        float const c = 1.0f - a1 * x1 - a2 * x2;
        float const d = 0.0f - a1 * y1 - a2 * y2;
        float const numerMag = sqrt(a * a + b * b);
        float const numerArg = atan2(b, a);
        float const denomMag = sqrt(c * c + d * d);
        float const denomArg = atan2(d, c);

        float const magnitude = numerMag / denomMag;
        float phase = numerArg - denomArg;

        // convert magnitude to dB scale
        float logMagnitude = std::clamp<float>(20.0f * std::log(magnitude) / std::log(10.0), -25.f, 25.f);

        // scale to pixel range
        float const halfFrameHeight = getHeight() / 2.0;
        logMagnitude = logMagnitude / 25.0 * halfFrameHeight;
        // invert and offset
        logMagnitude = -1.0 * logMagnitude + halfFrameHeight;

        // wrap phase
        if (phase > MathConstants<float>::pi) {
            phase = phase - MathConstants<float>::pi * 2.0;
        } else if (phase < -MathConstants<float>::pi) {
            phase = phase + MathConstants<float>::pi * 2.0;
        }
        // scale phase values to pixels
        float scaledPhase = halfFrameHeight * (-phase / MathConstants<float>::pi) + halfFrameHeight;

        return MagnitudeAndPhase{ logMagnitude, scaledPhase };
    }

    auto calcCoefficients() const
    {
        struct AlphaAndOmega
        {
            float alpha, omega;
        };
        
        float const nn = filterCentre * 120.0f + 16.766f;
        float const nn2 = (filterWidth + filterCentre) * 120.0f + 16.766f;
        float const f = mtof(nn);
        float const bwf = mtof(nn2);
        float const bw = bwf / f - 1.0f;

        float omega = MathConstants<float>::pi * 2.0 * f / 44100.0f;
        float alpha = std::sin(omega) * std::sinh(std::log(2.0) / 2.0 * bw * omega / std::sin(omega));

        return AlphaAndOmega{ alpha, omega };
    }
    

    void update()
    {
        switch (filterType) {
        case Allpass: {
            allpass();
            break;
        }
        case Lowpass: {
            lowpass();
            break;
        }
        case Highpass: {
            highpass();
            break;
        }
        case Bandpass: {
            bandpass();
            break;
        }
        case Resonant: {
            resonant();
            break;
        }
        case Bandstop: {
            notch();
            break;
        }
        case EQ: {
            peaking();
            break;
        }
        case Lowshelf: {
            lowshelf();
            break;
        }
        case Highshelf: {
            highshelf();
            break;
        }
        default:
            break;
        }

        magnitudePath.clear();

        for (int x = 0; x <= getWidth(); x++) {
            auto const nn = static_cast<float>(x) / getWidth() * 120.0f + 16.766f;
            auto const freq = mtof(nn);
            auto const result = calcMagnitudePhase(MathConstants<float>::pi * 2.0f * freq / 44100.0f, a1, a2, b0, b1, b2);

            if (!std::isfinite(result.magnitude)) {
                continue;
            }

            if (x == 0) {
                magnitudePath.startNewSubPath(x, result.magnitude);

            } else {
                magnitudePath.lineTo(x, result.magnitude);
            }
        }

        repaint();
    }

    static float mtof(float const note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        lastX1 = filterX1;
        lastX2 = filterX2;
        lastGain = filterGain;

        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (std::abs(e.mouseDownPosition.x - lastX1 * getWidth()) < 5 || std::abs(e.mouseDownPosition.x - lastX2 * getWidth()) < 5) {
            changeBandWidth(e.x, e.y, e.mouseDownPosition.x, e.mouseDownPosition.y);
        } else {
            moveBand(e.x, e.mouseDownPosition.x);
            if (canResizefilterAmplitude())
                moveGain(e.y, e.mouseDownPosition.y);
        }

        update();
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (std::abs(e.x - filterX1 * getWidth()) < 5 || std::abs(e.x - filterX2 * getWidth()) < 5) {
            setMouseCursor(MouseCursor::LeftRightResizeCursor);
        } else {
            setMouseCursor(MouseCursor::NormalCursor);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        update();
    }

    void resized() override
    {
        update();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds();
        auto const backgroundColour = convertColour(object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        auto const selectedOutlineColour = convertColour(object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto const outlineColour = convertColour(object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        nvgStrokeColor(nvg, convertColour(object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour)));
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, filterX1 * getWidth(), 0.0f);
        nvgLineTo(nvg, filterX1 * getWidth(), getHeight());
        nvgStroke(nvg);

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, filterX2 * getWidth(), 0.0f);
        nvgLineTo(nvg, filterX2 * getWidth(), getHeight());
        nvgStroke(nvg);

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0.0f, getHeight() / 2.0f);
        nvgLineTo(nvg, getWidth(), getHeight() / 2.0f);
        nvgStroke(nvg);

        nvgStrokeWidth(nvg, 1.0f);
        nvgLineStyle(nvg, NVG_BUTT);
        setJUCEPath(nvg, magnitudePath);
        nvgStrokeColor(nvg, convertColour(object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId)));
        nvgStroke(nvg);
    }


    void changeBandWidth(float const x, float const y, float const previousX, float const previousY)
    {
        float const xScale = x / getWidth();
        float const previousXScale = previousX / getWidth();

        float const dx = xScale - previousXScale;
        if (previousXScale < filterCentre) {
            if (xScale < 0.0f) {
                filterX1 = 0;
                filterX2 = filterWidth;
            } else if (xScale < filterCentre - 0.15f) {
                filterX1 = filterCentre - 0.15f;
                filterX2 = filterCentre + 0.15f;
            } else if (xScale > filterCentre) {
                filterX1 = filterCentre;
                filterX2 = filterCentre;
            } else {
                filterX1 = xScale;
                filterX2 = lastX2 - dx;
            }
        } else {
            if (xScale >= 1.0f) {
                filterX2 = 0;
                filterX1 = filterWidth;
            } else if (xScale > filterCentre + 0.15f) {
                filterX1 = filterCentre - 0.15f;
                filterX2 = filterCentre + 0.15f;
            } else if (xScale < filterCentre) {
                filterX1 = filterCentre;
                filterX2 = filterCentre;
            } else {
                filterX2 = xScale;
                filterX1 = lastX1 - dx;
            }
        }

        filterWidth = filterX2 - filterX1;
        filterCentre = filterX1 + filterWidth / 2.0f;

        moveGain(y, previousY);
    }

    void moveBand(float const x, float const previousX)
    {
        float const dx = (x - previousX) / getWidth();

        float const x1 = lastX1 + dx;
        float const x2 = lastX2 + dx;

        if (x1 < 0.0f) {
            filterX1 = 0;
            filterX2 = filterWidth;
        } else if (x2 >= 1.0f) {
            filterX1 = 1.0f - filterWidth;
            filterX2 = 1.0f;
        } else {
            filterX1 = x1;
            filterX2 = x2;
        }

        filterWidth = filterX2 - filterX1;
        filterCentre = filterX1 + filterWidth / 2.0f;
    }

    void moveGain(float const y, float const previousY)
    {
        float const dy = (y - previousY) / getHeight();
        filterGain = std::clamp<float>(lastGain + dy, 0.0f, 1.0f);
    }

    void setCoefficients(float const a0, float const a1, float const a2, float const b0, float const b1, float const b2)
    {
        this->a1 = -a1 / a0;
        this->a2 = -a2 / a0;
        this->b0 = b0 / a0;
        this->b1 = b1 / a0;
        this->b2 = b2 / a0;

        graphChangeCallback(this->a1, this->a2, this->b0, this->b1, this->b2);
    }

    // lowpass
    //    f0 = frequency in Hz
    //    bw = bandwidth where 1 is an octave
    void lowpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float const b1 = 1.0 - cos(omega);
        float const b0 = b1 / 2.0;
        float const b2 = b0;
        float const a0 = 1.0 + alpha;
        float const a1 = -2.0 * cos(omega);
        float const a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // highpass
    void highpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float const b1 = -1 * (1.0 + cos(omega));
        float const b0 = -b1 / 2.0;
        float const b2 = b0;
        float const a0 = 1.0 + alpha;
        float const a1 = -2.0 * cos(omega);
        float const a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // bandpass
    void bandpass()
    {
        auto [alpha, omega] = calcCoefficients();

        constexpr float b1 = 0;
        float const b0 = alpha;
        float const b2 = -b0;
        float const a0 = 1.0 + alpha;
        float const a1 = -2.0 * cos(omega);
        float const a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // resonant
    void resonant()
    {
        auto [alpha, omega] = calcCoefficients();

        constexpr float b1 = 0;
        float const b0 = sin(omega) / 2;
        float const b2 = -b0;
        float const a0 = 1.0 + alpha;
        float const a1 = -2.0 * cos(omega);
        float const a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // notch
    void notch()
    {
        auto [alpha, omega] = calcCoefficients();

        float const b1 = -2.0 * cos(omega);
        constexpr float b0 = 1;
        constexpr float b2 = 1;
        float const a0 = 1.0 + alpha;
        float const a1 = b1;
        float const a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void peaking()
    {
        auto [alpha, omega] = calcCoefficients();

        float const gain = std::min<float>(filterGain, 1.0f);
        float const amp = pow(10.0, -1.0 * (gain * 50.0 - 25.0) / 40.0);
        float const alphamulamp = alpha * amp;
        float const alphadivamp = alpha / amp;

        float const b1 = -2.0 * cos(omega);
        float const b0 = 1.0 + alphamulamp;
        float const b2 = 1.0 - alphamulamp;
        float const a0 = 1.0 + alphadivamp;
        float const a1 = b1;
        float const a2 = 1.0 - alphadivamp;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void lowshelf()
    {
        auto [alpha, omega] = calcCoefficients();

        float const gain = std::min<float>(filterGain, 1.0f);
        float const amp = pow(10.0, -1.0 * (gain * 50.0 - 25.0) / 40.0);

        float const alphaMod = 2.0 * sqrt(amp) * alpha;
        float const cosOmega = cos(omega);
        float const ampPlus = amp + 1.0;
        float const ampMin = amp - 1.0;

        float const b0 = amp * (ampPlus - ampMin * cosOmega + alphaMod);
        float const b1 = 2.0 * amp * (ampMin - ampPlus * cosOmega);
        float const b2 = amp * (ampPlus - ampMin * cosOmega - alphaMod);
        float const a0 = ampPlus + ampMin * cosOmega + alphaMod;
        float const a1 = -2.0 * (ampMin + ampPlus * cosOmega);
        float const a2 = ampPlus + ampMin * cosOmega - alphaMod;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void highshelf()
    {
        auto [alpha, omega] = calcCoefficients();

        float const gain = std::min<float>(filterGain, 1.0f);
        float const amp = pow(10.0, -1.0 * (gain * 50.0 - 25.0) / 40.0);

        float const alphaMod = 2.0 * sqrt(amp) * alpha;
        float const cosOmega = cos(omega);
        float const ampPlus = amp + 1.0;
        float const ampMin = amp - 1.0;

        float const b0 = amp * (ampPlus + ampMin * cosOmega + alphaMod);
        float const b1 = -2.0 * amp * (ampMin + ampPlus * cosOmega);
        float const b2 = amp * (ampPlus + ampMin * cosOmega - alphaMod);
        float const a0 = ampPlus - ampMin * cosOmega + alphaMod;
        float const a1 = 2.0 * (ampMin - ampPlus * cosOmega);
        float const a2 = ampPlus - ampMin * cosOmega - alphaMod;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void allpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float const b0 = 1.0 - alpha;
        float const b1 = -2.0 * cos(omega);
        float const b2 = 1.0 + alpha;
        float const a0 = b2;
        float const a1 = b1;
        float const a2 = b0;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }
};

class BicoeffObject final : public ObjectBase {

    BicoeffGraph graph;
    Value sizeProperty = SynchronousValue();

public:
    BicoeffObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , graph(parent)
    {
        addAndMakeVisible(graph);

        graph.graphChangeCallback = [this](float a1, float a2, float b0, float b1, float b2) {
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "biquad", { a1, a2, b0, b1, b2 });
        };

        objectParameters.addParamSize(&sizeProperty);
    }

    void resized() override
    {
        graph.setBounds(getLocalBounds());
    }

    void render(NVGcontext* nvg) override
    {
        graph.render(nvg);
    }

    void update() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {

            auto* patch = object->cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            sizeProperty = VarArray { var(w), var(h) };
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            setParameterExcludingListener(sizeProperty, VarArray { var(w), var(h) });
        }
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto gobj = ptr.get<t_gobj>()) {
                pd->sendDirectMessage(gobj.get(), "dim", { static_cast<float>(width), static_cast<float>(height) });
            }

            object->updateBounds();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = object->cnv->patch.getRawPointer();
            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());
            pd->sendDirectMessage(gobj.get(), "dim", { static_cast<float>(b.getWidth()) - 1, static_cast<float>(b.getHeight()) - 1 });
        }

        graph.saveProperties();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("allpass"): {
            graph.setFilterType(BicoeffGraph::Allpass);
            break;
        }
        case hash("lowpass"): {
            graph.setFilterType(BicoeffGraph::Lowpass);
            break;
        }
        case hash("highpass"): {
            graph.setFilterType(BicoeffGraph::Highpass);
            break;
        }
        case hash("bandpass"): {
            graph.setFilterType(BicoeffGraph::Bandpass);
            break;
        }
        case hash("bandstop"): {
            graph.setFilterType(BicoeffGraph::Bandstop);
            break;
        }
        case hash("resonant"): {
            graph.setFilterType(BicoeffGraph::Resonant);
            break;
        }
        case hash("eq"): {
            graph.setFilterType(BicoeffGraph::EQ);
            break;
        }
        case hash("lowshelf"): {
            graph.setFilterType(BicoeffGraph::Lowshelf);
            break;
        }
        case hash("highshelf"): {
            graph.setFilterType(BicoeffGraph::Highshelf);
            break;
        }
        default:
            break;
        }
    }
};
