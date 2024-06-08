/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Standalone/InternalSynth.h"

class MidiDeviceManager : public ChangeListener
    , public AsyncUpdater {

public:
    // Helper functions to encode/decode regular MIDI events into a sysex event
    // The reason we do this, is that we want to append extra information to the MIDI event when it comes in from pd or the device, but JUCE won't allow this
    // We still want to be able to use handy JUCE stuff for MIDI timing, so we treat every MIDI event as sysex
    static std::vector<uint16_t> encodeSysExData(std::vector<uint8_t> const& data)
    {
        std::vector<uint16_t> encodedData;
        for (auto& value : data) {
            if (value == 0xF0 || value == 0xF7) {
                // If the value is 0xF0 or 0xF7, encode them in the higher 8 bits. 0xF0 and 0xF8 are already at the top end, so we only need to shift them by 1 position to put it outside of MIDI range. We can't shift by 8, the sysex bytes could still be recognised as sysex bytes!
                encodedData.push_back(static_cast<uint16_t>(value) << 1);
            } else {
                // Otherwise, just cast the 8-bit value to a 16-bit value
                encodedData.push_back(static_cast<uint16_t>(value));
            }
        }
        return encodedData;
    }

    static std::vector<uint8_t> decodeSysExData(std::vector<uint16_t> const& encodedData)
    {
        std::vector<uint8_t> decodeData;
        for (auto& value : encodedData) {
            auto upperByte = value >> 1;
            if (upperByte == 0xF0 || upperByte == 0xF7) {
                decodeData.push_back(upperByte);
            } else {
                // Extract the lower 8 bits to obtain the original 8-bit data
                decodeData.push_back(static_cast<uint8_t>(value));
            }
        }
        return decodeData;
    }

    static MidiMessage convertToSysExFormat(MidiMessage m, int device)
    {
        if (ProjectInfo::isStandalone) {
            // We append the device index so we can use it as a selector later
            auto const* data = static_cast<uint8 const*>(m.getRawData());
            auto message = std::vector<uint8>(data, data + m.getRawDataSize());
            message.push_back(device);
            auto encodedMessage = encodeSysExData(message);

            // Temporarily convert all messages to sysex so we can add as much data as we want
            return MidiMessage::createSysExMessage(static_cast<void*>(encodedMessage.data()), encodedMessage.size() * sizeof(uint16_t)).withTimeStamp(m.getTimeStamp());
        }

        return m;
    }

    static MidiMessage convertFromSysExFormat(MidiMessage m, int& device)
    {
        if (ProjectInfo::isStandalone) {
            auto const* sysexData = reinterpret_cast<uint16_t const*>(m.getSysExData());
            auto sysexDataSize = m.getSysExDataSize() / sizeof(uint16_t);
            auto midiMessage = decodeSysExData(std::vector<uint16_t>(sysexData, sysexData + sysexDataSize));
            if (!sysexData)
                return m;

            device = midiMessage.back();
            midiMessage.pop_back();

            return MidiMessage(midiMessage.data(), midiMessage.size());
        }

        device = 0;
        return m;
    }

    MidiDeviceManager(MidiInputCallback* inputCallback)
    {
#if !JUCE_WINDOWS && !JUCE_IOS
        if (auto* newOut = MidiOutput::createNewDevice("from plugdata").release()) {
            fromPlugdata.reset(newOut);
        }
        if (auto* newIn = MidiInput::createNewDevice("to plugdata", inputCallback).release()) {
            toPlugdata.reset(newIn);
        }
#endif
        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            deviceManager->addChangeListener(this);
        }

        filteredMidiInputs = filteredMidiOutputs = 0;
        updateMidiDevices();
    }

    ~MidiDeviceManager()
    {
        saveMidiOutputSettings();
        clearInputFilter();
        clearOutputFilter();
    }

    void loadMidiOutputSettings()
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto midiOutputsTree = settingsTree.getChildWithName("EnabledMidiOutputPorts");

        for (int i = 0; i < midiOutputsTree.getNumChildren(); i++) {
            // This will try to enable the same MIDI devices that were enabled
            // last time. It the device doesn't exist, it will do nothing.
            // Note: We're checking the device names against the current
            // device table here, since the identifiers may change between
            // invocations.
            auto name = midiOutputsTree.getChild(i).getProperty("Name").toString();
            for (auto& output : lastMidiOutputs) {
                if (output.name == name) {
                    setMidiDeviceEnabled(false, output.identifier, true);
                    break;
                }
            }
        }
    }

    void handleAsyncUpdate() override
    {
        auto midiOutputsTree = SettingsFile::getInstance()->getValueTree().getChildWithName("EnabledMidiOutputPorts");

        midiOutputsTree.removeAllChildren(nullptr);

        for (auto& output : getOutputDevices()) {
            ValueTree midiOutputPort("MidiPort");
            midiOutputPort.setProperty("Name", output.name, nullptr);
            midiOutputPort.setProperty("Identifier", output.identifier, nullptr);

            midiOutputsTree.appendChild(midiOutputPort, nullptr);
        }
    }

    void saveMidiOutputSettings()
    {
        triggerAsyncUpdate();
    }

