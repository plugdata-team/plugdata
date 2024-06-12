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
        ScopedLock lock(audioBufferMutex);
        
        sampleRate = sourceSampleRate;
        mainBufferSize = sourceBufferSize;
        peakWindowSize = sampleRate / 60;
        bufferSize = jmax(peakWindowSize, mainBufferSize) * 3;

        peakBuffer.setSize(numChannels, peakWindowSize, true, true);
        buffer.setSize(numChannels, bufferSize, false, true);
        useNewPosition = true;
    }

    void write(AudioBuffer<float>& samples)
    {
        ScopedLock lock(audioBufferMutex);
        
        for (int ch = 0; ch < peakBuffer.getNumChannels(); ch++) {
            for (int i = 0; i < samples.getNumSamples(); i++) {
                buffer.setSample(ch, (writePosition + i) % buffer.getNumSamples(), samples.getSample(ch, i));
            }
        }

        writeTime = Time::getMillisecondCounter();
        oldWritePosition = writePosition;
        writePosition = (writePosition + samples.getNumSamples()) % buffer.getNumSamples();
        useNewPosition = true;
    }

    Array<float> getPeak()
    {
        ScopedLock lock(audioBufferMutex);
        
        if (sampleRate == 0)
            return { 0.0f, 0.0f };
        auto currentTime = Time::getMillisecondCounter();

        if (useNewPosition) {
            readTime = writeTime;
            readPosition = oldWritePosition;
            useNewPosition = false;
        }

        auto diff = currentTime - readTime;

        int readPos = readPosition + std::ceil((diff / 1000) * sampleRate) - mainBufferSize - peakWindowSize;

        while (readPos < 0)
            readPos += bufferSize;

        while (readPos >= buffer.getNumSamples())
            readPos -= bufferSize;

        for (int ch = 0; ch < std::min(2, peakBuffer.getNumChannels()); ch++) {
            // Get pointers to the source and destination data
            auto const* srcData = buffer.getReadPointer(ch);
            auto* destData = peakBuffer.getWritePointer(ch);

            int readPosWrapped = (readPos + peakWindowSize) % bufferSize;
            if (readPosWrapped < peakWindowSize) {
                // Calculate the length of the first copy operation
                int firstCopyLength = bufferSize - readPos;

                // Perform the first copy operation
                juce::FloatVectorOperations::copy(destData, srcData + readPos, firstCopyLength);

                // Perform the second copy operation
                juce::FloatVectorOperations::copy(destData + firstCopyLength, srcData, peakWindowSize - firstCopyLength);
            } else {
                // If no wrap-around is needed, perform a single SIMD copy operation
                juce::FloatVectorOperations::copy(destData, srcData + readPos, peakWindowSize);
            }
        }

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
    int oldWritePosition = 0;
    int readPosition = 0;
    bool useNewPosition = false;
    double writeTime = 0;
    double readTime = 0;
    CriticalSection audioBufferMutex;
};
