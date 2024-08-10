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

#include "Utility/Presets.h"
#include "Canvas.h"
#include "PluginMode.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Object.h"
#include "Statusbar.h"

#include "Dialogs/Dialogs.h"
#include "Dialogs/ConnectionMessageDisplay.h"

#include "Sidebar/Sidebar.h"

extern "C" {
#include "../Libraries/pd-cyclone/shared/common/file.h"
EXTERN char* pd_version;
}

bool gemWinSetCurrent();
bool gemWinUnsetCurrent();

AudioProcessor::BusesProperties PluginProcessor::buildBusesProperties()
{
#if JUCE_IOS

    if (ProjectInfo::isStandalone) {
        return BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true).withInput("Input", AudioChannelSet::mono(), true);
    }
    // If you intend to build AUv3 on macOS, you'll also need these
    if (ProjectInfo::isFx) {
        return BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true).withInput("Input", AudioChannelSet::stereo(), true);
    } else {
        return BusesProperties().withOutput("Output", AudioChannelSet::stereo(), true);
    }
#else
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
#endif
}

// ag: Note that this is just a fallback, we update this with live version
// data from the external if we have it.
String PluginProcessor::pdlua_version = "pdlua 0.12.0 (lua 5.4)";

PluginProcessor::PluginProcessor()
    : AudioProcessor(buildBusesProperties())
    , internalSynth(std::make_unique<InternalSynth>())
    , hostInfoUpdater(this)
{
    // Make sure to use dots for decimal numbers, pd requires that
    std::setlocale(LC_ALL, "C");

    {
        MessageManagerLock const mmLock; // Do we need this? Isn't this already on the messageManager?

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
#if ENABLE_GEM
    logMessage(gem_version);
#endif
    logMessage(heavylib_version);

    // Set up midi buffers
    midiBufferIn.ensureSize(2048);
    midiBufferOut.ensureSize(2048);
    midiBufferInternalSynth.ensureSize(2048);

    atoms_playhead.reserve(3);
    atoms_playhead.resize(1);

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
    setLimiterThreshold(settingsFile->getProperty<int>("limiter_threshold"));
    enableInternalSynth = settingsFile->getProperty<int>("internal_synth");

    auto currentThemeTree = settingsFile->getCurrentTheme();

    // ag: This needs to be done *after* the library data has been unpacked on
    // first launch.
    initialisePd(pdlua_version);
    logMessage(pdlua_version);

    updateSearchPaths();

    objectLibrary = std::make_unique<pd::Library>(this);

    setLatencySamples(pd::Instance::getBlockSize());
    settingsFile->startChangeListener();

    sendMessagesFromQueue();
}

PluginProcessor::~PluginProcessor()
{
    // Deleting the pd instance in ~PdInstance() will also free all the Pd patches
    patches.clear();
}

void PluginProcessor::flushMessageQueue()
{
    setThis();
    messageDispatcher->dequeueMessages();
}

void PluginProcessor::initialiseFilesystem()
{
    auto const& homeDir = ProjectInfo::appDataDir;
    auto const& versionDataDir = ProjectInfo::versionDataDir;
    auto deken = homeDir.getChildFile("Externals");
    auto patches = homeDir.getChildFile("Patches");

#if JUCE_IOS
    // TODO: remove this later. This is for iOS version transition
    auto oldDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");
    if(oldDir.isDirectory() && oldDir.getChildFile("Abstractions").isDirectory()) {
        oldDir.deleteRecursively();
    }
#elif !JUCE_WINDOWS
    if (!homeDir.exists())
        homeDir.createDirectory();
#endif

    auto initMutex = homeDir.getChildFile(".initialising");

    // If this is true, another instance of plugdata is already initialising
    // We wait a maximum of 5 seconds before we continue initialising, to prevent problems
    int wait = 0;
    while (initMutex.exists() && wait < 20) {
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 500);
        wait++;
    }

    initMutex.create();

    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!versionDataDir.exists()) {

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

        versionDataDir.getParentDirectory().createDirectory();
        auto tempVersionDataDir = versionDataDir.getParentDirectory().getChildFile("plugdata_version");

        auto file = ZipFile(memstream);
        file.uncompressTo(tempVersionDataDir.getParentDirectory());

        // Create filesystem for this specific version
        tempVersionDataDir.moveFileTo(versionDataDir);

        if (versionDataDir.isDirectory())
            internalSynth->extractSoundfont();
    }
    if (!deken.exists()) {
        deken.createDirectory();
    }
