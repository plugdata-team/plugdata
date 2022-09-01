/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <clocale>
#include "PluginProcessor.h"

#include "Canvas.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

extern "C"
{
    #include "x_libpd_extra_utils.h"
    EXTERN char* pd_version;
#if JUCE_WINDOWS && _WIN64
    // Need this to create directory junctions on Windows
    unsigned int WinExec(_Null_terminated_ const char* lpCmdLine, unsigned int uCmdShow);
#endif
}

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
    // Make sure to use dots for decimal numbers, pd requires that
    std::setlocale(LC_ALL, "C");
    
    // continuityChecker keeps track of whether audio is running and creates a backup scheduler in case it isn't
    continuityChecker.setCallback([this](t_float* in, t_float* out){
        
        if(isNonRealtime()) return;
        
        if(getCallbackLock()->tryEnter()) {
            
            // Dequeue messages
            sendMessagesFromQueue();
            
            libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
            libpd_process_nodsp();
            getCallbackLock()->exit();
        }
    });
    
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(ParameterID("volume", 1), "Volume", NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.75f, false), 1.0f));

    // General purpose automation parameters you can get by using "receive param1" etc.
    for (int n = 0; n < numParameters; n++)
    {
        auto id = ParameterID("param" + String(n + 1), 1);
        auto* parameter = parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(id, "Parameter " + String(n + 1), 0.0f, 1.0f, 0.0f));
        lastParameters[n] = 0;
        parameter->addListener(this);
    }

    volume = parameters.getRawParameterValue("volume");

    // Make sure that the parameter valuetree has a name, to prevent assertion failures
    parameters.replaceState(ValueTree("PlugData"));
    
    // scope for locking message manager
    {
        const MessageManagerLock mmLock;
        
        LookAndFeel::setDefaultLookAndFeel(&lnf.get());
        
        // On first startup, initialise abstractions and settings
        initialiseFilesystem();
        
        // Initialise library for text autocompletion
        objectLibrary.initialiseLibrary();
    }
    
    channelPointers.reserve(32);

    // Set up midi buffers
    midiBufferIn.ensureSize(2048);
    midiBufferOut.ensureSize(2048);
    midiBufferTemp.ensureSize(2048);
    midiBufferCopy.ensureSize(2048);

    atoms_playhead.reserve(3);
    atoms_playhead.resize(1);

    setCallbackLock(&AudioProcessor::getCallbackLock());

    sendMessagesFromQueue();

    objectLibrary.appDirChanged = [this]()
    {
        auto newTree = ValueTree::fromXml(settingsFile.loadFileAsString());

        // Prevents causing an update loop
        if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
        {
            settingsTree.removeListener(editor);
        }

        settingsTree.getChildWithName("Paths").copyPropertiesAndChildrenFrom(newTree.getChildWithName("Paths"), nullptr);

        // Direct children shouldn't be overwritten as that would break some valueTree links, for example in SettingsDialog
        for (auto child : settingsTree)
        {
            child.copyPropertiesAndChildrenFrom(newTree.getChildWithName(child.getType()), nullptr);
        }
        settingsTree.copyPropertiesFrom(newTree, nullptr);

        if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
        {
            settingsTree.addListener(editor);
            
            for(auto* cnv : editor->canvases) {
                // Make sure inlets/outlets are updated
                for(auto* box : cnv->boxes) box->updatePorts();
            }
        }

        updateSearchPaths();
        objectLibrary.updateLibrary();
        
        setTheme(static_cast<bool>(settingsTree.getProperty("Theme")));
    };

    if (settingsTree.hasProperty("Theme"))
    {
        setTheme(static_cast<bool>(settingsTree.getProperty("Theme")));
    }
    
    if(settingsTree.hasProperty("Oversampling")) {
        oversampling = static_cast<int>(settingsTree.getProperty("Oversampling"));
    }

    updateSearchPaths();
    
    setLatencySamples(pd::Instance::getBlockSize());

    logMessage("PlugData v" + String(ProjectInfo::versionString));
    logMessage("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false));
    logMessage("Libraries:");
    logMessage(else_version);
    logMessage(cyclone_version);
}

PlugDataAudioProcessor::~PlugDataAudioProcessor()
{
    // Save current settings before quitting
    saveSettings();
}

