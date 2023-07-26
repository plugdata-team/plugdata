/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class VUMeterObject final : public ObjectBase {

    IEMHelper iemHelper;
    Value sizeProperty;
    
public:
    VUMeterObject(void* ptr, Object* object)
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
            setParameterExcludingListener(sizeProperty, Array<var>{var(iem->x_w), var(iem->x_h)});
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
            
            setParameterExcludingListener(sizeProperty, Array<var>{var(width), var(height)});
            
            if (auto vu = ptr.get<t_vu>())
            {
                vu->x_gui.x_w = width;
                vu->x_gui.x_h = height;
            }
            
            object->updateBounds();
        }
        else {
            iemHelper.valueChanged(v);
        }
    }

    void update() override
    {
        if (auto vu = ptr.get<t_vu>()) {
            sizeProperty = Array<var>{var(vu->x_gui.x_w), var(vu->x_gui.x_h)};
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

    void paint(Graphics& g) override
    {
        auto values = std::vector<float> { ptr.getRaw<t_vu>()->x_fp, ptr.getRaw<t_vu>()->x_fr };

        int height = getHeight();
        int width = getWidth();

        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        auto outerBorderWidth = 2.0f;
        auto totalBlocks = 30;
        auto spacingFraction = 0.05f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockHeight = (height - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockWidth = width - doubleOuterBorderWidth;
        auto blockRectHeight = (1.0f - 2.0f * spacingFraction) * blockHeight;
        auto blockRectSpacing = spacingFraction * blockHeight;
        auto blockCornerSize = 0.1f * blockHeight;
        auto c = Colour(0xff42a2c8);

        float rms = Decibels::decibelsToGain(values[1] - 12.0f);

        float lvl = (float)std::exp(std::log(rms) / 3.0) * (rms > 0.002);
        auto numBlocks = roundToInt(totalBlocks * lvl);

        auto verticalGradient = ColourGradient(c, 0, getHeight(), Colours::red, 0, 0, false);
        verticalGradient.addColour(0.5f, c);
        verticalGradient.addColour(0.75f, Colours::orange);

        for (auto i = 0; i < totalBlocks; ++i) {
            if (i >= numBlocks)
                g.setColour(Colours::darkgrey);
            else
                g.setGradientFill(verticalGradient);

            g.fillRoundedRectangle(outerBorderWidth, outerBorderWidth + ((totalBlocks - i) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight, blockCornerSize);
        }

        float peak = Decibels::decibelsToGain(values[0] - 12.0f);
        float lvl2 = (float)std::exp(std::log(peak) / 3.0) * (peak > 0.002);
        auto numBlocks2 = roundToInt(totalBlocks * lvl2);

        g.setColour(Colours::white);
        g.fillRoundedRectangle(outerBorderWidth, outerBorderWidth + ((totalBlocks - numBlocks2) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight / 2.0f, blockCornerSize);

        // Get text value with 2 and 0 decimals
        // Prevent going past -100 for size reasons
        String textValue = String(std::max(values[1], -96.0f), 2);

        if (getWidth() > g.getCurrentFont().getStringWidth(textValue + " dB")) {
            // Check noscale flag, otherwise display next to slider
            Fonts::drawFittedText(g, textValue + " dB", Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Colours::white, 1, 1.0f, 11, Justification::centred);
        } else if (getWidth() > g.getCurrentFont().getStringWidth(textValue)) {
            Fonts::drawFittedText(g, textValue, Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Colours::white, 1, 1.0f, 11, Justification::centred);
        } else {
            Fonts::drawFittedText(g, String(std::max(values[1], -96.0f), 0), Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Colours::white, 1, 1.0f, 11, Justification::centred);
        }

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            IEMGUI_MESSAGES
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
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