#if !JUCE_IOS
    if (!patches.exists()) {
        patches.createDirectory();
    }
#endif
    
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

    auto oldlocation = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");
    auto backupLocation = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata.old");
    if (oldlocation.isDirectory() && !backupLocation.isDirectory()) {
        // don't bother copying this, it's huge!
        if (oldlocation.getChildFile("Toolchain").isDirectory())
            oldlocation.getChildFile("Toolchain").deleteRecursively();

        oldlocation.copyDirectoryTo(backupLocation);
        oldlocation.deleteRecursively();
    }

    auto shortcut = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata.LNK");
    ProjectInfo::appDataDir.createShortcut("plugdata", shortcut);
#elif JUCE_IOS
    versionDataDir.getChildFile("Abstractions").createSymbolicLink(homeDir.getChildFile("Abstractions"), true);
    versionDataDir.getChildFile("Documentation").createSymbolicLink(homeDir.getChildFile("Documentation"), true);
    versionDataDir.getChildFile("Extra").createSymbolicLink(homeDir.getChildFile("Extra"), true);

    auto docsPatchesDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("Patches");
    docsPatchesDir.createDirectory();
    if(!patches.isSymbolicLink()) {
        patches.deleteRecursively();
    }
    else {
        patches.deleteFile();
    }
    docsPatchesDir.createSymbolicLink(patches, true);
#else
    versionDataDir.getChildFile("Abstractions").createSymbolicLink(homeDir.getChildFile("Abstractions"), true);
    versionDataDir.getChildFile("Documentation").createSymbolicLink(homeDir.getChildFile("Documentation"), true);
    versionDataDir.getChildFile("Extra").createSymbolicLink(homeDir.getChildFile("Extra"), true);
#endif
    
    initMutex.deleteFile();
}

