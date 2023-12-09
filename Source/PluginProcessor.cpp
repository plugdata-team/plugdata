/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <clocale>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"
#include "Pd/Library.h"

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/SettingsFile.h"
#include "Utility/PluginParameter.h"
#include "Utility/OSUtils.h"
#include "Utility/AudioSampleRingBuffer.h"
#include "Utility/MidiDeviceManager.h"
#include "Dialogs/ConnectionMessageDisplay.h"

#include "Utility/Presets.h"
#include "Canvas.h"
#include "PluginMode.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Tabbar/Tabbar.h"
#include "Object.h"
#include "Statusbar.h"

#include "Dialogs/Dialogs.h"
#include "Sidebar/Sidebar.h"

extern "C" {
#include "../Libraries/cyclone/shared/common/file.h"
EXTERN char* pd_version;
}

AudioProcessor::BusesProperties PluginProcessor::buildBusesProperties()
{
    AudioProcessor::BusesProperties busesProperties;

    if (ProjectInfo::isStandalone) {
        busesProperties.addBus(true, "Main Input", AudioChannelSet::canonicalChannelSet(16), true);
        busesProperties.addBus(false, "Main Output", AudioChannelSet::canonicalChannelSet(16), true);
    } else {
        busesProperties.addBus(true, "Main Input", AudioChannelSet::stereo(), true);

        for (int i = 1; i < numInputBuses; i++)
            busesProperties.addBus(true, "Aux Input " + String(i), AudioChannelSet::stereo(), false);

        busesProperties.addBus(false, "Main Output", AudioChannelSet::stereo(), true);

        for (int i = 1; i < numOutputBuses; i++)
            busesProperties.addBus(false, "Aux Output" + String(i), AudioChannelSet::stereo(), false);
    }

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
    , internalSynth(std::make_unique<InternalSynth>())
{
    // Make sure to use dots for decimal numbers, pd requires that
    std::setlocale(LC_ALL, "C");

    {
        const MessageManagerLock mmLock; // Do we need this? Isn't this already on the messageManager?

        LookAndFeel::setDefaultLookAndFeel(&lnf.get());

        // Initialise directory structure and settings file
        initialiseFilesystem();
        settingsFile = SettingsFile::getInstance()->initialise();
    }

    statusbarSource = std::make_unique<StatusbarSource>();

    auto* volumeParameter = new PlugDataParameter(this, "volume", 0.8f, true, 0, 0.0f, 1.0f);
    addParameter(volumeParameter);
    volume = volumeParameter->getValuePointer();

    // XML tree for storing additional data in DAW session
    extraData = std::make_unique<XmlElement>("ExtraData");

    // General purpose automation parameters you can get by using "receive param1" etc.
    for (int n = 0; n < numParameters; n++) {
        auto* parameter = new PlugDataParameter(this, "param" + String(n + 1), 0.0f, false, n + 1, 0.0f, 1.0f);
        addParameter(parameter);
    }

    // Make sure that the parameter valuetree has a name, to prevent assertion failures
    // parameters.replaceState(ValueTree("plugdata"));

    logMessage("plugdata v" + String(ProjectInfo::versionString));
    auto gitHash = String(PLUGDATA_GIT_HASH);
    if (gitHash.isNotEmpty()) {
        logMessage("Nightly build: " + gitHash);
    }
    logMessage("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false));
    logMessage("Libraries:");
    logMessage(else_version);
    logMessage(cyclone_version);
    logMessage(heavylib_version);

    // Set up midi buffers
    midiBufferIn.ensureSize(2048);
    midiBufferOut.ensureSize(2048);
    midiBufferInternalSynth.ensureSize(2048);

    atoms_playhead.reserve(3);
    atoms_playhead.resize(1);

    sendMessagesFromQueue();

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
    enableInternalSynth = settingsFile->getProperty<int>("internal_synth");

    auto currentThemeTree = settingsFile->getCurrentTheme();

    // ag: This needs to be done *after* the library data has been unpacked on
    // first launch.
    initialisePd(pdlua_version);
    logMessage(pdlua_version);

    updateSearchPaths();

    objectLibrary = std::make_unique<pd::Library>(this);
    objectLibrary->appDirChanged = [this]() {
        // If we changed the settings from within the app, don't reload
        settingsFile->reloadSettings();
        auto newTheme = settingsFile->getProperty<String>("theme");
        if (PlugDataLook::currentTheme != newTheme) {
            setTheme(newTheme);
        }

        updateSearchPaths();
        objectLibrary->updateLibrary();
    };

    setLatencySamples(pd::Instance::getBlockSize());
}

PluginProcessor::~PluginProcessor()
{
    // Deleting the pd instance in ~PdInstance() will also free all the Pd patches
    patches.clear();
}

void PluginProcessor::initialiseFilesystem()
{
    auto const& homeDir = ProjectInfo::appDataDir;
    auto const& versionDataDir = ProjectInfo::versionDataDir;
    auto deken = homeDir.getChildFile("Externals");
    auto patches = homeDir.getChildFile("Patches");

    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!homeDir.exists() || !versionDataDir.exists()) {

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
        versionDataDir.getParentDirectory().createDirectory();
        versionDataDir.createDirectory();
        homeDir.getChildFile("plugdata_version").moveFileTo(versionDataDir);
    }
    if (!deken.exists()) {
        deken.createDirectory();
    }
    if (!patches.exists()) {
        patches.createDirectory();
    }

    auto testTonePatch = homeDir.getChildFile("testtone.pd");
    auto cpuTestPatch = homeDir.getChildFile("load-meter.pd");

    if (testTonePatch.exists())
        testTonePatch.deleteFile();
    if (cpuTestPatch.exists())
        cpuTestPatch.deleteFile();

    File(versionDataDir.getChildFile("./Documentation/7.stuff/tools/testtone.pd")).copyFileTo(testTonePatch);
    File(versionDataDir.getChildFile("./Documentation/7.stuff/tools/load-meter.pd")).copyFileTo(cpuTestPatch);

    // We want to recreate these symlinks so that they link to the abstractions/docs for the current plugdata version
    homeDir.getChildFile("Abstractions").deleteFile();
    homeDir.getChildFile("Documentation").deleteFile();
    homeDir.getChildFile("Extra").deleteFile();

    // We always want to update the symlinks in case an older version of plugdata was used
#if JUCE_WINDOWS
    // Get paths that need symlinks
    auto abstractionsPath = versionDataDir.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\");
    auto documentationPath = versionDataDir.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\");
    auto extraPath = versionDataDir.getChildFile("Extra").getFullPathName().replaceCharacters("/", "\\");
    auto dekenPath = deken.getFullPathName();
    auto patchesPath = patches.getFullPathName();

    // Create NTFS directory junctions
    OSUtils::createJunction(homeDir.getChildFile("Abstractions").getFullPathName().replaceCharacters("/", "\\").toStdString(), abstractionsPath.toStdString());
    OSUtils::createJunction(homeDir.getChildFile("Documentation").getFullPathName().replaceCharacters("/", "\\").toStdString(), documentationPath.toStdString());
    OSUtils::createJunction(homeDir.getChildFile("Extra").getFullPathName().replaceCharacters("/", "\\").toStdString(), extraPath.toStdString());

#elif JUCE_IOS
    // This is not ideal but on iOS, it seems to be the only way to make it work...
    versionDataDir.getChildFile("Abstractions").copyDirectoryTo(homeDir.getChildFile("Abstractions"));
    versionDataDir.getChildFile("Documentation").copyDirectoryTo(homeDir.getChildFile("Documentation"));
    versionDataDir.getChildFile("Extra").copyDirectoryTo(homeDir.getChildFile("Extra"));
#else
    versionDataDir.getChildFile("Abstractions").createSymbolicLink(homeDir.getChildFile("Abstractions"), true);
    versionDataDir.getChildFile("Documentation").createSymbolicLink(homeDir.getChildFile("Documentation"), true);
    versionDataDir.getChildFile("Extra").createSymbolicLink(homeDir.getChildFile("Extra"), true);
#endif
    
    internalSynth->extractSoundfont();
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
    pd::Interface::getSearchPaths(p, &numItems);
    auto currentPaths = StringArray(p, numItems);

    auto paths = pd::Library::defaultPaths;

    for (auto child : pathTree) {
        auto path = child.getProperty("Path").toString().replace("\\", "/");
        paths.addIfNotAlreadyThere(path);
    }

    for (auto const& path : paths) {
        if (currentPaths.contains(path.getFullPathName()))
            continue;
        libpd_add_to_search_path(path.getFullPathName().toRawUTF8());
    }

    for (auto const& path : DekenInterface::getExternalPaths()) {
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
    return ProjectInfo::projectName;
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
    return ProjectInfo::isMidiEffect();
}

double PluginProcessor::getTailLengthSeconds() const
{
    return getValue<float>(tailLength);
}

int PluginProcessor::getNumPrograms()
{
    return Presets::presets.size();
}

int PluginProcessor::getCurrentProgram()
{
    return lastSetProgram;
}

void PluginProcessor::setCurrentProgram(int index)
{
    if (isPositiveAndBelow(index, Presets::presets.size())) {
        MemoryOutputStream data;
        Base64::convertFromBase64(data, Presets::presets[index].second);
        if (data.getDataSize() > 0) {
            setStateInformation(data.getData(), static_cast<int>(data.getDataSize()));
            lastSetProgram = index;
        }
    }
}

const String PluginProcessor::getProgramName(int index)
{
    if (isPositiveAndBelow(index, Presets::presets.size())) {
        return Presets::presets[index].first;
    }

    return "Init preset";
}

void PluginProcessor::changeProgramName(int index, String const& newName)
{
}

void PluginProcessor::setOversampling(int amount)
{
    if (oversampling == amount)
        return;

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

    oversampler = std::make_unique<dsp::Oversampling<float>>(std::max(1, maxChannels), oversampling, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);

    oversampler->initProcessing(samplesPerBlock);

    if (enableInternalSynth && ProjectInfo::isStandalone) {
        internalSynth->prepare(sampleRate, samplesPerBlock, maxChannels);
    }

    audioAdvancement = 0;
    auto const pdBlockSize = static_cast<size_t>(Instance::getBlockSize());
    audioBufferIn.setSize(maxChannels, pdBlockSize);
    audioBufferOut.setSize(maxChannels, pdBlockSize);
    
    audioVectorIn.resize(maxChannels * pdBlockSize, 0.0f);
    audioVectorOut.resize(maxChannels * pdBlockSize, 0.0f);

    midiBufferIn.clear();
    midiBufferOut.clear();
    
    // If the block size is a multiple of 64 and we are not a plugin, we can optimise the process loop
    // Audio plugins can choose to send in a smaller block size when automation is happening
    variableBlockSize = !ProjectInfo::isStandalone || samplesPerBlock < pdBlockSize || samplesPerBlock % pdBlockSize != 0;
    
    if(variableBlockSize) {
        inputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(samplesPerBlock, pdBlockSize));
        outputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(samplesPerBlock, pdBlockSize));
    }

    midiByteIndex = 0;
    midiByteBuffer[0] = 0;
    midiByteBuffer[1] = 0;
    midiByteBuffer[2] = 0;

    cpuLoadMeasurer.reset(sampleRate, samplesPerBlock);

    startDSP();

    statusbarSource->setSampleRate(sampleRate);
    statusbarSource->setBufferSize(samplesPerBlock);
    statusbarSource->prepareToPlay(getTotalNumOutputChannels());

    limiter.prepare({ sampleRate, static_cast<uint32>(samplesPerBlock), std::max(1u, static_cast<uint32>(maxChannels)) });

    smoothedGain.reset(AudioProcessor::getSampleRate(), 0.02);
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

static bool hasRealEvents(MidiBuffer& buffer)
{
    
    return std::any_of(buffer.begin(), buffer.end(),
        [](auto const& event) {
            int dummy;
            return !MidiDeviceManager::convertFromSysExFormat(event.getMessage(), dummy).isSysEx();
        });
}

void PluginProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    AudioProcessLoadMeasurer::ScopedTimer cpuTimer(cpuLoadMeasurer, buffer.getNumSamples());

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    setThis();
    sendPlayhead();
    sendParameters();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
    
    auto targetBlock = dsp::AudioBlock<float>(buffer);
    auto blockOut = oversampling > 0 ? oversampler->processSamplesUp(targetBlock) : targetBlock;

    auto hasMidiInEvents = hasRealEvents(midiMessages);
    
    midiBufferIn.clear();
    midiBufferOut.clear();
    
    if(variableBlockSize)
    {
        processVariable(blockOut, midiMessages);
    }
    else {
        processConstant(blockOut, midiMessages);
    }
    
    auto hasMidiOutEvents = hasRealEvents(midiMessages);

    if (oversampling > 0) {
        oversampler->processSamplesDown(targetBlock);
    }
    
    for(auto& patch : patches)
    {
        patch->updateUndoRedoState();
    }

    auto targetGain = volume->load();
    float mappedTargetGain = 0.0f;

    //    Slider value 0.8 is default unity
    //    The top part of the slider 0.8 - 1.0 is mapped to linear gain 1.0 - 2.0
    //    The lower part of the slider 0.0 - 0.8 is mapped to a power function that approximates a log curve between 0.0 - 1.0
    //
    //    +---------+-----------------+-------+--------------+
    //    | Dynamic |        a        |   b   | Approximation|
    //    |  range  |                 |       |  |
    //    +---------+-----------------+-------+--------------+
    //    |  50 dB  |  3.1623e-3      | 5.757 |      x^3     |
    //    |  60 dB  |     1e-3        | 6.908 |      x^4     |
    //    |  70 dB  |  3.1623e-4      | 8.059 |      x^5     |
    //    |  80 dB  |     1e-4        | 9.210 |      x^6     |
    //    |  90 dB  |  3.1623e-5      | 10.36 |      x^6     |
    //    | 100 dB  |     1e-5        | 11.51 |      x^7     |
    //    +---------+-----------------+-------+--------------+
    //    Table 1: Values for a and b in the equation a·exp(b·x)
    //
    //    https://www.dr-lex.be/info-stuff/volumecontrols.html

    if (targetGain <= 0.8f)
        mappedTargetGain = pow(jmap(targetGain, 0.0f, 0.8f, 0.0f, 1.0f), 2.5f);
    else
        mappedTargetGain = jmap(targetGain, 0.8f, 1.0f, 1.0f, 2.0f);

    // apply smoothing to the main volume control
    smoothedGain.setTargetValue(mappedTargetGain);
    smoothedGain.applyGain(buffer, buffer.getNumSamples());

    statusbarSource->process(hasMidiInEvents, hasMidiOutEvents, totalNumOutputChannels);
    statusbarSource->setCPUUsage(cpuLoadMeasurer.getLoadAsPercentage());
    statusbarSource->peakBuffer.write(buffer);

    if (ProjectInfo::isStandalone) {
        for (auto bufferIterator : midiMessages) {
            auto* midiDeviceManager = ProjectInfo::getMidiDeviceManager();

            int device;
            auto message = MidiDeviceManager::convertFromSysExFormat(bufferIterator.getMessage(), device);

            if (enableInternalSynth && (device > midiDeviceManager->getOutputDevices().size() || device == 0)) {
                midiBufferInternalSynth.addEvent(message, 0);
            }
            if (isPositiveAndBelow(device, midiDeviceManager->getOutputDevices().size() + 1)) {
                midiDeviceManager->sendMidiOutputMessage(device, message);
            }
        }

        // If the internalSynth is enabled and loaded, let it process the midi
        if (enableInternalSynth && internalSynth->isReady()) {
            internalSynth->process(buffer, midiBufferInternalSynth);
        } else if (!enableInternalSynth && internalSynth->isReady()) {
            internalSynth->unprepare();
        } else if (enableInternalSynth && !internalSynth->isReady()) {
            internalSynth->prepare(getSampleRate(), AudioProcessor::getBlockSize(), std::max(totalNumInputChannels, totalNumOutputChannels));
        }
        midiBufferInternalSynth.clear();
    }

    if (protectedMode && buffer.getNumChannels() > 0) {

        //Take out inf and NaN values
        auto* const* writePtr = buffer.getArrayOfWritePointers();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            for (int n = 0; n < buffer.getNumSamples(); n++) {
                if (!std::isfinite(writePtr[ch][n])) {
                    writePtr[ch][n] = 0.0f;
                }
            }
        }

        auto block = dsp::AudioBlock<float>(buffer);
        limiter.process(block);
    }
}

