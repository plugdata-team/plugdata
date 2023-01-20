/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Pd/PdInstance.h"
#include "Pd/PdLibrary.h"
#include "LookAndFeel.h"
#include "Statusbar.h"

#include "Utility/InternalSynth.h"

class PlugDataLook;

class PluginEditor;
class PluginProcessor : public AudioProcessor
    , public pd::Instance
    , public Timer
    , public AudioProcessorParameter::Listener {
public:
    PluginProcessor();
    ~PluginProcessor() override;

    static AudioProcessor::BusesProperties buildBusesProperties();

    void setOversampling(int amount);
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(BusesLayout const& layouts) const override;
#endif

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
    void changeProgramName(int index, String const& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(void const* data, int sizeInBytes) override;

    void receiveNoteOn(int const channel, int const pitch, int const velocity) override;
    void receiveControlChange(int const channel, int const controller, int const value) override;
    void receiveProgramChange(int const channel, int const value) override;
    void receivePitchBend(int const channel, int const value) override;
    void receiveAftertouch(int const channel, int const value) override;
    void receivePolyAftertouch(int const channel, int const pitch, int const value) override;
    void receiveMidiByte(int const port, int const byte) override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void receiveDSPState(bool dsp) override;
    void receiveGuiUpdate(int type) override;

    void updateConsole() override;

    void synchroniseCanvas(void* cnv) override;
    void reloadAbstractions(File changedPatch, t_glist* except) override;

    void process(dsp::AudioBlock<float>, MidiBuffer&);

    void setCallbackLock(CriticalSection const* lock)
    {
        audioLock = lock;
    };

    CriticalSection const* getCallbackLock() override
    {
        return audioLock;
    };

    bool canAddBus(bool isInput) const override
    {
        return true;
    }

    bool canRemoveBus(bool isInput) const override
    {
        int nbus = getBusCount(isInput);
        return nbus > 0;
    }

    void initialiseFilesystem();
    void saveSettings();
    void updateSearchPaths();

    void sendMidiBuffer();
    void sendPlayhead();
    void sendParameters();

    void messageEnqueued() override;
    void performParameterChange(int type, int idx, float value) override;

    pd::Patch* loadPatch(String patch);
    pd::Patch* loadPatch(File const& patch);

    void titleChanged() override;

    void setTheme(String themeToUse);

    Colour getForegroundColour() override;
    Colour getBackgroundColour() override;
    Colour getTextColour() override;
    Colour getOutlineColour() override;

    // All opened patches
    OwnedArray<pd::Patch> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

    std::vector<float*> channelPointers;
    std::atomic<float>* volume;

    ValueTree settingsTree = ValueTree("plugdatasettings");

    pd::Library objectLibrary;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");
    File versionDataDir = homeDir.getChildFile(ProjectInfo::versionString);

    File settingsFile = homeDir.getChildFile("Settings.xml");
    File abstractions = versionDataDir.getChildFile("Abstractions");

    Value commandLocked = Value(var(false));

    AudioProcessorValueTreeState parameters;

    StatusbarSource statusbarSource;

    Value tailLength = Value(0.0f);

    SharedResourcePointer<PlugDataLook> lnf;

    static inline constexpr int numParameters = 512;
    static inline constexpr int numInputBuses = 16;
    static inline constexpr int numOutputBuses = 16;

    // Zero means no oversampling
    int oversampling = 0;
    int lastTab = -1;

    bool settingsChangedInternally = false;

#if PLUGDATA_STANDALONE
    std::atomic<float> standaloneParams[numParameters] = { 0 };
    OwnedArray<MidiOutput> midiOutputs;

    InternalSynth internalSynth;
    std::atomic<bool> enableInternalSynth = false;
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

    std::array<float, numParameters> lastParameters = { 0 };
    std::array<float, numParameters> changeGestureState = { 0 };

    std::vector<pd::Atom> atoms_playhead;

    int minIn = 2;
    int minOut = 2;

    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    CriticalSection const* audioLock;

    static inline const String else_version = "ELSE v1.0-rc6";
    static inline const String cyclone_version = "cyclone v0.6-1";
    // this gets updated with live version data later
    static String pdlua_version;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