void PluginProcessor::updateSearchPaths()
{
    // Reload pd search paths from settings
    auto pathTree = settingsFile->getPathsTree();

    setThis();

    lockAudioThread();
    
    libpd_clear_search_path();

    auto paths = pd::Library::defaultPaths;

    for (auto child : pathTree) {
        auto path = child.getProperty("Path").toString().replace("\\", "/");
        paths.addIfNotAlreadyThere(path);
    }

    for (auto const& path : paths) {
        libpd_add_to_search_path(path.getFullPathName().toRawUTF8());
    }

    for (auto const& path : DekenInterface::getExternalPaths()) {
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

String const PluginProcessor::getName() const
{
    return ProjectInfo::projectName;
}

bool PluginProcessor::acceptsMidi() const
{
#if JUCE_IOS
    return !ProjectInfo::isFx;
#endif
    return true;
}

bool PluginProcessor::producesMidi() const
{
#if JUCE_IOS
    return ProjectInfo::isStandalone;
#endif

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

String const PluginProcessor::getProgramName(int index)
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

    settingsFile->setProperty("oversampling", var(amount));

    oversampling = amount;
    auto blockSize = AudioProcessor::getBlockSize();
    auto sampleRate = AudioProcessor::getSampleRate();

    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PluginProcessor::setLimiterThreshold(int amount)
{
    auto threshold = (std::vector<float> { -12, -6, 0, 3 })[amount];
    limiter.setThreshold(threshold);

    settingsFile->setProperty("limiter_threshold", var(amount));
}

void PluginProcessor::setProtectedMode(bool enabled)
{
    protectedMode = enabled;
}

void PluginProcessor::numChannelsChanged()
{
    auto blockSize = AudioProcessor::getBlockSize();
    auto sampleRate = AudioProcessor::getSampleRate();

    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (approximatelyEqual(sampleRate, 0.0))
        return;

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

    if (variableBlockSize) {
        inputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(pdBlockSize, samplesPerBlock) * 3);
        outputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(pdBlockSize, samplesPerBlock) * 3);
        outputFifo->writeSilence(Instance::getBlockSize());
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
#if JUCE_IOS
    return (layouts.getMainOutputChannels() <= 2) && (layouts.getMainInputChannels() <= 2);
#endif

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

void PluginProcessor::settingsFileReloaded()
{
    auto newTheme = settingsFile->getProperty<String>("theme");
    if (PlugDataLook::currentTheme != newTheme) {
        setTheme(newTheme);
    }

    updateSearchPaths();
    if (objectLibrary)
        objectLibrary->updateLibrary();
}

void PluginProcessor::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer& midiBuffer)
{
    bypassBuffer.makeCopyOf(buffer);
    
    // It's better to keep sending blocks into Pd, so messaging can still work and there are no gaps in the users' audio stream
    processBlock(bypassBuffer, midiBuffer);
    
    for (int ch = 0; ch < getTotalNumOutputChannels(); ch++)
        buffer.clear (ch, 0, buffer.getNumSamples());
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

    if (variableBlockSize) {
        processVariable(blockOut, midiMessages);
    } else {
        processConstant(blockOut, midiMessages);
    }

    auto hasMidiOutEvents = hasRealEvents(midiMessages);

    if (oversampling > 0) {
        oversampler->processSamplesDown(targetBlock);
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

        // Take out inf and NaN values
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

void PluginProcessor::updatePatchUndoRedoState()
{
    if (isSuspended()) {
        for (auto& patch : patches) {
            patch->updateUndoRedoState();
        }
        return;
    }

    enqueueFunctionAsync([this]() {
        for (auto& patch : patches) {
            patch->updateUndoRedoState();
        }
    });
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

    for (int block = 0; block < numBlocks; block++) {
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                audioVectorIn.data() + (ch * blockSize),
                buffer.getChannelPointer(ch) + audioAdvancement,
                blockSize);
        }

        setThis();

        midiBufferIn.clear();
        midiBufferIn.addEvents(midiMessages, audioAdvancement, blockSize, 0);
        sendMidiBuffer();

        // Process audio
        performDSP(audioVectorIn.data(), audioVectorOut.data());

        sendMessagesFromQueue();

        if (connectionListener && plugdata_debugging_enabled())
            connectionListener->updateSignalData();

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                buffer.getChannelPointer(ch) + audioAdvancement,
                audioVectorOut.data() + (ch * blockSize),
                blockSize);
        }

        audioAdvancement += blockSize;
    }

    midiMessages.clear();
    midiMessages.addEvents(midiBufferOut, 0, buffer.getNumSamples(), 0);
}

void PluginProcessor::processVariable(dsp::AudioBlock<float> buffer, MidiBuffer& midiMessages)
{
    auto const pdBlockSize = Instance::getBlockSize();
    auto const numChannels = buffer.getNumChannels();

    inputFifo->writeAudioAndMidi(buffer, midiMessages);
    midiMessages.clear();

    audioAdvancement = 0; // Always has to be 0 if we use the AudioMidiFifo!

    while (inputFifo->getNumSamplesAvailable() >= pdBlockSize) {
        midiBufferIn.clear();
        inputFifo->readAudioAndMidi(audioBufferIn, midiBufferIn);

        for (int channel = 0; channel < audioBufferIn.getNumChannels(); channel++) {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                audioVectorIn.data() + (channel * pdBlockSize),
                audioBufferIn.getReadPointer(channel),
                pdBlockSize);
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

        if (connectionListener && plugdata_debugging_enabled())
            connectionListener->updateSignalData();

        for (int channel = 0; channel < numChannels; channel++) {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                audioBufferOut.getWritePointer(channel),
                audioVectorOut.data() + (channel * pdBlockSize),
                pdBlockSize);
        }

        outputFifo->writeAudioAndMidi(audioBufferOut, midiBufferOut);
    }

    outputFifo->readAudioAndMidi(buffer, midiMessages);
}

