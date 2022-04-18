/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PluginProcessor.h"

#include "Canvas.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

AudioProcessor::BusesProperties PlugDataAudioProcessor::buildBusesProperties()
{
    AudioProcessor::BusesProperties busesProperties;
    
    busesProperties.addBus(true, "Main Input", AudioChannelSet::stereo(), true);
    
    for (int i = 1; i < numInputBuses; i++) busesProperties.addBus(true, "Aux Input " + String(i), AudioChannelSet::stereo(), false);
    
    busesProperties.addBus(false, "Main Output", AudioChannelSet::stereo(), true);
    
    for (int i = 1; i < numOutputBuses; i++) busesProperties.addBus(false, "Aux " + String(i), AudioChannelSet::stereo(), false);
    
    return busesProperties;
}

PlugDataAudioProcessor::PlugDataAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor(buildBusesProperties()),
#endif
pd::Instance("PlugData"),
parameters(*this, nullptr)
{
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>("volume", "Volume", NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.75f, false), 1.0f));
    
    parameters.createAndAddParameter(std::make_unique<AudioParameterBool>("enabled", "Enabled", true));
    
    // General purpose automation parameters you can get by using "receive param1" etc.
    for (int n = 0; n < numParameters; n++)
    {
        String id = "param" + String(n + 1);
        parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(id, "Parameter " + String(n + 1), 0.0f, 1.0f, 0.0f));
        parameterValues[n] = parameters.getRawParameterValue(id);
        lastParameters[n] = 0;
    }
    
    volume = parameters.getRawParameterValue("volume");
    enabled = parameters.getRawParameterValue("enabled");
    
    parameters.replaceState(ValueTree("PlugData"));
    
    // On first startup, initialise abstractions and settings
    initialiseFilesystem();
    
    
    // Initialise library for text autocompletion
    objectLibrary.initialiseLibrary();
    
    // Update pd search paths for abstractions
    updateSearchPaths();
    
    // Set up midi buffers
    midiBufferIn.ensureSize(2048);
    midiBufferOut.ensureSize(2048);
    midiBufferTemp.ensureSize(2048);
    midiBufferCopy.ensureSize(2048);

    atoms_playhead.reserve(3);
    atoms_playhead.resize(1);
    
    
    setCallbackLock(&AudioProcessor::getCallbackLock());
    
    sendMessagesFromQueue();
    
    LookAndFeel::setDefaultLookAndFeel(&lnf.get());
    
    objectLibrary.appDirChanged = [this]()
    {
        auto newTree = ValueTree::fromXml(settingsFile.loadFileAsString());
        settingsTree.copyPropertiesAndChildrenFrom(newTree, nullptr);
        updateSearchPaths();
    };
    
    logMessage("PlugData v" + String(ProjectInfo::versionString));
}

PlugDataAudioProcessor::~PlugDataAudioProcessor()
{
    // Save current settings before quitting
    saveSettings();
}

void PlugDataAudioProcessor::initialiseFilesystem()
{
    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!appDir.exists() || !abstractions.exists())
    {
        appDir.createDirectory();
        
        MemoryInputStream binaryAbstractions(BinaryData::Abstractions_zip, BinaryData::Abstractions_zipSize, false);
        auto file = ZipFile(binaryAbstractions);
        file.uncompressTo(appDir);
    }
    
    // Check if settings file exists, if not, create the default
    if (!settingsFile.existsAsFile())
    {
        settingsFile.create();
        
        // Add default settings
        settingsTree.setProperty("ConnectionStyle", false, nullptr);
        
        auto pathTree = ValueTree("Paths");
        
        auto defaultPath = ValueTree("Path");
        defaultPath.setProperty("Path", abstractions.getFullPathName(), nullptr);
        
        pathTree.appendChild(defaultPath, nullptr);
        settingsTree.appendChild(pathTree, nullptr);
        
        settingsTree.appendChild(ValueTree("Keymap"), nullptr);
        
        saveSettings();
    }
    else
    {
        // Or load the settings when they exist already
        settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());
    }
}

void PlugDataAudioProcessor::saveSettings()
{
    // Save settings to file
    auto xml = settingsTree.toXmlString();
    settingsFile.replaceWithText(xml);
}

void PlugDataAudioProcessor::updateSearchPaths()
{
    // Reload pd search paths from settings
    auto pathTree = settingsTree.getChildWithName("Paths");
    
    libpd_clear_search_path();
    for (auto child : pathTree)
    {
        auto path = child.getProperty("Path").toString();
        libpd_add_to_search_path(path.toRawUTF8());
    }
    
    objectLibrary.updateLibrary();
}

