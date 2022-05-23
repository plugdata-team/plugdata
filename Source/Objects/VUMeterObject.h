

struct VUMeterObject final : public IEMObject
{
    VUMeterObject(void* ptr, Box* box) : IEMObject(ptr, box)
    {
        initialise();
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(30, maxSize, box->getWidth());
        int h = jlimit(80, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    float getValue() override
    {
        return static_cast<t_vu*>(ptr)->x_fp;
    }

    float getRMS()
    {
        return static_cast<t_vu*>(ptr)->x_fr;
    }

    void resized() override
    {
    }

    void paint(Graphics& g) override
    {
        g.fillAll(box->findColour(PlugDataColour::toolbarColourId));

        auto values = std::vector<float>{getValue(), getRMS()};

        int height = getHeight();
        int width = getWidth();

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

        for (auto i = 0; i < totalBlocks; ++i)
        {
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

        if (getWidth() > g.getCurrentFont().getStringWidth(textValue + " dB"))
        {
            // Check noscale flag, otherwise display next to slider
            g.setColour(Colours::white);
            g.drawFittedText(textValue + " dB", Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
        }
        else if (getWidth() > g.getCurrentFont().getStringWidth(textValue))
        {
            g.setColour(Colours::white);
            g.drawFittedText(textValue, Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
        }
        else
        {
            g.setColour(Colours::white);
            g.setFont(11);
            g.drawFittedText(String(std::max(values[1], -96.0f), 0), Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
        }
    }

    void updateValue() override
    {
        repaint();
    };
};