void PluginProcessor::sendPlayhead()
{
    AudioPlayHead* playhead = getPlayHead();

    if (!playhead)
        return;

    auto infos = playhead->getPosition();
    
    lockAudioThread();
    setThis();
    if (infos.hasValue()) {
        atoms_playhead[0] = static_cast<float>(infos->getIsPlaying());
        sendMessage("_playhead", "playing", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsRecording());
        sendMessage("_playhead", "recording", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsLooping());

        auto loopPoints = infos->getLoopPoints();
        if (loopPoints.hasValue()) {
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqStart));
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqEnd));
        } else {
            atoms_playhead.emplace_back(0.0f);
            atoms_playhead.emplace_back(0.0f);
        }
        sendMessage("_playhead", "looping", atoms_playhead);

        if (infos->getEditOriginTime().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getEditOriginTime());
            sendMessage("_playhead", "edittime", atoms_playhead);
        }

        if (infos->getFrameRate().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getFrameRate()->getEffectiveRate());
            sendMessage("_playhead", "framerate", atoms_playhead);
        }

        if (infos->getBpm().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getBpm());
            sendMessage("_playhead", "bpm", atoms_playhead);
        }

        if (infos->getPpqPositionOfLastBarStart().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getPpqPositionOfLastBarStart());
            sendMessage("_playhead", "lastbar", atoms_playhead);
        }

        if (infos->getTimeSignature().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getTimeSignature()->numerator);
            atoms_playhead.emplace_back(static_cast<float>(infos->getTimeSignature()->denominator));
            sendMessage("_playhead", "timesig", atoms_playhead);
        }
        
        auto ppq = infos->getPpqPosition();
        auto samplesTime = infos->getTimeInSamples();
        auto secondsTime = infos->getTimeInSeconds();
        if (ppq.hasValue() || samplesTime.hasValue() || secondsTime.hasValue()) {
            atoms_playhead.resize(3);
            atoms_playhead[0] = ppq.hasValue() ? static_cast<float>(*ppq) : 0.0f;
            atoms_playhead[1] = samplesTime.hasValue() ? static_cast<float>(*samplesTime) : 0.0f;
            atoms_playhead[2] = secondsTime.hasValue() ? static_cast<float>(*secondsTime) : 0.0f;
            sendMessage("_playhead", "position", atoms_playhead);
        }
        atoms_playhead.resize(1);
    }
    unlockAudioThread();
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

    editor->resized();
    return editor;
}

