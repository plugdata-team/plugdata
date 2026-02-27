/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class VUScale final : public ObjectLabel {
    StringArray scaleText = { "+12", "+6", "+2", "-0dB", "-2", "-6", "-12", "-20", "-30", "-50", "-99" };
    unsigned scaleDecim = 0b10001001001; // reverse bitwise for controlling which scale text shows when too small to display all

    NVGcolor labelColor;
    StackArray<NVGImage, 11> scaleImages;
    NVGcontext* lastContext = nullptr;

public:
    VUScale()
    {
    }

    ~VUScale() override
    {
    }

    void setLabelColour(Colour const& colour)
    {
        labelColor = nvgColour(colour);
        repaint();
    }

    void updateScales(NVGcontext* nvg)
    {
        // We calculate the largest size the text will ever be (canvas zoom * UI scale * desktop scale)
        auto constexpr maxUIScale = 3 * 2 * 2;
        auto constexpr maxScaledHeight = 20 * maxUIScale;
        auto const maxScaledWidth = getWidth() * maxUIScale;

        if (!scaleImages[0].isValid() || lastContext != nvg) {
            for (int i = 0; i < 11; i++) {
                // generate scale images that are max size of canvas * UI scale
                scaleImages[i] = NVGImage(nvg, maxScaledWidth, maxScaledHeight, [this, i](Graphics& g) {
                    g.addTransform(AffineTransform::scale(maxUIScale));
                    g.setColour(Colours::white);
                    // Draw + or -
                    g.setFont(Fonts::getMonospaceFont().withHeight(9));
                    g.drawText(scaleText.getReference(i).substring(0, 1), getLocalBounds().withHeight(20), Justification::centredLeft, false);
                    // Draw dB value
                    g.setFont(Fonts::getDefaultFont().withHeight(9));
                    g.drawText(scaleText.getReference(i).substring(1), getLocalBounds().withHeight(20).withLeft(5), Justification::centredLeft, false); }, NVGImage::AlphaImage | NVGImage::MipMap);
            }
            lastContext = nvg;
        }
    }

    void renderLabel(NVGcontext* nvg, float const scale) override
    {
        if (!isVisible())
            return;

        updateScales(nvg);

        bool const decimScaleText = getHeight() < 90;

        for (int i = 0; i < 11; i++) {
            if (decimScaleText && !(scaleDecim & 1 << i))
                continue;
            float const scaleTextYPos = static_cast<float>(i) * (getHeight() - 20) / 10.0f;
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, 0, scaleTextYPos, getWidth(), 20, 0, scaleImages[i].getImageId(), labelColor));
            nvgFillRect(nvg, 0, scaleTextYPos, getWidth(), 20);
        }
    }
};

class VUMeterObject final : public ObjectBase {

    IEMHelper iemHelper;
    Value sizeProperty = SynchronousValue();
    Value showScale = SynchronousValue();
    NVGcolor bgCol;

public:
    VUMeterObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamReceiveSymbol(&iemHelper.receiveSymbol);
        objectParameters.addParamBool("Show scale", ParameterCategory::cAppearance, &showScale, { "No", "Yes" }, 1);
        objectParameters.addParamColour("Background", ParameterCategory::cAppearance, &iemHelper.secondaryColour);
        iemHelper.addIemParameters(objectParameters, false, false, false, -1);

        updateLabel();

