/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include "Utility/Config.h"

#include "Pd/Instance.h"
#include "Pd/Patch.h"

namespace pd {
class Library;
}

class InternalSynth;
class SettingsFile;
class StatusbarSource;
class PlugDataLook;
class PluginEditor;
class PluginProcessor : public AudioProcessor
    , public pd::Instance {
public:
    PluginProcessor();

    ~PluginProcessor();

    static AudioProcessor::BusesProperties buildBusesProperties();

    void setOversampling(int amount);
    void setProtectedMode(bool enabled);
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
    void receiveSysMessage(String const& selector, std::vector<pd::Atom> const& list) override;

    void updateDrawables() override;

    void updateConsole() override;

    void reloadAbstractions(File changedPatch, t_glist* except) override;

    void process(dsp::AudioBlock<float>, MidiBuffer&);

    bool canAddBus(bool isInput) const override
    {
        return true;
    }

    bool canRemoveBus(bool isInput) const override
    {
        int nbus = getBusCount(isInput);
        return nbus > 0;
    }

    void savePatchTabPositions();

    void initialiseFilesystem();
    void updateSearchPaths();

    void sendMidiBuffer();
    void sendPlayhead();
    void sendParameters();

    bool isInPluginMode();

    void messageEnqueued() override;
    void performParameterChange(int type, String name, float value) override;

    // Jyg added this
    void fillDataBuffer(std::vector<pd::Atom> const& list) override;
    void parseDataBuffer(XmlElement const& xml) override;
    XmlElement* m_temp_xml;

    pd::Patch::Ptr loadPatch(String patch);
    pd::Patch::Ptr loadPatch(File const& patch);

    void titleChanged() override;

    void setTheme(String themeToUse, bool force = false);

    Colour getForegroundColour() override;
    Colour getBackgroundColour() override;
    Colour getTextColour() override;
    Colour getOutlineColour() override;

    // All opened patches
    Array<pd::Patch::Ptr, CriticalSection> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

    std::vector<float*> channelPointers;
    std::atomic<float>* volume;

    SettingsFile* settingsFile;

    std::unique_ptr<pd::Library> objectLibrary;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    static inline const String versionSuffix = "";
    File versionDataDir = homeDir.getChildFile(ProjectInfo::versionString + versionSuffix);

    File abstractions = versionDataDir.getChildFile("Abstractions");

    Value commandLocked = Value(var(false));

    std::unique_ptr<StatusbarSource> statusbarSource;

    Value tailLength = Value(0.0f);

    // Just so we never have to deal with deleting the default LnF
    SharedResourcePointer<PlugDataLook> lnf;

    static inline constexpr int numParameters = 512;
    static inline constexpr int numInputBuses = 16;
    static inline constexpr int numOutputBuses = 16;

    // Protected mode value will decide if we apply clipping to output and remove non-finite numbers
    std::atomic<bool> protectedMode = true;

    // Zero means no oversampling
    std::atomic<int> oversampling = 0;
    int lastLeftTab = -1;
    int lastRightTab = -1;

    // Only used by standalone!
    OwnedArray<MidiOutput> midiOutputs;
    std::unique_ptr<InternalSynth> internalSynth;
    std::atomic<bool> enableInternalSynth = false;

private:
    void processInternal();

    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothedGain;

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

    std::vector<pd::Atom> atoms_playhead;

    int minIn = 2;
    int minOut = 2;

    int lastSplitIndex = -1;
    int lastSetProgram = 0;
        
        
    dsp::Limiter<float> limiter;
    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    static inline const String else_version = "ELSE v1.0-rc8";
    static inline const String cyclone_version = "cyclone v0.7-0";
    // this gets updated with live version data later
    static String pdlua_version;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
