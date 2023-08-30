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

    void receiveNoteOn(int channel, int pitch, int const velocity) override;
    void receiveControlChange(int channel, int controller, int value) override;
    void receiveProgramChange(int channel, int value) override;
    void receivePitchBend(int channel, int value) override;
    void receiveAftertouch(int channel, int value) override;
    void receivePolyAftertouch(int channel, int pitch, int value) override;
    void receiveMidiByte(int port, int byte) override;
    void receiveSysMessage(String const& selector, std::vector<pd::Atom> const& list) override;
        
    void addTextToTextEditor(unsigned long ptr, String text) override;
    void showTextEditor(unsigned long ptr, Rectangle<int> bounds, String title) override;

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
    void performParameterChange(int type, String const& name, float value) override;

    // Jyg added this
    void fillDataBuffer(std::vector<pd::Atom> const& list) override;
    void parseDataBuffer(XmlElement const& xml) override;
    XmlElement* m_temp_xml;

    pd::Patch::Ptr loadPatch(String patch, int splitIdx = -1);
    pd::Patch::Ptr loadPatch(File const& patch, int splitIdx = -1);

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

    File abstractions = ProjectInfo::versionDataDir.getChildFile("Abstractions");

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
    MidiBuffer midiBufferInternalSynth;

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
        
    std::map<unsigned long, std::unique_ptr<Component>> textEditorDialogs;

    static inline const String else_version = "ELSE v1.0-rc9";
    static inline const String cyclone_version = "cyclone v0.7-0";
    // this gets updated with live version data later
    static String pdlua_version;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
