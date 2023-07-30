/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once


// Helper function to encode regular MIDI events into a sysex event
// The reason we do this, is that we want to append extra information to the MIDI event when it comes in from pd or the device, but JUCE won't allow this
// We still want to be able to use handy JUCE stuff for MIDI timing, so we treat every MIDI event as sysex
struct MidiHelperFunctions
{
    static std::vector<uint8> encodeSysExData(const std::vector<uint8>& originalData)
    {
        std::vector<uint8> encodedData;
        for (auto byte : originalData)
        {
            if (byte == 0xF0 || byte == 0xF7)
            {
                encodedData.push_back(0xFF); // Escape byte
                encodedData.push_back(byte ^ 0x20); // XOR with 0x20 (space character) to differentiate from original data
            }
            else
            {
                encodedData.push_back(byte);
            }
        }
        return encodedData;
    }

    // Decode SysEx data by converting escape bytes back to reserved bytes (0xF0 and 0xF7)
    static std::vector<uint8> decodeSysExData(const std::vector<uint8>& encodedData)
    {
        std::vector<uint8> decodedData;
        bool escaped = false;
        for (auto byte : encodedData)
        {
            if (byte == 0xFF)
            {
                escaped = true;
            }
            else
            {
                if (escaped)
                {
                    decodedData.push_back(byte ^ 0x20);
                    escaped = false;
                }
                else
                {
                    decodedData.push_back(byte);
                }
            }
        }
        return decodedData;
    }
    
    static MidiMessage convertToSysExFormat(MidiMessage m, int device)
    {
        if(ProjectInfo::isStandalone) {
            // We append the device index so we can use it as a selector later
            const auto* data = static_cast<const uint8*>(m.getRawData());
            auto newMessage = std::vector<uint8>(data, data + m.getRawDataSize());
            newMessage.push_back(device);
            newMessage = encodeSysExData(newMessage);
            
            // Temporarily convert all messages to sysex so we can add as much data as we want
            return MidiMessage::createSysExMessage(static_cast<void*>(newMessage.data()), newMessage.size()).withTimeStamp(m.getTimeStamp());
        }
        
        return m;
    }

    static MidiMessage convertFromSysExFormat(MidiMessage m, int& device)
    {
        if(ProjectInfo::isStandalone) {
            const auto* sysexData = static_cast<const uint8*>(m.getSysExData());
            auto parsedMessage = std::vector<uint8>(sysexData, sysexData + m.getSysExDataSize());
            parsedMessage = decodeSysExData(parsedMessage);
            
            device = parsedMessage.back();
            parsedMessage.pop_back();
            return MidiMessage(parsedMessage.data(), parsedMessage.size());
        }
        
        device = 0;
        return m;
    }
};
