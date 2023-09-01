/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>
#include "Utility/MidiDeviceManager.h"

class MidiSettingsToggle : public PropertiesPanel::BoolComponent {
public:
    MidiSettingsToggle(bool isMidiInput, PluginProcessor* pluginProcessor, MidiDeviceInfo& midiDeviceInfo, AudioDeviceManager& audioDeviceManager)
        : PropertiesPanel::BoolComponent(midiDeviceInfo.name, { "Disabled", "Enabled" })
        , deviceInfo(midiDeviceInfo)
        , deviceManager(audioDeviceManager)
        , isInput(isMidiInput)
        , processor(pluginProcessor)
    {
        toggleStateValue = ProjectInfo::getMidiDeviceManager()->isMidiDeviceEnabled(isInput, deviceInfo.identifier);
    }

private:
    void valueChanged(Value& v) override
    {
        repaint();
        ProjectInfo::getMidiDeviceManager()->setMidiDeviceEnabled(isInput, deviceInfo.identifier, getValue<bool>(toggleStateValue));
    }

    bool isInput;
    PluginProcessor* processor;
    MidiDeviceInfo deviceInfo;
    AudioDeviceManager& deviceManager;
};

class InternalSynthToggle : public PropertiesPanel::BoolComponent {
public:
    explicit InternalSynthToggle(PluginProcessor* audioProcessor)
        : PropertiesPanel::BoolComponent("Internal GM Synth", { "Disabled", "Enabled" })
        , processor(audioProcessor)
    {
        toggleStateValue = processor->enableInternalSynth.load();
    }

    void valueChanged(Value& v) override
    {
        repaint();

        processor->enableInternalSynth = getValue<bool>(toggleStateValue);
        processor->settingsFile->setProperty("internal_synth", static_cast<int>(processor->enableInternalSynth));
    }

    PluginProcessor* processor;
};

class StandaloneMIDISettings : public Component
    , private ChangeListener {
public:
    StandaloneMIDISettings(PluginProcessor* audioProcessor, AudioDeviceManager& audioDeviceManager)
        : processor(audioProcessor)
        , deviceManager(audioDeviceManager)
    {
        addAndMakeVisible(midiProperties);

        deviceManager.addChangeListener(this);
        ProjectInfo::getMidiDeviceManager()->updateMidiDevices();
        updateDevices();
    }

    ~StandaloneMIDISettings() override
    {
        deviceManager.removeChangeListener(this);
    }

private:
    void resized() override
    {
        midiProperties.setBounds(getLocalBounds());
    }

    void updateDevices()
    {
        midiProperties.clear();

        auto midiInputDevices = ProjectInfo::getMidiDeviceManager()->getInputDevicesUnfiltered();
        auto midiInputProperties = Array<PropertiesPanel::Property*>();

        auto midiOutputDevices = ProjectInfo::getMidiDeviceManager()->getOutputDevicesUnfiltered();
        auto midiOutputProperties = Array<PropertiesPanel::Property*>();

        for (auto& deviceInfo : midiInputDevices) {
            // The internal plugdata ports should be viewed from our perspective instead of that of an external application
            if (deviceInfo.name == "to plugdata") {
                midiInputProperties.add(new MidiSettingsToggle(false, processor, deviceInfo, deviceManager));
                continue;
            }

            midiInputProperties.add(new MidiSettingsToggle(true, processor, deviceInfo, deviceManager));
        }

        for (auto& deviceInfo : midiOutputDevices) {
            if (deviceInfo.name == "from plugdata") {
                midiOutputProperties.add(new MidiSettingsToggle(true, processor, deviceInfo, deviceManager));
                continue;
            }

            midiOutputProperties.add(new MidiSettingsToggle(false, processor, deviceInfo, deviceManager));
        }

        midiOutputProperties.add(new InternalSynthToggle(processor));

        midiProperties.addSection("MIDI Inputs", midiInputProperties);
        midiProperties.addSection("MIDI Outputs", midiOutputProperties);
    }

    void changeListenerCallback(ChangeBroadcaster*) override
    {
        updateDevices();
    }

    PluginProcessor* processor;
    AudioDeviceManager& deviceManager;
    PropertiesPanel midiProperties;
};