void PlugDataAudioProcessor::initialiseFilesystem()
{
    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!homeDir.exists() || !abstractions.exists())
    {
        MemoryInputStream binaryFilesystem(BinaryData::Filesystem_zip, BinaryData::Filesystem_zipSize, false);
        auto file = ZipFile(binaryFilesystem);
        file.uncompressTo(homeDir);
        
        // Create filesystem for this specific version
        homeDir.getChildFile("plugdata_version").moveFileTo(appDir);
        
        auto library = homeDir.getChildFile("Library");
        auto deken = homeDir.getChildFile("Deken");
        
        // For transitioning between v0.5.3 -> v0.6.0
        auto library_backup = homeDir.getChildFile("Library_backup");
        if(!library.exists()) {
            library.createDirectory();
        }
        
#if !JUCE_WINDOWS
        // This may not work on Windows, Windows users REALLY need to thrash their PlugData folder
        else if(library.getChildFile("Deken").isDirectory() && !library.getChildFile("Deken").isSymbolicLink()) {
            library.moveFileTo(library_backup);
            library.createDirectory();
        }
#endif
        
        deken.createDirectory();
        
#if JUCE_WINDOWS
        // Get paths that need symlinks
        auto abstractionsPath = appDir.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\");
        auto documentationPath = appDir.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\");
        auto dekenPath = deken.getFullPathName();
        
        // The mklink /J command creates a directory junction. This is the closest thing I can get to a directory symlink on Windows
        // Regular symlinks can only created by administrators
        auto abstractionsCommand = "cmd.exe /k mklink /J " + library.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\") + " " + abstractionsPath;
        auto documentationCommand = "cmd.exe /k mklink /J " + library.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\") + " " + documentationPath;
        auto dekenCommand = "cmd.exe /k mklink /J " + library.getChildFile("Deken").getFullPathName().replaceCharacters("/", "\\") + " " + dekenPath;
        
        // Execute junction command
        // TODO: write real C++ code for creating junctions
#if _WIN64
        // For some reason, this only links with 64-bit targets
        WinExec(abstractionsCommand.toRawUTF8(), 0);
        WinExec(documentationCommand.toRawUTF8(), 0);
        WinExec(dekenCommand.toRawUTF8(), 0);
#else   
        system(abstractionsCommand.fromFirstOccurrenceOf("/k", false, false).toRawUTF8());
        system(documentationCommand.fromFirstOccurrenceOf("/k", false, false).toRawUTF8());
        system(dekenCommand.fromFirstOccurrenceOf("/k", false, false).toRawUTF8());
#endif

#else
        appDir.getChildFile("Abstractions").createSymbolicLink(library.getChildFile("Abstractions"), true);
        appDir.getChildFile("Documentation").createSymbolicLink(library.getChildFile("Documentation"), true);
        deken.createSymbolicLink(library.getChildFile("Deken"), true);
#endif
    }
    
    // Check if settings file exists, if not, create the default
    if (!settingsFile.existsAsFile())
    {
        settingsFile.create();

        // Add default settings
        settingsTree.setProperty("BrowserPath", homeDir.getChildFile("Library").getFullPathName(), nullptr);
        settingsTree.setProperty("Theme", 1, nullptr);
        settingsTree.setProperty("GridEnabled", 1, nullptr);

        auto pathTree = ValueTree("Paths");
        auto library = homeDir.getChildFile("Library");

        auto firstPath = ValueTree("Path");
        firstPath.setProperty("Path", library.getChildFile("Abstractions").getFullPathName(), nullptr);

        auto secondPath = ValueTree("Path");
        secondPath.setProperty("Path", library.getChildFile("Deken").getFullPathName(), nullptr);

        pathTree.appendChild(firstPath, nullptr);
        pathTree.appendChild(secondPath, nullptr);

        settingsTree.appendChild(pathTree, nullptr);

        settingsTree.appendChild(ValueTree("Keymap"), nullptr);

        settingsTree.setProperty("DefaultFont", "Inter", nullptr);
        for (int i = 0; i < lnf->colourNames.size(); i++)
        {
            for (int j = 0; j < lnf->colourNames[i].size(); j++)
            {
                settingsTree.setProperty(lnf->colourNames[i][j], PlugDataLook::colourSettings[i][j].toString(), nullptr);
            }
        }

        saveSettings();
    }
    else
    {
        // Or load the settings when they exist already
        settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());

        if (settingsTree.hasProperty("DefaultFont"))
        {
            String fontname = settingsTree.getProperty("DefaultFont").toString();
            PlugDataLook::setDefaultFont(fontname);
        }

        for (int i = 0; i < lnf->colourNames.size(); i++)
        {
            for (int j = 0; j < lnf->colourNames[i].size(); j++)
            {
                if (settingsTree.hasProperty(lnf->colourNames[i][j]))
                {
                    PlugDataLook::colourSettings[i][j] = Colour::fromString(settingsTree.getProperty(lnf->colourNames[i][j]).toString());
                }
            }
        }
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

    setThis();
    
    getCallbackLock()->enter();
    
    libpd_clear_search_path();
    for (auto child : pathTree)
    {
        auto path = child.getProperty("Path").toString();
        libpd_add_to_search_path(path.toRawUTF8());
    }

    // Add ELSE path
    auto elsePath = appDir.getChildFile("Abstractions").getChildFile("else");
    if (elsePath.exists())
    {
        auto location = elsePath.getFullPathName();
        libpd_add_to_search_path(location.toRawUTF8());
    }

    getCallbackLock()->exit();
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
    return "Init preset";
}

