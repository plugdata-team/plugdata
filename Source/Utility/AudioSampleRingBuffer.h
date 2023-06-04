#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

/*
                    read                        write
                    (oldwrite pos - window size │
                    │                           │
                    ├───┐                       ├───────────────────────┐
                    │   │                       │                       │
                    ▼   ▼                       ▼                       ▼
┌───────────────────────┬───────────────────────┬───────────────────────┐
│A                      │B                      │C                      │
└───────────────────────┴───────────────────────┴───────────────────────┘
*/

class AudioSampleRingBuffer {
public:
    AudioSampleRingBuffer()
    {
    }

    void reset(double sourceSampleRate, int sourceBufferSize)
    {
        sampleRate = sourceSampleRate;
        mainBufferSize = sourceBufferSize;
        peakWindowSize = sampleRate / 60;
        bufferSize = jmax(peakWindowSize, mainBufferSize) * 3;
        peakBuffer.setSize(2, peakWindowSize, true, true);
        buffer.setSize(2, bufferSize, false, true);
    }

    void write(AudioBuffer<float>& samples)
    {
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < samples.getNumSamples(); i++) {
                buffer.setSample(ch, (writePosition + i) % buffer.getNumSamples(), samples.getSample(ch, i));
            }
        }
        writeTime.store(Time::getMillisecondCounterHiRes());
        oldWritePosition.store(writePosition);
        writePosition = (writePosition + samples.getNumSamples()) % buffer.getNumSamples();
        useNewPosition = true;
    }

    Array<float> getPeak()
    {
        if (sampleRate == 0)
            return { 0.0f, 0.0f };
        auto currentTime = Time::getMillisecondCounterHiRes();

        if (useNewPosition) {
            readTime = writeTime.load();
            readPosition = oldWritePosition.load();
            useNewPosition = false;
        }

        auto diff = currentTime - readTime;

        int readPos = readPosition + std::ceil((diff / 1000) * sampleRate) - mainBufferSize - peakWindowSize;

        if (readPos < 0)
            readPos += bufferSize;

        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < peakWindowSize; i++) {
                peakBuffer.setSample(ch, i, buffer.getSample(ch, (readPos + i) % bufferSize));
            }
        }

        Array<float> peak;
        for (int ch = 0; ch < 2; ch++) {
            peak.add(pow(peakBuffer.getMagnitude(ch, 0, peakWindowSize), 0.5f));
        }
        return peak;
    }

private:
    int bufferSize = 0;
    int mainBufferSize = 0;
    int sampleRate = 0;
    int peakWindowSize = 0;

    AudioBuffer<float> buffer;
    AudioBuffer<float> peakBuffer;

    int writePosition = 0;
    std::atomic<int> oldWritePosition = 0;
    int readPosition = 0;
    std::atomic<bool> useNewPosition = false;
    std::atomic<double> writeTime = 0;
    std::atomic<double> readTime = 0;
};
