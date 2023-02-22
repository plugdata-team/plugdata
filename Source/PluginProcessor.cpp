/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>

#ifdef JUCE_WINDOWS
#    include "Utility/OSUtils.h"
#endif

#include <clocale>
#include "PluginProcessor.h"

#include "Canvas.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

#include "Utility/PluginParameter.h"

extern "C" {
#include "x_libpd_extra_utils.h"
EXTERN char* pd_version;
}

AudioProcessor::BusesProperties PluginProcessor::buildBusesProperties()
{
    AudioProcessor::BusesProperties busesProperties;
#if PLUGDATA_STANDALONE
    busesProperties.addBus(true, "Main Input", AudioChannelSet::canonicalChannelSet(16), true);
    busesProperties.addBus(false, "Main Output", AudioChannelSet::canonicalChannelSet(16), true);
#else
    busesProperties.addBus(true, "Main Input", AudioChannelSet::stereo(), true);

    for (int i = 1; i < numInputBuses; i++)
        busesProperties.addBus(true, "Aux Input " + String(i), AudioChannelSet::stereo(), false);

    busesProperties.addBus(false, "Main Output", AudioChannelSet::stereo(), true);

    for (int i = 1; i < numOutputBuses; i++)
        busesProperties.addBus(false, "Aux " + String(i), AudioChannelSet::stereo(), false);

#endif
    return busesProperties;
}

// ag: Note that this is just a fallback, we update this with live version
// data from the external if we have it.
String PluginProcessor::pdlua_version = "pdlua 0.11.0 (lua 5.4)";

PluginProcessor::PluginProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(buildBusesProperties())
    ,
#endif
    pd::Instance("plugdata")
{
    // Make sure to use dots for decimal numbers, pd requires that
    std::setlocale(LC_ALL, "C");

    {
        const MessageManagerLock mmLock;

        LookAndFeel::setDefaultLookAndFeel(&lnf.get());

        // Initialise directory structure and settings file
        initialiseFilesystem();
        settingsFile = SettingsFile::getInstance()->initialise();
    }

    auto* volumeParameter = new PlugDataParameter(this, "volume", 1.0f, true);
    addParameter(volumeParameter);
    volume = volumeParameter->getValuePointer();

    // General purpose automation parameters you can get by using "receive param1" etc.
    for (int n = 0; n < numParameters; n++) {
        auto* parameter = new PlugDataParameter(this, "param" + String(n + 1), 0.0f, false);
        addParameter(parameter);
    }

    // Make sure that the parameter valuetree has a name, to prevent assertion failures
    // parameters.replaceState(ValueTree("plugdata"));

    logMessage("plugdata v" + String(ProjectInfo::versionString));
    logMessage("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false));
    logMessage("Libraries:");
    logMessage(else_version);
    logMessage(cyclone_version);

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

    objectLibrary.appDirChanged = [this]() {
        // If we changed the settings from within the app, don't reload
        settingsFile->reloadSettings();
        auto newTheme = settingsFile->getProperty<String>("theme");
        if(PlugDataLook::currentTheme != newTheme) {
            setTheme(newTheme);
        }

        if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
            for (auto* cnv : editor->canvases) {
                // Make sure inlets/outlets are updated
                for (auto* object : cnv->objects)
                    object->updateIolets();
            }

            editor->sendLookAndFeelChange();
        }

        updateSearchPaths();
        objectLibrary.updateLibrary();
    };

    auto themeName = settingsFile->getProperty<String>("theme");

    // Make sure theme exists
    if (!settingsFile->getTheme(themeName).isValid()) {

        settingsFile->setProperty("theme", PlugDataLook::selectedThemes[0]);
        themeName = PlugDataLook::selectedThemes[0];
    }

    setTheme(themeName, true);
    settingsFile->saveSettings();

    oversampling = settingsFile->getProperty<int>("oversampling");

    setProtectedMode(settingsFile->getProperty<int>("protected"));
#if PLUGDATA_STANDALONE
    enableInternalSynth = settingsFile->getProperty<int>("internal_synth");
#endif

    auto currentThemeTree = settingsFile->getCurrentTheme();

    updateSearchPaths();

    // ag: This needs to be done *after* the library data has been unpacked on
    // first launch.
    loadLibs(pdlua_version);
    logMessage(pdlua_version);

    // scope for locking message manager
    {
        const MessageManagerLock mmLock;

        // Initialise library for text autocompletion
        // Needs to be done after loadLibs
        objectLibrary.initialiseLibrary();
    }

    setLatencySamples(pd::Instance::getBlockSize());

#if PLUGDATA_STANDALONE && !JUCE_WINDOWS
    if (auto* newOut = MidiOutput::createNewDevice("from plugdata").release()) {
        midiOutputs.add(newOut)->startBackgroundThread();
    }
#endif
}

