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

struct Statusbar : public Component, public Timer, public Value::Listener
{
    PlugDataAudioProcessor& pd;

    explicit Statusbar(PlugDataAudioProcessor& processor);
    ~Statusbar();

    void resized() override;

    void modifierKeysChanged(const ModifierKeys& modifiers) override;

    void timerCallback() override;
    void valueChanged(Value& v) override;

    void zoom(bool zoomIn);
    void zoom(float zoomAmount);
    void defaultZoom();

    bool lastLockMode = false;

    LevelMeter* levelMeter;
    MidiBlinker* midiBlinker;

    std::unique_ptr<TextButton> powerButton, lockButton, connectionStyleButton, connectionPathfind, presentationButton, zoomIn, zoomOut, gridButton, themeButton, browserButton, automationButton;

    Label zoomLabel;

    Slider volumeSlider;

    Value locked;
    Value commandLocked;
    Value presentationMode;

    Value zoomScale;

    Value gridEnabled;
    Value theme;

    static constexpr int statusbarHeight = 28;
    
    ModifierKeys lastModifiers;

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
