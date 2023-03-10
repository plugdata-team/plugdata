/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "Utility/SettingsFile.h"
#include "Utility/ModifierKeyListener.h"

class Canvas;
class LevelMeter;
class MidiBlinker;
class PluginProcessor;
class gridSizeSlider;

class StatusbarSource : public Timer {

public:
    struct Listener {
        virtual void midiReceivedChanged(bool midiReceived) {};
        virtual void midiSentChanged(bool midiSent) {};
        virtual void audioProcessedChanged(bool audioProcessed) {};
        virtual void audioLevelChanged(float newLevel[2]) {};
        virtual void timerCallback() {};
    };

    StatusbarSource();

    void processBlock(AudioBuffer<float> const& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut, int outChannels);

    void prepareToPlay(int numChannels);

    void timerCallback() override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

private:
    std::atomic<int> lastMidiReceivedTime = 0;
    std::atomic<int> lastMidiSentTime = 0;
    std::atomic<int> lastAudioProcessedTime = 0;
    std::atomic<float> level[2] = { 0 };

    int numChannels;

    bool midiReceivedState = false;
    bool midiSentState = false;
    bool audioProcessedState = false;
    std::vector<Listener*> listeners;
};

class Statusbar : public Component
    , public SettingsFileListener
    , public Value::Listener
    , public StatusbarSource::Listener
    , public ModifierKeyListener {
    PluginProcessor* pd;

public:
    explicit Statusbar(PluginProcessor* processor);
    ~Statusbar();

    void lookAndFeelChanged() override;
    void paint(Graphics& g) override;

    void resized() override;

    void shiftKeyChanged(bool isHeld) override;
    void commandKeyChanged(bool isHeld) override;

    void propertyChanged(String name, var value) override;
    void valueChanged(Value& v) override;

    void attachToCanvas(Canvas* cnv);

    void audioProcessedChanged(bool audioProcessed) override;

    bool wasLocked = false; // Make sure it doesn't re-lock after unlocking (because cmd is still down)

    LevelMeter* levelMeter;
    MidiBlinker* midiBlinker;

    std::unique_ptr<TextButton> powerButton, lockButton, connectionStyleButton, connectionPathfind, presentationButton, gridButton, protectButton, directionButton, centreButton;

    TextButton oversampleSelector;

    Label zoomLabel;

    Slider volumeSlider;

    Value locked;
    Value commandLocked; // Temporary lock mode
    Value presentationMode;

    Value showDirection;

    static constexpr int statusbarHeight = 30;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};