PluginProcessor::~PluginProcessor()
{
    for (auto* patch : patches)
        patch->close();

    patches.clear();
}

void PluginProcessor::initialiseFilesystem()
{
    auto library = homeDir.getChildFile("Library");
    auto deken = homeDir.getChildFile("Deken");

    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!homeDir.exists() || !abstractions.exists()) {

        // Binary data shouldn't be too big, then the compiler will run out of memory
        // To prevent this, we split the binarydata into multiple files, and add them back together here
        std::vector<char> allData;
        int i = 0;
        while (true) {
            int size;
            auto* resource = BinaryData::getNamedResource((String("Filesystem_") + String(i) + "_zip").toRawUTF8(), size);

            if (!resource) {
                break;
            }

            allData.insert(allData.end(), resource, resource + size);
            i++;
        }

        MemoryInputStream memstream(allData.data(), allData.size(), false);

        homeDir.createDirectory();

        auto file = ZipFile(memstream);
        file.uncompressTo(homeDir);

        // Create filesystem for this specific version
        homeDir.getChildFile("plugdata_version").moveFileTo(versionDataDir);

        deken.createDirectory();
    }

    library.deleteRecursively();
    library.createDirectory();

    // We always want to update the symlinks in case an older version of plugdata is was used
#if JUCE_WINDOWS
    // Get paths that need symlinks
    auto abstractionsPath = versionDataDir.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\");
    auto documentationPath = versionDataDir.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\");
    auto extraPath = versionDataDir.getChildFile("Extra").getFullPathName().replaceCharacters("/", "\\");
    auto dekenPath = deken.getFullPathName();

    // Create NTFS directory junctions
    createJunction(library.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\").toStdString(), abstractionsPath.toStdString());

    createJunction(library.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\").toStdString(), documentationPath.toStdString());

    createJunction(library.getChildFile("Extra").getFullPathName().replaceCharacters("/", "\\").toStdString(), extraPath.toStdString());

    createJunction(library.getChildFile("Deken").getFullPathName().replaceCharacters("/", "\\").toStdString(), dekenPath.toStdString());

#else
    versionDataDir.getChildFile("Abstractions").createSymbolicLink(library.getChildFile("Abstractions"), true);
    versionDataDir.getChildFile("Documentation").createSymbolicLink(library.getChildFile("Documentation"), true);
    versionDataDir.getChildFile("Extra").createSymbolicLink(library.getChildFile("Extra"), true);
    deken.createSymbolicLink(library.getChildFile("Deken"), true);
#endif
}

void PluginProcessor::updateSearchPaths()
{
    // Reload pd search paths from settings
    auto pathTree = settingsFile->getPathsTree();

    setThis();

    lockAudioThread();

    // Get pd's search paths
    char* p[1024];
    int numItems;
    libpd_get_search_paths(p, &numItems);
    auto currentPaths = StringArray(p, numItems);

    auto paths = pd::Library::defaultPaths;

    for (auto child : pathTree) {
        auto path = child.getProperty("Path").toString().replace("\\", "/");
        paths.addIfNotAlreadyThere(path);
    }

    for (auto path : paths) {
        if (currentPaths.contains(path.getFullPathName()))
            continue;
        libpd_add_to_search_path(path.getFullPathName().toRawUTF8());
    }

    for (auto path : DekenInterface::getExternalPaths()) {
        if (currentPaths.contains(path))
            continue;
        libpd_add_to_search_path(path.replace("\\", "/").toRawUTF8());
    }

    auto librariesTree = settingsFile->getLibrariesTree();

    for (auto library : librariesTree) {
        if (!library.hasProperty("Name") || library.getProperty("Name").toString().isEmpty()) {
            librariesTree.removeChild(library, nullptr);
        }
    }

    // Load startup libraries that the user defined in settings
    for (auto library : librariesTree) {

        auto const libName = library.getProperty("Name").toString();

        // Load the library: this must be done after updating paths
        // If the library is already loaded, it will return true
        // This will load the libraries directly instead of on restart, not sure if Pd does that but it's actually nice
        if (!loadLibrary(libName)) {
            logError("Failed to load library: " + libName);
        }
    }

    unlockAudioThread();
}

const String PluginProcessor::getName() const
{
#if PLUGDATA_STANDALONE
    return "plugdata";
#else
    return JucePlugin_Name;
#endif
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::producesMidi() const
{
    return true;
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return static_cast<float>(tailLength.getValue());
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
}

const String PluginProcessor::getProgramName(int index)
{
    return "Init preset";
}

void PluginProcessor::changeProgramName(int index, String const& newName)
{
}

void PluginProcessor::setOversampling(int amount)
{
    settingsFile->setProperty("Oversampling", var(amount));
    settingsFile->saveSettings(); // TODO: i think this is unnecessary?

    oversampling = amount;
    auto blockSize = AudioProcessor::getBlockSize();
    auto sampleRate = AudioProcessor::getSampleRate();

    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PluginProcessor::setProtectedMode(bool enabled)
{
    protectedMode = enabled;
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    float oversampleFactor = 1 << oversampling;
    auto maxChannels = std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());

    prepareDSP(getTotalNumInputChannels(), getTotalNumOutputChannels(), sampleRate * oversampleFactor, samplesPerBlock * oversampleFactor);

    oversampler.reset(new dsp::Oversampling<float>(maxChannels, oversampling, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));

    oversampler->initProcessing(samplesPerBlock);

#if PLUGDATA_STANDALONE
    if (enableInternalSynth) {
        internalSynth.prepare(sampleRate, samplesPerBlock, maxChannels);
    }
#endif

    audioAdvancement = 0;
    auto const blksize = static_cast<size_t>(Instance::getBlockSize());
    auto const numIn = static_cast<size_t>(getTotalNumInputChannels());
    auto const nouts = static_cast<size_t>(getTotalNumOutputChannels());
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

void PluginProcessor::releaseResources()
{
    releaseDSP();
}

bool PluginProcessor::isBusesLayoutSupported(BusesLayout const& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#endif

    int ninch = 0;
    int noutch = 0;
    for (int bus = 0; bus < layouts.outputBuses.size(); bus++) {
        int nchb = layouts.getNumChannels(false, bus);

        if (layouts.outputBuses[bus].isDisabled())
            continue;

        if (nchb == 0)
            return false;

        noutch += nchb;
    }

    for (int bus = 0; bus < layouts.inputBuses.size(); bus++) {
        int nchb = layouts.getNumChannels(true, bus);

        if (layouts.inputBuses[bus].isDisabled())
            continue;

        if (nchb == 0)
            return false;

        ninch += nchb;
    }

    return ninch <= 32 && noutch <= 32;
}

void PluginProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    setThis();
    sendPlayhead();
    sendParameters();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    midiBufferCopy.clear();
    midiBufferCopy.addEvents(midiMessages, 0, buffer.getNumSamples(), audioAdvancement);

    auto targetBlock = dsp::AudioBlock<float>(buffer);
    auto blockOut = oversampling > 0 ? oversampler->processSamplesUp(targetBlock) : targetBlock;

    process(blockOut, midiMessages);

    if (oversampling > 0) {
        oversampler->processSamplesDown(targetBlock);
    }

    buffer.applyGain(getParameters()[0]->getValue());
    statusbarSource.processBlock(buffer, midiBufferCopy, midiMessages, totalNumOutputChannels);

#if PLUGDATA_STANDALONE
    for (auto* midiOutput : midiOutputs) {
        midiOutput->sendBlockOfMessages(midiMessages,
            Time::getMillisecondCounterHiRes(),
            AudioProcessor::getSampleRate());
    }

    // If the internalSynth is enabled and loaded, let it process the midi
    if (enableInternalSynth && internalSynth.isReady()) {
        internalSynth.process(buffer, midiMessages);
    } else if (!enableInternalSynth && internalSynth.isReady()) {
        internalSynth.unprepare();
    } else if (enableInternalSynth && !internalSynth.isReady()) {
        internalSynth.prepare(getSampleRate(), AudioProcessor::getBlockSize(), std::max(totalNumInputChannels, totalNumOutputChannels));
    }

#endif

    if (protectedMode) {
        auto* const* writePtr = buffer.getArrayOfWritePointers();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            for (int n = 0; n < buffer.getNumSamples(); n++) {
                if (!std::isfinite(writePtr[ch][n])) {
                    writePtr[ch][n] = 0.0f;
                } else {
                    writePtr[ch][n] = std::clamp(writePtr[ch][n], -1.0f, 1.0f);
                }
            }
        }
    }
}

void PluginProcessor::process(dsp::AudioBlock<float> buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    int const blockSize = Instance::getBlockSize();
    int const numSamples = static_cast<int>(buffer.getNumSamples());
    int const adv = audioAdvancement >= 64 ? 0 : audioAdvancement;
    int const numLeft = blockSize - adv;
    int const numIn = getTotalNumInputChannels();
    int const numOut = getTotalNumOutputChannels();

    channelPointers.clear();
    for (int ch = 0; ch < std::min<int>(std::max(numIn, numOut), buffer.getNumChannels()); ch++) {
        channelPointers.push_back(buffer.getChannelPointer(ch));
    }

    bool const midiConsume = acceptsMidi();
    bool const midiProduce = producesMidi();

    auto const maxOuts = std::max(numOut, std::max(numIn, numOut));
    for (int ch = numIn; ch < maxOuts; ch++) {
        buffer.getSingleChannelBlock(ch).clear();
    }

    // If the current number of samples in this block
    // is inferior to the number of samples required
    if (numSamples < numLeft) {
        // we save the input samples and we output
        // the missing samples of the previous tick.
        for (int j = 0; j < numIn; ++j) {
            int const index = j * blockSize + adv;
            FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j], numSamples);
        }
        for (int j = 0; j < numOut; ++j) {
            int const index = j * blockSize + adv;
            FloatVectorOperations::copy(channelPointers[j], audioBufferOut.data() + index, numSamples);
        }
        if (midiConsume) {
            midiBufferIn.addEvents(midiMessages, 0, numSamples, adv);
        }
        if (midiProduce) {
            midiMessages.clear();
            midiMessages.addEvents(midiBufferOut, adv, numSamples, -adv);
        }
        audioAdvancement += numSamples;
    }
    // If the current number of samples in this block
    // is superior to the number of samples required
    else {
        // we save the missing input samples, we output
        // the missing samples of the previous tick and
        // we call DSP perform method.
        MidiBuffer const& midiin = midiProduce ? midiBufferTemp : midiMessages;
        if (midiProduce) {
            midiBufferTemp.swapWith(midiMessages);
            midiMessages.clear();
        }

        for (int j = 0; j < numIn; ++j) {
            int const index = j * blockSize + adv;
            FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j], numLeft);
        }
        for (int j = 0; j < numOut; ++j) {
            int const index = j * blockSize + adv;
            FloatVectorOperations::copy(channelPointers[j], audioBufferOut.data() + index, numLeft);
        }
        if (midiConsume) {
            midiBufferIn.addEvents(midiin, 0, numLeft, adv);
        }
        if (midiProduce) {
            midiMessages.addEvents(midiBufferOut, adv, numLeft, -adv);
        }
        audioAdvancement = 0;
        processInternal();

        // If there are other DSP ticks that can be
        // performed, then we do it now.
        int pos = numLeft;
        while ((pos + blockSize) <= numSamples) {
            for (int j = 0; j < numIn; ++j) {
                int const index = j * blockSize;
                FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j] + pos, blockSize);
            }
            for (int j = 0; j < numOut; ++j) {
                int const index = j * blockSize;
                FloatVectorOperations::copy(channelPointers[j] + pos, audioBufferOut.data() + index, blockSize);
            }
            if (midiConsume) {
                midiBufferIn.addEvents(midiin, pos, blockSize, 0);
            }
            if (midiProduce) {
                midiMessages.addEvents(midiBufferOut, 0, blockSize, pos);
            }
            processInternal();
            pos += blockSize;
        }

        // If there are samples that can't be
        // processed, then save them for later
        // and outputs the remaining samples
        int const remaining = numSamples - pos;
        if (remaining > 0) {
            for (int j = 0; j < numIn; ++j) {
                int const index = j * blockSize;
                FloatVectorOperations::copy(audioBufferIn.data() + index, channelPointers[j] + pos, remaining);
            }
            for (int j = 0; j < numOut; ++j) {
                int const index = j * blockSize;
                FloatVectorOperations::copy(channelPointers[j] + pos, audioBufferOut.data() + index, remaining);
            }
            if (midiConsume) {
                midiBufferIn.addEvents(midiin, pos, remaining, 0);
            }
            if (midiProduce) {
                midiMessages.addEvents(midiBufferOut, 0, remaining, pos);
            }
            audioAdvancement = remaining;
        }
    }
}

