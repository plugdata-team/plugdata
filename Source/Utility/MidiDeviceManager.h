/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Standalone/InternalSynth.h"

struct MidiDeviceManager : public ChangeListener
{
    
    // Helper functions to encode/decode regular MIDI events into a sysex event
    // The reason we do this, is that we want to append extra information to the MIDI event when it comes in from pd or the device, but JUCE won't allow this
    // We still want to be able to use handy JUCE stuff for MIDI timing, so we treat every MIDI event as sysex
    static std::vector<uint16_t> encodeSysExData(const std::vector<uint8_t>& data) {
        std::vector<uint16_t> encoded_data;
        for (auto& value : data) {
            if (value == 0xF0 || value == 0xF7) {
                // If the value is 0xF0 or 0xF7, encode them in the higher 8 bits
                encoded_data.push_back(static_cast<uint16_t>(value) << 8);
            } else {
                // Otherwise, just cast the 8-bit value to a 16-bit value
                encoded_data.push_back(static_cast<uint16_t>(value));
            }
        }
        return encoded_data;
    }

    static std::vector<uint8_t> decodeSysExData(const std::vector<uint16_t>& encoded_data) {
        std::vector<uint8_t> decoded_data;
        for (auto& value : encoded_data) {
            auto upperByte = value >> 8;
            if(upperByte == 0xF0 || upperByte == 0xF7)
            {
                decoded_data.push_back(upperByte);
            }
            else {
                // Extract the lower 8 bits to obtain the original 8-bit data
                decoded_data.push_back(static_cast<uint8_t>(value));
            }

        }
        return decoded_data;
    }
    
    static MidiMessage convertToSysExFormat(MidiMessage m, int device)
    {
        if(ProjectInfo::isStandalone) {
            // We append the device index so we can use it as a selector later
            const auto* data = static_cast<const uint8*>(m.getRawData());
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
            const auto* sysexData = reinterpret_cast<const uint16_t*>(m.getSysExData());
            auto sysexDataSize = m.getSysExDataSize() / sizeof(uint16_t);
            auto midiMessage = decodeSysExData(std::vector<uint16_t>(sysexData, sysexData + sysexDataSize));

            device = midiMessage.back();
            midiMessage.pop_back();
            return MidiMessage(midiMessage.data(), midiMessage.size());
        }

        device = 0;
        return m;
    }
    
    MidiDeviceManager(MidiInputCallback* inputCallback)
    {
#if !JUCE_WINDOWS
    if (ProjectInfo::isStandalone) {
        if (auto* newOut = MidiOutput::createNewDevice("from plugdata").release()) {
            fromPlugdata.reset(newOut);
        }
        if (auto* newIn = MidiInput::createNewDevice("to plugdata", inputCallback).release()) {
            toPlugdata.reset(newIn);
        }
    }
#endif
        if(auto* deviceManager = ProjectInfo::getDeviceManager())
        {
            deviceManager->addChangeListener(this);
        }

        sortedMidiInputs = sortedMidiOutputs = 0;
        updateMidiDevices();
    }

    ~MidiDeviceManager()
    {
        clearSortedMidiInputs();
        clearSortedMidiOutputs();
    }

    void changeListenerCallback(ChangeBroadcaster* origin) override
    {
        updateMidiDevices();
    }

    void clearSortedMidiInputs()
    {
        if (sortedMidiInputs) delete sortedMidiInputs;
        sortedMidiInputs = 0;
    }

    void clearSortedMidiOutputs()
    {
        if (sortedMidiOutputs) delete sortedMidiOutputs;
        sortedMidiOutputs = 0;
    }

    void updateMidiDevices()
    {
        midiDeviceMutex.lock();
        lastMidiInputs = MidiInput::getAvailableDevices();
        lastMidiOutputs = MidiOutput::getAvailableDevices();
        
        for(int i = 0; i < lastMidiInputs.size(); i++)
        {
            if(toPlugdata && lastMidiInputs[i].name == "from plugdata")
            {
                lastMidiInputs.set(i, toPlugdata->getDeviceInfo());
            }
        }
        
        for(int i = 0; i < lastMidiOutputs.size(); i++)
        {
            if(fromPlugdata && lastMidiOutputs[i].name == "to plugdata")
            {
                lastMidiOutputs.set(i, fromPlugdata->getDeviceInfo());
            }
        }
        
        midiDeviceMutex.unlock();
        clearSortedMidiInputs();
        clearSortedMidiOutputs();
    }
    
    Array<MidiDeviceInfo> getInputDevicesUnsorted()
    {
        return lastMidiInputs;
    }
    
    Array<MidiDeviceInfo> getOutputDevicesUnsorted()
    {
        return lastMidiOutputs;
    }