const String PlugDataAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PlugDataAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PlugDataAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PlugDataAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PlugDataAudioProcessor::getTailLengthSeconds() const
{
    return static_cast<float>(tailLength.getValue());
}

int PlugDataAudioProcessor::getNumPrograms()
{
    return 1;  // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int PlugDataAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PlugDataAudioProcessor::setCurrentProgram(int index)
{
}

const String PlugDataAudioProcessor::getProgramName(int index)
{
    return {};
}

void PlugDataAudioProcessor::changeProgramName(int index, const String& newName)
{
}

void PlugDataAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    prepareDSP(getTotalNumInputChannels(), getTotalNumOutputChannels(), sampleRate);
    // sendCurrentBusesLayoutInformation();
    audioAdvancement = 0;
    const auto blksize = static_cast<size_t>(Instance::getBlockSize());
    const auto numIn = std::max(static_cast<size_t>(getTotalNumInputChannels()), static_cast<size_t>(2));
    const auto nouts = std::max(static_cast<size_t>(getTotalNumOutputChannels()), static_cast<size_t>(2));
    audioBufferIn.resize(numIn * blksize);
    audioBufferOut.resize(nouts * blksize);
    std::fill(audioBufferOut.begin(), audioBufferOut.end(), 0.f);
    std::fill(audioBufferIn.begin(), audioBufferIn.end(), 0.f);
    midiBufferIn.clear();
    midiBufferOut.clear();
    midiBufferTemp.clear();
    
    midiByteIndex = 0;
    midiByteBuffer[0] = 0;
    midiByteBuffer[1] = 0;
    midiByteBuffer[2] = 0;
    startDSP();
    processingBuffer.setSize(2, samplesPerBlock);
    
    statusbarSource.prepareToPlay(getTotalNumOutputChannels());
    
    //audioStarted = true;
}

void PlugDataAudioProcessor::releaseResources()
{
    releaseDSP();
    //audioStarted = false;
}

//#ifndef JucePlugin_PreferredChannelConfigurations
bool PlugDataAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#endif
    
    int ninch = 0;
    int noutch = 0;
    for (int bus = 0; bus < layouts.outputBuses.size(); bus++)
    {
        int nchb = layouts.getNumChannels(false, bus);
        
        if(layouts.outputBuses[bus].isDisabled()) continue;
        
        if (nchb > 2)
            return false;
        if (nchb == 0)
            return false;
        noutch += nchb;
    }
    
    for (int bus = 0; bus < layouts.inputBuses.size(); bus++)
    {
        int nchb = layouts.getNumChannels(true, bus);
        
        if(layouts.inputBuses[bus].isDisabled()) continue;
        
        if (nchb > 2)
            return false;
        if (nchb == 0)
            return false;
        ninch += nchb;
    }
    
    if (ninch > 32 || noutch > 32)
        return false;
    
    return true;
}

void PlugDataAudioProcessor::processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    
    processingBuffer.setSize(2, buffer.getNumSamples());
    
    processingBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    processingBuffer.copyFrom(1, 0, buffer, totalNumInputChannels == 2 ? 1 : 0, 0, buffer.getNumSamples());
}

void PlugDataAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (int n = 0; n < numParameters; n++)
    {
        if (parameterValues[n]->load() != lastParameters[n])
        {
            lastParameters[n] = parameterValues[n]->load();
            
            parameterAtom[0] = {pd::Atom(lastParameters[n])};
            
            String toSend = ("param" + String(n + 1));
            sendList(toSend.toRawUTF8(), parameterAtom);
        }
    }
    
    midiBufferCopy.clear();
    midiBufferCopy.addEvents(midiMessages, 0, buffer.getNumSamples(), audioAdvancement);
    
    process(buffer, midiMessages);
    
    buffer.applyGain(getParameters()[0]->getValue());
    
    statusbarSource.processBlock(buffer, midiBufferCopy, midiMessages);
}

