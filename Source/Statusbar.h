/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LookAndFeel.h"
#include "Utility/SettingsFile.h"
#include "Utility/ModifierKeyListener.h"
#include "Utility/AudioSampleRingBuffer.h"
#include "Utility/Buttons.h"

class Canvas;
class LevelMeter;
class MIDIBlinker;
class CPUMeter;
class PluginProcessor;
class VolumeSlider;
class OversampleSelector;


class StatusbarSource : public Timer {

public:
    struct Listener {
        virtual void midiReceivedChanged(bool midiReceived) {};
        virtual void midiSentChanged(bool midiSent) {};
        virtual void audioProcessedChanged(bool audioProcessed) {};
        virtual void audioLevelChanged(Array<float> peak) {};
        virtual void cpuUsageChanged(float newCpuUsage) {};
        virtual void timerCallback() {};
    };

    StatusbarSource();

    void processBlock(MidiBuffer& midiIn, MidiBuffer& midiOut, int outChannels);

    void setSampleRate(double sampleRate);

    void setBufferSize(int bufferSize);

    void prepareToPlay(int numChannels);

    void timerCallback() override;

    void addListener(Listener* l);
    void removeListener(Listener* l);
    
    void setCPUUsage(float cpuUsage);

    AudioSampleRingBuffer peakBuffer;

private:
    std::atomic<int> lastMidiReceivedTime = 0;
    std::atomic<int> lastMidiSentTime = 0;
    std::atomic<int> lastAudioProcessedTime = 0;
    std::atomic<float> level[2] = { 0 };
    std::atomic<float> peakHold[2] = { 0 };
    std::atomic<float> cpuUsage;

    int peakHoldDelay[2] = { 0 };

    int numChannels;
    int bufferSize;

    double sampleRate = 44100;

    bool midiReceivedState = false;
    bool midiSentState = false;
    bool audioProcessedState = false;
    std::vector<Listener*> listeners;
};

class VolumeSlider;
class Statusbar : public Component
    , public SettingsFileListener
    , public StatusbarSource::Listener
    , public ModifierKeyListener {
    PluginProcessor* pd;

public:
    explicit Statusbar(PluginProcessor* processor);
    ~Statusbar() override;

    void paint(Graphics& g) override;

    void resized() override;

    void propertyChanged(String const& name, var const& value) override;

    void audioProcessedChanged(bool audioProcessed) override;

    bool wasLocked = false; // Make sure it doesn't re-lock after unlocking (because cmd is still down)

    std::unique_ptr<LevelMeter> levelMeter;
    std::unique_ptr<VolumeSlider> volumeSlider;
    std::unique_ptr<MIDIBlinker> midiBlinker;
    std::unique_ptr<CPUMeter> cpuMeter;


    SmallIconButton powerButton, centreButton, fitAllButton, protectButton;

    SmallIconButton overlayButton, overlaySettingsButton;

    SmallIconButton snapEnableButton, snapSettingsButton;

    SmallIconButton alignmentButton;

    std::unique_ptr<OversampleSelector> oversampleSelector;

    Label zoomLabel;

    Value showDirection;

    static constexpr int statusbarHeight = 30;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    int firstSeparatorPosition;
    int secondSeparatorPosition;
    int thirdSeparatorPosition;
    int fourthSeparatorPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};