void PluginProcessor::processConstant(dsp::AudioBlock<float> buffer, MidiBuffer& midiMessages)
{
    int blockSize = Instance::getBlockSize();
    int numBlocks = buffer.getNumSamples() / blockSize;
    audioAdvancement = 0;
    
    if (producesMidi()) {
        midiByteIndex = 0;
        midiByteBuffer[0] = 0;
        midiByteBuffer[1] = 0;
        midiByteBuffer[2] = 0;
        midiBufferOut.clear();
    }
    
    for(int block = 0; block < numBlocks; block++)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                                              audioVectorIn.data() + (ch * blockSize),
                                              buffer.getChannelPointer(ch) + audioAdvancement,
                                              blockSize
                                              );
        }
 
        setThis();
        
        midiBufferIn.clear();
        midiBufferIn.addEvents(midiMessages, audioAdvancement, blockSize, 0);
        sendMidiBuffer();
        
        // Process audio
        performDSP(audioVectorIn.data(), audioVectorOut.data());
        
        sendMessagesFromQueue();
        
        if(connectionListener && plugdata_debugging_enabled()) connectionListener->updateSignalData();
        
        messageDispatcher->dispatch();
        
        for (int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                buffer.getChannelPointer(ch) + audioAdvancement,
                audioVectorOut.data() + (ch * blockSize),
                blockSize
            );
        }
        
        audioAdvancement += blockSize;
    }
    
    midiMessages.clear();
    midiMessages.addEvents(midiBufferOut, 0, buffer.getNumSamples(), 0);
}