void PlugDataAudioProcessor::process(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    const int blockSize = Instance::getBlockSize();
    const int numSamples = buffer.getNumSamples();
    const int adv = audioAdvancement >= 64 ? 0 : audioAdvancement;
    const int numLeft = blockSize - adv;
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const float** bufferIn = buffer.getArrayOfReadPointers();
    float** bufferOut = buffer.getArrayOfWritePointers();
    const bool midiConsume = acceptsMidi();
    const bool midiProduce = producesMidi();
    
    auto const maxOuts = std::max(numOut, buffer.getNumChannels());
    for (int i = numIn; i < maxOuts; ++i)
    {
        buffer.clear(i, 0, numSamples);
    }
    
    // If the current number of samples in this block
    // is inferior to the number of samples required
    if (numSamples < numLeft)
    {
        // we save the input samples and we output
        // the missing samples of the previous tick.
        for (int j = 0; j < numIn; ++j)
        {
            const int index = j * blockSize + adv;
            std::copy_n(bufferIn[j], numSamples, audioBufferIn.data() + index);
        }
        for (int j = 0; j < numOut; ++j)
        {
            const int index = j * blockSize + adv;
            std::copy_n(audioBufferOut.data() + index, numSamples, bufferOut[j]);
        }
        if (midiConsume)
        {
            midiBufferIn.addEvents(midiMessages, 0, numSamples, adv);
        }
        if (midiProduce)
        {
            midiMessages.clear();
            midiMessages.addEvents(midiBufferOut, adv, numSamples, -adv);
        }
        audioAdvancement += numSamples;
    }
    // If the current number of samples in this block
    // is superior to the number of samples required
    else
    {
        // we save the missing input samples, we output
        // the missing samples of the previous tick and
        // we call DSP perform method.
        MidiBuffer const& midiin = midiProduce ? midiBufferTemp : midiMessages;
        if (midiProduce)
        {
            midiBufferTemp.swapWith(midiMessages);
            midiMessages.clear();
        }
        
        for (int j = 0; j < numIn; ++j)
        {
            const int index = j * blockSize + adv;
            std::copy_n(bufferIn[j], numLeft, audioBufferIn.data() + index);
        }
        for (int j = 0; j < numOut; ++j)
        {
            const int index = j * blockSize + adv;
            std::copy_n(audioBufferOut.data() + index, numLeft, bufferOut[j]);
        }
        if (midiConsume)
        {
            midiBufferIn.addEvents(midiin, 0, numLeft, adv);
        }
        if (midiProduce)
        {
            midiMessages.addEvents(midiBufferOut, adv, numLeft, -adv);
        }
        audioAdvancement = 0;
        processInternal();
        
        // If there are other DSP ticks that can be
        // performed, then we do it now.
        int pos = numLeft;
        while ((pos + blockSize) <= numSamples)
        {
            for (int j = 0; j < numIn; ++j)
            {
                const int index = j * blockSize;
                std::copy_n(bufferIn[j] + pos, blockSize, audioBufferIn.data() + index);
            }
            for (int j = 0; j < numOut; ++j)
            {
                const int index = j * blockSize;
                std::copy_n(audioBufferOut.data() + index, blockSize, bufferOut[j] + pos);
            }
            if (midiConsume)
            {
                midiBufferIn.addEvents(midiin, pos, blockSize, 0);
            }
            if (midiProduce)
            {
                midiMessages.addEvents(midiBufferOut, 0, blockSize, pos);
            }
            processInternal();
            pos += blockSize;
        }
        
        // If there are samples that can't be
        // processed, then save them for later
        // and outputs the remaining samples
        const int remaining = numSamples - pos;
        if (remaining > 0)
        {
            for (int j = 0; j < numIn; ++j)
            {
                const int index = j * blockSize;
                std::copy_n(bufferIn[j] + pos, remaining, audioBufferIn.data() + index);
            }
            for (int j = 0; j < numOut; ++j)
            {
                const int index = j * blockSize;
                std::copy_n(audioBufferOut.data() + index, remaining, bufferOut[j] + pos);
            }
            if (midiConsume)
            {
                midiBufferIn.addEvents(midiin, pos, remaining, 0);
            }
            if (midiProduce)
            {
                midiMessages.addEvents(midiBufferOut, 0, remaining, pos);
            }
            audioAdvancement = remaining;
        }
    }
}