void PluginProcessor::sendPlayhead()
{
    AudioPlayHead* playhead = getPlayHead();

    if (!playhead)
        return;

    auto infos = playhead->getPosition();

    setThis();
    if (infos.hasValue()) {
        atoms_playhead[0] = static_cast<float>(infos->getIsPlaying());
        sendMessage("playhead", "playing", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsRecording());
        sendMessage("playhead", "recording", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsLooping());

        auto loopPoints = infos->getLoopPoints();
        if (loopPoints.hasValue()) {
            atoms_playhead.push_back(static_cast<float>(loopPoints->ppqStart));
            atoms_playhead.push_back(static_cast<float>(loopPoints->ppqEnd));
        } else {
            atoms_playhead.push_back(0.0f);
            atoms_playhead.push_back(0.0f);
        }
        sendMessage("playhead", "looping", atoms_playhead);

        if (infos->getEditOriginTime().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getEditOriginTime());
            sendMessage("playhead", "edittime", atoms_playhead);
        }

        if (infos->getFrameRate().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getFrameRate()->getEffectiveRate());
            sendMessage("playhead", "framerate", atoms_playhead);
        }

        if (infos->getBpm().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getBpm());
            sendMessage("playhead", "bpm", atoms_playhead);
        }

        if (infos->getPpqPositionOfLastBarStart().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getPpqPositionOfLastBarStart());
            sendMessage("playhead", "lastbar", atoms_playhead);
        }

        if (infos->getTimeSignature().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getTimeSignature()->numerator);
            atoms_playhead.push_back(static_cast<float>(infos->getTimeSignature()->denominator));
            sendMessage("playhead", "timesig", atoms_playhead);
        }

        if (infos->getPpqPosition().hasValue()) {

            atoms_playhead[0] = static_cast<float>(*infos->getPpqPosition());
        } else {
            atoms_playhead[0] = 0.0f;
        }

        if (infos->getTimeInSamples().hasValue()) {
            atoms_playhead[1] = static_cast<float>(*infos->getTimeInSamples());
        } else {
            atoms_playhead[1] = 0.0f;
        }

        if (infos->getTimeInSeconds().hasValue()) {
            atoms_playhead.push_back(static_cast<float>(*infos->getTimeInSeconds()));
        } else {
            atoms_playhead.push_back(0.0f);
        }

        sendMessage("playhead", "position", atoms_playhead);
        atoms_playhead.resize(1);
    }
}

