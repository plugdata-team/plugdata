/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>
#include "Utility/MidiDeviceManager.h"

class MidiSettingsToggle : public PropertiesPanel::BoolComponent {
public:
    MidiSettingsToggle(bool isMidiInput, PluginProcessor* pluginProcessor, MidiDeviceInfo& midiDeviceInfo)
        : PropertiesPanel::BoolComponent(midiDeviceInfo.name, { "Disabled", "Enabled" })
        , isInput(isMidiInput)
        , processor(pluginProcessor)
        , deviceInfo(midiDeviceInfo)
    {
        toggleStateValue = processor->getMidiDeviceManager().isMidiDeviceEnabled(isInput, deviceInfo.identifier);
    }

    PropertiesPanelProperty* createCopy() override
    {
        return new MidiSettingsToggle(isInput, processor, deviceInfo);
    }

private:
    void valueChanged(Value& v) override
    {
        repaint();
        processor->getMidiDeviceManager().setMidiDeviceEnabled(isInput, deviceInfo.identifier, getValue<bool>(toggleStateValue));
    }

    bool isInput;
    PluginProcessor* processor;
    MidiDeviceInfo deviceInfo;
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

class MidiSettingsPanel : public SettingsDialogPanel
    , private ChangeListener {
public:
    MidiSettingsPanel(PluginProcessor* audioProcessor)
        : processor(audioProcessor)
    {
        if (auto* audioDeviceManager = ProjectInfo::getDeviceManager()) {
            deviceManager = audioDeviceManager; // TODO: find an alternative solution for plugins!
            deviceManager->addChangeListener(this);
        }
        
        addAndMakeVisible(midiProperties);
        processor->getMidiDeviceManager().updateMidiDevices();
        updateDevices();
    }

    ~MidiSettingsPanel() override
    {
        if(deviceManager) deviceManager->removeChangeListener(this);
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &midiProperties;
    }

private:
    void resized() override
    {
        midiProperties.setBounds(getLocalBounds());
    }

    void updateDevices()
    {
        midiProperties.clear();

        auto midiInputDevices = processor->getMidiDeviceManager().getInputDevicesUnfiltered();
        auto midiInputProperties = PropertiesArray();

        auto midiOutputDevices = processor->getMidiDeviceManager().getOutputDevicesUnfiltered();
        auto midiOutputProperties = PropertiesArray();

        for (auto& deviceInfo : midiInputDevices) {
            // The internal plugdata ports should be viewed from our perspective instead of that of an external application
            if (deviceInfo.name == "to plugdata") {
                midiInputProperties.add(new MidiSettingsToggle(false, processor, deviceInfo));
                continue;
            }

            midiInputProperties.add(new MidiSettingsToggle(true, processor, deviceInfo));
        }

        for (auto& deviceInfo : midiOutputDevices) {
            if (deviceInfo.name == "from plugdata") {
                midiOutputProperties.add(new MidiSettingsToggle(true, processor, deviceInfo));
                continue;
            }

            midiOutputProperties.add(new MidiSettingsToggle(false, processor, deviceInfo));
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
    AudioDeviceManager* deviceManager = nullptr;
    PropertiesPanel midiProperties;
};