void PlugDataAudioProcessor::changeProgramName(int index, const String& newName)
{
}

void PlugDataAudioProcessor::setOversampling(int amount)
{
    settingsTree.setProperty("Oversampling", var(amount), nullptr);
    saveSettings();
    
    oversampling = amount;
    auto blockSize = AudioProcessor::getBlockSize();
    auto sampleRate = AudioProcessor::getSampleRate();
    
    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PlugDataAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    float oversampleFactor = 1 << oversampling;
    auto maxChannels = std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());
    
    prepareDSP(getTotalNumInputChannels(), getTotalNumOutputChannels(), sampleRate * oversampleFactor, samplesPerBlock * oversampleFactor);
    
    oversampler.reset (new dsp::Oversampling<float> (maxChannels, oversampling, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    
    oversampler->initProcessing (samplesPerBlock);
    
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

    statusbarSource.prepareToPlay(getTotalNumOutputChannels());
}

void PlugDataAudioProcessor::releaseResources()
{
    releaseDSP();
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

        if (layouts.outputBuses[bus].isDisabled()) continue;

        if (nchb > 2) return false;
        if (nchb == 0) return false;
        noutch += nchb;
    }

    for (int bus = 0; bus < layouts.inputBuses.size(); bus++)
    {
        int nchb = layouts.getNumChannels(true, bus);

        if (layouts.inputBuses[bus].isDisabled()) continue;

        if (nchb > 2) return false;
        if (nchb == 0) return false;
        ninch += nchb;
    }

    return ninch <= 32 && noutch <= 32;
}



void PlugDataAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    continuityChecker.setNonRealtime(isNonRealtime());
    continuityChecker.setTimer();
    
    setThis();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    midiBufferCopy.clear();
    midiBufferCopy.addEvents(midiMessages, 0, buffer.getNumSamples(), audioAdvancement);

    auto targetBlock = dsp::AudioBlock<float>(buffer);
    auto blockOut = oversampling > 0 ? oversampler->processSamplesUp(targetBlock) : targetBlock;
    
    process(blockOut, midiMessages);
        
    if(oversampling > 0) {
        oversampler->processSamplesDown(targetBlock);
    }

    buffer.applyGain(getParameters()[0]->getValue());
    statusbarSource.processBlock(buffer, midiBufferCopy, midiMessages, totalNumOutputChannels);
}