void PluginProcessor::sendParameters()
{
    for (auto* param : getParameters()) {
        auto* pldParam = dynamic_cast<PlugDataParameter*>(param);
        auto newvalue = pldParam->getUnscaledValue();
        if (pldParam->isEnabled() && pldParam->getLastValue() != newvalue) {
            auto title = pldParam->getTitle();
            sendFloat(title.toRawUTF8(), pldParam->getUnscaledValue());
            pldParam->setLastValue(newvalue);
        }
    }
}

void PluginProcessor::messageEnqueued()
{
    if (isNonRealtime() || isSuspended()) {
        sendMessagesFromQueue();
    } else {
        if (tryLockAudioThread()) {
            sendMessagesFromQueue();
            unlockAudioThread();
        }
    }
}

void PluginProcessor::sendMidiBuffer()
{
    if (acceptsMidi()) {
        for (auto const& event : midiBufferIn) {
            auto const message = event.getMessage();
            if (message.isNoteOn()) {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), message.getVelocity());
            } else if (message.isNoteOff()) {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), 0);
            } else if (message.isController()) {
                sendControlChange(message.getChannel(), message.getControllerNumber(), message.getControllerValue());
            } else if (message.isPitchWheel()) {
                sendPitchBend(message.getChannel(), message.getPitchWheelValue() - 8192);
            } else if (message.isChannelPressure()) {
                sendAfterTouch(message.getChannel(), message.getChannelPressureValue());
            } else if (message.isAftertouch()) {
                sendPolyAfterTouch(message.getChannel(), message.getNoteNumber(), message.getAfterTouchValue());
            } else if (message.isProgramChange()) {
                sendProgramChange(message.getChannel(), message.getProgramChangeNumber());
            } else if (message.isSysEx()) {
                for (int i = 0; i < message.getSysExDataSize(); ++i) {
                    sendSysEx(0, static_cast<int>(message.getSysExData()[i]));
                }
            } else if (message.isMidiClock() || message.isMidiStart() || message.isMidiStop() || message.isMidiContinue() || message.isActiveSense() || (message.getRawDataSize() == 1 && message.getRawData()[0] == 0xff)) {
                for (int i = 0; i < message.getRawDataSize(); ++i) {
                    sendSysRealTime(0, static_cast<int>(message.getRawData()[i]));
                }
            }

            for (int i = 0; i < message.getRawDataSize(); i++) {
                sendMidiByte(0, static_cast<int>(message.getRawData()[i]));
            }
        }
        midiBufferIn.clear();
    }
}

