/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Pd/PdInstance.h"
#include "Pd/PdLibrary.h"
#include "Standalone/PlugDataWindow.h"
#include "Statusbar.h"

class PlugDataLook;

class PlugDataPluginEditor;
class PlugDataAudioProcessor : public AudioProcessor, public pd::Instance, public Timer
{
public:
    PlugDataAudioProcessor();
    ~PlugDataAudioProcessor() override;

    static AudioProcessor::BusesProperties buildBusesProperties();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    std::atomic<int> callbackType = 0;
    void timerCallback() override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void receiveNoteOn (const int channel, const int pitch, const int velocity) override;
    void receiveControlChange (const int channel, const int controller, const int value) override;
    void receiveProgramChange (const int channel, const int value) override;
    void receivePitchBend (const int channel, const int value) override;
    void receiveAftertouch (const int channel, const int value) override;
    void receivePolyAftertouch (const int channel, const int pitch, const int value) override;
    void receiveMidiByte (const int port, const int byte) override;

    void receiveParameter (int idx, float value) override;

    void receiveDSPState (bool dsp) override;
    void receiveGuiUpdate (int type) override;

    void updateConsole() override;

    void synchroniseCanvas (void* cnv) override;

    void receivePrint (const String& message) override
    {
        if (message.isNotEmpty())
        {
            if (! message.startsWith ("error:"))
            {
                logError (message.substring (7));
            }
            else if (! message.startsWith ("verbose(4):"))
            {
                logError (message.substring (12));
            }
            else
            {
                logMessage (message);
            }
        }
    };

    void process (AudioSampleBuffer&, MidiBuffer&);

    void setCallbackLock (const CriticalSection* lock)
    {
        audioLock = lock;
    };

    const CriticalSection* getCallbackLock() override
    {
        return audioLock;
    };

    bool canAddBus (bool isInput) const override
    {
        return true;
    }

    bool canRemoveBus (bool isInput) const override
    {
        int nbus = getBusCount (isInput);
        return nbus > 0;
    }

    void initialiseFilesystem();
    void saveSettings();
    void updateSearchPaths();

    void sendMidiBuffer();
    void sendPlayhead();
    void sendParameters();

    void messageEnqueued() override;

    pd::Patch* loadPatch (String patch);
    pd::Patch* loadPatch (const File& patch);

    void titleChanged() override;

    void setTheme (bool themeToUse);

    Colour getForegroundColour() override;
    Colour getBackgroundColour() override;
    Colour getTextColour() override;
    Colour getOutlineColour() override;

    // All opened patches
    OwnedArray<pd::Patch> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

    AudioBuffer<float> processingBuffer;

    std::atomic<float>* volume;

    std::vector<pd::Atom> parameterAtom = std::vector<pd::Atom> (1);

    ValueTree settingsTree = ValueTree ("PlugDataSettings");

    pd::Library objectLibrary;

    File homeDir = File::getSpecialLocation (File::SpecialLocationType::userApplicationDataDirectory).getChildFile ("PlugData");
    File appDir = homeDir.getChildFile ("Library");

    File settingsFile = homeDir.getChildFile ("Settings.xml");
    File abstractions = appDir.getChildFile ("Abstractions");

    Value locked = Value (var (false));
    Value commandLocked = Value (var (false));
    Value zoomScale = Value (1.0f);

    AudioProcessorValueTreeState parameters;

    StatusbarSource statusbarSource;

    Value tailLength = Value (0.0f);

    SharedResourcePointer<PlugDataLook> lnf;

    static inline constexpr int numParameters = 512;
    static inline constexpr int numInputBuses = 16;
    static inline constexpr int numOutputBuses = 16;

#if PLUGDATA_STANDALONE
    std::atomic<float> standaloneParams[numParameters] = { 0 };
#endif

private:
    void processInternal();

    int audioAdvancement = 0;
    std::vector<float> audioBufferIn;
    std::vector<float> audioBufferOut;

    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    MidiBuffer midiBufferTemp;
    MidiBuffer midiBufferCopy;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = { 0 };
    size_t midiByteIndex = 0;

    std::array<std::atomic<float>*, numParameters> parameterValues = { nullptr };
    std::array<float, numParameters> lastParameters = { 0 };

    std::vector<pd::Atom> atoms_playhead;

    int minIn = 2;
    int minOut = 2;

    const CriticalSection* audioLock;

#if ! PLUGDATA_STANDALONE

    // Timer for grouping change messages when informing the DAW
    struct ParameterTimer : public Timer
    {
        RangedAudioParameter* parameter;

        void notifyChange (RangedAudioParameter* param)
        {
            if (! isTimerRunning())
            {
                parameter = param;
                parameter->beginChangeGesture();
            }
            // Reset timer
            startTimer (500);
        }

        void timerCallback() override
        {
            parameter->endChangeGesture();
            stopTimer();
        }
    };

    ParameterTimer parameterTimers[numParameters];
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlugDataAudioProcessor)
};
