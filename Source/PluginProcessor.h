/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>

#if PERFETTO
#    include <melatonin_perfetto/melatonin_perfetto.h>
#endif

#include "Utility/Config.h"
#include "Utility/Limiter.h"
#include "Utility/SettingsFile.h"
#include "Utility/AudioMidiFifo.h"
#include "Utility/SeqLock.h"
#include "Utility/MidiDeviceManager.h"

#include "Pd/Instance.h"
#include "Pd/Patch.h"

namespace pd {
class Library;
}

class PlugDataParameter;
class Autosave;
class InternalSynth;
class SettingsFile;
class StatusbarSource;
struct PlugDataLook;
class PluginEditor;
class ConnectionMessageDisplay;
class Object;
class PluginProcessor final : public AudioProcessor
    , public pd::Instance
    , public SettingsFileListener {
public:
    PluginProcessor();

    ~PluginProcessor() override;

    static AudioProcessor::BusesProperties buildBusesProperties();

    void setOversampling(int amount);
    void setLimiterThreshold(int amount);
    void setEnableLimiter(bool enabled);
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void numChannelsChanged() override;
    void releaseResources() override {};

    void updateAllEditorsLNF();

    void flushMessageQueue();
    void doubleFlushMessageQueue();

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(BusesLayout const& layouts) const override;
#endif

    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer&) override;

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

    pd::Patch::Ptr findPatchInPluginMode(int editorIndex);

    void receiveNoteOn(int channel, int pitch, int velocity) override;
    void receiveControlChange(int channel, int controller, int value) override;
    void receiveProgramChange(int channel, int value) override;
    void receivePitchBend(int channel, int value) override;
    void receiveAftertouch(int channel, int value) override;
    void receivePolyAftertouch(int channel, int pitch, int value) override;
    void receiveMidiByte(int port, int byte) override;
    void receiveSysMessage(SmallString const& selector, SmallArray<pd::Atom> const& list) override;

    void addTextToTextEditor(uint64_t ptr, SmallString const& text) override;
    void hideTextEditorDialog(uint64_t ptr) override;
    void showTextEditorDialog(uint64_t ptr, SmallString const& title, std::function<void(String, uint64_t)> save, std::function<void(uint64_t)> close) override;
    void raiseTextEditorDialog(uint64_t ptr) override;
    void clearTextEditor(uint64_t ptr) override;
    bool isTextEditorDialogShown(uint64_t ptr) override;

    void updateConsole(int numMessages, bool newWarning) override;

    void reloadAbstractions(File changedPatch, t_glist* except) override;

    void processConstant(dsp::AudioBlock<float>);
    void processVariable(dsp::AudioBlock<float>, MidiBuffer& midiBuffer);

    MidiDeviceManager& getMidiDeviceManager();

    bool canAddBus(bool isInput) const override
    {
        return true;
    }

    bool canRemoveBus(bool const isInput) const override
    {
        int const nbus = getBusCount(isInput);
        return nbus > 0;
    }

    void settingsFileReloaded() override;

    static void initialiseFilesystem();
    void updateSearchPaths();

    void sendMidiBuffer(int device, MidiBuffer& buffer);
    void sendPlayhead();
    void sendParameters();

    void updateEnabledParameters();
    SmallArray<PlugDataParameter*> getEnabledParameters();

    SmallArray<PluginEditor*> getEditors() const;

    void performParameterChange(int type, SmallString const& name, float value) override;
    void enableAudioParameter(SmallString const& name) override;
    void disableAudioParameter(SmallString const& name) override;
    void setParameterRange(SmallString const& name, float min, float max) override;
    void setParameterMode(SmallString const& name, int mode) override;

    void performLatencyCompensationChange(float value) override;
    void sendParameterInfoChangeMessage();

    void fillDataBuffer(SmallArray<pd::Atom> const& list) override;
    void parseDataBuffer(XmlElement const& xml) override;
    std::unique_ptr<XmlElement> extraData;

    pd::Patch::Ptr loadPatch(String patch);
    pd::Patch::Ptr loadPatch(URL const& patchURL);

    void titleChanged() override;

    void setTheme(String themeToUse, bool force = false);

    int lastUIWidth = 1000, lastUIHeight = 650;

    AtomicValue<float>* volume;
    ValueTree pluginModeTheme;
    float pluginModeScale = 1.0f;
        
    String currentThemeName;

    SettingsFile* settingsFile;

    std::unique_ptr<pd::Library> objectLibrary;

    File abstractions = ProjectInfo::versionDataDir.getChildFile("Abstractions");

    Value commandLocked = Value(var(false));

    std::unique_ptr<StatusbarSource> statusbarSource;

    Value tailLength = Value(0.0f);

    // Just so we never have to deal with deleting the default LnF
    SharedResourcePointer<PlugDataLook> lnf;

    static constexpr int numParameters = 512;
    static constexpr int numInputBuses = 16;
    static constexpr int numOutputBuses = 16;

    // Protected mode value will decide if we apply clipping to output and remove non-finite numbers
    AtomicValue<bool> enableLimiter = true;

    // Zero means no oversampling
    AtomicValue<int> oversampling = 0;

    std::unique_ptr<InternalSynth> internalSynth;

    OwnedArray<PluginEditor> openedEditors;

    AtomicValue<ConnectionMessageDisplay*, Sequential> connectionListener = nullptr;
    std::unique_ptr<Autosave> autosave;

private:
    int customLatencySamples = 0;

    SmoothedValue<float> smoothedGain;

    AtomicValue<int> audioAdvancement = 0;

    bool variableBlockSize = false;
    AudioBuffer<float> audioBufferIn;
    AudioBuffer<float> audioBufferOut;
    AudioBuffer<float> bypassBuffer;

    HeapArray<float> audioVectorIn;
    HeapArray<float> audioVectorOut;

    std::unique_ptr<AudioMidiFifo> inputFifo;
    std::unique_ptr<AudioMidiFifo> outputFifo;

    MidiBuffer blockMidiBuffer;
    MidiBuffer midiBufferInternalSynth;

    MidiDeviceManager midiDeviceManager;

    AudioProcessLoadMeasurer cpuLoadMeasurer;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = {};
    size_t midiByteIndex = 0;

    SmallArray<pd::Atom> atoms_playhead;
    SmallArray<PlugDataParameter*> enabledParameters;

    int lastSetProgram = 0;

    Limiter limiter;
    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    UnorderedMap<uint64_t, std::unique_ptr<Component>> textEditorDialogs;

#if PERFETTO
    std::unique_ptr<perfetto::TracingSession> tracingSession;
#endif

    static inline String const else_version = "ELSE v1.0-rc13";
    static inline String const cyclone_version = "cyclone v0.9-2";
    static inline String const heavylib_version = "heavylib v0.4";
    static inline String const gem_version = "Gem v0.94";
    // this gets updated with live version data later
    static String pdlua_version;

    class HostInfoUpdater final : public AsyncUpdater {
    public:
        explicit HostInfoUpdater(PluginProcessor* parentProcessor)
            : processor(*parentProcessor)
        {
        }

        void update()
        {
            if (ProjectInfo::isStandalone)
                return;
#if JUCE_IOS
            handleAsyncUpdate(); // iOS doesn't like it if we do this asynchronously
#else
            triggerAsyncUpdate();
#endif
        }

    private:
        void handleAsyncUpdate() override
        {
            auto const details = AudioProcessorListener::ChangeDetails {}.withParameterInfoChanged(true);
            processor.updateHostDisplay(details);
        }

        PluginProcessor& processor;
    };

    HostInfoUpdater hostInfoUpdater;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