void PluginProcessor::processVariable(dsp::AudioBlock<float> buffer, MidiBuffer& midiMessages)
{
    auto const blockSize = Instance::getBlockSize();
    auto const numChannels = buffer.getNumChannels();
    
    inputFifo->writeAudioAndMidi(buffer, midiMessages);
    
    audioAdvancement = 0;
    
    while(inputFifo->getNumSamplesAvailable() >= blockSize)
    {
        midiBufferIn.clear();
        inputFifo->readAudioAndMidi(audioBufferIn, midiBufferIn);

        for (int channel = 0; channel < audioBufferIn.getNumChannels(); ++channel)
        {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                audioVectorIn.data() + (channel * blockSize),
                audioBufferIn.getReadPointer(channel),
                blockSize
            );
        }
        
        if (producesMidi()) {
            midiByteIndex = 0;
            midiByteBuffer[0] = 0;
            midiByteBuffer[1] = 0;
            midiByteBuffer[2] = 0;
            midiBufferOut.clear();
        }
        
        setThis();

        sendMidiBuffer();
        
        // Process audio
        performDSP(audioVectorIn.data(), audioVectorOut.data());

        sendMessagesFromQueue();
        
        if(connectionListener && plugdata_debugging_enabled()) connectionListener->updateSignalData();
        
        messageDispatcher->dispatch();
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                audioBufferOut.getWritePointer(channel),
                audioVectorOut.data() + (channel * blockSize),
                blockSize
            );
        }
    
        outputFifo->writeAudioAndMidi(audioBufferOut, midiBufferOut);
        audioAdvancement += blockSize;
    }
    
    auto totalDelay = std::max(buffer.getNumSamples() / blockSize, buffer.getNumSamples());
    if(outputFifo->getNumSamplesAvailable() >= totalDelay)
    {
        outputFifo->readAudioAndMidi(buffer, midiMessages);
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
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqStart));
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqEnd));
        } else {
            atoms_playhead.emplace_back(0.0f);
            atoms_playhead.emplace_back(0.0f);
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
            atoms_playhead.emplace_back(static_cast<float>(infos->getTimeSignature()->denominator));
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
            atoms_playhead.emplace_back(static_cast<float>(*infos->getTimeInSeconds()));
        } else {
            atoms_playhead.emplace_back(0.0f);
        }

        sendMessage("playhead", "position", atoms_playhead);
        atoms_playhead.resize(1);
    }
}

