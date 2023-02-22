/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#define _USE_MATH_DEFINES
#include <math.h>

class BicoeffGraph : public Component {

    float a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0;

    float filterGain = 0.75f;

    float filterWidth, filterCentre;
    float filterX1, filterX2;
    float lastCentre, lastX1, lastX2, lastGain;

    Object* object;

    Path magnitudePath;

public:
    std::function<void(float, float, float, float, float)> graphChangeCallback = [](float, float, float, float, float) {};

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

    BicoeffGraph(Object* parent)
        : object(parent)
    {
        filterWidth = 0.2f;
        filterCentre = 0.5f;
        filterX1 = filterCentre - (filterWidth / 2.0f);
        filterX2 = filterCentre + (filterWidth / 2.0f);

        setSize(300, 300);
        update();
    }

    void setFilterType(FilterType type)
    {
        filterType = type;
        update();
        saveProperties();
    }

    // Hack to make it remember the bounds and filter type...
    void saveProperties()
    {
        auto b = object->getObjectBounds();
        auto dim = String(" -dim ") + String(b.getWidth()) + " " + String(b.getHeight());
        auto type = String(" -type ") + String(filterTypeNames[static_cast<int>(filterType)]);
        auto buftext = String("bicoeff") + dim + type;
        auto* ptr = static_cast<t_object*>(object->getPointer());
        binbuf_text(ptr->te_binbuf, buftext.toRawUTF8(), buftext.getNumBytesAsUTF8());
    }

    bool canResizefilterWidth()
    {
        return filterType == Highshelf || filterType == Lowshelf || filterType == EQ || filterType == Allpass;
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

        for (float x = 0; x <= getWidth(); x++) {
            auto nn = (static_cast<float>(x) / getWidth()) * 120.0f + 16.766f;
            auto freq = mtof(nn);
            auto result = calcMagnitudePhase((M_PI * 2.0 * freq) / 44100.0f, a1, a2, b0, b1, b2);

            if (!std::isfinite(result.first)) {
                continue;
            }

            if (x == 0) {
                magnitudePath.startNewSubPath(x, result.first);
                // phasePath.startNewSubPath(x, result.second);

            } else {
                magnitudePath.lineTo(x, result.first);
                // phasePath.lineTo(x, result.second);
            }
        }

        repaint();
    }

    float mtof(float note)
    {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
        ;
    }