void PluginProcessor::processInternal()
{
    setThis();

    // clear midi out
    if (producesMidi()) {
        midiByteIndex = 0;
        midiByteBuffer[0] = 0;
        midiByteBuffer[1] = 0;
        midiByteBuffer[2] = 0;
        midiBufferOut.clear();
    }

    // Dequeue messages
    sendMessagesFromQueue();
    sendMidiBuffer();

    // Process audio
    FloatVectorOperations::copy(audioBufferIn.data() + (2 * 64), audioBufferOut.data() + (2 * 64), (minOut - 2) * 64);
    performDSP(audioBufferIn.data(), audioBufferOut.data());
}

bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PluginProcessor::createEditor()
{
    auto* editor = new PluginEditor(*this);

    setThis();

    for (auto* patch : patches) {
        auto* cnv = editor->canvases.add(new Canvas(editor, *patch, nullptr));
        editor->addTab(cnv, true);
    }

    editor->resized();

    if (isPositiveAndBelow(lastTab, patches.size())) {
        editor->tabbar.setCurrentTabIndex(lastTab);
    }

    return editor;
}

void PluginProcessor::getStateInformation(MemoryBlock& destData)
{
    setThis();

    // Store pure-data state
    MemoryOutputStream ostream(destData, false);

    ostream.writeInt(patches.size());

    // Save path and content for patch
    lockAudioThread();
    for (auto& patch : patches) {
        ostream.writeString(patch->getCanvasContent());
        ostream.writeString(patch->getCurrentFile().getFullPathName());
    }
    unlockAudioThread();

    ostream.writeInt(getLatencySamples());
    ostream.writeInt(oversampling);
    ostream.writeFloat(static_cast<float>(tailLength.getValue()));

    XmlElement xml = XmlElement("plugdata_save");
    xml.setAttribute("Version", PLUGDATA_VERSION);
    PlugDataParameter::saveStateInformation(xml, getParameters());

    MemoryBlock xmlBlock;
    copyXmlToBinary(xml, xmlBlock);

    ostream.writeInt(static_cast<int>(xmlBlock.getSize()));
    ostream.write(xmlBlock.getData(), xmlBlock.getSize());

    if (auto* editor = getActiveEditor()) {
        ostream.writeInt(editor->getWidth());
        ostream.writeInt(editor->getHeight());
    } else {
        ostream.writeInt(lastUIWidth);
        ostream.writeInt(lastUIHeight);
    }
}

