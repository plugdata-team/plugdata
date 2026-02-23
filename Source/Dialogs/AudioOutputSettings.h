/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "Components/DraggableNumber.h"
#include "PluginEditor.h"

class OversampleSettings final : public Component {
public:
    std::function<void(int)> onChange = [](int) { };

    explicit OversampleSettings(int const currentSelection)
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
            button->onClick = [this, i] {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
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
        auto const buttonWidth = b.getWidth() / 4;

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

class LimiterSettings final : public Component {
public:
    std::function<void(int)> onChange = [](int) { };

    explicit LimiterSettings(int const currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        three.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = SmallArray<TextButton*> { &one, &two, &three, &four };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i] {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
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
        auto const buttonWidth = b.getWidth() / 4;

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

class AudioOutputSettings final : public Component {

public:
    enum Type {
        Limiter,
        Oversampling
    };

    AudioOutputSettings(PluginProcessor* pd, AudioOutputSettings::Type const typeToShow, std::function<void()> const& changeCallback)
        : limiterSettings(SettingsFile::getInstance()->getProperty<int>("limiter_threshold"))
        , oversampleSettings(std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3))
        , type(typeToShow)
        , onChange(changeCallback)
    {
        if (type == Limiter) {
            addAndMakeVisible(limiterSettings);
            limiterSettings.onChange = [this, pd](int const value) {
                pd->setLimiterThreshold(value);
                onChange();
            };
        } else {
            addAndMakeVisible(oversampleSettings);
            oversampleSettings.onChange = [this, pd](int const value) {
                pd->setOversampling(value);
                onChange();
            };
        }

        setSize(170, 60);
    }

    ~AudioOutputSettings() override
    {
        isShowing = false;
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4.0f).withTrimmedTop(24);

        if (type == Limiter) {
            limiterSettings.setBounds(bounds.removeFromTop(28));
        } else {
            oversampleSettings.setBounds(bounds.removeFromTop(28));
        }
    }

    void paint(Graphics& g) override
    {
        if (type == Limiter) {
            g.setColour(PlugDataColours::popupMenuTextColour);
            g.setFont(Fonts::getBoldFont().withHeight(15));
            g.drawText("Limiter Threshold", 0, 0, getWidth(), 24, Justification::centred);
        } else {
            g.setColour(PlugDataColours::popupMenuTextColour);
            g.setFont(Fonts::getBoldFont().withHeight(15));
            g.drawText("Oversampling", 0, 0, getWidth(), 24, Justification::centred);
        }

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawLine(4, 24, getWidth() - 8, 24);
    }

    static void show(PluginEditor* editor, Rectangle<int> const bounds, AudioOutputSettings::Type typeToShow, std::function<void()> changeCallback = [] { })
    {
        if (isShowing)
            return;

        isShowing = true;

        auto audioOutputSettings = std::make_unique<AudioOutputSettings>(editor->pd, typeToShow, changeCallback);
        editor->showCalloutBox(std::move(audioOutputSettings), bounds);
    }

private:
    static inline bool isShowing = false;
    LimiterSettings limiterSettings;
    OversampleSettings oversampleSettings;
    Type type;
    std::function<void()> onChange;
};