void PluginProcessor::getStateInformation(MemoryBlock& destData)
{
    setThis();

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

    ostream.writeInt(getLatencySamples() - Instance::getBlockSize());
    ostream.writeInt(oversampling);
    ostream.writeFloat(getValue<float>(tailLength));

    auto xml = XmlElement("plugdata_save");
    xml.setAttribute("Version", PLUGDATA_VERSION);

    // In the future, we're gonna load everything from xml, to make it easier to add new properties
    // By putting this here, we can prepare for making this change without breaking existing DAW saves
    xml.setAttribute("Oversampling", oversampling);
    xml.setAttribute("Latency", getLatencySamples() - Instance::getBlockSize());
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
    if (extraData) {
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
    if (extraDataStored) {
        xml.removeChildElement(extraData.get(), false);
    }
}

void PluginProcessor::setStateInformation(void const* data, int sizeInBytes)
{
    if (sizeInBytes == 0)
        return;

    MemoryInputStream istream(data, sizeInBytes, false);

    lockAudioThread();

    setThis();
    
    patches.clear();

    std::vector<pd::WeakReference> openedPatches;
    // Close all patches
    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
        openedPatches.push_back(pd::WeakReference(cnv, this));
    }
    for(auto patch : openedPatches)
    {
        if(auto cnv = patch.get<t_glist*>()) {
            libpd_closefile(cnv.get());
        }
    }
    
    int numPatches = istream.readInt();

    Array<std::pair<String, File>> patches;

    for (int i = 0; i < numPatches; i++) {
        auto state = istream.readString();
        auto path = istream.readString();

        auto presetDir = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Presets");
        path = path.replace("${PRESET_DIR}", presetDir.getFullPathName());
        patches.add({ state, File(path) });
    }

    auto legacyLatency = istream.readInt();
    auto legacyOversampling = istream.readInt();
    auto legacyTail = istream.readFloat();

    auto xmlSize = istream.readInt();

    auto* xmlData = new char[xmlSize];
    istream.read(xmlData, xmlSize);

    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));

    auto openPatch = [this](String const& content, File const& location, bool pluginMode = false, int splitIndex = 0) {
        // CHANGED IN v0.9.0:
        // We now prefer loading the patch content over the patch file, if possible
        // This generally makes it work more like the users expect, but before we couldn't get it to load abstractions (this is now fixed)
        if (content.isNotEmpty()) {
            auto locationIsValid = location.getParentDirectory().exists() && location.getFullPathName().isNotEmpty();
            // Force pd to use this path for the next opened patch
            // This makes sure the patch can find abstractions/resources, even though it's loading a patch from state
            if(locationIsValid) {
                glob_forcefilename(generateSymbol(location.getFileName().toRawUTF8()), generateSymbol(location.getParentDirectory().getFullPathName().replaceCharacter('\\', '/').toRawUTF8()));
            }
            
            auto patchPtr = loadPatch(content);
            patchPtr->splitViewIndex = splitIndex;
            patchPtr->openInPluginMode = pluginMode;
            if (!locationIsValid || location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) {
                patchPtr->setUntitled();
            } else {
                patchPtr->setCurrentFile(URL(location));
                patchPtr->setTitle(location.getFileName());
            }
        } else {
            auto patchPtr = loadPatch(URL(location));
            patchPtr->splitViewIndex = splitIndex;
            patchPtr->openInPluginMode = pluginMode;
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
            setLatencySamples(legacyLatency + Instance::getBlockSize());
            setOversampling(legacyOversampling);
            tailLength = legacyTail;
        } else {
            setOversampling(xmlState->getDoubleAttribute("Oversampling"));
            setLatencySamples(xmlState->getDoubleAttribute("Latency") + Instance::getBlockSize());
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
            if (auto* editor = getActiveEditor()) {
                MessageManager::callAsync([editor = Component::SafePointer(editor), windowWidth, windowHeight]() {
                    if (!editor)
                        return;
#if !JUCE_IOS
                    editor->setSize(windowWidth, windowHeight);
#endif
                });
            }
        }

        // Retrieve additional extra-data from DAW
        parseDataBuffer(*xmlState);
    }

    unlockAudioThread();
    
    delete[] xmlData;

    
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        editor->getTabComponent().triggerAsyncUpdate();
        editor->sidebar->updateAutomationParameters();  // After loading a state, we need to update all the parameters
    }

    // Let host know our parameter layout (likely) changed
    hostInfoUpdater.update();
}

pd::Patch::Ptr PluginProcessor::loadPatch(URL const& patchURL)
{
    auto patchFile = patchURL.getLocalFile();

    lockAudioThread();

#if JUCE_IOS
    auto tempFile = File::createTempFile(".pd");
    auto patchContent = patchFile.loadFileAsString();

    auto inputStream = patchURL.createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress));
    tempFile.appendText(inputStream->readEntireStreamAsString());

    auto dirname = patchFile.getParentDirectory().getFullPathName().replace("\\", "/");
    auto filename = patchFile.getFileName();

    if(!glob_hasforcedfilename()) {
        glob_forcefilename(generateSymbol(filename), generateSymbol(dirname));
    }
    auto newPatch = openPatch(tempFile);
    if (newPatch) {
        if (auto patch = newPatch->getPointer()) {
            newPatch->setTitle(filename);
            newPatch->setCurrentFile(patchURL);
        }
    }
#else
    auto newPatch = openPatch(patchFile);
