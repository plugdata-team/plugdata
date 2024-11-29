/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <concurrentqueue/concurrentqueue.h>
#include "Utility/Containers.h"

class MidiDeviceManager : public ChangeListener
    , public AsyncUpdater
    , public MidiInputCallback
{

public:
    MidiDeviceManager()
    {
#if !JUCE_WINDOWS && !JUCE_IOS
        if (ProjectInfo::isStandalone) {
            toPlugdata = inputPorts[0].devices.add(MidiInput::createNewDevice("to plugdata", this));
            fromPlugdata = outputPorts[0].devices.add(MidiOutput::createNewDevice("from plugdata"));
        }
#endif
        
        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            deviceManager->addChangeListener(this);
        }

        updateMidiDevices();
        midiBufferIn.ensureSize(2048);
        midiBufferOut.ensureSize(2048);
    }

    ~MidiDeviceManager()
    {
        saveMidiSettings();
    }

    void prepareToPlay(float sampleRate)
    {
        currentSampleRate = sampleRate;
        lastCallbackTime = Time::getMillisecondCounterHiRes();
    }

    void updateMidiDevices()
    {
        availableMidiInputs.clear();
        availableMidiOutputs.clear();
        availableMidiInputs.add_array(MidiInput::getAvailableDevices());
        availableMidiOutputs.add_array(MidiOutput::getAvailableDevices());
        
#if !JUCE_WINDOWS && !JUCE_IOS
        if (ProjectInfo::isStandalone) {
            for (int i = 0; i < availableMidiInputs.size(); i++) {
                if (toPlugdata && availableMidiInputs[i].name == "from plugdata") {
                    availableMidiInputs[i] = toPlugdata->getDeviceInfo();
                }
            }

            for (int i = 0; i < availableMidiOutputs.size(); i++) {
                if (fromPlugdata && availableMidiOutputs[i].name == "to plugdata") {
                    availableMidiOutputs[i] = fromPlugdata->getDeviceInfo();
                }
            }
        }
#endif
    }

    SmallArray<MidiDeviceInfo> getInputDevices()
    {
        return availableMidiInputs;
    }

    SmallArray<MidiDeviceInfo> getOutputDevices()
    {
        return availableMidiOutputs;
    }

    String getPortDescription(bool isInput, int port)
    {
        if (isInput && inputPorts[port].enabled) {
            auto numDevices = inputPorts[port].devices.size();
            if (numDevices == 1) {
                return "Port " + String(port + 1) + " (" + String(inputPorts[port].devices.getFirst()->getName()) + ")";
            } else if (numDevices > 0) {
                return "Port " + String(port + 1) + " (" + String(numDevices) + " devices)";
            }
        } else if(!isInput && outputPorts[port].enabled){
            auto numDevices = outputPorts[port].devices.size();
            if (numDevices == 1) {
                return "Port " + String(port + 1) + " (" + String(outputPorts[port].devices.getFirst()->getName()) + ")";
            } else if (numDevices > 0) {
                return "Port " + String(port + 1) + " (" + String(numDevices) + " devices)";
            }
        }

        return "Port " + String(port + 1);
    }

    int getMidiDevicePort(bool isInput, MidiDeviceInfo& info)
    {
        int portIndex = -1;
        if (isInput) {
            for (auto& port : inputPorts) {
                auto hasDevice = std::find_if(port.devices.begin(), port.devices.end(), [info](MidiInput* input) { return input->getIdentifier() == info.identifier; }) != port.devices.end();
                if (hasDevice)
                    return portIndex;
                portIndex++;
            }
        } else {
            for (auto& port : outputPorts) {
                auto hasDevice = std::find_if(port.devices.begin(), port.devices.end(), [info](MidiOutput* output) { return output->getIdentifier() == info.identifier; }) != port.devices.end();
                if (hasDevice)
                    return portIndex;
                portIndex++;
            }
        }

        return -1;
    }

    template<typename MidiDeviceType, typename T>
    MidiDeviceType* moveMidiDevice(StackArray<T, 9>& ports, String const& identifier, int targetPort)
    {
        int portIdx = 0;
        for (auto& port : ports) {
            auto deviceIter = std::find_if(port.devices.begin(), port.devices.end(), [identifier](auto* device) { return device && (device->getIdentifier() == identifier); });
            if (deviceIter != port.devices.end()) {
                if(targetPort == portIdx && ports[targetPort].enabled)
                    return nullptr;
                
                int idx = std::distance(port.devices.begin(), deviceIter);
                auto* device = port.devices.removeAndReturn(idx);
                ports[targetPort].devices.add(device);
                port.enabled = port.devices.size() && portIdx;
                ports[targetPort].enabled = targetPort >= 1;
                return device;
            }
            portIdx++;
        }

        return nullptr;
    }
    
    
    void setMidiDevicePort(bool isInput, String const& name, String const& identifier, int port)
    {
        bool shouldBeEnabled = port >= 0;
        if (isInput) {
            auto* device = moveMidiDevice<MidiInput>(inputPorts, identifier, port + 1);
            if (!device && shouldBeEnabled) {
                if (auto midiIn = MidiInput::openDevice(identifier, this)) {
                    auto* input = inputPorts[port + 1].devices.add(midiIn.release());
                    input->start();
                    inputPorts[port + 1].enabled = true;
                }
            } else if (device && shouldBeEnabled) {
                device->start();
            } else if (device && !shouldBeEnabled) {
                device->stop();
            }
        } else {
            auto* device = moveMidiDevice<MidiOutput>(outputPorts, identifier, port + 1);
            
            if (!device && shouldBeEnabled) {
                if (auto midiOut = MidiOutput::openDevice(identifier)) {
                    auto* output = outputPorts[port + 1].devices.add(midiOut.release());
                    output->startBackgroundThread();
                    outputPorts[port + 1].enabled = true;
                }
            } else if (device && shouldBeEnabled) {
                device->startBackgroundThread();
            } else if (device && !shouldBeEnabled) {
                device->stopBackgroundThread();
            }
        }
        triggerAsyncUpdate();
    }

    // Function to enqueue external MIDI (like the DAW's MIDI coming in with processBlock)
    void enqueueMidiInput(int port, MidiBuffer& buffer)
    {
        auto& inputPort = inputPorts[port + 1];
        if(inputPort.enabled)
        {
            for(auto m : buffer)
            {
                auto message = m.getMessage();
                auto sampleNumber = (int) ((message.getTimeStamp() - 0.001 * lastCallbackTime) * currentSampleRate);
                inputPort.queue.enqueue({message, sampleNumber});
            }
        }
    }

    // Handle midi input events in a callback
    void dequeueMidiInput(int numSamples, std::function<void(int, int, MidiBuffer&)> inputCallback)
    {
        auto timeNow = Time::getMillisecondCounterHiRes();
        auto msElapsed = timeNow - lastCallbackTime;

        lastCallbackTime = timeNow;
        int numSourceSamples = jmax (1, roundToInt (msElapsed * 0.001 * currentSampleRate));
        int startSample = 0;
        int scale = 1 << 16;
        
        int port = 0;
        for (auto& inputPort : inputPorts) {
            if (!inputPort.enabled) continue;
            
            midiBufferIn.clear();
            std::pair<MidiMessage, int> message;
            
            if(numSourceSamples > numSamples) {
                const int maxBlockLengthToUse = numSamples << 5;
                if (numSourceSamples > maxBlockLengthToUse)
                {
                    // TODO: check if this is correct
                     startSample = numSourceSamples - maxBlockLengthToUse;
                     numSourceSamples = maxBlockLengthToUse;
                }
                
                scale = (numSamples << 10) / numSourceSamples;
                
                while(inputPort.queue.try_dequeue(message))
                {
                    auto& [midiMessage, samplePosition] = message;
                    const auto pos = ((samplePosition - startSample) * scale) >> 10;
                    midiBufferIn.addEvent(midiMessage, pos);
                }
                inputCallback(port, numSamples, midiBufferIn);
            }
            else {
                startSample = numSamples - numSourceSamples;
                while(inputPort.queue.try_dequeue(message))
                {
                    auto& [midiMessage, samplePosition] = message;
                    const auto pos = jlimit (0, numSamples - 1, samplePosition + startSample);
                    midiBufferIn.addEvent(midiMessage, pos);
                }
                inputCallback(port, numSamples, midiBufferIn);
            }
            port++;
        }
    }

    // Adds output message to buffer
    void enqueueMidiOutput(int port, MidiMessage const& message, int samplePosition)
    {
        auto& outputPort = outputPorts[port + 1];
        if(outputPort.enabled)
        {
            outputPort.queue.enqueue({message, samplePosition});
        }
    }

    // Read output buffer for a port. Used to pass back into the DAW or into the internal GM synth
    void dequeueMidiOutput(int port, MidiBuffer& buffer, int numSamples)
    {
        auto& outputPort = outputPorts[port + 1];
        if (outputPort.enabled) {
            std::pair<MidiMessage, int> message;
            while(outputPort.queue.try_dequeue(message))
            {
                auto& [midiMessage, samplePosition] = message;
                outputPort.buffer.addEvent(midiMessage, samplePosition);
            }
            buffer.addEvents(buffer, 0, numSamples, 0);
        }
    }

    // Sends pending MIDI output messages, and return a block with all messages
    void sendAndCollectMidiOutput(MidiBuffer& allOutputBuffer)
    {
        for (auto& outputPort : outputPorts) {
            if(outputPort.enabled)
            {
                std::pair<MidiMessage, int> message;
                while(outputPort.queue.try_dequeue(message))
                {
                    auto& [midiMessage, samplePosition] = message;
                    outputPort.buffer.addEvent(midiMessage, samplePosition);
                    allOutputBuffer.addEvent(midiMessage, samplePosition);
                }
                
                if(!outputPort.buffer.isEmpty()) {
                    for (auto* device : outputPort.devices) {
                        device->sendBlockOfMessages(outputPort.buffer, Time::getMillisecondCounterHiRes(), currentSampleRate);
                    }
                    outputPort.buffer.clear();
                }
            }
        }
    }
    
    void clearMidiOutputBuffers()
    {
        for (auto& outputPort : outputPorts) {
            if(outputPort.enabled && !outputPort.buffer.isEmpty())
            {
                outputPort.buffer.clear();
            }
        }
    }
    
    // Load last MIDI settings from our settings file
    void loadMidiSettings()
    {
        updateMidiDevices();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto midiOutputsTree = settingsTree.getChildWithName("EnabledMidiOutputPorts");

        for (int i = 0; i < midiOutputsTree.getNumChildren(); i++) {
            auto midiPort = midiOutputsTree.getChild(i);
            auto name = midiPort.getProperty("Name").toString();
            auto port = midiPort.hasProperty("Port") ? static_cast<int>(midiPort.getProperty("Port")) : 0;
            for (auto& output : availableMidiOutputs) {
                if (output.name == name) {
                    setMidiDevicePort(false, output.name, output.identifier, port);
                    break;
                }
            }
        }

        auto midiInputsTree = settingsTree.getChildWithName("EnabledMidiInputPorts");
        for (int i = 0; i < midiInputsTree.getNumChildren(); i++) {
            auto midiPort = midiInputsTree.getChild(i);
            auto name = midiPort.getProperty("Name").toString();
            auto port = midiPort.hasProperty("Port") ? static_cast<int>(midiPort.getProperty("Port")) : 0;
            for (auto& input : availableMidiInputs) {
                if (input.name == name) {
                    setMidiDevicePort(true, input.name, input.identifier, port);
                    break;
                }
            }
        }
    }

    // Store current MIDI settings in our settings file
    void saveMidiSettings()
    {
        auto midiOutputsTree = SettingsFile::getInstance()->getValueTree().getChildWithName("EnabledMidiOutputPorts");

        midiOutputsTree.removeAllChildren(nullptr);

        for (auto& port : outputPorts) {
            if(!port.enabled) continue;
            for (auto const* device : port.devices) {
                ValueTree midiOutputPort("MidiPort");
                midiOutputPort.setProperty("Name", device->getName(), nullptr);
                midiOutputPort.setProperty("Port", outputPorts.index_of_address(port) - 1, nullptr);
                midiOutputsTree.appendChild(midiOutputPort, nullptr);
            }
        }

        auto midiInputsTree = SettingsFile::getInstance()->getValueTree().getChildWithName("EnabledMidiInputPorts");
        midiInputsTree.removeAllChildren(nullptr);
        for (auto& port : inputPorts) {
            if(!port.enabled) continue;
            for (auto const* device : port.devices) {
                ValueTree midiInputPort("MidiPort");
                midiInputPort.setProperty("Name", device->getName(), nullptr);
                midiInputPort.setProperty("Port", inputPorts.index_of_address(port) - 1, nullptr);
                midiInputsTree.appendChild(midiInputPort, nullptr);
            }
        }
    }

    void getLastMidiOutputEvents(MidiBuffer& buffer, int numSamples)
    {
        for (auto& port : outputPorts) {
            if (!port.enabled) continue;
            buffer.addEvents(port.buffer, 0, numSamples, 0);
        }
    }