void PlugDataAudioProcessor::sendPlayhead()
{
    AudioPlayHead* playhead = getPlayHead();
    AudioPlayHead::CurrentPositionInfo infos;
    if(playhead && playhead->getCurrentPosition(infos))
    {
        atoms_playhead[0] = static_cast<float>(infos.isPlaying);
        sendMessage("playhead", "playing", atoms_playhead);
        atoms_playhead[0] = static_cast<float>(infos.isRecording);
        sendMessage("playhead", "recording", atoms_playhead);
        atoms_playhead[0] = static_cast<float>(infos.isLooping);
        atoms_playhead.push_back(static_cast<float>(infos.ppqLoopStart));
        atoms_playhead.push_back(static_cast<float>(infos.ppqLoopEnd));
        sendMessage("playhead", "looping", atoms_playhead);
        atoms_playhead.resize(1);
        atoms_playhead[0] = static_cast<float>(infos.editOriginTime);
        sendMessage("playhead", "edittime", atoms_playhead);
        atoms_playhead[0] = static_cast<float>(infos.frameRate.getEffectiveRate());
        
        sendMessage("playhead", "framerate", atoms_playhead);
        
        atoms_playhead[0] = static_cast<float>(infos.bpm);
        sendMessage("playhead", "bpm", atoms_playhead);
        atoms_playhead[0] = static_cast<float>(infos.ppqPositionOfLastBarStart);
        sendMessage("playhead", "lastbar", atoms_playhead);
        atoms_playhead[0] = static_cast<float>(infos.timeSigNumerator);
        atoms_playhead.push_back(static_cast<float>(infos.timeSigDenominator));
        sendMessage("playhead", "timesig", atoms_playhead);
        
        atoms_playhead[0] = static_cast<float>(infos.ppqPosition);
        atoms_playhead[1] = static_cast<float>(infos.timeInSamples);
        atoms_playhead.push_back(static_cast<float>(infos.timeInSeconds));
        sendMessage("playhead", "position", atoms_playhead);
        atoms_playhead.resize(1);
    }
}

void PlugDataAudioProcessor::messageEnqueued()
{
    if (isNonRealtime() || isSuspended())
    {
        sendMessagesFromQueue();
    }
    else
    {
        const CriticalSection* cs = getCallbackLock();
        if (cs->tryEnter())
        {
            sendMessagesFromQueue();
            cs->exit();
        }
    }
}

void PlugDataAudioProcessor::sendMidiBuffer()
{
    if (acceptsMidi())
    {
        for (const auto& event : midiBufferIn)
        {
            auto const message = event.getMessage();
            if (message.isNoteOn())
            {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), message.getVelocity());
            }
            else if (message.isNoteOff())
            {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), 0);
            }
            else if (message.isController())
            {
                sendControlChange(message.getChannel(), message.getControllerNumber(), message.getControllerValue());
            }
            else if (message.isPitchWheel())
            {
                sendPitchBend(message.getChannel(), message.getPitchWheelValue() - 8192);
            }
            else if (message.isChannelPressure())
            {
                sendAfterTouch(message.getChannel(), message.getChannelPressureValue());
            }
            else if (message.isAftertouch())
            {
                sendPolyAfterTouch(message.getChannel(), message.getNoteNumber(), message.getAfterTouchValue());
            }
            else if (message.isProgramChange())
            {
                sendProgramChange(message.getChannel(), message.getProgramChangeNumber());
            }
            else if (message.isSysEx())
            {
                for (int i = 0; i < message.getSysExDataSize(); ++i)
                {
                    sendSysEx(0, static_cast<int>(message.getSysExData()[i]));
                }
            }
            else if (message.isMidiClock() || message.isMidiStart() || message.isMidiStop() || message.isMidiContinue() || message.isActiveSense() || (message.getRawDataSize() == 1 && message.getRawData()[0] == 0xff))
            {
                for (int i = 0; i < message.getRawDataSize(); ++i)
                {
                    sendSysRealTime(0, static_cast<int>(message.getRawData()[i]));
                }
            }
            
            for (int i = 0; i < message.getRawDataSize(); i++)
            {
                sendMidiByte(0, static_cast<int>(message.getRawData()[i]));
            }
        }
        midiBufferIn.clear();
    }
}

void PlugDataAudioProcessor::processInternal()
{
    // setThis();
    
    // Dequeue messages
    sendMessagesFromQueue();
    sendPlayhead();
    sendMidiBuffer();
    
    // Process audio
    if (static_cast<bool>(enabled->load()))
    {
        std::copy_n(audioBufferOut.data() + (2 * 64), (minOut - 2) * 64, audioBufferIn.data() + (2 * 64));
        
        performDSP(audioBufferIn.data(), audioBufferOut.data());
    }
    
    else
    {
        std::fill(audioBufferIn.begin(), audioBufferIn.end(), 0.f);
        
        performDSP(audioBufferIn.data(), audioBufferOut.data());
        
        std::fill(audioBufferOut.begin(), audioBufferOut.end(), 0.f);
    }
    
    // Midi out
    
    if (producesMidi())
    {
        midiByteIndex = 0;
        midiByteBuffer[0] = 0;
        midiByteBuffer[1] = 0;
        midiByteBuffer[2] = 0;
        midiBufferOut.clear();
    }
}