#endif
    unlockAudioThread();

    if (!newPatch->getPointer()) {
        logError("Couldn't open patch");
        return nullptr;
    }

    patches.add(newPatch);
    auto* patch = patches.getLast().get();
    patch->setCurrentFile(URL(patchFile));

    return patch;
}

pd::Patch::Ptr PluginProcessor::loadPatch(String patchText)
{
    if (patchText.isEmpty())
        patchText = pd::Instance::defaultPatch;

    auto patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patchText);

    auto patch = loadPatch(URL(patchFile));

    // Set to unknown file when loading temp patch
    patch->setCurrentFile(URL("file://"));

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

    updateAllEditorsLNF();

    // Only update iolet geometry if we need to
    // This is based on if the previous or current differ
    auto previousIoletGeom = oldThemeTree.getProperty("iolet_spacing_edge");
    auto currentIoletGeom = themeTree.getProperty("iolet_spacing_edge");
    // if both previous and current have iolet property, propertyState = 0;
    // if one does, propertyState =  1;
    // if previous and current both don't have iolet spacing property, propertyState = 2
    int propertyState = previousIoletGeom.isVoid() + currentIoletGeom.isVoid();
    if ((propertyState == 1) || (propertyState == 0 ? static_cast<int>(previousIoletGeom) != static_cast<int>(currentIoletGeom) : 0)) {
        updateIoletGeometryForAllObjects();
    }
}

void PluginProcessor::updateAllEditorsLNF()
{
    for (auto& editor : getEditors())
        editor->sendLookAndFeelChange();
}

void PluginProcessor::updateIoletGeometryForAllObjects()
{
    // update all object's iolet position
    for (auto& editor : getEditors()){
        for (auto& cnv : editor->getCanvases()){
            for (auto& obj : cnv->objects){
                obj->updateIoletGeometry();
            }
        }
    }
    // update all connections to make sure they attach to the correct iolet positions
    for (auto& editor : getEditors()){
        for (auto& cnv : editor->getCanvases()){
            for (auto& con : cnv->connections){
                con->forceUpdate();
            }
        }
    }
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
            auto editors = getEditors();

            auto patch = URL(File(directory).getChildFile(filename));
            
            if (!editors.isEmpty()) {
                editors[0]->getTabComponent().openPatch(patch);
            }
            else {
                loadPatch(patch);
            }
        }
        break;
    }
    case hash("menunew"): {
        if (list.size() >= 2) {
            auto filename = list[0].toString();
            auto directory = list[1].toString();
            auto editors = getEditors();

            auto patchPtr = loadPatch(defaultPatch);
            patchPtr->setCurrentFile(File(directory).getChildFile(filename).getFullPathName());
            patchPtr->setTitle(filename);
            if (!editors.isEmpty())
                editors[0]->getTabComponent().triggerAsyncUpdate();
        }
        break;
    }
    case hash("dsp"): {
        bool dsp = list[0].getFloat();
        for (auto* editor : getEditors()) {
            editor->statusbar->showDSPState(dsp);
        }
        break;
    }
    case hash("pluginmode"): {
        // TODO: it would be nicer if we could specifically target the correct editor here, instead of picking the first one and praying
        auto editors = getEditors();
        if(!patches.isEmpty()) {
            if(list.size())
            {
                auto pluginModeThemeOrPath = list[0].toString();
                if(pluginModeThemeOrPath.endsWith(".plugdatatheme"))
                {
                    auto themeFile = patches[0]->getPatchFile().getParentDirectory().getChildFile(pluginModeThemeOrPath);
                    if(themeFile.existsAsFile())
                    {
                        pluginModeTheme = ValueTree::fromXml(themeFile.loadFileAsString());
                    }
                }
                else {
                    auto themesTree = SettingsFile::getInstance()->getValueTree().getChildWithName("ColourThemes");
                    auto theme = themesTree.getChildWithProperty("theme", pluginModeThemeOrPath);
                    if(theme.isValid()) {
                        pluginModeTheme = theme;
                    }
                }
            }
            
            if (!editors.isEmpty()) {
                auto* editor = editors[0];
                if (auto* cnv = editor->getCurrentCanvas()) {
                    editor->getTabComponent().openInPluginMode(cnv->patch);
                }
            } else {
                patches[0]->openInPluginMode = true;
            }
        }
        break;
    }
    case hash("quit"):
    case hash("verifyquit"): {
        if (ProjectInfo::isStandalone) {
            bool askToSave = hash(selector) == hash("verifyquit");
            for (auto* editor : getEditors()) {
                editor->quit(askToSave);
            }
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
                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("clear"), 0, nullptr);
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
                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), generateSymbol("end"), 0, nullptr);
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