    void mouseDown(MouseEvent const& e) override
    {
        lastCentre = filterCentre;
        lastX1 = filterX1;
        lastX2 = filterX2;
        lastGain = filterGain;

        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (canResizefilterWidth() && (std::abs(e.mouseDownPosition.x - (lastX1 * getWidth())) < 5 || std::abs(e.mouseDownPosition.x - (lastX2 * getWidth())) < 5)) {
            changeBandWidth(e.x, e.y, e.mouseDownPosition.x, e.mouseDownPosition.y);
        } else {
            moveBand(e.x, e.mouseDownPosition.x);
            moveGain(e.y, e.mouseDownPosition.y);
        }

        update();
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (canResizefilterWidth()) {
            if (std::abs(e.x - (filterX1 * getWidth())) < 5 || std::abs(e.x - (filterX2 * getWidth())) < 5) {
                setMouseCursor(MouseCursor::LeftRightResizeCursor);
            } else {
                setMouseCursor(MouseCursor::NormalCursor);
            }
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

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        g.setColour(object->findColour(PlugDataColour::outlineColourId));

        if (canResizefilterWidth()) {
            g.drawVerticalLine(filterX1 * getWidth(), 0, getHeight());
            g.drawVerticalLine(filterX2 * getWidth(), 0, getHeight());
        } else {
            g.drawVerticalLine(filterCentre, 0, getHeight());
        }

        g.drawHorizontalLine(getHeight() / 2.0f, 0, getWidth());

        // g.setColour(Colours::green);
        // g.strokePath(phasePath, PathStrokeType(1.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::square));

        g.setColour(object->findColour(PlugDataColour::canvasTextColourId));
        g.strokePath(magnitudePath, PathStrokeType(1.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::square));

        bool selected = object->cnv->isSelected(object) && !object->cnv->isGraph;

        g.setColour(object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    std::pair<float, float> calcMagnitudePhase(float f, float a1, float a2, float b0, float b1, float b2)
    {
        float x1 = cos(-1.0 * f);
        float x2 = cos(-2.0 * f);
        float y1 = sin(-1.0 * f);
        float y2 = sin(-2.0 * f);

        float a = b0 + b1 * x1 + b2 * x2;
        float b = b1 * y1 + b2 * y2;
        float c = 1.0f - a1 * x1 - a2 * x2;
        float d = 0.0f - a1 * y1 - a2 * y2;
        float numerMag = sqrt(a * a + b * b);
        float numerArg = atan2(b, a);
        float denomMag = sqrt(c * c + d * d);
        float denomArg = atan2(d, c);

        float magnitude = numerMag / denomMag;
        float phase = numerArg - denomArg;

        float fHz = f * 44100.0f / (2.0f * M_PI);

        // convert magnitude to dB scale
        float logMagnitude = std::clamp<float>(20.0f * std::log(magnitude) / std::log(10.0), -25.f, 25.f);

        // scale to pixel range
        float halfFrameHeight = getHeight() / 2.0;
        logMagnitude = logMagnitude / 25.0 * halfFrameHeight;
        // invert and offset
        logMagnitude = -1.0 * logMagnitude + halfFrameHeight;

        // wrap phase
        if (phase > M_PI) {
            phase = phase - (M_PI * 2.0);
        } else if (phase < -M_PI) {
            phase = phase + (M_PI * 2.0);
        }
        // scale phase values to pixels
        float scaledPhase = halfFrameHeight * (-phase / M_PI) + halfFrameHeight;

        return { logMagnitude, scaledPhase };
    }

    std::pair<float, float> calcCoefficients()
    {
        float nn = (filterCentre)*120.0f + 16.766f;
        float nn2 = (filterWidth + filterCentre) * 120.0f + 16.766f;
        float f = mtof(nn);
        float bwf = mtof(nn2);
        float bw = (bwf / f) - 1.0f;

        float omega = (M_PI * 2.0 * f) / 44100.0f;
        float alpha = std::sin(omega) * std::sinh(std::log(2.0) / 2.0 * bw * omega / std::sin(omega));

        return { alpha, omega };
    }

    void changeBandWidth(float x, float y, float previousX, float previousY)
    {
        float xScale = x / getWidth();
        float previousXScale = previousX / getWidth();

        float dx = xScale - previousXScale;
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
        filterCentre = filterX1 + (filterWidth / 2.0f);

        moveGain(y, previousY);
    }

    void moveBand(float x, float previousX)
    {
        float dx = (x - previousX) / getWidth();

        float x1 = lastX1 + dx;
        float x2 = lastX2 + dx;

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
        filterCentre = filterX1 + (filterWidth / 2.0f);
    }

    void moveGain(float y, float previousY)
    {
        float dy = (y - previousY) / getHeight();
        filterGain = std::clamp<float>(lastGain + dy, 0.0f, 1.0f);
    }

    void setCoefficients(float a0, float a1, float a2, float b0, float b1, float b2)
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

        float b1 = 1.0 - cos(omega);
        float b0 = b1 / 2.0;
        float b2 = b0;
        float a0 = 1.0 + alpha;
        float a1 = -2.0 * cos(omega);
        float a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // highpass
    void highpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float b1 = -1 * (1.0 + cos(omega));
        float b0 = -b1 / 2.0;
        float b2 = b0;
        float a0 = 1.0 + alpha;
        float a1 = -2.0 * cos(omega);
        float a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // bandpass
    void bandpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float b1 = 0;
        float b0 = alpha;
        float b2 = -b0;
        float a0 = 1.0 + alpha;
        float a1 = -2.0 * cos(omega);
        float a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // resonant
    void resonant()
    {
        auto [alpha, omega] = calcCoefficients();

        float b1 = 0;
        float b0 = sin(omega) / 2;
        float b2 = -b0;
        float a0 = 1.0 + alpha;
        float a1 = -2.0 * cos(omega);
        float a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    // notch
    void notch()
    {
        auto [alpha, omega] = calcCoefficients();

        float b1 = -2.0 * cos(omega);
        float b0 = 1;
        float b2 = 1;
        float a0 = 1.0 + alpha;
        float a1 = b1;
        float a2 = 1.0 - alpha;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void peaking()
    {
        auto [alpha, omega] = calcCoefficients();

        float gain = std::min<float>(filterGain, 1.0f);
        float amp = pow(10.0, (-1.0 * (gain * 50.0 - 25.0)) / 40.0);
        float alphamulamp = alpha * amp;
        float alphadivamp = alpha / amp;

        float b1 = -2.0 * cos(omega);
        float b0 = 1.0 + alphamulamp;
        float b2 = 1.0 - alphamulamp;
        float a0 = 1.0 + alphadivamp;
        float a1 = b1;
        float a2 = 1.0 - alphadivamp;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void lowshelf()
    {
        auto [alpha, omega] = calcCoefficients();

        float gain = std::min<float>(filterGain, 1.0f);
        float amp = pow(10.0, (-1.0 * (gain * 50.0 - 25.0)) / 40.0);

        float alphaMod = 2.0 * sqrt(amp) * alpha;
        float cosOmega = cos(omega);
        float ampPlus = amp + 1.0;
        float ampMin = amp - 1.0;

        float b0 = amp * (ampPlus - ampMin * cosOmega + alphaMod);
        float b1 = 2.0 * amp * (ampMin - ampPlus * cosOmega);
        float b2 = amp * (ampPlus - ampMin * cosOmega - alphaMod);
        float a0 = ampPlus + ampMin * cosOmega + alphaMod;
        float a1 = -2.0 * (ampMin + ampPlus * cosOmega);
        float a2 = ampPlus + ampMin * cosOmega - alphaMod;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void highshelf()
    {
        auto [alpha, omega] = calcCoefficients();

        float gain = std::min<float>(filterGain, 1.0f);
        float amp = pow(10.0, (-1.0 * (gain * 50.0 - 25.0)) / 40.0);

        float alphaMod = 2.0 * sqrt(amp) * alpha;
        float cosOmega = cos(omega);
        float ampPlus = amp + 1.0;
        float ampMin = amp - 1.0;

        float b0 = amp * (ampPlus + ampMin * cosOmega + alphaMod);
        float b1 = -2.0 * amp * (ampMin + ampPlus * cosOmega);
        float b2 = amp * (ampPlus + ampMin * cosOmega - alphaMod);
        float a0 = ampPlus - ampMin * cosOmega + alphaMod;
        float a1 = 2.0 * (ampMin - ampPlus * cosOmega);
        float a2 = ampPlus - ampMin * cosOmega - alphaMod;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }

    void allpass()
    {
        auto [alpha, omega] = calcCoefficients();

        float b0 = 1.0 - alpha;
        float b1 = -2.0 * cos(omega);
        float b2 = 1.0 + alpha;
        float a0 = b2;
        float a1 = b1;
        float a2 = b0;

        setCoefficients(a0, a1, a2, b0, b1, b2);
    }
};

class BicoeffObject : public ObjectBase {

    BicoeffGraph graph;

public:
    BicoeffObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , graph(parent)
    {
        addAndMakeVisible(graph);

        graph.graphChangeCallback = [this](float a1, float a2, float b0, float b1, float b2) {
            t_atom at[5];
            SETFLOAT(at, a1);
            SETFLOAT(at + 1, a2);
            SETFLOAT(at + 2, b0);
            SETFLOAT(at + 3, b1);
            SETFLOAT(at + 4, b2);

            pd_typedmess(static_cast<t_pd*>(ptr), pd->generateSymbol("biquad"), 5, at);
        };
    }

    void resized() override
    {
        graph.setBounds(getLocalBounds());
    }

    void updateBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->unlockAudioThread();

        object->setObjectBounds(bounds);
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(object->cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        t_atom size[2];
        SETFLOAT(size, b.getWidth());
        SETFLOAT(size + 1, b.getHeight());
        pd_typedmess(static_cast<t_pd*>(ptr), pd->generateSymbol("dim"), 2, size);

        graph.saveProperties();
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
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