void PluginProcessor::sendParameters()
{
    for (auto* param : getParameters()) {
        // We used to do dynamic_cast here, but since it gets called very often and param is always PlugDataParameter, we use reinterpret_cast now
        auto* pldParam = reinterpret_cast<PlugDataParameter*>(param);
        if (!pldParam->isEnabled())
            continue;

        auto newvalue = pldParam->getUnscaledValue();
        if (!approximatelyEqual(pldParam->getLastValue(), newvalue)) {
            auto title = pldParam->getTitle();
            sendFloat(title.toRawUTF8(), pldParam->getUnscaledValue());
            pldParam->setLastValue(newvalue);
        }
    }
}

void PluginProcessor::sendMidiBuffer()
{
    if (acceptsMidi()) {
        for (auto const& event : midiBufferIn) {

            int device;
            auto message = MidiDeviceManager::convertFromSysExFormat(event.getMessage(), device);

            auto channel = message.getChannel() + (device << 4);

            if (message.isNoteOn()) {
                sendNoteOn(channel, message.getNoteNumber(), message.getVelocity());
            } else if (message.isNoteOff()) {
                sendNoteOn(channel, message.getNoteNumber(), 0);
            } else if (message.isController()) {
                sendControlChange(channel, message.getControllerNumber(), message.getControllerValue());
            } else if (message.isPitchWheel()) {
                sendPitchBend(channel, message.getPitchWheelValue() - 8192);
            } else if (message.isChannelPressure()) {
                sendAfterTouch(channel, message.getChannelPressureValue());
            } else if (message.isAftertouch()) {
                sendPolyAfterTouch(channel, message.getNoteNumber(), message.getAfterTouchValue());
            } else if (message.isProgramChange()) {
                sendProgramChange(channel, message.getProgramChangeNumber());
            } else if (message.isSysEx()) {
                for (int i = 0; i < message.getSysExDataSize(); ++i) {
                    sendSysEx(device, static_cast<int>(message.getSysExData()[i]));
                }
            } else if (message.isMidiClock() || message.isMidiStart() || message.isMidiStop() || message.isMidiContinue() || message.isActiveSense() || (message.getRawDataSize() == 1 && message.getRawData()[0] == 0xff)) {
                for (int i = 0; i < message.getRawDataSize(); ++i) {
                    sendSysRealTime(device, static_cast<int>(message.getRawData()[i]));
                }
            }

            for (int i = 0; i < message.getRawDataSize(); i++) {
                sendMidiByte(device, static_cast<int>(message.getRawData()[i]));
            }
        }
        midiBufferIn.clear();
    }
}

bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PluginProcessor::createEditor()
{
    auto* editor = new PluginEditor(*this);
    setThis();

    // If standalone, add to ownedArray of opened editor
    // In plugin, the deletion of PluginEditor is handled automatically
    if (ProjectInfo::isStandalone) {
        openedEditors.add(editor);
    }

    for (auto const& patch : patches) {
        auto* cnv = editor->canvases.add(new Canvas(editor, *patch, nullptr));
        editor->addTab(cnv, patch->splitViewIndex);
    }

    editor->resized();
    return editor;
}

bool PluginProcessor::isInPluginMode()
{
    for (auto& patch : patches) {
        if (patch->openInPluginMode) {
            return true;
        }
    }

    return false;
}

void PluginProcessor::getStateInformation(MemoryBlock& destData)
{
    setThis();

    savePatchTabPositions();

    // Store pure-data and parameter state
    MemoryOutputStream ostream(destData, false);

    ostream.writeInt(patches.size());

    // Save path and content for patch
    lockAudioThread();

    auto presetDir = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Presets");

    auto patchesTree = new XmlElement("Patches");

    for (auto const& patch : patches) {

        auto content = patch->getCanvasContent();
        auto patchFile = patch->getCurrentFile().getFullPathName();

        // Write legacy format
        ostream.writeString(content);
        ostream.writeString(patchFile);

        auto* patchTree = new XmlElement("Patch");
        // Write new format
        patchTree->setAttribute("Content", content);
        patchTree->setAttribute("Location", patchFile);
        patchTree->setAttribute("PluginMode", patch->openInPluginMode);
        patchTree->setAttribute("SplitIndex", patch->splitViewIndex);

        patchesTree->addChildElement(patchTree);
    }
    unlockAudioThread();

    ostream.writeInt(getLatencySamples());
    ostream.writeInt(oversampling);
    ostream.writeFloat(getValue<float>(tailLength));

    auto xml = XmlElement("plugdata_save");
    xml.setAttribute("Version", PLUGDATA_VERSION);

    // In the future, we're gonna load everything from xml, to make it easier to add new properties
    // By putting this here, we can prepare for making this change without breaking existing DAW saves
    xml.setAttribute("Oversampling", oversampling);
    xml.setAttribute("Latency", getLatencySamples());
    xml.setAttribute("TailLength", getValue<float>(tailLength));
    xml.setAttribute("Legacy", false);

    // TODO: make multi-window friendly
    if (auto* editor = getActiveEditor()) {
        xml.setAttribute("Width", editor->getWidth());
        xml.setAttribute("Height", editor->getHeight());
    } else {
        xml.setAttribute("Width", lastUIWidth);
        xml.setAttribute("Height", lastUIHeight);
    }

    xml.addChildElement(patchesTree);

    PlugDataParameter::saveStateInformation(xml, getParameters());

    // store additional extra-data in DAW session if they exist.
    bool extraDataStored = false;
    if (extraData)  {
        if (extraData->getNumChildElements() > 0) {
            xml.addChildElement(extraData.get());
            extraDataStored = true;
        }
    }

    MemoryBlock xmlBlock;
    copyXmlToBinary(xml, xmlBlock);

    ostream.writeInt(static_cast<int>(xmlBlock.getSize()));
    ostream.write(xmlBlock.getData(), xmlBlock.getSize());

    // then detach extraData XmlElement from temporary tree xml for later re-use
    if (extraDataStored)  {
        xml.removeChildElement(extraData.get(), false);
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
    MessageManager::callAsync([this]() {
        for (auto* editor : getEditors()) {
            for (auto split : editor->splitView.splits) {
                split->getTabComponent()->clearTabs();
            }
            editor->canvases.clear();
        }
    });

    lockAudioThread();

    setThis();
    patches.clear();

    int numPatches = istream.readInt();

    Array<std::pair<String, File>> patches;

    for (int i = 0; i < numPatches; i++) {
        auto state = istream.readString();
        auto path = istream.readString();

        auto presetDir = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Presets");
        path = path.replace("${PRESET_DIR}", presetDir.getFullPathName());

        auto location = File(path);

        patches.add({ state, location });
    }

    auto legacyLatency = istream.readInt();
    auto legacyOversampling = istream.readInt();
    auto legacyTail = istream.readFloat();

    auto xmlSize = istream.readInt();

    auto* xmlData = new char[xmlSize];
    istream.read(xmlData, xmlSize);

    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));

    auto openPatch = [this](String const& content, File const& location, bool pluginMode = false, int splitIndex = 0) {
        if (location.getFullPathName().isNotEmpty() && location.existsAsFile()) {
            auto patch = loadPatch(location, openedEditors[0], splitIndex);
            if (patch) {
                patch->setTitle(location.getFileName());
                patch->openInPluginMode = pluginMode;
            }
        } else {
            if (location.getParentDirectory().exists()) {
                auto parentPath = location.getParentDirectory().getFullPathName();
                libpd_add_to_search_path(parentPath.toRawUTF8());
            }
            auto patch = loadPatch(content, openedEditors[0], splitIndex);
            if (patch && ((location.exists() && location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) || !location.exists())) {
                patch->setTitle("Untitled Patcher");
                patch->openInPluginMode = pluginMode;
                patch->splitViewIndex = splitIndex;
            } else if (patch && location.existsAsFile()) {
                patch->setCurrentFile(location);
                patch->setTitle(location.getFileName());
                patch->openInPluginMode = pluginMode;
                patch->splitViewIndex = splitIndex;
            }
        }
    };

    if (xmlState) {
        // If xmltree contains new patch format, use that
        if (auto* patchTree = xmlState->getChildByName("Patches")) {
            for (auto p : patchTree->getChildWithTagNameIterator("Patch")) {
                auto content = p->getStringAttribute("Content");
                auto location = p->getStringAttribute("Location");
                auto pluginMode = p->getBoolAttribute("PluginMode");

                int splitIndex = 0;
                if (p->hasAttribute("SplitIndex")) {
                    splitIndex = p->getIntAttribute("SplitIndex");
                }

                auto presetDir = ProjectInfo::versionDataDir.getChildFile("Extra").getChildFile("Presets");
                location = location.replace("${PRESET_DIR}", presetDir.getFullPathName());

                openPatch(content, location, pluginMode, splitIndex);
            }
        }
        // Otherwise, load from legacy format
        else {
            for (auto& [content, location] : patches) {
                openPatch(content, location);
            }
        }

        jassert(xmlState);

        PlugDataParameter::loadStateInformation(*xmlState, getParameters());

        auto versionString = String("0.6.1"); // latest version that didn't have version inside the daw state

        if (!xmlState->hasAttribute("Legacy") || xmlState->getBoolAttribute("Legacy")) {
            setLatencySamples(legacyLatency);
            setOversampling(legacyOversampling);
            tailLength = legacyTail;
        } else {
            setOversampling(xmlState->getDoubleAttribute("Oversampling"));
            setLatencySamples(xmlState->getDoubleAttribute("Latency"));
            tailLength = xmlState->getDoubleAttribute("TailLength");
        }

        if (xmlState->hasAttribute("Version")) {
            versionString = xmlState->getStringAttribute("Version");
        }

        if (xmlState->hasAttribute("Height") && xmlState->hasAttribute("Width")) {
            int windowWidth = xmlState->getIntAttribute("Width", 1000);
            int windowHeight = xmlState->getIntAttribute("Height", 650);
            lastUIWidth = windowWidth;
            lastUIHeight = windowHeight;
            // TODO: make multi-window friendly
            if (auto* editor = getActiveEditor()) {
                MessageManager::callAsync([editor = Component::SafePointer(editor), windowWidth, windowHeight]() {
                    if (!editor)
                        return;
                    editor->setSize(windowWidth, windowHeight);
                });
            }
        }

        // Retrieve additional extra-data from DAW
        parseDataBuffer(*xmlState);
    }

    unlockAudioThread();

    delete[] xmlData;

    MessageManager::callAsync([this]() {
        for (auto* editor : getEditors()) {
            editor->sidebar->updateAutomationParameters();

            if (editor->pluginMode && !editor->pd->isInPluginMode()) {
                editor->pluginMode->closePluginMode();
            }
        }
    });
}