void PluginProcessor::setStateInformation(void const* data, int sizeInBytes)
{
    if (sizeInBytes == 0)
        return;

    // By calling this asynchronously on the message thread and also suspending processing on the audio thread, we can make sure this is safe
    // The DAW can call this function from basically any thread, hence the need for this
    // Audio will only be reactivated once this action is completed

    MemoryInputStream istream(data, sizeInBytes, false);

    // Close any opened patches
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        MessageManager::callAsync([editor = Component::SafePointer(editor)]() {
            if (!editor)
                return;
            editor->tabbar.clearTabs();
            editor->canvases.clear();
        });
    }

    suspendProcessing(true);
    setThis();

    for (auto& patch : patches)
        patch->close();
    patches.clear();

    int numPatches = istream.readInt();

    for (int i = 0; i < numPatches; i++) {
        auto state = istream.readString();
        auto location = File(istream.readString());

        if (location.getParentDirectory().exists()) {
            auto parentPath = location.getParentDirectory().getFullPathName();
            // Add patch path to search path to make sure it finds abstractions in the saved patch!
            // TODO: is there any way to make this local the the canvas?
            libpd_add_to_search_path(parentPath.toRawUTF8());
        }

        auto* patch = loadPatch(state);

        if ((location.exists() && location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) || !location.exists()) {
            patch->setTitle("Untitled Patcher");
        } else if (location.existsAsFile()) {
            patch->setCurrentFile(location);
            patch->setTitle(location.getFileName());
        }
    }

    auto latency = istream.readInt();
    auto oversampling = istream.readInt();
    auto tail = istream.readFloat();
    auto xmlSize = istream.readInt();

    tailLength = var(tail);

    auto* xmlData = new char[xmlSize];
    istream.read(xmlData, xmlSize);

    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));

    jassert(xmlState);

    if (xmlState) {
        PlugDataParameter::loadStateInformation(*xmlState, getParameters());

        auto versionString = String("0.6.1");

        if (xmlState->hasAttribute("Version")) {
            versionString = xmlState->getStringAttribute("Version");
        }

        if (versionString.startsWith("0.7") && !istream.isExhausted()) {
            int windowWidth = istream.readInt();
            int windowHeight = istream.readInt();

            lastUIWidth = windowWidth;
            lastUIHeight = windowHeight;
            if (auto* editor = getActiveEditor()) {
                MessageManager::callAsync([editor = Component::SafePointer(editor), windowWidth, windowHeight]() {
                    if (!editor)
                        return;
                    editor->setSize(windowWidth, windowHeight);
                });
            }
        }
    }

    setLatencySamples(latency);
    setOversampling(oversampling);
    delete[] xmlData;

    suspendProcessing(false);

    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        MessageManager::callAsync([editor = Component::SafePointer(editor)]() {
            if (!editor)
                return;
            editor->sidebar.updateAutomationParameters();
        });
    }
}

