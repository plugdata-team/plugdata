/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>
#include "Utility/MidiDeviceManager.h"

class MidiSettingsComboBox : public PropertiesPanel::ComboComponent
    , private Value::Listener {
public:
    MidiSettingsComboBox(bool isMidiInput, PluginProcessor* pluginProcessor, MidiDeviceInfo& midiDeviceInfo)
        : PropertiesPanel::ComboComponent(getRealDeviceName(midiDeviceInfo.name), { "Disabled", "Port 1", "Port 2", "Port 3", "Port 4", "Port 5", "Port 6", "Port 7", "Port 8" })
        , isInput(isMidiInput)
        , processor(pluginProcessor)
        , deviceInfo(midiDeviceInfo)
    {
        comboValue.referTo(comboBox.getSelectedIdAsValue());
        comboValue = processor->getMidiDeviceManager().getMidiDevicePort(isInput, deviceInfo) + 2;
        comboValue.addListener(this);
    }
        
    static String getRealDeviceName(String const& name)
    {
        if(name == "from plugdata") return "to plugdata";
        if(name == "to plugdata") return "from plugdata";
        return name;
    }

    PropertiesPanelProperty* createCopy() override
    {
        return new MidiSettingsComboBox(isInput, processor, deviceInfo);
    }

private:
    void valueChanged(Value& v) override
    {
        repaint();
        auto port = getValue<int>(comboValue);
        processor->getMidiDeviceManager().setMidiDevicePort(isInput, deviceInfo.name, deviceInfo.identifier, port - 2);
    }

    bool isInput;
    PluginProcessor* processor;
    MidiDeviceInfo deviceInfo;
    Value comboValue;
};

class InternalSynthToggle : public PropertiesPanel::ComboComponent
    , private Value::Listener {
public:
    explicit InternalSynthToggle(PluginProcessor* audioProcessor)
        : PropertiesPanel::ComboComponent("Internal GM Synth", { "Disabled", "Port 1", "Port 2", "Port 3", "Port 4", "Port 5", "Port 6", "Port 7", "Port 8" })
        , processor(audioProcessor)
    {
        comboValue.referTo(comboBox.getSelectedIdAsValue());
        comboValue = processor->internalSynthPort.load() + 2;
        comboValue.addListener(this);
    }

    void valueChanged(Value& v) override
    {
        repaint();

        processor->internalSynthPort = getValue<int>(comboValue) - 2;
        processor->settingsFile->setProperty("internal_synth", static_cast<int>(processor->internalSynthPort));
    }

    PluginProcessor* processor;
    Value comboValue;
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
        if (deviceManager)
            deviceManager->removeChangeListener(this);
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

        auto midiInputDevices = processor->getMidiDeviceManager().getInputDevices();
        auto midiInputProperties = PropertiesArray();

        auto midiOutputDevices = processor->getMidiDeviceManager().getOutputDevices();
        auto midiOutputProperties = PropertiesArray();

        for (auto& deviceInfo : midiInputDevices) {
            // The internal plugdata ports should be viewed from our perspective instead of that of an external application
            if (deviceInfo.name == "from plugdata") {
                midiInputProperties.add(new MidiSettingsComboBox(false, processor, deviceInfo));
                continue;
            }

            midiInputProperties.add(new MidiSettingsComboBox(true, processor, deviceInfo));
        }

        for (auto& deviceInfo : midiOutputDevices) {
            if (deviceInfo.name == "to plugdata") {
                midiOutputProperties.add(new MidiSettingsComboBox(true, processor, deviceInfo));
                continue;
            }

            midiOutputProperties.add(new MidiSettingsComboBox(false, processor, deviceInfo));
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
