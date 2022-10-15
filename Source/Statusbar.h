/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <JuceHeader.h>

struct LevelMeter;
struct MidiBlinker;
struct PlugDataAudioProcessor;

struct Statusbar : public Component, public Value::Listener, public Timer
{
    PlugDataAudioProcessor& pd;

    explicit Statusbar(PlugDataAudioProcessor& processor);
    ~Statusbar();
    
    void paint(Graphics& g) override;

    void resized() override;

    void modifierKeysChanged(const ModifierKeys& modifiers) override;

    void valueChanged(Value& v) override;
    
    void timerCallback() override;

    bool lastLockMode = false; // For restoring lock state after presentation mode
    bool wasLocked = false; // Make sure it doesn't re-lock after unlocking (because cmd is still down)
    
    LevelMeter* levelMeter;
    MidiBlinker* midiBlinker;

    std::unique_ptr<TextButton> powerButton, lockButton, connectionStyleButton, connectionPathfind, presentationButton, zoomIn, zoomOut, gridButton, themeButton, browserButton, automationButton;

    TextButton oversampleSelector;
    
    Label zoomLabel;

    Slider volumeSlider;

    Value locked;
    Value commandLocked; // Temporary lock mode
    Value presentationMode;



    static constexpr int statusbarHeight = 28;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};

struct StatusbarSource
{
    StatusbarSource();

    void processBlock(const AudioBuffer<float>& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut, int outChannels);

    void prepareToPlay(int numChannels);

    std::atomic<bool> midiReceived = false;
    std::atomic<bool> midiSent = false;
    std::atomic<float> level[2] = {0};

    int numChannels;

    Time lastMidiIn;
    Time lastMidiOut;
};