pd::Patch::Ptr PluginProcessor::loadPatch(File const& patchFile, PluginEditor* editor, int splitIndex)
{
    // First, check if patch is already opened
    for (auto const& patch : patches) {
        if (patch->getCurrentFile() == patchFile) {

            MessageManager::callAsync([this, patch]() mutable {
                for (auto* editor : getEditors()) {
                    for (auto* cnv : editor->canvases) {
                        if (cnv->patch == *patch) {
                            cnv->getTabbar()->setCurrentTabIndex(cnv->getTabIndex());
                        }
                    }
                    editor->pd->logError("Patch is already open");
                }
            });

            // Patch is already opened
            return nullptr;
        }
    }

    // Stop the audio callback when loading a new patch
    // TODO: why though?
    lockAudioThread();

    auto newPatch = openPatch(patchFile);

    unlockAudioThread();

    if (!newPatch->getPointer()) {
        logError("Couldn't open patch");
        return nullptr;
    }

    patches.add(newPatch);
    auto* patch = patches.getLast().get();

    if(editor) {
        MessageManager::callAsync([this, patch, splitIndex, _editor = Component::SafePointer<PluginEditor>(editor)]() mutable {
            if(!_editor) return;
            // There are some subroutines that get called when we create a canvas, that will lock the audio thread
            // By locking it around this whole function, we can prevent slowdowns from constantly locking/unlocking the audio thread
            lockAudioThread();

            auto* cnv = _editor->canvases.add(new Canvas(_editor.getComponent(), *patch, nullptr));

            unlockAudioThread();

            _editor->addTab(cnv, splitIndex);
        });
    }
    patch->setCurrentFile(patchFile);

    return patch;
}

pd::Patch::Ptr PluginProcessor::loadPatch(String patchText, PluginEditor* editor, int splitIndex)
{
    if (patchText.isEmpty())
        patchText = pd::Instance::defaultPatch;

    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patchText);

    auto patch = loadPatch(patchFile, editor, splitIndex);

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

    if (!force && oldThemeTree.isValid() && themeTree.isEquivalentTo(oldThemeTree))
        return;

    lnf->setTheme(themeTree);

    for (auto* editor : getEditors()) {
        editor->sendLookAndFeelChange();
        editor->getTopLevelComponent()->repaint();
        editor->repaint();
    }
}

Colour PluginProcessor::getOutlineColour()
{
    return lnf->findColour(PlugDataColour::guiObjectInternalOutlineColour);
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
    auto device = (channel - 1) >> 4;
    auto deviceChannel = channel - (device * 16);

    if (velocity == 0) {
        midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::noteOff(deviceChannel, pitch, uint8(0)), device), audioAdvancement);
    } else {
        midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::noteOn(deviceChannel, pitch, static_cast<uint8>(velocity)), device), audioAdvancement);
    }
}

void PluginProcessor::receiveControlChange(int const channel, int const controller, int const value)
{
    auto device = channel >> 4;
    auto deviceChannel = channel - (device * 16);

    midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::controllerEvent(deviceChannel, controller, value), device), audioAdvancement);
}

void PluginProcessor::receiveProgramChange(int const channel, int const value)
{
    auto device = channel >> 4;
    auto deviceChannel = channel - (device * 16);

    midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::programChange(deviceChannel, value), device), audioAdvancement);
}

void PluginProcessor::receivePitchBend(int const channel, int const value)
{
    auto device = channel >> 4;
    auto deviceChannel = channel - (device * 16);

    midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::pitchWheel(deviceChannel, value + 8192), device), audioAdvancement);
}