bool PlugDataAudioProcessor::hasEditor() const
{
    return true;  // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PlugDataAudioProcessor::createEditor()
{
    auto* editor = new PlugDataPluginEditor(*this);
    
    setThis();
    
    if (patches.isEmpty())
    {
        auto patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(defaultPatch);
        
        openPatch(patchFile);
        
        auto* cnv = editor->canvases.add(new Canvas(*editor, getPatch(), false));
        
        getPatch().setTitle("Untitled Patcher");
        
        editor->addTab(cnv);
        
        // Set to unknown file when loading temp patch
        setCurrentFile(File());
    }
    else
    {
        for (auto& patch : patches)
        {
            auto* cnv = editor->canvases.add(new Canvas(*editor, patch, false));
            editor->addTab(cnv);
        }
    }
    
    editor->resized();
    
    return editor;
}

void PlugDataAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryBlock xmlBlock;
    
    suspendProcessing(true); // These functions can be called from any thread, so suspend processing prevent threading issues
    
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, xmlBlock);
    
    // Store pure-data state
    MemoryOutputStream ostream(destData, false);
    
    ostream.writeString(getPatch().getCanvasContent());
    ostream.writeInt(getLatencySamples());
    ostream.writeFloat(static_cast<float>(tailLength.getValue()));
    ostream.writeInt(static_cast<int>(xmlBlock.getSize()));
    ostream.write(xmlBlock.getData(), xmlBlock.getSize());
    ostream.writeString(getCurrentFile().getFullPathName());
    
    suspendProcessing(false);
}

void PlugDataAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (sizeInBytes == 0) return;
    
    // Copy data to make sure it doesn't expire before our async function is called
    void* copy = malloc(sizeInBytes);
    memcpy(copy, data, sizeInBytes);
    
    // By calling this asynchronously on the message thread and also suspending processing on the audio thread, we can make sure this is safe
    // The DAW can call this function from basically any thread, hence the need for this
    MessageManager::callAsync([this, copy, sizeInBytes]() mutable {
        
        MemoryInputStream istream(copy, sizeInBytes, false);
        
        suspendProcessing(true);
        
        String state = istream.readString();
        auto latency = istream.readInt();
        auto tail = istream.readFloat();
        auto xmlSize = istream.readInt();
        
        tailLength = var(tail);
        
        void* xmlData = static_cast<void*>(new char[xmlSize]);
        istream.read(xmlData, xmlSize);
        
        std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));
        
        if (xmlState)
            if (xmlState->hasTagName(parameters.state.getType())) parameters.replaceState(ValueTree::fromXml(*xmlState));
        
        File location;
        if (!istream.isExhausted())
        {
            location = istream.readString();
            if (location.exists())
            {
                setCurrentFile(location);
                
                String parentPath = location.getParentDirectory().getFullPathName();
                // Add patch path to search path to make sure it finds the externals!
                libpd_add_to_search_path(parentPath.toRawUTF8());
            }
        }
        
        loadPatch(state);
        
        if ((location.exists() && location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) || !location.exists())
        {
            getPatch().setTitle("Untitled Patcher");
        }
        else
        {
            getPatch().setTitle(location.getFileName());
        }
        
        setLatencySamples(latency);
        
        suspendProcessing(false);
        
        freebytes(copy, sizeInBytes);
    });
}

void PlugDataAudioProcessor::loadPatch(File patch)
{
    
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        editor->tabbar.clearTabs();
        editor->canvases.clear();
    }
    
    patches.clear();
    
    openPatch(patch);
    patches.addIfNotAlreadyThere(getPatch());
    
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        const MessageManagerLock mmLock;
        auto* cnv = editor->canvases.add(new Canvas(*editor, getPatch(), false));
        cnv->synchronise();
        editor->addTab(cnv);
    }
    
    setCurrentFile(patch);
    
}

void PlugDataAudioProcessor::loadPatch(String patch)
{
    if(patch.isEmpty()) patch = pd::Instance::defaultPatch;
    
    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patch);
    
    loadPatch(patchFile);
    
    // Set to unknown file when loading temp patch
    setCurrentFile(File());
}

