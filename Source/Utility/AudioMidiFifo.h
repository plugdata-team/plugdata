/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/
#pragma once

class AudioMidiFifo {
public:
    AudioMidiFifo(int const channels, int const maxSize)
    {
        setSize(channels, maxSize);
    }

    void setSize(int const channels, int const maxSize)
    {
        fifo.setTotalSize(maxSize + 1);
        audioBuffer.setSize(channels, maxSize + 1);

        clear();
    }

    void clear()
    {
        fifo.reset();
        audioBuffer.clear();
        midiBuffer.clear();
    }

    int getNumSamplesAvailable() const { return fifo.getNumReady(); }
    int getNumSamplesFree() const { return fifo.getFreeSpace(); }

    void writeAudioAndMidi(dsp::AudioBlock<float> const& audioSrc, MidiBuffer const& midiSrc)
    {
        jassert(getNumSamplesFree() >= audioSrc.getNumSamples());
        jassert(audioSrc.getNumChannels() == audioBuffer.getNumChannels());

        midiBuffer.addEvents(midiSrc, 0, audioSrc.getNumSamples(), fifo.getNumReady());

        int start1, size1, start2, size2;
        fifo.prepareToWrite(audioSrc.getNumSamples(), start1, size1, start2, size2);

        if (size1 > 0)
            audioSrc.copyTo(audioBuffer, 0, start1, size1);
        if (size2 > 0)
            audioSrc.copyTo(audioBuffer, size1, start2, size2);

        fifo.finishedWrite(size1 + size2);
    }

    void readAudioAndMidi(dsp::AudioBlock<float>& audioDst, MidiBuffer& midiDst)
    {
        jassert(getNumSamplesAvailable() >= audioDst.getNumSamples());
        jassert(audioDst.getNumChannels() == audioBuffer.getNumChannels());

        midiDst.addEvents(midiBuffer, 0, audioDst.getNumSamples(), 0);

        // Move all the remaining midi events forward by the number of samples removed
        MidiBuffer temp;
        temp.addEvents(midiBuffer, audioDst.getNumSamples(), fifo.getNumReady(), -audioDst.getNumSamples());
        midiBuffer = temp;

        int start1, size1, start2, size2;
        fifo.prepareToRead(audioDst.getNumSamples(), start1, size1, start2, size2);

        if (size1 > 0)
            audioDst.copyFrom(audioBuffer, start1, 0, size1);
        if (size2 > 0)
            audioDst.copyFrom(audioBuffer, start2, size1, size2);

        fifo.finishedRead(size1 + size2);
    }

    void writeSilence(int const numSamples)
    {
        jassert(getNumSamplesFree() >= numSamples);

        int start1, size1, start2, size2;
        fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

        if (size1 > 0)
            audioBuffer.clear(start1, size1);
        if (size2 > 0)
            audioBuffer.clear(start2, size2);

        fifo.finishedWrite(size1 + size2);
    }

    void writeAudioAndMidi(juce::AudioBuffer<float> const& audioSrc, juce::MidiBuffer const& midiSrc)
    {
        jassert(getNumSamplesFree() >= audioSrc.getNumSamples());
        jassert(audioSrc.getNumChannels() == audioBuffer.getNumChannels());

        midiBuffer.addEvents(midiSrc, 0, audioSrc.getNumSamples(), fifo.getNumReady());

        int start1, size1, start2, size2;
        fifo.prepareToWrite(audioSrc.getNumSamples(), start1, size1, start2, size2);

        int const channels = juce::jmin(audioBuffer.getNumChannels(), audioSrc.getNumChannels());
        for (int ch = 0; ch < channels; ch++) {
            if (size1 > 0)
                audioBuffer.copyFrom(ch, start1, audioSrc, ch, 0, size1);
            if (size2 > 0)
                audioBuffer.copyFrom(ch, start2, audioSrc, ch, size1, size2);
        }

        fifo.finishedWrite(size1 + size2);
    }

    void readAudioAndMidi(juce::AudioBuffer<float>& audioDst, juce::MidiBuffer& midiDst)
    {
        jassert(getNumSamplesAvailable() >= audioDst.getNumSamples());
        jassert(audioDst.getNumChannels() == audioBuffer.getNumChannels());

        midiDst.addEvents(midiBuffer, 0, audioDst.getNumSamples(), 0);

        // Move all the remaining midi events forward by the number of samples removed
        juce::MidiBuffer temp;
        temp.addEvents(midiBuffer, audioDst.getNumSamples(), fifo.getNumReady(), -audioDst.getNumSamples());
        midiBuffer = temp;

        int start1, size1, start2, size2;
        fifo.prepareToRead(audioDst.getNumSamples(), start1, size1, start2, size2);

        int const numCh = juce::jmin(audioBuffer.getNumChannels(), audioDst.getNumChannels());
        for (int ch = 0; ch < numCh; ch++) {
            if (size1 > 0)
                audioDst.copyFrom(ch, 0, audioBuffer, ch, start1, size1);
            if (size2 > 0)
                audioDst.copyFrom(ch, size1, audioBuffer, ch, start2, size2);
        }

        fifo.finishedRead(size1 + size2);
    }

private:
    AbstractFifo fifo { 1 };
    AudioBuffer<float> audioBuffer;
    MidiBuffer midiBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMidiFifo)
};