        iemHelper.iemColourChangedCallback = [this] {
            bgCol = nvgColour(Colour::fromString(iemHelper.secondaryColour.toString()));
        };
    }

    void onConstrainerCreate() override
    {
        // we need to make this a specific size as it has two inlets
        // which will become squashed together if too close
        constrainer->setMinimumSize(20, 20 * 2);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(iem->x_w), var(iem->x_h) });
        }
    }

    bool hideInlet() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlet() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        String const text = iemHelper.labelText.toString();

        ObjectLabel* label = nullptr;
        VUScale* vuScale = nullptr;
        if (labels.isEmpty()) {
            label = labels.add(new ObjectLabel());
            vuScale = reinterpret_cast<VUScale*>(labels.add(new VUScale()));
            object->cnv->addChildComponent(label);
            object->cnv->addChildComponent(vuScale);
        } else {
            label = labels[0];
            vuScale = reinterpret_cast<VUScale*>(labels[1]);
        }

        if (label && text.isNotEmpty()) {
            auto bounds = iemHelper.getLabelBounds();
            bounds.translate(0, bounds.getHeight() / -2.0f);

            label->setFont(Font(FontOptions(bounds.getHeight())));
            label->setBounds(bounds);
            label->setText(text, dontSendNotification);
            label->setColour(Label::textColourId, iemHelper.getLabelColour());
            label->setVisible(true);
        }
        if (vuScale) {
            auto const vuScaleBounds = Rectangle<int>(object->getBounds().getTopRight().x - 3, object->getBounds().getTopRight().y, 20, object->getBounds().getHeight());
            vuScale->setBounds(vuScaleBounds);
            vuScale->setVisible(getValue<bool>(showScale));
            vuScale->setLabelColour(iemHelper.getLabelColour());
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

            if (auto vu = ptr.get<t_vu>()) {
                vu->x_gui.x_w = width;
                vu->x_gui.x_h = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(showScale)) {
            if (auto vu = ptr.get<t_vu>()) {
                auto const showVU = getValue<bool>(showScale);
                vu->x_scale = showVU;
            }
            updateLabel();
        } else {
            iemHelper.valueChanged(v);
        }
    }

    void update() override
    {
        if (auto vu = ptr.get<t_vu>()) {
            sizeProperty = VarArray { var(vu->x_gui.x_w), var(vu->x_gui.x_h) };
            showScale = vu->x_scale;
        }

        updateLabel();
        iemHelper.update();
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        iemHelper.setPdBounds(b);
    }

    void render(NVGcontext* nvg) override
    {
        if (!ptr.isValid())
            return;

        float const values[2] = { ptr.get<t_vu>()->x_fp, ptr.get<t_vu>()->x_fr };

        auto const b = getLocalBounds();
        auto const bS = b.reduced(0.5f);
        // Object background
        nvgDrawRoundedRect(nvg, bS.getX(), bS.getY(), bS.getWidth(), bS.getHeight(), bgCol, bgCol, Corners::objectCornerRadius);

        auto const rms = Decibels::decibelsToGain(values[1] - 10.0f);
        auto const peak = Decibels::decibelsToGain(values[0] - 10.0f);
        auto const barLength = jmin(std::exp(std::log(rms) / 3.0f) * (rms > 0.002f), 1.0f) * b.getHeight();
        auto const peakPosition = jmin(std::exp(std::log(peak) / 3.0f) * (peak > 0.002f), 1.0f) * (b.getHeight() - 5.0f);

        auto getColourForLevel = [](float const level) {
            if (level < -12) {
                return nvgRGBA(66, 163, 198, 255);
            }
            if (level > 0) {
                return nvgRGBA(255, 0, 0, 255);
            }
            return nvgRGBA(255, 127, 0, 255);
        };

        NVGcolor const peakColour = getColourForLevel(values[0]);
        NVGcolor const barColour = getColourForLevel(values[1]);

        // VU Bar
        nvgFillColor(nvg, barColour);
        nvgBeginPath(nvg);
        nvgRoundedRectVarying(nvg, 4, getHeight() - barLength, getWidth() - 8, barLength, 0.0f, 0.0f, Corners::objectCornerRadius, Corners::objectCornerRadius);
        nvgFill(nvg);

        nvgBeginPath(nvg);
        int const increment = getHeight() / 30;
        for (int i = 0; i < 30; i++) {

            nvgMoveTo(nvg, 0, i * increment + 3);
            nvgLineTo(nvg, getWidth(), i * increment + 3);
        }
        nvgStrokeWidth(nvg, 1.0f);
        nvgStrokeColor(nvg, bgCol);
        nvgStroke(nvg);

        // Peak
        nvgFillColor(nvg, peakColour);
        nvgFillRect(nvg, 0, getHeight() - peakPosition - 5.0f, getWidth(), 5.0f);

        // Object outline
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"): {
            repaint();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }
};