void PlugDataAudioProcessor::setTheme(bool themeToUse) {
    lnf->setTheme(themeToUse);
    if(auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {
        editor->getTopLevelComponent()->repaint();
        editor->repaint();
        if(auto* cnv = editor->getCurrentCanvas()) {
            cnv->repaint();
        }
    }

}

Colour PlugDataAudioProcessor::getForegroundColour() {
    return lnf->findColour(Slider::thumbColourId);
}

Colour PlugDataAudioProcessor::getBackgroundColour() {
    return lnf->findColour(ResizableWindow::backgroundColourId);
}

void PlugDataAudioProcessor::receiveNoteOn(const int channel, const int pitch, const int velocity)
{
    if (velocity == 0)
    {
        midiBufferOut.addEvent(MidiMessage::noteOff(channel, pitch, uint8(0)), audioAdvancement);
    }
    else
    {
        midiBufferOut.addEvent(MidiMessage::noteOn(channel, pitch, static_cast<uint8>(velocity)), audioAdvancement);
    }
}




void PlugDataAudioProcessor::receiveControlChange(const int channel, const int controller, const int value)
{
    midiBufferOut.addEvent(MidiMessage::controllerEvent(channel, controller, value), audioAdvancement);
}

void PlugDataAudioProcessor::receiveProgramChange(const int channel, const int value)
{
    midiBufferOut.addEvent(MidiMessage::programChange(channel, value), audioAdvancement);
}

void PlugDataAudioProcessor::receivePitchBend(const int channel, const int value)
{
    midiBufferOut.addEvent(MidiMessage::pitchWheel(channel, value + 8192), audioAdvancement);
}

void PlugDataAudioProcessor::receiveAftertouch(const int channel, const int value)
{
    midiBufferOut.addEvent(MidiMessage::channelPressureChange(channel, value), audioAdvancement);
}

void PlugDataAudioProcessor::receivePolyAftertouch(const int channel, const int pitch, const int value)
{
    midiBufferOut.addEvent(MidiMessage::aftertouchChange(channel, pitch, value), audioAdvancement);
}

void PlugDataAudioProcessor::receiveMidiByte(const int port, const int byte)
{
    if (midiByteIsSysex)
    {
        if (byte == 0xf7)
        {
            midiBufferOut.addEvent(MidiMessage::createSysExMessage(midiByteBuffer, static_cast<int>(midiByteIndex)), audioAdvancement);
            midiByteIndex = 0;
            midiByteIsSysex = false;
        }
        else
        {
            midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
            if (midiByteIndex == 512)
            {
                midiByteIndex = 511;
            }
        }
    }
    else if (midiByteIndex == 0 && byte == 0xf0)
    {
        midiByteIsSysex = true;
    }
    else
    {
        midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
        if (midiByteIndex >= 3)
        {
            midiBufferOut.addEvent(MidiMessage(midiByteBuffer, 3), audioAdvancement);
            midiByteIndex = 0;
        }
    }
}

void PlugDataAudioProcessor::timerCallback()
{
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        if (!callbackType) return;
        
        if (callbackType == 1)
        {
            editor->updateValues();
        }
        else if (callbackType == 3)
        {
            editor->updateValues();
        }
        else
        {
            for (auto* tmpl : editor->getCurrentCanvas()->templates)
            {
                static_cast<DrawableTemplate*>(tmpl)->update();
            }
        }
        
        callbackType = 0;
    }
}

void PlugDataAudioProcessor::receiveGuiUpdate(int type)
{
    if (callbackType != 0 && callbackType != type)
    {
        callbackType = 3;
    }
    else
    {
        callbackType = type;
    }
    
    startTimer(15);
}

void PlugDataAudioProcessor::updateConsole()
{
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        editor->sidebar.updateConsole();
    }
}

void PlugDataAudioProcessor::synchroniseCanvas(void* cnv)
{
    MessageManager::callAsync([this, cnv]() mutable {
        if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
        {
            for(auto* canvas : editor->canvases) {
                if(canvas->patch.getPointer() == cnv) {
                    canvas->synchronise();
                }
            }
        }
    });
};

void PlugDataAudioProcessor::titleChanged()
{
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        for (int n = 0; n < editor->tabbar.getNumTabs(); n++)
        {
            editor->tabbar.setTabName(n, editor->getCanvas(n)->patch.getTitle());
        }
    }
}

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugDataAudioProcessor();
}