private:
    void changeListenerCallback(ChangeBroadcaster* origin) override
    {
        updateMidiDevices();
    }

    void clearInputFilter()
    {
        if (filteredMidiInputs)
            delete filteredMidiInputs;
        filteredMidiInputs = 0;
    }

    void clearOutputFilter()
    {
        if (filteredMidiOutputs)
            delete filteredMidiOutputs;
        filteredMidiOutputs = 0;
    }

public:
    void updateMidiDevices()
    {
        midiDeviceMutex.lock();
        lastMidiInputs = MidiInput::getAvailableDevices();
        lastMidiOutputs = MidiOutput::getAvailableDevices();

        for (int i = 0; i < lastMidiInputs.size(); i++) {
            if (toPlugdata && lastMidiInputs[i].name == "from plugdata") {
                lastMidiInputs.set(i, toPlugdata->getDeviceInfo());
            }
        }

        for (int i = 0; i < lastMidiOutputs.size(); i++) {
            if (fromPlugdata && lastMidiOutputs[i].name == "to plugdata") {
                lastMidiOutputs.set(i, fromPlugdata->getDeviceInfo());
            }
        }

        midiDeviceMutex.unlock();
        clearInputFilter();
        clearOutputFilter();
    }

    Array<MidiDeviceInfo> getInputDevicesUnfiltered()
    {
        return lastMidiInputs;
    }

    Array<MidiDeviceInfo> getOutputDevicesUnfiltered()
    {
        return lastMidiOutputs;
    }

private:
    // Helper class to reorder MIDI inputs and outputs. Currently this just
    // sorts the ports by their enabled status, so that the enabled ports come
    // first. In the future, we might also apply secondary criteria, e.g., to
    // implement freely assigned port numbers resembling what vanilla Pd has.
    class compareDevs {
        MidiDeviceManager* self;
        bool isInput;

    public:
        compareDevs(MidiDeviceManager* x, bool in)
            : self(x)
            , isInput(in)
        {
        }
        int compareElements(MidiDeviceInfo const& dev1, MidiDeviceInfo const& dev2) const
        {
            auto id1 = dev1.identifier;
            auto id2 = dev2.identifier;
            bool enabled1 = self->isMidiDeviceEnabled(isInput, id1);
            bool enabled2 = self->isMidiDeviceEnabled(isInput, id2);
            if (enabled1 > enabled2)
                return -1;
            else if (enabled1 < enabled2)
                return 1;
            else
                return 0;
        }
    };

