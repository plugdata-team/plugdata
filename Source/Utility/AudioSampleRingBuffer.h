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

    void reset(double sourceSampleRate, int sourceBufferSize, int numChannels)
    {
        sampleRate = sourceSampleRate;
        mainBufferSize = sourceBufferSize;
        peakWindowSize = sampleRate / 60;
        bufferSize = jmax(peakWindowSize, mainBufferSize) * 3;
        
        audioBufferMutex.lock();
        peakBuffer.setSize(numChannels, peakWindowSize, true, true);
        buffer.setSize(numChannels, bufferSize, false, true);
        audioBufferMutex.unlock();
        useNewPosition = true;
    }

    void write(AudioBuffer<float>& samples)
    {
        audioBufferMutex.lock();
        for (int ch = 0; ch < peakBuffer.getNumChannels(); ch++) {
            for (int i = 0; i < samples.getNumSamples(); i++) {
                buffer.setSample(ch, (writePosition + i) % buffer.getNumSamples(), samples.getSample(ch, i));
            }
        }
        audioBufferMutex.unlock();
        
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

        while (readPos < 0)
            readPos += bufferSize;
        
        while (readPos >= buffer.getNumSamples())
            readPos -= bufferSize;

        audioBufferMutex.lock();
        for (int ch = 0; ch < std::min(2, peakBuffer.getNumChannels()); ch++) {
            for (int i = 0; i < peakWindowSize; i++) {
                peakBuffer.setSample(ch, i, buffer.getSample(ch, (readPos + i) % bufferSize));
            }
        }
        audioBufferMutex.unlock();

        Array<float> peak;
        for (int ch = 0; ch < peakBuffer.getNumChannels(); ch++) {
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
    std::mutex audioBufferMutex;
};
