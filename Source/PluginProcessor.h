/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Pd/PdInstance.h"
#include "Pd/PdLibrary.h"
#include "PluginEditor.h"

class PlugDataDarkLook;

class PlugDataPluginEditor;
class PlugDataAudioProcessor : public AudioProcessor, public pd::Instance, public Timer, public PatchLoader
{
   public:
    PlugDataAudioProcessor();
    ~PlugDataAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

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
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void receiveNoteOn(const int channel, const int pitch, const int velocity) override;
    void receiveControlChange(const int channel, const int controller, const int value) override;
    void receiveProgramChange(const int channel, const int value) override;
    void receivePitchBend(const int channel, const int value) override;
    void receiveAftertouch(const int channel, const int value) override;
    void receivePolyAftertouch(const int channel, const int pitch, const int value) override;
    void receiveMidiByte(const int port, const int byte) override;

    void receiveGuiUpdate(int type) override;

    void updateConsole() override;

    void receivePrint(const std::string& message) override
    {
        if (!message.empty())
        {
            if (!message.compare(0, 6, "error:"))
            {
                const auto temp = String(message);
                logError(temp.substring(7));
            }
            else if (!message.compare(0, 11, "verbose(4):"))
            {
                const auto temp = String(message);
                logError(temp.substring(12));
            }
            else
            {
                logMessage(message);
            }
        }
    };

    void process(AudioSampleBuffer&, MidiBuffer&);

    void setBypass(bool bypass)
    {
        *enabled = !bypass;
    }

    void setCallbackLock(const CriticalSection* lock)
    {
        audioLock = lock;
    };

    const CriticalSection* getCallbackLock() override
    {
        return audioLock;
    };

    std::atomic<uint64> lastAudioCallback;

    void initialiseFilesystem();
    void saveSettings();
    void updateSearchPaths();

    void sendMidiBuffer();

    void messageEnqueued() override;

    void loadPatch(String patch) override;
    void loadPatch(File patch) override;

    void titleChanged() override;

    // All opened patches
    Array<pd::Patch> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

    AudioBuffer<float> processingBuffer;

    std::atomic<float>* volume;

    std::vector<pd::Atom> parameterAtom = std::vector<pd::Atom>(1);

    ValueTree settingsTree = ValueTree("PlugDataSettings");

    pd::Library objectLibrary;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("PlugData");
    File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");

    File settingsFile = appDir.getChildFile("Settings.xml");
    File abstractions = appDir.getChildFile("Abstractions");

    bool locked = false;
    bool commandLocked = false;

    AudioProcessorValueTreeState parameters;

    foleys::LevelMeterSource meterSource;

   private:
    void processInternal();

    std::atomic<float>* enabled;

    int audioAdvancement = 0;
    std::vector<float> audioBufferIn;
    std::vector<float> audioBufferOut;

    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    MidiBuffer midiBufferTemp;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = {0};
    size_t midiByteIndex = 0;

    std::array<std::atomic<float>*, 8> parameterValues = {nullptr};
    std::array<float, 8> lastParameters = {0};

    int minIn = 2;
    int minOut = 2;

    const CriticalSection* audioLock;

    std::unique_ptr<LookAndFeel> lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataAudioProcessor)
};
