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

struct Statusbar : public Component, public Timer, public KeyListener, public ChangeListener
{
    PlugDataAudioProcessor& pd;

    explicit Statusbar(PlugDataAudioProcessor& processor);
    ~Statusbar();
    
    void changeListenerCallback(ChangeBroadcaster* source) override;

    void resized() override;

    void timerCallback() override;

    bool keyPressed(const KeyPress& k, Component*) override
    {
        return false;
    };
    bool keyStateChanged(bool isKeyDown, Component*) override;

    void zoom(bool zoomIn);
    void zoom(float zoomAmount);
    void defaultZoom();

    LevelMeter* levelMeter;
    MidiBlinker* midiBlinker;

    std::unique_ptr<TextButton> bypassButton, lockButton, connectionStyleButton, connectionPathfind, presentationButton, zoomIn, zoomOut, backgroundColour;
    
    
    Label zoomLabel;

    Slider volumeSlider;

    Value locked;
    Value commandLocked;
    Value presentationMode;

    Value zoomScale;

    Value connectionStyle;

    static constexpr int statusbarHeight = 28;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};

struct StatusbarSource
{
    StatusbarSource();

    void processBlock(const AudioBuffer<float>& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut);

    void prepareToPlay(int numChannels);

    std::atomic<bool> midiReceived = false;
    std::atomic<bool> midiSent = false;
    std::atomic<float> level[2] = {0};

    int numChannels;

    Time lastMidiIn;
    Time lastMidiOut;
};