public:
    // Sorted and filtered arrays of MIDI input and output ports based on the
    // enabled MIDI ports selected by the user. We keep these separate from
    // the raw port arrays in lastMidiInputs and lastMidiOutputs, so that the
    // MIDI setup can present the user with a consistent view of all currently
    // available devices, while the following reordered and filtered lists of
    // enabled ports are used in the app for popup menus and to map port
    // numbers used by the Pd engine.
    Array<MidiDeviceInfo> getInputDevices()
    {
        if (!ProjectInfo::getDeviceManager()) {
            // just in case we get called during startup when the device
            // manager hasn't been created yet
            return lastMidiInputs;
        }
        if (!filteredMidiInputs) {
            // we cache the filtered device list so that we don't have to
            // recompute it each time
            midiDeviceMutex.lock();
            filteredMidiInputs = new Array<MidiDeviceInfo>(lastMidiInputs);
            midiDeviceMutex.unlock();
            compareDevs cmp(this, true);
            // make sure to do a stable sort here, so that enabled ports stay
            // in the same relative order as in the MIDI setup
            filteredMidiInputs->sort(cmp, true);
            int i, n = filteredMidiInputs->size();
            // this assumes that all disabled ports come last
            for (i = 0; i < n && isMidiDeviceEnabled(true, (*filteredMidiInputs)[i].identifier); i++)
                ;
            // remove all disabled ports from the end of the array
            filteredMidiInputs->removeLast(n - i);
        }
        return *filteredMidiInputs;
    }

    Array<MidiDeviceInfo> getOutputDevices()
    {
        if (!ProjectInfo::getDeviceManager()) {
            return lastMidiOutputs;
        }
        if (!filteredMidiOutputs) {
            midiDeviceMutex.lock();
            filteredMidiOutputs = new Array<MidiDeviceInfo>(lastMidiOutputs);
            midiDeviceMutex.unlock();
            compareDevs cmp(this, false);
            filteredMidiOutputs->sort(cmp, true);
            int i, n = filteredMidiOutputs->size();
            for (i = 0; i < n && isMidiDeviceEnabled(false, (*filteredMidiOutputs)[i].identifier); i++)
                ;
            filteredMidiOutputs->removeLast(n - i);
        }
        return *filteredMidiOutputs;
    }

    bool isMidiDeviceEnabled(bool isInput, String const& identifier)
    {
        if (fromPlugdata && identifier == fromPlugdata->getIdentifier()) {
            return internalOutputEnabled;
        }
        if (toPlugdata && identifier == toPlugdata->getIdentifier()) {
            return internalInputEnabled;
        }
        if (isInput) {
            return ProjectInfo::getDeviceManager()->isMidiInputDeviceEnabled(identifier);
        } else {
            for (auto* midiOut : midiOutputs) {
                if (midiOut->getIdentifier() == identifier) {
                    return true;
                }
            }
        }

        return false;
    }

    void setMidiDeviceEnabled(bool isInput, String const& identifier, bool shouldBeEnabled)
    {
        if (fromPlugdata && identifier == fromPlugdata->getIdentifier()) {
            if (shouldBeEnabled != internalOutputEnabled)
                clearOutputFilter();
            internalOutputEnabled = shouldBeEnabled;
            saveMidiOutputSettings();
        } else if (toPlugdata && identifier == toPlugdata->getIdentifier()) {
            if (shouldBeEnabled != internalInputEnabled) {
                clearInputFilter();
                internalInputEnabled = shouldBeEnabled;
                if (internalInputEnabled) {
                    toPlugdata->start();
                } else {
                    toPlugdata->stop();
                }
            }
        } else if (isInput) {
            if (shouldBeEnabled != isMidiDeviceEnabled(true, identifier)) {
                ProjectInfo::getDeviceManager()->setMidiInputDeviceEnabled(identifier, shouldBeEnabled);
                clearInputFilter();
            }
        } else if (shouldBeEnabled != isMidiDeviceEnabled(false, identifier)) {
            clearOutputFilter();
            if (shouldBeEnabled) {
                auto* device = midiOutputs.add(MidiOutput::openDevice(identifier));
                if (device)
                    device->startBackgroundThread();
            } else {
                for (auto* midiOut : midiOutputs) {
                    if (midiOut->getIdentifier() == identifier) {
                        midiOutputs.removeObject(midiOut);
                        break;
                    }
                }
            }

            saveMidiOutputSettings();
        }
    }

    void sendMidiOutputMessage(int device, MidiMessage& message)
    {
        // Device ID 0 means all devices
        if (device == 0) {
            for (auto* midiOutput : midiOutputs) {
                midiOutput->sendMessageNow(message);
            }
            if (fromPlugdata && internalOutputEnabled)
                fromPlugdata->sendMessageNow(message);
            return;
        }

        auto idToFind = getOutputDevices()[device - 1].identifier;
        // The order of midiOutputs is not necessarily the same as that of lastMidiOutputs, that's why we need to check

        if (fromPlugdata && idToFind == fromPlugdata->getIdentifier()) {
            fromPlugdata->sendMessageNow(message);
            return;
        }

        for (auto* midiOutput : midiOutputs) {
            if (idToFind == midiOutput->getIdentifier()) {
                midiOutput->sendMessageNow(message);
                break;
            }
        }
    }

    int getMidiInputDeviceIndex(String const& identifier)
    {
        auto devices = getInputDevices();

        for (int i = 0; i < devices.size(); i++) {
            if (devices[i].identifier == identifier) {
                return i;
            }
        }

        return -1;
    }

private:
    bool internalOutputEnabled = false;
    bool internalInputEnabled = false;

    std::unique_ptr<MidiInput> toPlugdata;
    std::unique_ptr<MidiOutput> fromPlugdata;

    // These arrays hold the actual midi ports
    OwnedArray<MidiOutput> midiOutputs;

    std::mutex midiDeviceMutex;

    // List of ports in the canonical order
    // This can't be accessed from the audio thread so we need to store it when it changes
    Array<MidiDeviceInfo> lastMidiInputs;
    Array<MidiDeviceInfo> lastMidiOutputs;

    Array<MidiDeviceInfo>* filteredMidiInputs;
    Array<MidiDeviceInfo>* filteredMidiOutputs;
};