// set custom plugin latency
void PluginProcessor::performLatencyCompensationChange(float value)
{
    if (!approximatelyEqual<int>(customLatencySamples, value)) {
        customLatencySamples = value;

        for (auto& editor : getEditors()) {
            editor->statusbar->setLatencyDisplay(customLatencySamples);
        }

        setLatencySamples(customLatencySamples + Instance::getBlockSize());
    }
}

void PluginProcessor::sendParameterInfoChangeMessage()
{
    hostInfoUpdater.triggerAsyncUpdate();
}

void PluginProcessor::setParameterRange(String const& name, float min, float max)
{
    for (auto* p : getParameters()) {
        auto* param = dynamic_cast<PlugDataParameter*>(p);
        if (param->isEnabled() && param->getTitle() == name) {
            max = std::max(max, min + 0.000001f);
            param->setRange(min, max);
            break;
        }
    }

    for (auto* editor : getEditors()) {
        editor->sidebar->updateAutomationParameters();
    }
}

void PluginProcessor::setParameterMode(String const& name, int mode)
{
    for (auto* p : getParameters()) {
        auto* param = dynamic_cast<PlugDataParameter*>(p);
        if (param->isEnabled() && param->getTitle() == name) {
            param->setMode(static_cast<PlugDataParameter::Mode>(std::clamp<int>(mode, 1, 4)));
            break;
        }
    }

    for (auto* editor : getEditors()) {
        editor->sidebar->updateAutomationParameters();
    }
}

void PluginProcessor::enableAudioParameter(String const& name)
{
    int numEnabled = 0;
    for (auto* p : getParameters()) {
        auto* param = dynamic_cast<PlugDataParameter*>(p);
        numEnabled += param->isEnabled();
        if (param->isEnabled() && param->getTitle() == name) {
            return;
        }
    }

    for (auto* p : getParameters()) {
        auto* param = dynamic_cast<PlugDataParameter*>(p);
        if (!param->isEnabled()) {
            param->setEnabled(true);
            param->setName(name);
            param->setIndex(numEnabled + 1);
            param->notifyDAW();
            break;
        }
    }

    for (auto* editor : getEditors()) {
        editor->sidebar->updateAutomationParameters();
    }
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

            // Send new value to DAW
            pldParam->setUnscaledValueNotifyingHost(value);

            if (ProjectInfo::isStandalone) {
                for (auto* editor : getEditors()) {
                    editor->sidebar->updateAutomationParameterValue(pldParam);
                }
            }
        }
    }
}

void PluginProcessor::fillDataBuffer(std::vector<pd::Atom> const& vec)
{
    if (!vec[0].isSymbol()) {
        logMessage("databuffer accepts only lists beginning with a Symbol atom");
        return;
    }
    String child_name = String(vec[0].toString());

    if (extraData) {

        int const numChildren = extraData->getNumChildElements();
        if (numChildren > 0) {
            // Searching if a previously created child element exists, with same name as vec[0]. If true, delete it.
            XmlElement* list = extraData->getChildByName(child_name);
            if (list)
                extraData->removeChildElement(list, true);
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
    if (ProjectInfo::isStandalone) {
        for (auto* editor : openedEditors) {
            editors.add(editor);
        }
    } else {
        if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
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

        for (auto* canvas : editor->getCanvases()) {
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
        editor->getTabComponent().repaint();
    }
}

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
