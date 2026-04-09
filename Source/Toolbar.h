/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <readerwriterqueue.h>

#include "LookAndFeel.h"
#include "Utility/SettingsFile.h"
#include "Components/Buttons.h"

class Canvas;
class VolumeComponent;
class MIDIBlinker;
class CPUMeter;
class PluginProcessor;
class LatencyDisplayButton;
class LimiterButton;
class StatusBadge;
class PowerButton;

class ToolbarSource final : public Timer {

public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void midiReceivedChanged(bool midiReceived) { ignoreUnused(midiReceived); }
        virtual void midiSentChanged(bool midiSent) { ignoreUnused(midiSent); }
        virtual void midiMessageReceived(MidiMessage const& message) { ignoreUnused(message); }
        virtual void midiMessageSent(MidiMessage const& message) { ignoreUnused(message); }
        virtual void audioProcessedChanged(bool audioProcessed) { ignoreUnused(audioProcessed); }
        virtual void audioLevelChanged(SmallArray<float> peak) { ignoreUnused(peak); }
        virtual void cpuUsageChanged(float newCpuUsage) { ignoreUnused(newCpuUsage); }
        virtual void recordingTimeChanged(float newRecordingTime) { ignoreUnused(newRecordingTime); }
        virtual void timerCallback() { }
    };

    ToolbarSource();

    void process(MidiBuffer const& midiInput, MidiBuffer const& midiOutput);

    void setSampleRate(double sampleRate);

    void setBufferSize(int bufferSize);

    void prepareToPlay(int numChannels);

    void timerCallback() override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

    void setCPUUsage(float cpuUsage);

    void setRecorderTime(float time);

    class AudioPeakMeter {
    public:
        AudioPeakMeter() = default;

        void reset(double const sourceSampleRate, int const numChannels)
        {
            // Calculate the number of samples per frame
            maxBuffersPerFrame = 1.0f / 30.0f / (64.f / sourceSampleRate);

            sampleQueue.clear();
            for (int ch = 0; ch < numChannels; ch++) {
                sampleQueue.emplace_back(maxBuffersPerFrame * 4);
            }

            lastPeak.resize(numChannels, 0);
        }

        void write(AudioBuffer<float> const& samples)
        {
            for (int ch = 0; ch < std::min<int>(sampleQueue.size(), samples.getNumChannels()); ch++) {
                int index = 0;
                StackArray<float, 64> sampleBuffer;
                for (int i = 0; i < samples.getNumSamples(); i++) {
                    sampleBuffer[index] = samples.getSample(ch, i);
                    index++;
                    if (index >= 64) {
                        if (!sampleQueue[ch].try_enqueue(sampleBuffer)) {
                            break;
                        }
                        index = 0;
                    }
                }
            }
        }

        SmallArray<float> getPeak()
        {
            SmallArray<float> peak;
            peak.resize(sampleQueue.size());

            for (int ch = 0; ch < sampleQueue.size(); ch++) {
                StackArray<float, 64> sampleBuffer;
                float magnitude = 0.0f;
                int foundSamples = 0;
                while (sampleQueue[ch].try_dequeue(sampleBuffer)) {
                    for (int i = 0; i < 64; i++) {
                        magnitude = std::max(magnitude, sampleBuffer[i]);
                    }
                    // We receive about 22 buffers per frame, so stop after that so we don't starve the next dequeue
                    // Only keep on reading if we need to "Catch up"
                    if (foundSamples > maxBuffersPerFrame && sampleQueue[ch].size_approx() < maxBuffersPerFrame / 2)
                        break;
                    foundSamples++;
                }
                peak[ch] = foundSamples ? magnitude : lastPeak[ch];
                lastPeak[ch] = peak[ch];
            }

            return peak;
        }

    private:
        SmallArray<float> lastPeak;
        HeapArray<moodycamel::ReaderWriterQueue<StackArray<float, 64>>> sampleQueue;
        int maxBuffersPerFrame;
    };

    AudioPeakMeter peakBuffer;

private:
    AtomicValue<int, Relaxed> lastMidiReceivedTime = 0;
    AtomicValue<int, Relaxed> lastMidiSentTime = 0;
    AtomicValue<int, Relaxed> lastAudioProcessedTime = 0;
    AtomicValue<float, Relaxed> cpuUsage;
    AtomicValue<float, Relaxed> recorderTime;

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
class AudioToolbar final : public Component
    , public ToolbarSource::Listener {
public:
    AudioToolbar(PluginProcessor* processor, PluginEditor* editor);

    ~AudioToolbar() override;

    void showDSPState(bool const dspState);
    void showLimiterState(bool enabled);
    void updateOversampling();
    void setLatencyDisplay(int samples);

    void recordingTimeChanged(float time) override;

    void lookAndFeelChanged() override;
    void resized() override;

private:
    PluginProcessor* pd;

    std::unique_ptr<CPUMeter> cpuMeter;
    std::unique_ptr<MIDIBlinker> midiBlinker;
    std::unique_ptr<VolumeComponent> volumeComponent;
    std::unique_ptr<PowerButton> powerButton;
    std::unique_ptr<LimiterButton> limiterButton;

    std::unique_ptr<SliderParameterAttachment> volumeAttachment;
    std::unique_ptr<StatusBadge> oversamplingBadge, dawLatencyBadge, recordingBadge;

#if JUCE_MAC
    struct ToolbarDragListener : public MouseListener {
        ToolbarDragListener(Component* parent)
            : parent(parent)
        {
            parent->addMouseListener(this, true);
        }

        void mouseEnter(MouseEvent const& e) override;
        void mouseExit(MouseEvent const& e) override;
        Component* parent;
    };
    ToolbarDragListener toolbarDragListener = ToolbarDragListener(this);
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToolbar)
};
