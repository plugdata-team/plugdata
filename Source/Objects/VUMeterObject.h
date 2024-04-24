/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class VUMeterObject final : public ObjectBase {

    IEMHelper iemHelper;
    Value sizeProperty = SynchronousValue();

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
        objectParameters.addParamSendSymbol(&iemHelper.sendSymbol, "nosndno");
        iemHelper.addIemParameters(objectParameters, false, false, -1);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(iem->x_w), var(iem->x_h) });
        }
    }

    bool hideInlets() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
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
        auto values = std::vector<float> { ptr.getRaw<t_vu>()->x_fp, ptr.getRaw<t_vu>()->x_fr };
        auto backgroundColour = convertColour(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        auto selectedOutlineColour = convertColour(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        
        int height = getHeight();
        int width = getWidth();

        nvgDrawRoundedRect(nvg, 0, 0, width, height, backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        auto outerBorderWidth = 2.0f;
        auto totalBlocks = 30;
        auto spacingFraction = 0.05f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockHeight = (height - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockWidth = width - doubleOuterBorderWidth;
        auto blockRectHeight = (1.0f - 2.0f * spacingFraction) * blockHeight;
        auto blockRectSpacing = spacingFraction * blockHeight;
        auto blockCornerSize = 0.1f * blockHeight;

        float rms = Decibels::decibelsToGain(values[1] - 12.0f);
        float lvl = (float)std::exp(std::log(rms) / 3.0) * (rms > 0.002);
        auto numBlocks = roundToInt(totalBlocks * lvl);

        auto verticalGradient1 = nvgLinearGradient(nvg, 0, getHeight() * 0.25f, 0, getHeight() * 0.5f, nvgRGBAf(1, 0.5f, 0, 1), nvgRGBAf(0.26f, 0.64f, 0.78f, 1.0f));
        auto verticalGradient2 = nvgLinearGradient(nvg, 0, 0, 0, getHeight() * 0.25f, nvgRGBAf(1, 0, 0, 1), nvgRGBAf(1, 0.5f, 0, 1));

        for (auto i = 1; i < totalBlocks; ++i) {
            NVGpaint gradient;
            if (i >= numBlocks) {
                nvgFillColor(nvg, nvgRGBAf(0.3f, 0.3f, 0.3f, 1.0f)); // Dark grey for inactive blocks
            } else {
                gradient = (i < totalBlocks * 0.75f) ? verticalGradient1 : verticalGradient2;
                nvgFillPaint(nvg, gradient);
            }

            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, outerBorderWidth, outerBorderWidth + ((totalBlocks - i) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight, blockCornerSize);
            nvgFill(nvg);
        }

        float peak = Decibels::decibelsToGain(values[0] - 12.0f);
        float lvl2 = (float)std::exp(std::log(peak) / 3.0) * (peak > 0.002);
        auto numBlocks2 = roundToInt(totalBlocks * lvl2);

        nvgFillColor(nvg, nvgRGBAf(1, 1, 1, 1)); // White for the peak block
        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, outerBorderWidth, outerBorderWidth + ((totalBlocks - numBlocks2) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight / 2.0f, blockCornerSize);
        nvgFill(nvg);

        // Get text value with 2 and 0 decimals
        // Prevent going past -100 for size reasons
        String textValue = String(std::max(values[1], -96.0f), 2);

        if (width > nvgTextBounds(nvg, 0, 0, textValue.toRawUTF8(), nullptr, nullptr)) {
            // Check noscale flag, otherwise display next to slider
            nvgFontSize(nvg, 11);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, nvgRGBAf(1, 1, 1, 1)); // White color
            nvgText(nvg, width * 0.5f, height - 20, textValue.toRawUTF8(), nullptr);
        } else {
            // Fallback for smaller sizes
            textValue = String(std::max(values[1], -96.0f), 0);
            nvgFontSize(nvg, 11);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, nvgRGBAf(1, 1, 1, 1)); // White color
            nvgText(nvg, width * 0.5f, height - 20, textValue.toRawUTF8(), nullptr);
        }
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