void PluginProcessor::receiveAftertouch(int const channel, int const value)
{
    auto device = channel >> 4;
    auto deviceChannel = channel - (device * 16);

    midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::channelPressureChange(deviceChannel, value), device), audioAdvancement);
}

void PluginProcessor::receivePolyAftertouch(int const channel, int const pitch, int const value)
{
    auto device = channel >> 4;
    auto deviceChannel = channel - (device * 16);

    midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::aftertouchChange(deviceChannel, pitch, value), device), audioAdvancement);
}

void PluginProcessor::receiveMidiByte(int const port, int const byte)
{
    auto device = port >> 4;

    if (midiByteIsSysex) {
        if (byte == 0xf7) {
            midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage::createSysExMessage(midiByteBuffer, static_cast<int>(midiByteIndex)), device), audioAdvancement);
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
        // Handle single-byte messages
        if (midiByteIndex == 0 && byte >= 0xf8 && byte <= 0xff) {
            midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage(static_cast<uint8>(byte)), device), audioAdvancement);
        }
        // Handle 3-byte messages
        else {
            midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
            if (midiByteIndex >= 3) {
                midiBufferOut.addEvent(MidiDeviceManager::convertToSysExFormat(MidiMessage(midiByteBuffer, 3), device), audioAdvancement);
                midiByteIndex = 0;
            }
        }
    }
}

void PluginProcessor::receiveSysMessage(String const& selector, std::vector<pd::Atom> const& list)
{
    switch (hash(selector)) {
    case hash("open"): {
        if (list.size() >= 2) {
            auto filename = list[0].toString();
            auto directory = list[1].toString();

            auto patch = File(directory).getChildFile(filename);
            loadPatch(patch, openedEditors[0]);
        }
        break;
    }
    case hash("menunew"): {
        if (list.size() >= 2) {
            auto filename = list[0].toString();
            auto directory = list[1].toString();

            auto patchPtr = loadPatch(defaultPatch, openedEditors[0]);
            patchPtr->setCurrentFile(File(directory).getChildFile(filename).getFullPathName());
            patchPtr->setTitle(filename);
        }
        break;
    }
    case hash("dsp"): {
        bool dsp = list[0].getFloat();
        MessageManager::callAsync(
            [this, dsp]() mutable {
                for (auto* editor : getEditors()) {
                    editor->statusbar->powerButton.setToggleState(dsp, dontSendNotification);
                }
            });
        break;
    }
    case hash("quit"):
    case hash("verifyquit"): {
        if (ProjectInfo::isStandalone) {
            bool askToSave = hash(selector) == hash("verifyquit");
            MessageManager::callAsync(
                [this, askToSave]() mutable {
                    // TODO: make multi-window friendly
                    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
                        editor->quit(askToSave);
                    }
                });
        } else {
            logWarning("Quitting Pd not supported in plugin");
        }
        break;
    }
    }
}

void PluginProcessor::addTextToTextEditor(unsigned long ptr, String text)
{
    Dialogs::appendTextToTextEditorDialog(textEditorDialogs[ptr].get(), text);
}
void PluginProcessor::showTextEditor(unsigned long ptr, Rectangle<int> bounds, String title)
{
    static std::unique_ptr<Dialog> saveDialog = nullptr;

    textEditorDialogs[ptr].reset(Dialogs::showTextEditorDialog("", title, [this, title, ptr](String const& lastText, bool hasChanged) {
        if (!hasChanged) {
            textEditorDialogs[ptr].reset(nullptr);
            return;
        }

        Dialogs::showAskToSaveDialog(
            &saveDialog, textEditorDialogs[ptr].get(), "", [this, ptr, title, text = lastText](int result) mutable {
                if (result == 2) {

                    lockAudioThread();
                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("clear"), 0, NULL);
                    unlockAudioThread();

                    // remove repeating spaces
                    text = text.replace("\r ", "\r");
                    text = text.replace(";\r", ";");
                    text = text.replace("\r;", ";");
                    text = text.replace(" ;", ";");
                    text = text.replace("; ", ";");
                    text = text.replace(",", " , ");
                    text = text.replaceCharacters("\r", " ");

                    while (text.contains("  ")) {
                        text = text.replace("  ", " ");
                    }
                    text = text.trimStart();
                    auto lines = StringArray::fromTokens(text, ";", "\"");

                    int count = 0;
                    for (auto const& line : lines) {
                        count++;
                        auto words = StringArray::fromTokens(line, " ", "\"");

                        auto atoms = std::vector<t_atom>();
                        atoms.reserve(words.size() + 1);

                        for (auto const& word : words) {
                            atoms.emplace_back();
                            // check if string is a valid number
                            auto charptr = word.getCharPointer();
                            auto ptr = charptr;
                            CharacterFunctions::readDoubleValue(ptr); // Removes double value from char*
                            if (*charptr == ',') {
                                SETCOMMA(&atoms.back());
                            } else if (ptr - charptr == word.getNumBytesAsUTF8() && ptr - charptr != 0) {
                                SETFLOAT(&atoms.back(), word.getFloatValue());
                            } else {
                                SETSYMBOL(&atoms.back(), generateSymbol(word));
                            }
                        }

                        if (count != lines.size()) {
                            atoms.emplace_back();
                            SETSEMI(&atoms.back());
                        }

                        lockAudioThread();

                        pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("addline"), atoms.size(), atoms.data());

                        unlockAudioThread();
                    }

                    t_atom fake_path;
                    SETSYMBOL(&fake_path, generateSymbol(title.toRawUTF8()));

                    lockAudioThread();

                    // pd_typedmess(reinterpret_cast<t_pd*>(ptr), generateSymbol("path"), 1, &fake_path);
                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), generateSymbol("end"), 0, NULL);
                    unlockAudioThread();

                    textEditorDialogs[ptr].reset(nullptr);
                }
                if (result == 1) {
                    textEditorDialogs[ptr].reset(nullptr);
                }
            },
            15, false);
    }));
}