private:
    
    void handleIncomingMidiMessage(MidiInput* input, MidiMessage const& message) override
    {
        auto port = [this, input]() -> int {
            int portNum = 0;
            for (auto& port : inputPorts) {
                if (port.devices.contains(input))
                    return portNum;
                portNum++;
            }
            return 0;
        }();

        if (inputPorts[port].enabled) {
            auto sampleNumber = (int) ((message.getTimeStamp() - 0.001f * lastCallbackTime) * currentSampleRate);
            inputPorts[port].queue.enqueue({message, sampleNumber});
        }
    }

    void handleAsyncUpdate() override
    {
        saveMidiSettings();
    }

    void changeListenerCallback(ChangeBroadcaster* origin) override
    {
        updateMidiDevices();
    }

    float currentSampleRate = 44100.f;
    std::atomic<float> lastCallbackTime = 0.0f;

    struct MidiInputPort
    {
        std::atomic<bool> enabled = false;
        OwnedArray<MidiInput> devices;
        moodycamel::ConcurrentQueue<std::pair<MidiMessage, int>> queue;
    };
    
    struct MidiOutputPort
    {
        std::atomic<bool> enabled = false;
        OwnedArray<MidiOutput> devices;
        // We need this queue because we are not inside the global Pd lock, and MIDI data can be enqueued from the message thread (but inside the audio lock)
        moodycamel::ConcurrentQueue<std::pair<MidiMessage, int>> queue;
        MidiBuffer buffer;
    };
   
    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    
    MidiInput* toPlugdata = nullptr;
    MidiOutput* fromPlugdata = nullptr;
        
    StackArray<MidiInputPort, 9> inputPorts;
    StackArray<MidiOutputPort, 9> outputPorts;
    
    SmallArray<MidiDeviceInfo> availableMidiInputs;
    SmallArray<MidiDeviceInfo> availableMidiOutputs;
};
