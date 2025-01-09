/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <readerwriterqueue.h>

#include "LookAndFeel.h"
#include "Utility/SettingsFile.h"
#include "Utility/ModifierKeyListener.h"
#include "Utility/AudioPeakMeter.h"
#include "Components/Buttons.h"

class Canvas;
class LevelMeter;
class MIDIBlinker;
class CPUMeter;
class PluginProcessor;
class VolumeSlider;
class LatencyDisplayButton;
class CommandButton;
class StatusbarTextButton;

class StatusbarSource final : public Timer {

public:
    struct Listener {
        virtual void midiReceivedChanged(bool midiReceived) { ignoreUnused(midiReceived); }
        virtual void midiSentChanged(bool midiSent) { ignoreUnused(midiSent); }
        virtual void midiMessageReceived(MidiMessage const& message) { ignoreUnused(message); }
        virtual void midiMessageSent(MidiMessage const& message) { ignoreUnused(message); }
        virtual void audioProcessedChanged(bool audioProcessed) { ignoreUnused(audioProcessed); }
        virtual void audioLevelChanged(SmallArray<float> peak) { ignoreUnused(peak); }
        virtual void cpuUsageChanged(float newCpuUsage) { ignoreUnused(newCpuUsage); }
        virtual void timerCallback() { }
    };

    StatusbarSource();

    void process(MidiBuffer const& midiInput, MidiBuffer const& midiOutput, int outChannels);

    void setSampleRate(double sampleRate);

    void setBufferSize(int bufferSize);

    void prepareToPlay(int numChannels);

    void timerCallback() override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

    void setCPUUsage(float cpuUsage);

    AudioPeakMeter peakBuffer;

private:
    AtomicValue<int, Relaxed> lastMidiReceivedTime = 0;
    AtomicValue<int, Relaxed> lastMidiSentTime = 0;
    AtomicValue<int, Relaxed> lastAudioProcessedTime = 0;
    AtomicValue<float, Relaxed> cpuUsage;

    moodycamel::ReaderWriterQueue<MidiMessage> lastMidiSent;
    moodycamel::ReaderWriterQueue<MidiMessage> lastMidiReceived;

    int bufferSize;

    double sampleRate = 44100;

    bool midiReceivedState = false;
    bool midiSentState = false;
    bool audioProcessedState = false;
    HeapArray<Listener*> listeners;
};

class VolumeSlider;
class ZoomLabel;
class Statusbar final : public Component
    , public AsyncUpdater
    , public StatusbarSource::Listener
    , public ModifierKeyListener {
    PluginProcessor* pd;
    PluginEditor* editor;

public:
    explicit Statusbar(PluginProcessor* processor, PluginEditor* editor);
    ~Statusbar() override;

    void paint(Graphics& g) override;

    void resized() override;

    void audioProcessedChanged(bool audioProcessed) override;

    void setLatencyDisplay(int value);
    void updateZoomLevel();

    void showDSPState(bool dspState);
    void setHasActiveCanvas(bool hasActiveCanvas);

    static constexpr int statusbarHeight = 30;

    void showCommandInput();

    void setCommandButtonText(String& text);

    void setWelcomePanelShown(bool isShowing);

private:
    void mouseDown(MouseEvent const& e) override;

    void handleAsyncUpdate() override;

    std::unique_ptr<LevelMeter> levelMeter;
    std::unique_ptr<VolumeSlider> volumeSlider;
    std::unique_ptr<MIDIBlinker> midiBlinker;
    std::unique_ptr<CPUMeter> cpuMeter;

    SmallIconButton zoomComboButton, centreButton;
    SmallIconButton overlayButton, overlaySettingsButton;
    SmallIconButton snapEnableButton, snapSettingsButton;
    SmallIconButton powerButton;
    SmallIconButton sidebarExpandButton, helpButton;
    Label plugdataString;
    std::unique_ptr<CommandButton> commandInputButton;

    SafePointer<CallOutBox> commandInputCallout;

    std::unique_ptr<StatusbarTextButton> limiterButton;
    std::unique_ptr<StatusbarTextButton> oversampleButton;

    std::unique_ptr<LatencyDisplayButton> latencyDisplayButton;

    std::unique_ptr<ZoomLabel> zoomLabel;

    float currentZoomLevel = 100.f;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;
    std::unique_ptr<SliderParameterAttachment> volumeAttachment;

    float firstSeparatorPosition, secondSeparatorPosition;
    bool welcomePanelIsShown = true;

    friend class ZoomLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};