pd::Patch* PluginProcessor::loadPatch(File const& patchFile)
{
    // First, check if patch is already opened
    int i = 0;
    for (auto* patch : patches) {
        if (patch->getCurrentFile() == patchFile) {
            if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
                MessageManager::callAsync([i, _editor = Component::SafePointer(editor)]() mutable {
                    if (!_editor)
                        return;
                    _editor->tabbar.setCurrentTabIndex(i);
                    _editor->pd->logError("Patch is already open");
                });
            }

            // Patch is already opened
            return nullptr;
        }
        i++;
    }

    // Stop the audio callback when loading a new patch
    suspendProcessing(true);

    auto newPatch = openPatch(patchFile);

    suspendProcessing(false);

    if (!newPatch.getPointer()) {
        logError("Couldn't open patch");
        return nullptr;
    }

    auto* patch = patches.add(new pd::Patch(newPatch));

    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        MessageManager::callAsync([i, patch, _editor = Component::SafePointer(editor)]() mutable {
            if (!_editor)
                return;
            auto* cnv = _editor->canvases.add(new Canvas(_editor, *patch, nullptr));
            _editor->addTab(cnv, true);
        });
    }

    patch->setCurrentFile(patchFile);

    return patch;
}

pd::Patch* PluginProcessor::loadPatch(String patchText)
{
    if (patchText.isEmpty())
        patchText = pd::Instance::defaultPatch;

    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patchText);

    auto* patch = loadPatch(patchFile);

    // Set to unknown file when loading temp patch
    patch->setCurrentFile(File());

    return patch;
}

void PluginProcessor::setTheme(String themeToUse, bool force)
{
    auto oldThemeTree = settingsFile->getTheme(PlugDataLook::currentTheme);
    auto themeTree = settingsFile->getTheme(themeToUse);
    // Check if theme name is valid
    if (!themeTree.isValid()) {
        themeToUse = PlugDataLook::selectedThemes[0];
        themeTree = settingsFile->getTheme(themeToUse);
    }
    
    if(!force && oldThemeTree.isValid() && themeTree.isEquivalentTo(oldThemeTree)) return;

    lnf->setTheme(themeTree);

    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        /* maybe we don't need this anymore?
        if (auto* cnv = editor->getCurrentCanvas()) {
            // Calling synchonise here is not neat, but it's a way to make sure both colours and other theme properties get applied...
            cnv->synchronise();
        } */

        editor->getTopLevelComponent()->repaint();
        editor->repaint();
    }
}

Colour PluginProcessor::getOutlineColour()
{
    return lnf->findColour(PlugDataColour::outlineColourId);
}

Colour PluginProcessor::getForegroundColour()
{
    return lnf->findColour(PlugDataColour::canvasTextColourId);
}

Colour PluginProcessor::getBackgroundColour()
{
    return lnf->findColour(PlugDataColour::guiObjectBackgroundColourId);
}

Colour PluginProcessor::getTextColour()
{
    return lnf->findColour(PlugDataColour::toolbarTextColourId);
}

void PluginProcessor::receiveNoteOn(int const channel, int const pitch, int const velocity)
{
    if (velocity == 0) {
        midiBufferOut.addEvent(MidiMessage::noteOff(channel, pitch, uint8(0)), audioAdvancement);
    } else {
        midiBufferOut.addEvent(MidiMessage::noteOn(channel, pitch, static_cast<uint8>(velocity)), audioAdvancement);
    }
}

void PluginProcessor::receiveControlChange(int const channel, int const controller, int const value)
{
    midiBufferOut.addEvent(MidiMessage::controllerEvent(channel, controller, value), audioAdvancement);
}

void PluginProcessor::receiveProgramChange(int const channel, int const value)
{
    midiBufferOut.addEvent(MidiMessage::programChange(channel, value), audioAdvancement);
}

