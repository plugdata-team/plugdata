/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "NVGSurface.h"

class VUScale : public ObjectLabel {
    StringArray scaleText = { "+12", "+6", "+2", "-0dB", "-2", "-6", "-12", "-20", "-30", "-50", "-99" };
    unsigned scaleDecim = 0b10001001001; // reverse bitwise for controlling which scale text shows when too small to display all

    NVGcolor labelColor;

public:
    VUScale()
    {
    }

    ~VUScale()
    {
    }

    void setLabelColour(const Colour& colour)
    {
        labelColor = convertColour(colour);
        repaint();
    }

    virtual void renderLabel(NVGcontext* nvg, float scale) override
    {
        if (!isVisible())
            return;

        // TODO: Hack to hold all images for each context, consider moving somewhere central
        static std::unordered_map<NVGcontext*, std::array<NVGImage, 11>> scales;

        // We calculate the largest size the text will ever be (canvas zoom * UI scale * desktop scale)
        auto const maxUIScale = 3 * 2 * 2;
        auto const maxScaledWidth = getWidth() * maxUIScale;
        auto const maxScaledHeight = 20 * maxUIScale;

        auto& scaleImages = scales[nvg];

        if (!scaleImages[0].isValid()) {
            for (int i = 0; i < 11; i++) {
                // generate scale images that are max size of canvas * UI scale
                scaleImages[i] = NVGImage(nvg, maxScaledWidth, maxScaledHeight, [this, i](Graphics& g){
                    g.addTransform(AffineTransform::scale(maxUIScale));
                    g.setColour(Colours::white);
                    // Draw + or -
                    g.setFont(Fonts::getMonospaceFont().withHeight(9));
                    g.drawText(scaleText.getReference(i).substring(0, 1), getLocalBounds().withHeight(20), Justification::centredLeft, false);
                    // Draw dB value
                    g.setFont(Fonts::getDefaultFont().withHeight(9));
                    g.drawText(scaleText.getReference(i).substring(1), getLocalBounds().withHeight(20).withLeft(5), Justification::centredLeft, false);
                }, NVGImage::AlphaImage | NVGImage::MipMap);
            }
        }

        const bool decimScaleText = getHeight() < 90;

        for (int i = 0; i < 11; i++){
            if (decimScaleText && !(scaleDecim & (1 << i)))
                continue;
            const float scaleTextYPos = static_cast<float>(i) * (getHeight() - 20) / 10.0f;
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, 0, scaleTextYPos, getWidth(), 20, 0, scaleImages[i].getImageId(), labelColor));
            nvgFillRect(nvg, 0, scaleTextYPos, getWidth(), 20);
        }
    }
};

class VUMeterObject final : public ObjectBase {

    IEMHelper iemHelper;
    Value sizeProperty = SynchronousValue();
    Value showScale = SynchronousValue();

public:
    VUMeterObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        // we need to make this a specific size as it has two inlets
        // which will become squashed together if too close
        onConstrainerCreate = [this]() {
            constrainer->setMinimumSize(20, 20 * 2);
        };

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamReceiveSymbol(&iemHelper.receiveSymbol);
        objectParameters.addParamBool("Show scale", ParameterCategory::cAppearance, &showScale, { "No", "Yes" }, 1);
        iemHelper.addIemParameters(objectParameters, false, false, -1);

        updateLabel();
        if(auto vu = ptr.get<t_vu>()) showScale = vu->x_scale;
        valueChanged(showScale);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(iem->x_w), var(iem->x_h) });
        }
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
        String const text = iemHelper.labelText.toString();

        ObjectLabel* label = nullptr;
        VUScale* vuScale = nullptr;
        if (labels.isEmpty()) {
            label = labels.add(new ObjectLabel());
            vuScale = reinterpret_cast<VUScale*>(labels.add(new VUScale()));
            object->cnv->addChildComponent(label);
            object->cnv->addChildComponent(vuScale);
        }
        else {
            label = labels[0];
            vuScale = reinterpret_cast<VUScale*>(labels[1]);
        }

        if (label && text.isNotEmpty()) {
            auto bounds = iemHelper.getLabelBounds();
            bounds.translate(0, bounds.getHeight() / -2.0f);

            label->setFont(Font(bounds.getHeight()));
            label->setBounds(bounds);
            label->setText(text, dontSendNotification);
            label->setColour(Label::textColourId, iemHelper.getLabelColour());
            label->setVisible(true);
        }
        if (vuScale) {
            auto vuScaleBounds = Rectangle<int>(object->getBounds().getTopRight().x - 3, object->getBounds().getTopRight().y, 20, object->getBounds().getHeight());
            vuScale->setBounds(vuScaleBounds);
            vuScale->setVisible(getValue<bool>(showScale));
            vuScale->setLabelColour(iemHelper.getLabelColour());
        }
    }
    
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto vu = ptr.get<t_vu>()) {
                vu->x_gui.x_w = width;
                vu->x_gui.x_h = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(showScale)) {
            if (auto vu = ptr.get<t_vu>()) {
                auto showVU = getValue<bool>(showScale);
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
            sizeProperty = Array<var> { var(vu->x_gui.x_w), var(vu->x_gui.x_h) };
        }

        iemHelper.update();
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b);
    }

    void render(NVGcontext* nvg) override
    {
        if(!ptr.isValid()) return;

        float values[2] = { ptr.get<t_vu>()->x_fp, ptr.get<t_vu>()->x_fr };

        auto b =  getLocalBounds();
        auto bS = b.reduced(0.5f);
        // Object background
        nvgDrawRoundedRect(nvg, bS.getX(), bS.getY(), bS.getWidth(), bS.getHeight(), cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol, Corners::objectCornerRadius);

        auto rms = Decibels::decibelsToGain(values[1] - 10.0f);
        auto peak = Decibels::decibelsToGain(values[0] - 10.0f);
        auto barLength = jmin(std::exp(std::log(rms) / 3.0f) * (rms > 0.002f), 1.0f) * b.getHeight();
        auto peakPosition = jmin(std::exp(std::log(peak) / 3.0f) * (peak > 0.002f), 1.0f) * (b.getHeight() - 5.0f);
        
        NVGcolor barColour;
        if(values[1] < -12)
        {
            barColour = nvgRGBA(66, 163, 198, 255);
        }
        else if(values[1] > 0)
        {
            barColour = nvgRGBA(255, 0, 0, 255);
        }
        else {
            barColour = nvgRGBA(255, 127, 0, 255);
        }
        
        // VU Bar
        nvgFillColor(nvg, barColour);
        nvgBeginPath(nvg);
        nvgRoundedRectVarying(nvg, 0, getHeight() - barLength, getWidth(), barLength, 0.0f, 0.0f, Corners::objectCornerRadius, Corners::objectCornerRadius);
        nvgFill(nvg);

        // Peak
        nvgFillColor(nvg, cnv->objectOutlineCol);
        nvgFillRect(nvg, 0, getHeight() - peakPosition - 5.0f, getWidth(), 5.0f);

        // Object outline
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"): {
            repaint();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            break;
        }
        }
    }
};