void PlugDataAudioProcessor::process(dsp::AudioBlock<float> buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    const int blockSize = Instance::getBlockSize();
    const int numSamples = static_cast<int>(buffer.getNumSamples());
    const int adv = audioAdvancement >= 64 ? 0 : audioAdvancement;
    const int numLeft = blockSize - adv;
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    channelPointers.clear();
    for(int ch = 0; ch < std::max(numIn, numOut); ch++) {
        channelPointers.push_back(buffer.getChannelPointer(ch));
    }

    const bool midiConsume = acceptsMidi();
    const bool midiProduce = producesMidi();

    auto const maxOuts = std::max(numOut, std::max(numIn, numOut));
    for (int ch = numIn; ch < maxOuts; ch++)
    {
        buffer.getSingleChannelBlock(ch).clear();
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
            FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j], numSamples);
        }
        for (int j = 0; j < numOut; ++j)
        {
            const int index = j * blockSize + adv;
            FloatVectorOperations::copy(channelPointers[j], audioBufferOut.data() + index, numSamples);
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
            FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j], numLeft);
        }
        for (int j = 0; j < numOut; ++j)
        {
            const int index = j * blockSize + adv;
            FloatVectorOperations::copy(channelPointers[j], audioBufferOut.data() + index, numLeft);
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
                FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j] + pos, blockSize);
            }
            for (int j = 0; j < numOut; ++j)
            {
                const int index = j * blockSize;
                FloatVectorOperations::copy(channelPointers[j] + pos, audioBufferOut.data() + index, blockSize);
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
                FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j] + pos, remaining);
            }
            for (int j = 0; j < numOut; ++j)
            {
                const int index = j * blockSize;
                FloatVectorOperations::copy(channelPointers[j] + pos, audioBufferOut.data() + index, remaining);
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
    if(!playhead) return;
    
    auto infos = playhead->getPosition();
    
    if (infos.hasValue())
    {
        atoms_playhead[0] = static_cast<float>(infos->getIsPlaying());
        sendMessage("playhead", "playing", atoms_playhead);
        
        atoms_playhead[0] = static_cast<float>(infos->getIsRecording());
        sendMessage("playhead", "recording", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsLooping());
        
        auto loopPoints = infos->getLoopPoints();
        if(loopPoints.hasValue()) {
            atoms_playhead.push_back(static_cast<float>(loopPoints->ppqStart));
            atoms_playhead.push_back(static_cast<float>(loopPoints->ppqEnd));
        }
        else {
            atoms_playhead.push_back(0.0f);
            atoms_playhead.push_back(0.0f);
        }
        sendMessage("playhead", "looping", atoms_playhead);
        
        if(infos->getEditOriginTime().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getEditOriginTime());
            sendMessage("playhead", "edittime", atoms_playhead);
        }

        if(infos->getFrameRate().hasValue()) {
            atoms_playhead[0] = static_cast<float>(infos->getFrameRate()->getEffectiveRate());
            sendMessage("playhead", "framerate", atoms_playhead);
        }

       
        if(infos->getBpm().hasValue()) {
            atoms_playhead[0] = static_cast<float>(*infos->getBpm());
            sendMessage("playhead", "bpm", atoms_playhead);
        }
        
        if(infos->getPpqPositionOfLastBarStart().hasValue()) {
            atoms_playhead[0] = static_cast<float>(*infos->getPpqPositionOfLastBarStart());
            sendMessage("playhead", "lastbar", atoms_playhead);
        }
        
        if(infos->getTimeSignature().hasValue()) {
            atoms_playhead[0] = static_cast<float>(infos->getTimeSignature()->numerator);
            atoms_playhead.push_back(static_cast<float>(infos->getTimeSignature()->denominator));
            sendMessage("playhead", "timesig", atoms_playhead);
        }

        if(infos->getPpqPosition().hasValue()) {
            
            atoms_playhead[0] = static_cast<float>(*infos->getPpqPosition());
        }
        else {
            atoms_playhead[0] = 0.0f;
        }
        
        if(infos->getTimeInSamples().hasValue()) {
            atoms_playhead[1] = static_cast<float>(*infos->getTimeInSamples());
        }
        else {
            atoms_playhead[1] = 0.0f;
        }
        
        if(infos->getTimeInSeconds().hasValue()) {
            atoms_playhead.push_back(static_cast<float>(*infos->getTimeInSeconds()));
        }
        else {
            atoms_playhead.push_back(0.0f);
        }
        
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
    setThis();

    // clear midi out
    if (producesMidi())
    {
        midiByteIndex = 0;
        midiByteBuffer[0] = 0;
        midiByteBuffer[1] = 0;
        midiByteBuffer[2] = 0;
        midiBufferOut.clear();
    }

    // Dequeue messages
    sendMessagesFromQueue();
    sendPlayhead();
    sendMidiBuffer();
    sendParameters();

    // Process audio
    FloatVectorOperations::copy(audioBufferIn.data() + (2 * 64), audioBufferOut.data() + (2 * 64), (minOut - 2) * 64);
    performDSP(audioBufferIn.data(), audioBufferOut.data());
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

        auto* patch = patches.add(new pd::Patch(openPatch(patchFile)));

        auto* cnv = editor->canvases.add(new Canvas(*editor, *patch, nullptr));

        patch->setCurrentFile(File());
        patch->setTitle("Untitled Patcher");

        editor->addTab(cnv, true);
    }
    else
    {
        for (auto* patch : patches)
        {
            auto* cnv = editor->canvases.add(new Canvas(*editor, *patch, nullptr));
            editor->addTab(cnv, true);
        }
    }

    editor->resized();

    return editor;
}

void PlugDataAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryBlock xmlBlock;

    suspendProcessing(true);  // These functions can be called from any thread, so suspend processing prevent threading issues

    setThis();
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, xmlBlock);

    // Store pure-data state
    MemoryOutputStream ostream(destData, false);

    ostream.writeInt(patches.size());

    for (auto& patch : patches)
    {
        ostream.writeString(patch->getCanvasContent());
        ostream.writeString(patch->getCurrentFile().getFullPathName());
    }

    ostream.writeInt(getLatencySamples());
    ostream.writeInt(oversampling);
    ostream.writeFloat(static_cast<float>(tailLength.getValue()));
    ostream.writeInt(static_cast<int>(xmlBlock.getSize()));
    ostream.write(xmlBlock.getData(), xmlBlock.getSize());

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
    // Audio will only be reactivated once this action is completed
    MessageManager::callAsync(
        [this, copy, sizeInBytes]() mutable
        {
            MemoryInputStream istream(copy, sizeInBytes, false);

            suspendProcessing(true);

            // Close any opened patches
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
            {
                editor->tabbar.clearTabs();
                editor->canvases.clear();
            }

            for (auto& patch : patches) patch->close();
            patches.clear();

            int numPatches = istream.readInt();

            for (int i = 0; i < numPatches; i++)
            {
                auto state = istream.readString();
                auto location = File(istream.readString());

                auto* patch = loadPatch(state);

                if ((location.exists() && location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) || !location.exists())
                {
                    patch->setTitle("Untitled Patcher");
                }
                else if (location.existsAsFile())
                {
                    patch->setCurrentFile(location);
                    patch->setTitle(location.getFileName());

                    auto parentPath = location.getParentDirectory().getFullPathName();
                    // Add patch path to search path to make sure it finds the externals!
                    libpd_add_to_search_path(parentPath.toRawUTF8());
                }
            }

            auto latency = istream.readInt();
            auto oversampling = istream.readInt();
            auto tail = istream.readFloat();
            auto xmlSize = istream.readInt();

            tailLength = var(tail);

            void* xmlData = static_cast<void*>(new char[xmlSize]);
            istream.read(xmlData, xmlSize);

            std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));

            if (xmlState) {
                if (xmlState->hasTagName(parameters.state.getType())) {
                    parameters.replaceState(ValueTree::fromXml(*xmlState));
                }
            }

            setLatencySamples(latency);
            setOversampling(oversampling);
            
            suspendProcessing(false);

            freebytes(copy, sizeInBytes);
            
        });
}

pd::Patch* PlugDataAudioProcessor::loadPatch(const File& patchFile)
{
    // First, check if patch is already opened
    int i = 0;
    for(auto* patch : patches) {
        if(patch->getCurrentFile() == patchFile)
        {
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
            {
                MessageManager::callAsync([i, _editor = Component::SafePointer(editor)]() mutable {
                    if(!_editor) return;
                    _editor->tabbar.setCurrentTabIndex(i);
                    _editor->pd.logError("Patch is already open");
                });
            }
        
            // Patch is already opened
            return nullptr;
        }
        i++;
    }

    auto newPatch = openPatch(patchFile);
    
    if (!newPatch.getPointer())
    {
        logError("Couldn't open patch");
        return nullptr;
    }
    
    auto* patch = patches.add(new pd::Patch(newPatch));

    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        MessageManager::callAsync([i, patch, _editor = Component::SafePointer(editor)]() mutable {
            if(!_editor) return;
            auto* cnv = _editor->canvases.add(new Canvas(*_editor, *patch, nullptr));
            _editor->addTab(cnv, true);
        });
    }

    patch->setCurrentFile(patchFile);

    return patch;
}

pd::Patch* PlugDataAudioProcessor::loadPatch(String patchText)
{
    if (patchText.isEmpty()) patchText = pd::Instance::defaultPatch;

    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patchText);

    auto* patch = loadPatch(patchFile);

    // Set to unknown file when loading temp patch
    patch->setCurrentFile(File());

    return patch;
}