void PluginProcessor::receivePitchBend(int const channel, int const value)
{
    midiBufferOut.addEvent(MidiMessage::pitchWheel(channel, value + 8192), audioAdvancement);
}

void PluginProcessor::receiveAftertouch(int const channel, int const value)
{
    midiBufferOut.addEvent(MidiMessage::channelPressureChange(channel, value), audioAdvancement);
}

void PluginProcessor::receivePolyAftertouch(int const channel, int const pitch, int const value)
{
    midiBufferOut.addEvent(MidiMessage::aftertouchChange(channel, pitch, value), audioAdvancement);
}

void PluginProcessor::receiveMidiByte(int const port, int const byte)
{
    if (midiByteIsSysex) {
        if (byte == 0xf7) {
            midiBufferOut.addEvent(MidiMessage::createSysExMessage(midiByteBuffer, static_cast<int>(midiByteIndex)), audioAdvancement);
            midiByteIndex = 0;
            midiByteIsSysex = false;
        } else {
            midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
            if (midiByteIndex == 512) {
                midiByteIndex = 511;
            }
        }
    } else if (midiByteIndex == 0 && byte == 0xf0) {
        midiByteIsSysex = true;
    } else {
        midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
        if (midiByteIndex >= 3) {
            midiBufferOut.addEvent(MidiMessage(midiByteBuffer, 3), audioAdvancement);
            midiByteIndex = 0;
        }
    }
}

void PluginProcessor::performParameterChange(int type, String name, float value)
{
    // Type == 1 means it sets the change gesture state
    if (type) {
        for (auto* param : getParameters()) {
            auto* pldParam = dynamic_cast<PlugDataParameter*>(param);

            if (!pldParam->isEnabled() || pldParam->getTitle() != name)
                continue;

            if (pldParam->getGestureState() == value) {
                logMessage("parameter change " + name + (value ? " already started" : " not started"));
            } else if (pldParam->isEnabled() && pldParam->getTitle() == name) {
                pldParam->setGestureState(value);
            }
        }
    } else { // otherwise set parameter value
        for (auto* param : getParameters()) {
            auto* pldParam = dynamic_cast<PlugDataParameter*>(param);
            if (!pldParam->isEnabled() || pldParam->getTitle() != name)
                continue;

            // Update values in automation panel
            // if (pldParam->getLastValue() == value)
            //    return;

            // pldParam->setLastValue(value);

            // Send new value to DAW
            pldParam->setUnscaledValueNotifyingHost(value);

#if PLUGDATA_STANDALONE
            if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
                editor->sidebar.updateAutomationParameters();
            }
#endif
        }
    }
}

void PluginProcessor::receiveDSPState(bool dsp)
{
    MessageManager::callAsync(
        [this, dsp]() mutable {
            if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
                editor->statusbar.powerButton->setToggleState(dsp, dontSendNotification);
            }
        });
}

void PluginProcessor::updateDrawables()
{
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        MessageManager::callAsync([cnv = editor->getCurrentCanvas()]() {
            if (cnv)
                cnv->updateDrawables();
        });
    }
}

void PluginProcessor::updateConsole()
{
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        editor->sidebar.updateConsole();
    }
}

void PluginProcessor::reloadAbstractions(File changedPatch, t_glist* except)
{
    suspendProcessing(true);

    auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor());

    setThis();

    // Ensure that all messages are dequeued before we start deleting objects
    sendMessagesFromQueue();

    isPerformingGlobalSync = true;

    pd::Patch::reloadPatch(changedPatch, except);

    // Synchronising can potentially delete some other canvases, so make sure we use a safepointer
    Array<Component::SafePointer<Canvas>> canvases;

    if (editor) {
        for (auto* canvas : editor->canvases) {
            canvases.add(canvas);
        }
    }

    for (auto& cnv : canvases) {
        if (cnv.getComponent())
            cnv->synchronise();
    }

    isPerformingGlobalSync = false;

    suspendProcessing(false);

    if (editor) {
        editor->updateCommandStatus();
    }
}

void PluginProcessor::synchroniseCanvas(void* cnv)
{
    MessageManager::callAsync(
        [this, cnv]() mutable {
            if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
                for (auto* canvas : editor->canvases) {
                    if (canvas->patch.getPointer() == cnv) {
                        canvas->synchronise();
                    }
                }
            }
        });
}

void PluginProcessor::titleChanged()
{
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        for (int n = 0; n < editor->tabbar.getNumTabs(); n++) {
            auto* cnv = editor->getCanvas(n);
            if (!cnv)
                return;
            editor->tabbar.setTabName(n, cnv->patch.getTitle());
        }
    }
}

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
