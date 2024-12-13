#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

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

    void write(AudioBuffer<float>& samples)
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
