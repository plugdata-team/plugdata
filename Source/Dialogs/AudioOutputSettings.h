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

class AudioSettingsCallout final : public Component {
public:
    explicit AudioSettingsCallout(PluginEditor* editor)
        : pd(editor->pd)
        , limiterSettings(SettingsFile::getInstance()->getProperty<int>("limiter_threshold"))
        , oversampleSettings(std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3))
    {
        limiterLabel.setText("Limiter", dontSendNotification);
        limiterLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        addAndMakeVisible(limiterLabel);

        limiterSettings.onChange = [this](int const value) {
            pd->setLimiterThreshold(value);
        };
        addAndMakeVisible(limiterSettings);

        oversamplingLabel.setText("Oversampling", dontSendNotification);
        oversamplingLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        addAndMakeVisible(oversamplingLabel);

        oversampleSettings.onChange = [this, editor](int const value) {
            pd->setOversampling(value);
            editor->audioToolbar->updateOversampling();
        };
        addAndMakeVisible(oversampleSettings);

        audioSettingsButton.setButtonText("Audio Settings...");
        audioSettingsButton.onClick = [this, editor] {
            Dialogs::showSettingsDialog(editor);
            closeCalloutBox();
        };

        audioSettingsButton.setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
        audioSettingsButton.setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
        audioSettingsButton.setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
        audioSettingsButton.setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
        audioSettingsButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        addAndMakeVisible(audioSettingsButton);

        setSize(200, 146);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(10, 8);
        constexpr int labelHeight = 18;
        constexpr int selectorHeight = 28;
        constexpr int gap = 6;

        limiterLabel.setBounds(b.removeFromTop(labelHeight));
        limiterSettings.setBounds(b.removeFromTop(selectorHeight));
        b.removeFromTop(gap);

        oversamplingLabel.setBounds(b.removeFromTop(labelHeight));
        oversampleSettings.setBounds(b.removeFromTop(selectorHeight));
        b.removeFromTop(gap + 4);

        audioSettingsButton.setBounds(b.removeFromTop(selectorHeight - 4));
    }

    void closeCalloutBox()
    {
        MessageManager::callAsync([_callout = SafePointer(findParentComponentOfClass<CallOutBox>())]() {
            if (_callout)
                _callout->dismiss();
        });
    }

private:
    PluginProcessor* pd;

    Label limiterLabel;
    LimiterSettings limiterSettings;

    Label oversamplingLabel;
    OversampleSettings oversampleSettings;

    TextButton audioSettingsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsCallout)
};

