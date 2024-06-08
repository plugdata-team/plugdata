#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "Components/DraggableNumber.h"
#include "PluginEditor.h"

class OversampleSettings : public Component {
public:
    std::function<void(int)> onChange = [](int) {};

    explicit OversampleSettings(int currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        eight.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = Array<TextButton*> { &one, &two, &four, &eight };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i]() {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        buttons[currentSelection]->setToggleState(true, dontSendNotification);

        setSize(180, 50);
    }

private:
    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 4);
        auto buttonWidth = b.getWidth() / 4;

        one.setBounds(b.removeFromLeft(buttonWidth));
        two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        eight.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    TextButton one = TextButton("1x");
    TextButton two = TextButton("2x");
    TextButton four = TextButton("4x");
    TextButton eight = TextButton("8x");
};

class LimiterSettings : public Component {
public:
    std::function<void(int)> onChange = [](int) {};

    explicit LimiterSettings(int currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        three.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = Array<TextButton*> { &one, &two, &three, &four };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i]() {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        buttons[currentSelection]->setToggleState(true, dontSendNotification);

        setSize(180, 50);
    }

private:
    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 4);
        auto buttonWidth = b.getWidth() / 4;

        one.setBounds(b.removeFromLeft(buttonWidth));
        two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        three.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    TextButton one = TextButton("-12db");
    TextButton two = TextButton("-6db");
    TextButton three = TextButton("0db");
    TextButton four = TextButton("3db");
};

class AudioOutputSettings : public Component {

public:
    AudioOutputSettings(PluginProcessor* pd)
        : limiterSettings(SettingsFile::getInstance()->getProperty<int>("limiter_threshold"))
        , oversampleSettings(SettingsFile::getInstance()->getProperty<int>("oversampling"))
    {
        addAndMakeVisible(limiterSettings);
        limiterSettings.onChange = [pd](int value) {
            pd->setLimiterThreshold(value);
        };

        addAndMakeVisible(oversampleSettings);
        oversampleSettings.onChange = [pd](int value) {
            pd->setOversampling(value);
        };

        setSize(170, 125);
    }

    ~AudioOutputSettings()
    {
        isShowing = false;
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4.0f).withTrimmedTop(24);

        limiterSettings.setBounds(bounds.removeFromTop(28));

        bounds.removeFromTop(32);
        oversampleSettings.setBounds(bounds.removeFromTop(28));
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Limiter Threshold", 0, 0, getWidth(), 24, Justification::centred);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(4, 24, getWidth() - 8, 24);

        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Oversampling", 0, 56, getWidth(), 24, Justification::centred);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(4, 84, getWidth() - 8, 84);
    }

    static void show(PluginEditor* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto audioOutputSettings = std::make_unique<AudioOutputSettings>(editor->pd);
        editor->showCalloutBox(std::move(audioOutputSettings), bounds);
    }

private:
    static inline bool isShowing = false;

    LimiterSettings limiterSettings;
    OversampleSettings oversampleSettings;
};
