/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <JuceHeader.h>

#include <memory>

struct LevelMeter;
struct MidiBlinker;
struct PlugDataAudioProcessor;

struct Statusbar : public Component, public Timer, public KeyListener
{
    PlugDataAudioProcessor& pd;

    Statusbar(PlugDataAudioProcessor& processor);
    ~Statusbar();

    void resized() override;

    void timerCallback() override;

    bool keyPressed(const KeyPress& key, Component*) override;
    bool keyStateChanged(bool isKeyDown, Component*) override;

    void zoom(bool zoomIn);
    void zoom(float zoomAmount);

    LevelMeter* levelMeter;
    MidiBlinker* midiBlinker;

    std::unique_ptr<TextButton> bypassButton;
    std::unique_ptr<TextButton> lockButton;
    std::unique_ptr<TextButton> connectionStyleButton;
    std::unique_ptr<TextButton> connectionPathfind;

    std::unique_ptr<TextButton> zoomIn;
    std::unique_ptr<TextButton> zoomOut;

    Label zoomLabel;

    Slider volumeSlider;

    Value locked;
    Value commandLocked;

    Value zoomScale;

    Value connectionStyle;

    static constexpr int statusbarHeight = 25;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};

struct StatusbarSource
{
    StatusbarSource();

    void processBlock(const AudioBuffer<float>& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut);

    void prepareToPlay(int numChannels);

    std::atomic<bool> midiReceived;
    std::atomic<bool> midiSent;
    std::atomic<float> level[2];

    int numChannels;

    Time lastMidiIn;
    Time lastMidiOut;
};
