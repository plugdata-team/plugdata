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
#include "Utility/Limiter.h"
#include "Utility/SettingsFile.h"
#include <Utility/AudioMidiFifo.h>

#include "Pd/Instance.h"
#include "Pd/Patch.h"

namespace pd {
class Library;
}

class InternalSynth;
class SettingsFile;
class StatusbarSource;
struct PlugDataLook;
class PluginEditor;
class ConnectionMessageDisplay;
class PluginProcessor : public AudioProcessor
    , public pd::Instance, public SettingsFileListener {
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

    String const getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    String const getProgramName(int index) override;
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

    void updateConsole(int numMessages, bool newWarning) override;

    void reloadAbstractions(File changedPatch, t_glist* except) override;

    void processConstant(dsp::AudioBlock<float>, MidiBuffer&);
    void processVariable(dsp::AudioBlock<float>, MidiBuffer&);

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
    void updatePatchUndoRedoState();
        
    void settingsFileReloaded() override;

    void initialiseFilesystem();
    void updateSearchPaths();

    void sendMidiBuffer();
    void sendPlayhead();
    void sendParameters();

    bool isInPluginMode();

    Array<PluginEditor*> getEditors() const;

    void performParameterChange(int type, String const& name, float value) override;

    // Jyg added this
    void fillDataBuffer(std::vector<pd::Atom> const& list) override;
    void parseDataBuffer(XmlElement const& xml) override;
    std::unique_ptr<XmlElement> extraData;

    pd::Patch::Ptr loadPatch(String patch, PluginEditor* editor, int splitIndex = 0);
    pd::Patch::Ptr loadPatch(File const& patch, PluginEditor* editor, int splitIndex = 0);

    void titleChanged() override;

    void setTheme(String themeToUse, bool force = false);

    Colour getForegroundColour() override;
    Colour getBackgroundColour() override;
    Colour getTextColour() override;
    Colour getOutlineColour() override;
        
    // All opened patches
    Array<pd::Patch::Ptr, CriticalSection> patches;

    int lastUIWidth = 1000, lastUIHeight = 650;

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

    OwnedArray<PluginEditor> openedEditors;
    Component::SafePointer<ConnectionMessageDisplay> connectionListener;

private:
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothedGain;

    int audioAdvancement = 0;

    bool variableBlockSize = false;
    AudioBuffer<float> audioBufferIn;
    AudioBuffer<float> audioBufferOut;

    std::vector<float> audioVectorIn;
    std::vector<float> audioVectorOut;

    std::unique_ptr<AudioMidiFifo> inputFifo;
    std::unique_ptr<AudioMidiFifo> outputFifo;

    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    MidiBuffer midiBufferInternalSynth;

    AudioProcessLoadMeasurer cpuLoadMeasurer;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = { 0 };
    size_t midiByteIndex = 0;

    std::vector<pd::Atom> atoms_playhead;

    int lastSetProgram = 0;

    Limiter limiter;
    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    std::map<unsigned long, std::unique_ptr<Component>> textEditorDialogs;

    static inline String const else_version = "ELSE v1.0-rc10";
    static inline String const cyclone_version = "cyclone v0.8-0";
    static inline String const heavylib_version = "heavylib v0.3.1";
    // this gets updated with live version data later
    static String pdlua_version;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