    // helper class to sort devices by their enabled status
    class compareDevs {
      MidiDeviceManager *self;
      bool isInput;
    public:
      compareDevs(MidiDeviceManager *x, bool in) : self(x), isInput(in) {}
      int compareElements (const MidiDeviceInfo& dev1, const MidiDeviceInfo& dev2)
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
    
    Array<MidiDeviceInfo> getInputDevices()
    {
        if (!ProjectInfo::getDeviceManager()) {
            // just in case we get called during startup when the device
            // manager hasn't been created yet
            return lastMidiInputs;
        }
        if (!sortedMidiInputs) {
            // we cache the sorted device list so that we don't have to
            // recompute it each time
            midiDeviceMutex.lock();
            sortedMidiInputs = new Array<MidiDeviceInfo>(lastMidiInputs);
            midiDeviceMutex.unlock();
            compareDevs cmp(this, true);
            sortedMidiInputs->sort(cmp, true);
        }
        return *sortedMidiInputs;
    }
    
    Array<MidiDeviceInfo> getOutputDevices()
    {
        if (!ProjectInfo::getDeviceManager()) {
            return lastMidiOutputs;
        }
        if (!sortedMidiOutputs) {
            midiDeviceMutex.lock();
            sortedMidiOutputs = new Array<MidiDeviceInfo>(lastMidiOutputs);
            midiDeviceMutex.unlock();
            compareDevs cmp(this, false);
            sortedMidiOutputs->sort(cmp, true);
        }
        return *sortedMidiOutputs;
    }
    
    bool isMidiDeviceEnabled(bool isInput, const String& identifier)
    {
        if(fromPlugdata && identifier == fromPlugdata->getIdentifier())
        {
            return internalOutputEnabled;
        }
        if(toPlugdata && identifier == toPlugdata->getIdentifier())
        {
            return internalInputEnabled;
        }
        if(isInput)
        {
            return ProjectInfo::getDeviceManager()->isMidiInputDeviceEnabled(identifier);
        }
        else {
            for (auto* midiOut : midiOutputs) {
                if (midiOut->getIdentifier() == identifier) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void setMidiDeviceEnabled(bool isInput, const String& identifier, bool shouldBeEnabled)
    {
        if(fromPlugdata && identifier == fromPlugdata->getIdentifier())
        {
            if(shouldBeEnabled != internalOutputEnabled)
                clearSortedMidiOutputs();
            internalOutputEnabled = shouldBeEnabled;
        }
        else if(toPlugdata && identifier == toPlugdata->getIdentifier())
        {
            if(shouldBeEnabled != internalInputEnabled) {
                clearSortedMidiInputs();
                internalInputEnabled = shouldBeEnabled;
                if(internalInputEnabled)
                {
                    toPlugdata->start();
                }
                else {
                    toPlugdata->stop();
                }
            }
        }
        else if(isInput)
        {
            if (shouldBeEnabled != isMidiDeviceEnabled(true, identifier)) {
                ProjectInfo::getDeviceManager()->setMidiInputDeviceEnabled(identifier, shouldBeEnabled);
                clearSortedMidiInputs();
            }
        }
        else if(shouldBeEnabled != isMidiDeviceEnabled(false, identifier)) {
            clearSortedMidiOutputs();
            if(shouldBeEnabled)
            {
                auto* device = midiOutputs.add(MidiOutput::openDevice(identifier));
                if(device) device->startBackgroundThread();
            }
            else {
                for (auto* midiOut : midiOutputs) {
                    if (midiOut->getIdentifier() == identifier) {
                        midiOutputs.removeObject(midiOut);
                    }
                }
            }
        }
    }
    
    MidiOutput* getMidiOutputByIndexIfEnabled(int index)
    {
        auto idToFind = getOutputDevices()[index].identifier;
        // The order of midiOutputs is not necessarily the same as that of lastMidiOutputs, that's why we need to check
        
        if(idToFind == fromPlugdata->getIdentifier())
        {
            return internalOutputEnabled ? fromPlugdata.get() : nullptr;
        }
        for(auto* midiOutput : midiOutputs)
        {
            if(idToFind == midiOutput->getIdentifier())
            {
                return midiOutput;
            }
        }
        
        return nullptr;
    }
    
    int getMidiInputDeviceIndex(const String& identifier)
    {
        auto devices = getInputDevices();
        
        for(int i = 0; i < devices.size(); i++)
        {
            if(devices[i].identifier == identifier)
            {
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

    Array<MidiDeviceInfo> *sortedMidiInputs;
    Array<MidiDeviceInfo> *sortedMidiOutputs;
};
