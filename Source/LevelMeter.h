/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "LookAndFeel.h"
#include <JuceHeader.h>

#include <memory>
#include "../Libraries/ff_meters/ff_meters.h"

// Widget that shows a Foleys level meter and a volume slider

struct LevelMeter : public Component {
    LevelMeter(AudioProcessorValueTreeState& state, foleys::LevelMeterSource& source)
    {
        

        meter.setMeterSource(&source);

        lnf.setColour(foleys::LevelMeter::lmTextColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTextClipColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTextDeactiveColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTicksColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmOutlineColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmBackgroundColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmBackgroundClipColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmMeterForegroundColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterOutlineColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmMeterBackgroundColour, juce::Colours::darkgrey);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientMidColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientMaxColour, juce::Colours::red);

        meter.setLookAndFeel(&lnf);
        addAndMakeVisible(meter);

        addAndMakeVisible(volumeSlider);
        volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        volumeSlider.setValue(0.75);
        volumeSlider.setRange(0.0f, 1.0f);
    }

    ~LevelMeter() override
    {
        meter.setLookAndFeel(nullptr);
        setLookAndFeel(nullptr);
    }

    void resized() override
    {
        meter.setBounds(getLocalBounds());
        volumeSlider.setBounds(getLocalBounds().expanded(5));
    }

    foleys::LevelMeter meter = foleys::LevelMeter(foleys::LevelMeter::Horizontal | foleys::LevelMeter::Minimal);
    foleys::LevelMeterLookAndFeel lnf;

    Slider volumeSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
