/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Utility/Containers.h"

class MidiDeviceManager : public ChangeListener
    , public AsyncUpdater
    , public MidiInputCallback {

public:
    MidiDeviceManager()
    {
#if !JUCE_WINDOWS && !JUCE_IOS
        if (ProjectInfo::isStandalone) {
            if (auto* newOut = MidiOutput::createNewDevice("from plugdata").release()) {
                outputPorts[-1].add(newOut);
                fromPlugdata = newOut;
            }
            if (auto* newIn = MidiInput::createNewDevice("to plugdata", this).release()) {
                inputPorts[-1].add(newIn);
                toPlugdata = newIn;
            }
        }
#endif
        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            deviceManager->addChangeListener(this);
        }

        updateMidiDevices();
        midiBufferIn.ensureSize(2048);
        midiBufferOut[0].ensureSize(2048);
    }

    ~MidiDeviceManager()
    {
        saveMidiSettings();
    }

    void prepareToPlay(float sampleRate)
    {
        currentSampleRate = sampleRate;
        for (auto& [port, collector] : midiMessageCollector)
            collector->reset(sampleRate);
    }

    void updateMidiDevices()
    {
        midiDeviceMutex.lock();
        availableMidiInputs.clear();
        availableMidiOutputs.clear();
        availableMidiInputs.add_array(MidiInput::getAvailableDevices());
        availableMidiOutputs.add_array(MidiOutput::getAvailableDevices());

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

        midiDeviceMutex.unlock();
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
        if (isInput) {
            auto portIter = inputPorts.find(port);
            if (portIter != inputPorts.end()) {
                auto numDevices = portIter->second.size();
                if (numDevices == 1) {
                    return "Port " + String(port + 1) + " (" + String(portIter->second[0]->getName()) + ")";
                } else if (numDevices > 0) {
                    return "Port " + String(port + 1) + " (" + String(numDevices) + " devices)";
                }
            }
        } else {
            auto portIter = outputPorts.find(port);
            if (portIter != outputPorts.end()) {
                auto numDevices = portIter->second.size();
                if (numDevices == 1) {
                    return "Port " + String(port + 1) + " (" + String(portIter->second[0]->getName()) + ")";
                } else if (numDevices > 0) {
                    return "Port " + String(port + 1) + " (" + String(numDevices) + " devices)";
                }
            }
        }

        return "Port " + String(port + 1);
    }

    int getMidiDevicePort(bool isInput, String const& identifier)
    {
        if (fromPlugdata && identifier == fromPlugdata->getIdentifier()) {
            isInput = false;
        }
        if (toPlugdata && identifier == toPlugdata->getIdentifier()) {
            isInput = true;
        }

        if (isInput) {
            for (auto& [port, devices] : inputPorts) {
                auto hasDevice = std::find_if(devices.begin(), devices.end(), [identifier](MidiInput* input) { return input->getIdentifier() == identifier; }) != devices.end();
                if (hasDevice)
                    return port;
            }
        } else {
            for (auto& [port, devices] : outputPorts) {
                auto hasDevice = std::find_if(devices.begin(), devices.end(), [identifier](MidiOutput* output) { return output->getIdentifier() == identifier; }) != devices.end();
                if (hasDevice)
                    return port;
            }
        }

        return -1;
    }

    // Helper to move internal MIDI ports inside the
    template<typename T>
    T* moveMidiDevice(UnorderedSegmentedMap<int, OwnedArray<T>>& ports, String const& identifier, int targetPort)
    {
        for (auto& [port, devices] : ports) {
            auto deviceIter = std::find_if(devices.begin(), devices.end(), [identifier](auto* device) { return device && (device->getIdentifier() == identifier); });
            if (deviceIter != devices.end()) {
                int idx = std::distance(devices.begin(), deviceIter);
                auto* device = devices.removeAndReturn(idx);
                ports[targetPort].add(device);
                return device;
            }
        }

        return nullptr;
    }

    void setMidiDevicePort(bool isInput, String const& identifier, int port)
    {
        bool shouldBeEnabled = port >= 0;
        if (isInput) {
            auto* device = moveMidiDevice(inputPorts, identifier, port);
            if (!device && shouldBeEnabled) {
                if (auto midiIn = MidiInput::openDevice(identifier, this)) {
                    auto* input = inputPorts[port].add(midiIn.release());
                    input->start();
                }
            } else if (device && shouldBeEnabled) {
                device->start();
            } else if (device && !shouldBeEnabled) {
                device->stop();
            }
        } else {
            auto* device = moveMidiDevice(outputPorts, identifier, port);
            if (!device && shouldBeEnabled) {
                if (auto midiOut = MidiOutput::openDevice(identifier)) {
                    auto* output = outputPorts[port].add(midiOut.release());
                    output->startBackgroundThread();
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
        auto collectorIter = midiMessageCollector.find(port);
        if (collectorIter == midiMessageCollector.end()) {
            midiMessageCollector[port] = std::make_unique<MidiMessageCollector>();
            collectorIter = midiMessageCollector.find(port);
            (*collectorIter).second->reset(currentSampleRate);
        }
        for (auto event : buffer) {
            (*collectorIter).second->addMessageToQueue(event.getMessage());
        }
    }

    // Handle midi input events in a callback
    void dequeueMidiInput(int blockSize, std::function<void(int, int, MidiBuffer&)> inputCallback)
    {
        for (auto& [port, collector] : midiMessageCollector) {
            if (port < 0)
                continue;
            midiBufferIn.clear();
            collector->removeNextBlockOfMessages(midiBufferIn, blockSize);
            inputCallback(port, blockSize, midiBufferIn);
        }
    }

    // Adds output message to buffer
    void enqueueMidiOutput(int port, MidiMessage const& message, int samplePosition)
    {
        midiBufferOut[port].addEvent(message, samplePosition);
    }

    // Read output buffer for a port. Used to pass back into the DAW or into the internal GM synth
    void dequeueMidiOutput(int port, MidiBuffer& buffer, int numSamples)
    {
        auto& outputBuffer = midiBufferOut[port];
        buffer.addEvents(outputBuffer, 0, numSamples, 0);
    }

    // Send all MIDI output to target devices
    void sendMidiOutput()
    {
        for (auto& [port, events] : midiBufferOut) {
            if (port < 0)
                continue;

            for (auto* device : outputPorts[port]) {
                device->sendBlockOfMessages(events, Time::getMillisecondCounterHiRes(), currentSampleRate);
            }
            events.clear();
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
            auto port = midiPort.hasProperty("Port") ? static_cast<int>(midiPort.getProperty("Port")) : -1;
            for (auto& output : availableMidiOutputs) {
                if (output.name == name) {
                    setMidiDevicePort(false, output.identifier, port);
                    break;
                }
            }
        }

        auto midiInputsTree = settingsTree.getChildWithName("EnabledMidiInputPorts");
        for (int i = 0; i < midiInputsTree.getNumChildren(); i++) {
            auto midiPort = midiInputsTree.getChild(i);
            auto name = midiPort.getProperty("Name").toString();
            auto port = midiPort.hasProperty("Port") ? static_cast<int>(midiPort.getProperty("Port")) : -1;
            for (auto& input : availableMidiInputs) {
                if (input.name == name) {
                    setMidiDevicePort(true, input.identifier, port);
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

        for (auto const& [port, devices] : outputPorts) {
            if (port < 0)
                continue;
            for (auto const* device : devices) {
                ValueTree midiOutputPort("MidiPort");
                midiOutputPort.setProperty("Name", device->getName(), nullptr);
                midiOutputPort.setProperty("Port", port, nullptr);
                midiOutputsTree.appendChild(midiOutputPort, nullptr);
            }
        }

        auto midiInputsTree = SettingsFile::getInstance()->getValueTree().getChildWithName("EnabledMidiInputPorts");
        midiInputsTree.removeAllChildren(nullptr);
        for (auto const& [port, devices] : inputPorts) {
            if (port < 0)
                continue;
            for (auto const* device : devices) {
                ValueTree midiInputPort("MidiPort");
                midiInputPort.setProperty("Name", device->getName(), nullptr);
                midiInputPort.setProperty("Port", port, nullptr);
                midiInputsTree.appendChild(midiInputPort, nullptr);
            }
        }
    }

    void getLastMidiOutputEvents(MidiBuffer& buffer, int numSamples)
    {
        for (auto& [port, events] : midiBufferOut) {
            if (port < 0)
                continue;

            buffer.addEvents(events, 0, numSamples, 0);
        }
    }

private:
    void handleIncomingMidiMessage(MidiInput* input, MidiMessage const& message) override
    {
        auto port = [this, input]() {
            for (auto& [port, devices] : inputPorts) {
                if (devices.contains(input))
                    return port;
            }
            return -1;
        }();

        if (port >= 0) {
            auto collectorIter = midiMessageCollector.find(port);
            if (collectorIter == midiMessageCollector.end()) {
                midiMessageCollector[port] = std::make_unique<MidiMessageCollector>();
                collectorIter = midiMessageCollector.find(port);
                (*collectorIter).second->reset(currentSampleRate);
            }
            collectorIter->second->addMessageToQueue(message);
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

    MidiBuffer midiBufferIn;
    UnorderedSegmentedMap<int, MidiBuffer> midiBufferOut;
    UnorderedSegmentedMap<int, std::unique_ptr<MidiMessageCollector>> midiMessageCollector;
    UnorderedSegmentedMap<int, OwnedArray<MidiInput>> inputPorts;
    UnorderedSegmentedMap<int, OwnedArray<MidiOutput>> outputPorts;

    MidiInput* toPlugdata = nullptr;
    MidiOutput* fromPlugdata = nullptr;

    std::mutex midiDeviceMutex;
    SmallArray<MidiDeviceInfo> availableMidiInputs;
    SmallArray<MidiDeviceInfo> availableMidiOutputs;
};