void PlugDataAudioProcessor::setTheme(bool themeToUse)
{
    lnf->setTheme(themeToUse);
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        editor->getTopLevelComponent()->repaint();
        editor->repaint();

        if (auto* cnv = editor->getCurrentCanvas())
        {
            cnv->viewport->repaint();

            // Some objects with setBufferedToImage need manual repainting
            for (auto* box : cnv->boxes) box->repaint();
            for (auto* con : cnv->connections) reinterpret_cast<Component*>(con)->repaint();
            cnv->repaint();
        }
    }
}

Colour PlugDataAudioProcessor::getOutlineColour()
{
    // currently the same as text colour, but still a function to make it easy to change in the future
    return lnf->findColour(PlugDataColour::canvasOutlineColourId);
}

Colour PlugDataAudioProcessor::getForegroundColour()
{
    // currently the same as text colour, but still a function to make it easy to change in the future
    return lnf->findColour(PlugDataColour::textColourId).interpolatedWith(lnf->findColour(PlugDataColour::canvasColourId), 0.25f);
}

Colour PlugDataAudioProcessor::getBackgroundColour()
{
    return lnf->findColour(PlugDataColour::toolbarColourId);
}

Colour PlugDataAudioProcessor::getTextColour()
{
    return lnf->findColour(PlugDataColour::textColourId);
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

// Only for standalone: check which parameters have changed and forward them to pd
void PlugDataAudioProcessor::sendParameters()
{
#if PLUGDATA_STANDALONE
    for (int idx = 0; idx < numParameters; idx++)
    {
        float value = standaloneParams[idx].load();
        if (value != lastParameters[idx])
        {
            auto paramID = "param" + String(idx + 1);
            sendFloat(paramID.toRawUTF8(), value);
            lastParameters[idx] = value;
        }
    }
#endif
}

void PlugDataAudioProcessor::performParameterChange(int type, int idx, float value)
{
    // Type == 1 means it sets the change gesture state
    if(type)
    {
        if(changeGestureState[idx] == value) {
            logMessage("parameter change " + String(idx) + (value ? " already started" : " not started"));
        }
        else {
            #if !PLUGDATA_STANDALONE
            auto* parameter = parameters.getParameter("param" + String(idx + 1));
            value ? parameter->beginChangeGesture() : parameter->endChangeGesture();
            #endif
            changeGestureState[idx] = value;
        }
    }
    else { // otherwise set parameter value
#if PLUGDATA_STANDALONE
        // Set the value
        standaloneParams[idx].store(value);
        
        // Update values in automation panel
        if(lastParameters[idx] == value) return;
        if(auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {
            editor->sidebar.updateAutomationParameters();
        }
        lastParameters[idx] = value;
#else
        auto paramID = "param" + String(idx + 1);
        if(lastParameters[idx] == value) return; // Prevent feedback
        // Send new value to DAW
        parameters.getParameter(paramID)->setValueNotifyingHost(value);
        lastParameters[idx] = value;
#endif
    }
}

// Callback when parameter values change
void PlugDataAudioProcessor::parameterValueChanged (int idx, float value)
{
    enqueueFunction([this, idx, value]() mutable {
        auto paramID = "param" + String(idx);
        sendFloat(paramID.toRawUTF8(), value);
        lastParameters[idx - 1] = value;
    });
}

void PlugDataAudioProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    
}

void PlugDataAudioProcessor::receiveDSPState(bool dsp)
{
    MessageManager::callAsync(
        [this, dsp]() mutable
        {
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
            {
                editor->statusbar.powerButton->setToggleState(dsp, dontSendNotification);
            }
        });
}

void PlugDataAudioProcessor::receiveGuiUpdate(int type)
{
    callbackType |= (1 << type);
    
    if(!isTimerRunning()) {

        startTimer(16);
    }
}

void PlugDataAudioProcessor::timerCallback()
{
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
    {
        if (!callbackType) return;

        if (callbackType & 2)
        {
             editor->updateValues();
        }
        if (callbackType & 4)
        {
             editor->updateDrawables();
        }
        if (callbackType & 8)
        {
             editor->updateGuiParameters();
        }

        callbackType = 0;
    }
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
    MessageManager::callAsync(
        [this, cnv]() mutable
        {
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor()))
            {
                for (auto* canvas : editor->canvases)
                {
                    if (canvas->patch.getPointer() == cnv)
                    {
                        canvas->synchronise();
                    }
                }
            }
        });
}

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