void PluginProcessor::performParameterChange(int type, String const& name, float value)
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

            if (ProjectInfo::isStandalone) {
                for (auto* editor : getEditors()) {
                    editor->sidebar->updateAutomationParameters();
                }
            }
        }
    }
}

// JYG added this
void PluginProcessor::fillDataBuffer(std::vector<pd::Atom> const& vec)
{
    if (!vec[0].isSymbol()) {
        logMessage("databuffer accepts only lists beginning with a Symbol atom");
        return;
    }
    String child_name = String(vec[0].toString());

    if (extraData) {

        int const numChildren = extraData->getNumChildElements();
        if (numChildren > 0)    {
            // Searching if a previously created child element exists, with same name as vec[0]. If true, delete it.
                XmlElement* list = extraData->getChildByName(child_name);
                if (list)
                    extraData->removeChildElement (list, true) ;
        }
        XmlElement* list = extraData->createNewChildElement(child_name);
        if (list) {
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i].isFloat()) {
                    list->setAttribute(String("float") + String(i + 1), vec[i].getFloat());
                } else if (vec[i].isSymbol()) {
                    list->setAttribute(String("string") + String(i + 1), String(vec[i].toString()));
                } else {
                    list->setAttribute(String("atom") + String(i + 1), String("unknown"));
                }
            }
        } else {
            logMessage("Error: can't allocate memory for saving plugin databuffer.");
        }
    } else {
        logMessage("Error, databuffer extraData has not been allocated.");
    }
}

void PluginProcessor::parseDataBuffer(XmlElement const& xml)
{
    // source : void CamomileAudioProcessor::loadInformation(XmlElement const& xml)

    bool loaded = false;
    XmlElement const* extra_data = xml.getChildByName(juce::StringRef("ExtraData"));
    if (extra_data) {
        int const nlists = extra_data->getNumChildElements();
        std::vector<pd::Atom> vec;
        for (int i = 0; i < nlists; ++i) {
            XmlElement const* list = extra_data->getChildElement(i);
            if (list) {
                int const natoms = list->getNumAttributes();
                vec.resize(natoms);

                for (int j = 0; j < natoms; ++j) {
                    String const& name = list->getAttributeName(j);
                    if (name.startsWith("float")) {
                        vec[j] = static_cast<float>(list->getDoubleAttribute(name));
                    } else if (name.startsWith("string")) {
                        vec[j] = generateSymbol(list->getStringAttribute(name));
                    } else {
                        vec[j] = generateSymbol(String("unknown"));
                    }
                }

                sendList("from_daw_databuffer", vec);
                loaded = true;
            }
        }
    }

    if (!loaded) {
        sendBang("from_daw_databuffer");
    }
}

void PluginProcessor::updateConsole(int numMessages, bool newWarning)
{
    for (auto* editor : getEditors()) {
        editor->sidebar->updateConsole(numMessages, newWarning);
    }
}

Array<PluginEditor*> PluginProcessor::getEditors() const
{
    Array<PluginEditor*> editors;
    if(ProjectInfo::isStandalone)
    {
        for (auto* editor : openedEditors) {
            editors.add(editor);
        }
    }
    else {
        if(auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
        {
            editors.add(editor);
        }
    }

    return editors;
}

void PluginProcessor::reloadAbstractions(File changedPatch, t_glist* except)
{
    setThis();

    // Ensure that all messages are dequeued before we start deleting objects
    sendMessagesFromQueue();

    isPerformingGlobalSync = true;

    pd::Patch::reloadPatch(changedPatch, except);

    for (auto* editor : getEditors()) {

        // Synchronising can potentially delete some other canvases, so make sure we use a safepointer
        Array<Component::SafePointer<Canvas>> canvases;

        for (auto* canvas : editor->canvases) {
            canvases.add(canvas);
        }

        for (auto& cnv : canvases) {
            if (cnv.getComponent()) {
                cnv->synchronise();
                cnv->handleUpdateNowIfNeeded();
            }
        }

        editor->updateCommandStatus();
    }

    isPerformingGlobalSync = false;
}

void PluginProcessor::titleChanged()
{
    for (auto* editor : getEditors()) {
        for (auto split : editor->splitView.splits) {
            auto tabbar = split->getTabComponent();
            for (int n = 0; n < tabbar->getNumTabs(); n++) {
                auto* cnv = tabbar->getCanvas(n);
                if (!cnv)
                    return;

                tabbar->setTabText(n, cnv->patch.getTitle() + String(cnv->patch.isDirty() ? "*" : ""));
            }
        }
    }
}

void PluginProcessor::savePatchTabPositions()
{
    Array<std::tuple<pd::Patch*, int>> sortedPatches;
    // TODO: make multi-window friendly
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        for (auto* cnv : editor->canvases) {
            cnv->patch.splitViewIndex = editor->splitView.getTabComponentSplitIndex(cnv->getTabbar());
            sortedPatches.add({ &cnv->patch, cnv->getTabIndex() });
        }
    }

    std::sort(sortedPatches.begin(), sortedPatches.end(), [](auto const& a, auto const& b) {
        auto& [patchA, idxA] = a;
        auto& [patchB, idxB] = b;

        if (patchA->splitViewIndex == patchB->splitViewIndex)
            return idxA < idxB;

        return patchA->splitViewIndex < patchB->splitViewIndex;
    });

    patches.getLock().enter();
    int i = 0;
    for (auto& [patch, tabIdx] : sortedPatches) {

        if (i >= patches.size())
            break;

        patches.set(i, patch);
        i++;
    }
    patches.getLock().exit();
}

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
