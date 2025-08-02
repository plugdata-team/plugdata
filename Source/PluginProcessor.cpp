/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <clocale>
#include <memory>
#include <fstream>

#include <xz/src/liblzma/api/lzma.h>
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
#include "Utility/MidiDeviceManager.h"
#include "Utility/Autosave.h"
#include "Standalone/InternalSynth.h"

#include "Utility/Presets.h"
#include "Canvas.h"
#include "PluginMode.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Object.h"
#include "Statusbar.h"

#include "Dialogs/Dialogs.h"
#include "Components/ConnectionMessageDisplay.h"

#include "Sidebar/Sidebar.h"

#include "Object.h"

extern "C" {
#include <pd-cyclone/shared/common/file.h>
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
#if PERFETTO
    MelatoninPerfetto::get().beginSession();
#endif

    // Make sure to use dots for decimal numbers, pd requires that
    std::setlocale(LC_NUMERIC, "C");

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
    auto const gitHash = String(PLUGDATA_GIT_HASH);
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
    midiBufferInternalSynth.ensureSize(2048);

    atoms_playhead.reserve(3);
    atoms_playhead.resize(1);

    autosave = std::make_unique<Autosave>(this);

    auto themeName = settingsFile->getProperty<String>("theme");

    // Make sure theme exists
    if (!settingsFile->getTheme(themeName).isValid()) {

        settingsFile->setProperty("theme", PlugDataLook::selectedThemes[0]);
        themeName = PlugDataLook::selectedThemes[0];
    }

    setTheme(themeName, true);
    settingsFile->saveSettings();

    oversampling = std::clamp(settingsFile->getProperty<int>("oversampling"), 0, 3);

    setEnableLimiter(settingsFile->getProperty<int>("protected"));
    setLimiterThreshold(settingsFile->getProperty<int>("limiter_threshold"));

    if(ProjectInfo::isStandalone) {
        midiDeviceManager.setInternalSynthPort(settingsFile->getProperty<int>("internal_synth"));
    }
    else {
        midiDeviceManager.setInternalSynthPort(0);
    }
    
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
    startDSP();
}

PluginProcessor::~PluginProcessor()
{
    // Deleting the pd instance in ~PdInstance() will also free all the Pd patches
    patches.clear();

#if PERFETTO
    MelatoninPerfetto::get().endSession();
#endif
}

void PluginProcessor::flushMessageQueue()
{
    setThis();
    messageDispatcher->dequeueMessages();
}

void PluginProcessor::doubleFlushMessageQueue()
{
    setThis();
    lockAudioThread();
    messageDispatcher->dequeueMessages();
    messageDispatcher->dequeueMessages();
    unlockAudioThread();
}

void PluginProcessor::initialiseFilesystem()
{
    auto const& homeDir = ProjectInfo::appDataDir;
    auto const& versionDataDir = ProjectInfo::versionDataDir;
    auto dekenDir = homeDir.getChildFile("Externals");
    auto patchesDir = homeDir.getChildFile("Patches");

#if JUCE_IOS
    // TODO: remove this later. This is for iOS version transition
    auto oldDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");
    if (oldDir.isDirectory() && oldDir.getChildFile("Abstractions").isDirectory()) {
        oldDir.deleteRecursively();
    }
#elif !JUCE_WINDOWS
    if (!homeDir.exists())
        homeDir.createDirectory();
#endif

    auto const initMutex = homeDir.getChildFile(".initialising");

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
        HeapArray<char> allData;
        int i = 0;
        while (true) {
            int size;
            auto* resource = BinaryData::getNamedResource((String("Filesystem_") + String(i)).toRawUTF8(), size);

            if (!resource) {
                break;
            }

            allData.insert(allData.end(), resource, resource + size);
            i++;
        }
        
        versionDataDir.getParentDirectory().createDirectory();
        auto const tempVersionDataDir = versionDataDir.getParentDirectory().getChildFile("plugdata_version");

        // Decompress .xz data using liblzma
        HeapArray<uint8_t> decompressedData;
        decompressedData.reserve(40 * 1024 * 1024);
        {
            lzma_stream strm = LZMA_STREAM_INIT;
            if (lzma_stream_decoder(&strm, UINT64_MAX, 0) != LZMA_OK)
                return; // TODO: handle failure!
            
            strm.next_in = reinterpret_cast<const uint8_t*>(allData.data());
            strm.avail_in = allData.size();

            uint8_t buffer[8192];
            lzma_ret ret;

            do {
                strm.next_out = buffer;
                strm.avail_out = sizeof(buffer);

                ret = lzma_code(&strm, LZMA_FINISH);
                size_t written = sizeof(buffer) - strm.avail_out;
                decompressedData.insert(decompressedData.end(), buffer, buffer + written);

                if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
                    lzma_end(&strm);
                    return; // TODO: handle failure!
                }
            } while (ret != LZMA_STREAM_END);

            lzma_end(&strm);
        }

        // Parse and extract .tar manually
        auto extractTar = [](const uint8_t* data, size_t size, const File& destRoot) -> bool {
            size_t offset = 0;
            while (offset + 512 <= size) {
                const uint8_t* header = data + offset;
                if (header[0] == '\0') break; // End of archive

                // Get file name
                std::string name(reinterpret_cast<const char*>(header), 100);
                if (name.empty()) break;

                // Get file size (octal)
                size_t fileSize = std::strtoull(reinterpret_cast<const char*>(header + 124), nullptr, 8);
                File outFile = destRoot.getChildFile(String(name));

                // Determine type
                char typeFlag = header[156];
                if (typeFlag == '5') {
                    outFile.createDirectory();
                } else if (typeFlag == '0' || typeFlag == '\0') {
                    outFile.getParentDirectory().createDirectory();
                    std::ofstream out(outFile.getFullPathName().toRawUTF8(), std::ios::binary);
                    size_t fileOffset = offset + 512;
                    out.write(reinterpret_cast<const char*>(data + fileOffset), fileSize);
                    out.close();
                }

                size_t totalEntrySize = 512 + ((fileSize + 511) & ~511); // pad to next 512
                offset += totalEntrySize;
            }

            return true;
        };

        versionDataDir.getParentDirectory().createDirectory();
        if (!extractTar(decompressedData.data(), decompressedData.size(), tempVersionDataDir.getParentDirectory())) {
            DBG("Failed to extract tar archive");
            return;
        }
        // Create filesystem for this specific version
        tempVersionDataDir.moveFileTo(versionDataDir);
    }
    if (!dekenDir.exists()) {
        dekenDir.createDirectory();
    }
#if !JUCE_IOS
    if (!patchesDir.exists()) {
        patchesDir.createDirectory();
    }
#endif

    auto const testTonePatch = homeDir.getChildFile("testtone.pd");
    auto const cpuTestPatch = homeDir.getChildFile("load-meter.pd");

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
    auto dekenPath = dekenDir.getFullPathName();
    auto patchesPath = patchesDir.getFullPathName();

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
    if (!patchesDir.isSymbolicLink()) {
        patchesDir.deleteRecursively();
    } else {
        patchesDir.deleteFile();
    }
    docsPatchesDir.createSymbolicLink(patchesDir, true);
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
    auto const pathTree = settingsFile->getPathsTree();

    setThis();

    lockAudioThread();

    libpd_clear_search_path();

    auto paths = SmallArray<File, 12>(pd::Library::defaultPaths.begin(), pd::Library::defaultPaths.end());

    for (auto child : pathTree) {
        auto path = child.getProperty("Path").toString().replace("\\", "/");
        paths.add_unique(path);
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

void PluginProcessor::setCurrentProgram(int const index)
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

String const PluginProcessor::getProgramName(int const index)
{
    if (isPositiveAndBelow(index, Presets::presets.size())) {
        return Presets::presets[index].first;
    }

    return "Init preset";
}

void PluginProcessor::changeProgramName(int index, String const& newName)
{
}

void PluginProcessor::setOversampling(int const amount)
{
    settingsFile->setProperty("oversampling", var(amount));

    if (oversampling == amount)
        return;

    oversampling = amount;
    auto const blockSize = AudioProcessor::getBlockSize();
    auto const sampleRate = AudioProcessor::getSampleRate();

    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PluginProcessor::setLimiterThreshold(int const amount)
{
    auto const threshold = StackArray<float, 4> { -12.f, -6.f, 0.f, 3.f }[amount];
    limiter.setThreshold(threshold);

    settingsFile->setProperty("limiter_threshold", var(amount));
}

void PluginProcessor::setEnableLimiter(bool const enabled)
{
    enableLimiter = enabled;
}

void PluginProcessor::numChannelsChanged()
{
    auto const blockSize = AudioProcessor::getBlockSize();
    auto const sampleRate = AudioProcessor::getSampleRate();

    suspendProcessing(true);
    prepareToPlay(sampleRate, blockSize);
    suspendProcessing(false);
}

void PluginProcessor::prepareToPlay(double const sampleRate, int const samplesPerBlock)
{
    if (approximatelyEqual(sampleRate, 0.0))
        return;

    float const oversampleFactor = 1 << oversampling;
    auto maxChannels = std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());

    prepareDSP(getTotalNumInputChannels(), getTotalNumOutputChannels(), sampleRate * oversampleFactor, samplesPerBlock * oversampleFactor);

    oversampler = std::make_unique<dsp::Oversampling<float>>(std::max(1, maxChannels), oversampling, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);

    oversampler->initProcessing(samplesPerBlock);

    auto const internalSynthPort = midiDeviceManager.getInternalSynthPort();
    if (internalSynthPort >= 0 && ProjectInfo::isStandalone) {
        internalSynth->prepare(sampleRate, samplesPerBlock, maxChannels);
    }

    audioAdvancement = 0;
    auto const pdBlockSize = static_cast<size_t>(Instance::getBlockSize());
    audioBufferIn.setSize(maxChannels, pdBlockSize);
    audioBufferOut.setSize(maxChannels, pdBlockSize);

    audioVectorIn.resize(maxChannels * pdBlockSize, 0.0f);
    audioVectorOut.resize(maxChannels * pdBlockSize, 0.0f);

    // If the block size is a multiple of 64 and we are not a plugin, we can optimise the process loop
    // Audio plugins can choose to send in a smaller block size when automation is happening
    variableBlockSize = !ProjectInfo::isStandalone || samplesPerBlock < pdBlockSize || samplesPerBlock % pdBlockSize != 0;

    if (variableBlockSize) {
        inputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(pdBlockSize, samplesPerBlock * oversampleFactor) * 3);
        outputFifo = std::make_unique<AudioMidiFifo>(maxChannels, std::max<int>(pdBlockSize, samplesPerBlock * oversampleFactor) * 3);
        outputFifo->writeSilence(Instance::getBlockSize());
    }

    midiByteIndex = 0;
    midiByteBuffer[0] = 0;
    midiByteBuffer[1] = 0;
    midiByteBuffer[2] = 0;

    midiBufferInternalSynth.ensureSize(2048);

    midiDeviceManager.prepareToPlay(sampleRate * oversampleFactor);

    cpuLoadMeasurer.reset(sampleRate, samplesPerBlock);

    statusbarSource->setSampleRate(sampleRate);
    statusbarSource->setBufferSize(samplesPerBlock);
    statusbarSource->prepareToPlay(getTotalNumOutputChannels());

    limiter.prepare({ sampleRate, static_cast<uint32>(samplesPerBlock), std::max(1u, static_cast<uint32>(maxChannels)) });

    smoothedGain.reset(AudioProcessor::getSampleRate(), 0.02);
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
        int const nchb = layouts.getNumChannels(false, bus);

        if (layouts.outputBuses[bus].isDisabled())
            continue;

        if (nchb == 0)
            return false;

        noutch += nchb;
    }

    for (int bus = 0; bus < layouts.inputBuses.size(); bus++) {
        int const nchb = layouts.getNumChannels(true, bus);

        if (layouts.inputBuses[bus].isDisabled())
            continue;

        if (nchb == 0)
            return false;

        ninch += nchb;
    }

    return ninch <= 32 && noutch <= 32;
}

void PluginProcessor::settingsFileReloaded()
{
    auto const newTheme = settingsFile->getProperty<String>("theme");
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
        buffer.clear(ch, 0, buffer.getNumSamples());
}

void PluginProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiBuffer)
{
    ScopedNoDenormals noDenormals;
    AudioProcessLoadMeasurer::ScopedTimer cpuTimer(cpuLoadMeasurer, buffer.getNumSamples());

    auto const totalNumInputChannels = getTotalNumInputChannels();
    auto const totalNumOutputChannels = getTotalNumOutputChannels();
    auto midiInputHistory = midiBuffer;

    setThis();
    sendPlayhead();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    auto targetBlock = dsp::AudioBlock<float>(buffer);
    auto const blockOut = oversampling > 0 ? oversampler->processSamplesUp(targetBlock) : targetBlock;

    if (variableBlockSize) {
        processVariable(blockOut, midiBuffer);
    } else {
        midiBuffer.clear();
        processConstant(blockOut);
    }

    if (oversampling > 0) {
        oversampler->processSamplesDown(targetBlock);
    }

    auto const targetGain = volume->load();
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

    midiDeviceManager.sendAndCollectMidiOutput(midiBuffer);
    auto const internalSynthPort = midiDeviceManager.getInternalSynthPort();

    // If the internalSynth is enabled and loaded, let it process the midi
    if (internalSynthPort >= 0 && internalSynth->isReady()) {
        midiBufferInternalSynth.clear();
        midiDeviceManager.dequeueMidiOutput(internalSynthPort, midiBufferInternalSynth, blockOut.getNumSamples());
        internalSynth->process(buffer, midiBufferInternalSynth);
    } else if (internalSynthPort < 0 && internalSynth->isReady()) {
        internalSynth->unprepare();
    } else if (internalSynthPort >= 0 && !internalSynth->isReady()) {
        internalSynth->prepare(getSampleRate(), AudioProcessor::getBlockSize(), std::max(totalNumInputChannels, totalNumOutputChannels));
    }
    midiBufferInternalSynth.clear();

    midiInputHistory.addEvents(midiDeviceManager.getInputHistory(), 0, buffer.getNumSamples(), 0);
    statusbarSource->process(midiInputHistory, midiDeviceManager.getOutputHistory(), totalNumOutputChannels);
    midiDeviceManager.clearMidiOutputBuffers(blockOut.getNumSamples());

    statusbarSource->setCPUUsage(cpuLoadMeasurer.getLoadAsPercentage());
    statusbarSource->peakBuffer.write(buffer);

    if (enableLimiter && buffer.getNumChannels() > 0) {
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

// only used for standalone, and if blocksize if a multiple of 64
void PluginProcessor::processConstant(dsp::AudioBlock<float> buffer)
{
    int const pdBlockSize = Instance::getBlockSize();
    int const numBlocks = buffer.getNumSamples() / pdBlockSize;

    for (int block = 0; block < numBlocks; block++) {
        if (producesMidi()) {
            midiByteIndex = 0;
            midiByteBuffer[0] = 0;
            midiByteBuffer[1] = 0;
            midiByteBuffer[2] = 0;
        }

        midiDeviceManager.dequeueMidiInput(pdBlockSize, [this](int const port, int const blockSize, MidiBuffer& buffer) {
            sendMidiBuffer(port, buffer);
        });

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                audioVectorIn.data() + ch * pdBlockSize,
                buffer.getChannelPointer(ch) + audioAdvancement,
                pdBlockSize);
        }
        setThis();

        sendParameters();
        sendMessagesFromQueue();

        // Process audio
        performDSP(audioVectorIn.data(), audioVectorOut.data());

        if (connectionListener && plugdata_debugging_enabled())
            connectionListener.load()->updateSignalData();

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                buffer.getChannelPointer(ch) + audioAdvancement,
                audioVectorOut.data() + ch * pdBlockSize,
                pdBlockSize);
        }

        audioAdvancement += pdBlockSize;
    }

    audioAdvancement = 0;
}

void PluginProcessor::processVariable(dsp::AudioBlock<float> buffer, MidiBuffer& midiBuffer)
{
    auto const pdBlockSize = Instance::getBlockSize();
    auto const numChannels = buffer.getNumChannels();

    inputFifo->writeAudioAndMidi(buffer, midiBuffer);

    audioAdvancement = 0; // Always has to be 0 if we use the AudioMidiFifo!

    while (outputFifo->getNumSamplesAvailable() < buffer.getNumSamples()) {
        if (producesMidi()) {
            midiByteIndex = 0;
            midiByteBuffer[0] = 0;
            midiByteBuffer[1] = 0;
            midiByteBuffer[2] = 0;
        }

        blockMidiBuffer.clear();
        inputFifo->readAudioAndMidi(audioBufferIn, blockMidiBuffer);

        if (!ProjectInfo::isStandalone) {
            sendMidiBuffer(1, blockMidiBuffer);
        }

        midiDeviceManager.dequeueMidiInput(pdBlockSize, [this](int const port, int const blockSize, MidiBuffer& buffer) {
            sendMidiBuffer(port, buffer);
        });

        for (int channel = 0; channel < audioBufferIn.getNumChannels(); channel++) {
            // Copy the channel data into the vector
            juce::FloatVectorOperations::copy(
                audioVectorIn.data() + channel * pdBlockSize,
                audioBufferIn.getReadPointer(channel),
                pdBlockSize);
        }

        setThis();

        sendParameters();
        sendMessagesFromQueue();

        // Process audio
        performDSP(audioVectorIn.data(), audioVectorOut.data());

        if (connectionListener && plugdata_debugging_enabled())
            connectionListener.load()->updateSignalData();

        for (int channel = 0; channel < numChannels; channel++) {
            // Use FloatVectorOperations to copy the vector data into the audioBuffer
            juce::FloatVectorOperations::copy(
                audioBufferOut.getWritePointer(channel),
                audioVectorOut.data() + channel * pdBlockSize,
                pdBlockSize);
        }

        blockMidiBuffer.clear();
        outputFifo->writeAudioAndMidi(audioBufferOut, blockMidiBuffer);
    }

    midiBuffer.clear();
    outputFifo->readAudioAndMidi(buffer, midiBuffer);
}

void PluginProcessor::sendPlayhead()
{
    AudioPlayHead const* playhead = getPlayHead();

    if (!playhead)
        return;

    auto infos = playhead->getPosition();

    lockAudioThread();
    setThis();
    if (infos.hasValue()) {
        atoms_playhead[0] = static_cast<float>(infos->getIsPlaying());
        sendMessage("__playhead", "playing", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsRecording());
        sendMessage("__playhead", "recording", atoms_playhead);

        atoms_playhead[0] = static_cast<float>(infos->getIsLooping());

        auto loopPoints = infos->getLoopPoints();
        if (loopPoints.hasValue()) {
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqStart));
            atoms_playhead.emplace_back(static_cast<float>(loopPoints->ppqEnd));
        } else {
            atoms_playhead.emplace_back(0.0f);
            atoms_playhead.emplace_back(0.0f);
        }
        sendMessage("__playhead", "looping", atoms_playhead);

        if (infos->getEditOriginTime().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getEditOriginTime());
            sendMessage("__playhead", "edittime", atoms_playhead);
        }

        if (infos->getFrameRate().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getFrameRate()->getEffectiveRate());
            sendMessage("__playhead", "framerate", atoms_playhead);
        }

        if (infos->getBpm().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getBpm());
            sendMessage("__playhead", "bpm", atoms_playhead);
        }

        if (infos->getPpqPositionOfLastBarStart().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(*infos->getPpqPositionOfLastBarStart());
            sendMessage("__playhead", "lastbar", atoms_playhead);
        }

        if (infos->getTimeSignature().hasValue()) {
            atoms_playhead.resize(1);
            atoms_playhead[0] = static_cast<float>(infos->getTimeSignature()->numerator);
            atoms_playhead.emplace_back(static_cast<float>(infos->getTimeSignature()->denominator));
            sendMessage("__playhead", "timesig", atoms_playhead);
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

SmallArray<PlugDataParameter*> PluginProcessor::getEnabledParameters()
{
    return enabledParameters;
}

void PluginProcessor::updateEnabledParameters()
{
    ScopedLock lock(audioLock);
    enabledParameters.clear();

    for (auto* param : getParameters()) {
        if (auto* p = dynamic_cast<PlugDataParameter*>(param)) {
            if (p->isEnabled()) {
                enabledParameters.add(p);
            }
        }
    }
}

void PluginProcessor::sendParameters()
{
    ScopedLock lock(audioLock);
    for (auto* param : enabledParameters) {
        if (EXPECT_UNLIKELY(param->wasChanged())) {
            auto title = param->getTitle();
            sendFloat(title.data(), param->getUnscaledValue());
            param->setUnchanged();
        }
    }
}

MidiDeviceManager& PluginProcessor::getMidiDeviceManager()
{
    return midiDeviceManager;
}

void PluginProcessor::sendMidiBuffer(int const device, MidiBuffer& buffer)
{
    if (acceptsMidi()) {
        for (auto event : buffer) {
            auto message = event.getMessage();
            auto const channel = message.getChannel() + (device << 4);

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

    auto patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");

    auto const patchesTree = new XmlElement("Patches");

    // Save path and content for patch
    lockAudioThread();
    for (auto const& patch : patches) {

        auto content = patch->getCanvasContent();
        auto patchFile = patch->getCurrentFile().getFullPathName();
        
        if(patchFile.startsWith(patchesDir.getFullPathName()))
        {
            patchFile = patchFile.replace(patchesDir.getFullPathName(), "${PATCHES_DIR}");
        }
        // Write legacy format
        ostream.writeString(content);
        ostream.writeString(patchFile);

        auto* patchTree = new XmlElement("Patch");
        // Write new format
        patchTree->setAttribute("Content", content);
        patchTree->setAttribute("Location", patchFile);
        patchTree->setAttribute("SplitIndex", patch->splitViewIndex);
        patchTree->setAttribute("PluginMode", patch->openInPluginMode);
        patchTree->setAttribute("PluginModeScale", patch->pluginModeScale);
    
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
    if (auto const* editor = getActiveEditor()) {
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

void PluginProcessor::setStateInformation(void const* data, int const sizeInBytes)
{
    if (sizeInBytes == 0)
        return;

    MemoryInputStream istream(data, sizeInBytes, false);

    lockAudioThread();

    setThis();

    patches.clear();

    SmallArray<pd::WeakReference> openedPatches;
    // Close all patches
    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
        openedPatches.add(pd::WeakReference(cnv, this));
    }
    for (auto patch : openedPatches) {
        if (auto cnv = patch.get<t_glist*>()) {
            libpd_closefile(cnv.get());
        }
    }

    int const numPatches = istream.readInt();

    SmallArray<std::pair<String, File>> newPatches;

    for (int i = 0; i < numPatches; i++) {
        auto state = istream.readString();
        auto path = istream.readString();

        auto presetDir = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Presets");
        path = path.replace("${PRESET_DIR}", presetDir.getFullPathName());
        
        auto patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");
        path = path.replace("${PATCHES_DIR}", patchesDir.getFullPathName());
        newPatches.emplace_back(state, File(path));
    }

    auto const legacyLatency = istream.readInt();
    auto const legacyOversampling = istream.readInt();
    auto const legacyTail = istream.readFloat();

    auto const xmlSize = istream.readInt();

    auto* xmlData = new char[xmlSize];
    istream.read(xmlData, xmlSize);
    
    std::unique_ptr<XmlElement> const xmlState(getXmlFromBinary(xmlData, xmlSize));

    jassert(xmlState);
    
    auto openPatch = [this](String const& content, File const& location, bool const pluginMode = false, int pluginModeScale = 100, int const splitIndex = 0) {
        // CHANGED IN v0.9.0:
        // We now prefer loading the patch content over the patch file, if possible
        if (content.isNotEmpty()) {
            auto const locationIsValid = location.getParentDirectory().exists() && location.getFullPathName().isNotEmpty() && !location.isRoot();
            // Force pd to use this path for the next opened patch
            // This makes sure the patch can find abstractions/resources, even though it's loading a patch from state
            if (locationIsValid) {
                glob_forcefilename(generateSymbol(location.getFileName().toRawUTF8()), generateSymbol(location.getParentDirectory().getFullPathName().replaceCharacter('\\', '/').toRawUTF8()));
            }

            auto const patchPtr = loadPatch(content);
            patchPtr->splitViewIndex = splitIndex;
            patchPtr->openInPluginMode = pluginMode;
            patchPtr->pluginModeScale = pluginModeScale;
            if (!locationIsValid || location.getParentDirectory() == File::getSpecialLocation(File::tempDirectory)) {
                patchPtr->setUntitled();
            } else {
                patchPtr->setCurrentFile(URL(location));
                patchPtr->setTitle(location.getFileName());
            }
        } else {
            auto const patchPtr = loadPatch(URL(location));
            patchPtr->splitViewIndex = splitIndex;
            patchPtr->openInPluginMode = pluginMode;
            patchPtr->pluginModeScale = pluginModeScale;
        }
    };

    if (xmlState) {
        PlugDataParameter::loadStateInformation(*xmlState, getParameters());

        // If xmltree contains new patch format, use that
        if (auto const* patchTree = xmlState->getChildByName("Patches")) {
            for (auto const p : patchTree->getChildWithTagNameIterator("Patch")) {
                auto content = p->getStringAttribute("Content");
                auto location = p->getStringAttribute("Location");
                auto const pluginMode = p->getBoolAttribute("PluginMode");
                auto const pluginModeScale = p->getIntAttribute("PluginModeScale", 100);
                
                int splitIndex = 0;
                if (p->hasAttribute("SplitIndex")) {
                    splitIndex = p->getIntAttribute("SplitIndex");
                }

                auto presetDir = ProjectInfo::versionDataDir.getChildFile("Extra").getChildFile("Presets");
                location = location.replace("${PRESET_DIR}", presetDir.getFullPathName());

                auto patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");
                location = location.replace("${PATCHES_DIR}", patchesDir.getFullPathName());
                
                openPatch(content, location, pluginMode, pluginModeScale, splitIndex);
                
            }
        }
        // Otherwise, load from legacy format
        else {
            for (auto& [content, location] : newPatches) {
                openPatch(content, location);
            }
        }

        updateEnabledParameters();

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
                MessageManager::callAsync([editor = Component::SafePointer(editor), windowWidth, windowHeight] {
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
        editor->sidebar->updateAutomationParameters(); // After loading a state, we need to update all the parameters
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

    if (!glob_hasforcedfilename()) {
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

    if (initialiseIntoPluginmode) {
        newPatch->openInPluginMode = true;
        initialiseIntoPluginmode = false;
    }

    unlockAudioThread();

    if (!newPatch->getPointer()) {
        logError("Couldn't open patch");
        return nullptr;
    }

    patches.add(newPatch);
    auto* patch = patches.back().get();

    patch->setCurrentFile(URL(patchFile));

    return patch;
}

pd::Patch::Ptr PluginProcessor::loadPatch(String patchText)
{
    if (patchText.isEmpty())
        patchText = pd::Instance::defaultPatch;

    auto const patchFile = File::createTempFile(".pd");
    patchFile.replaceWithText(patchText);

    auto patch = loadPatch(URL(patchFile));

    // Set to unknown file when loading temp patch
    patch->setCurrentFile(URL("file://"));

    return patch;
}

void PluginProcessor::setTheme(String themeToUse, bool const force)
{
    auto const oldThemeTree = settingsFile->getTheme(PlugDataLook::currentTheme);
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
    auto const previousIoletGeom = oldThemeTree.getProperty("iolet_spacing_edge");
    auto const currentIoletGeom = themeTree.getProperty("iolet_spacing_edge");
    // if both previous and current have iolet property, propertyState = 0;
    // if one does, propertyState =  1;
    // if previous and current both don't have iolet spacing property, propertyState = 2
    int const propertyState = previousIoletGeom.isVoid() + currentIoletGeom.isVoid();
    if (propertyState == 1 || (propertyState == 0 ? static_cast<int>(previousIoletGeom) != static_cast<int>(currentIoletGeom) : 0)) {
        PluginEditor::updateIoletGeometryForAllObjects(this);
    }

    currentThemeName = themeToUse;
}

void PluginProcessor::updateAllEditorsLNF()
{
    for (auto const& editor : getEditors())
        editor->sendLookAndFeelChange();
}

void PluginProcessor::receiveNoteOn(int const channel, int const pitch, int const velocity)
{
    auto const port = (channel - 1) >> 4;
    auto const deviceChannel = channel - port * 16;

    if (velocity == 0) {
        midiDeviceManager.enqueueMidiOutput(port, MidiMessage::noteOff(deviceChannel, pitch, static_cast<uint8>(0)), audioAdvancement);
    } else {
        midiDeviceManager.enqueueMidiOutput(port, MidiMessage::noteOn(deviceChannel, pitch, static_cast<uint8>(velocity)), audioAdvancement);
    }
}

// Return the patch that belongs to this editor that will be in plugin mode
// At this point the editor is NOT in plugin mode yet
pd::Patch::Ptr PluginProcessor::findPatchInPluginMode(int const editorIndex)
{
    for (auto& patch : patches) {
        if (editorIndex == patch->windowIndex && patch->openInPluginMode) {
            return patch;
        }
    }
    return nullptr;
}

void PluginProcessor::receiveControlChange(int const channel, int const controller, int const value)
{
    auto const port = channel >> 4;
    auto const deviceChannel = channel - port * 16;

    midiDeviceManager.enqueueMidiOutput(port, MidiMessage::controllerEvent(deviceChannel, controller, value), audioAdvancement);
}

void PluginProcessor::receiveProgramChange(int const channel, int const value)
{
    auto const port = channel >> 4;
    auto const deviceChannel = channel - port * 16;

    midiDeviceManager.enqueueMidiOutput(port, MidiMessage::programChange(deviceChannel, value), audioAdvancement);
}

void PluginProcessor::receivePitchBend(int const channel, int const value)
{
    auto const port = channel >> 4;
    auto const deviceChannel = channel - port * 16;

    midiDeviceManager.enqueueMidiOutput(port, MidiMessage::pitchWheel(deviceChannel, value + 8192), audioAdvancement);
}

void PluginProcessor::receiveAftertouch(int const channel, int const value)
{
    auto const port = channel >> 4;
    auto const deviceChannel = channel - port * 16;

    midiDeviceManager.enqueueMidiOutput(port, MidiMessage::channelPressureChange(deviceChannel, value), audioAdvancement);
}

void PluginProcessor::receivePolyAftertouch(int const channel, int const pitch, int const value)
{
    auto const port = channel >> 4;
    auto const deviceChannel = channel - port * 16;

    midiDeviceManager.enqueueMidiOutput(port, MidiMessage::aftertouchChange(deviceChannel, pitch, value), audioAdvancement);
}

void PluginProcessor::receiveMidiByte(int const channel, int const byte)
{
    auto const port = channel >> 4;

    if (midiByteIsSysex) {
        if (byte == 0xf7) {
            midiDeviceManager.enqueueMidiOutput(port, MidiMessage::createSysExMessage(midiByteBuffer, static_cast<int>(midiByteIndex)), audioAdvancement);
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
            midiDeviceManager.enqueueMidiOutput(port, MidiMessage(static_cast<uint8>(byte)), audioAdvancement);
        }
        // Handle 3-byte messages
        else {
            midiByteBuffer[midiByteIndex++] = static_cast<uint8>(byte);
            if (midiByteIndex >= 3) {
                midiDeviceManager.enqueueMidiOutput(port, MidiMessage(midiByteBuffer, 3), audioAdvancement);
                midiByteIndex = 0;
            }
        }
    }
}

void PluginProcessor::receiveSysMessage(SmallString const& selector, SmallArray<pd::Atom> const& list)
{
    switch (hash(selector)) {
    case hash("open"): {
        if (list.size() >= 2) {
            auto filename = list[0].toString();
            auto directory = list[1].toString();
            auto editors = getEditors();

            auto patch = URL(File(directory).getChildFile(filename));

            if (patch.getLocalFile().existsAsFile()) {
                if (!editors.empty()) {
                    editors[0]->getTabComponent().openPatch(patch);
                } else {
                    loadPatch(patch);
                }
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
            if (!editors.empty())
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
    case hash("limit"): {
        bool limit = list[0].getFloat();
        for (auto* editor : getEditors()) {
            editor->pd->setEnableLimiter(limit);
            editor->statusbar->showLimiterState(limit);
        }
        break;
    }
    case hash("pluginmode"): {
        // TODO: it would be nicer if we could specifically target the correct editor here, instead of picking the first one and praying
        auto editors = getEditors();
        {
            if (patches.not_empty()) {
                float pluginModeFloatArgument = 1.0;
                if (list.size()) {
                    if (list[0].isFloat()) {
                        pluginModeFloatArgument = list[0].getFloat();
                    } else {
                        auto pluginModeThemeOrPath = list[0].toString();
                        if (pluginModeThemeOrPath.endsWith(".plugdatatheme")) {
                            auto themeFile = patches[0]->getPatchFile().getParentDirectory().getChildFile(pluginModeThemeOrPath);
                            if (themeFile.existsAsFile()) {
                                auto themeTree = ValueTree::fromXml(themeFile.loadFileAsString());
                                if (themeTree.isValid()) {
                                    pluginModeTheme = themeTree;
                                }
                            }
                        } else {
                            auto themesTree = SettingsFile::getInstance()->getValueTree().getChildWithName("ColourThemes");
                            auto theme = themesTree.getChildWithProperty("theme", pluginModeThemeOrPath);
                            if (theme.isValid()) {
                                pluginModeTheme = theme;
                            }
                        }
                    }
                }

                if (!editors.empty()) {
                    auto* editor = editors[0];
                    if (auto* cnv = editor->getCurrentCanvas()) {
                        if (pluginModeFloatArgument)
                            editor->getTabComponent().openInPluginMode(cnv->patch);
                        else if (editor->isInPluginMode())
                            editor->pluginMode->closePluginMode();
                    }
                } else {
                    if (pluginModeFloatArgument)
                        patches[0]->openInPluginMode = true;
                    else
                        patches[0]->openInPluginMode = false;
                }
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

void PluginProcessor::addTextToTextEditor(uint64_t const ptr, SmallString const& text)
{
    MessageManager::callAsync([this, ptr, editorText = text.toString()](){
        if(textEditorDialogs.contains(ptr)) {
            Dialogs::appendTextToTextEditorDialog(textEditorDialogs[ptr].get(), editorText);
        }
    });
}

void PluginProcessor::clearTextEditor(uint64_t const ptr)
{
    MessageManager::callAsync([this, ptr](){
        if(textEditorDialogs.contains(ptr)) {
            Dialogs::clearTextEditorDialog(textEditorDialogs[ptr].get());
        }
    });
}

bool PluginProcessor::isTextEditorDialogShown(uint64_t const ptr)
{
    return textEditorDialogs.count(ptr) && textEditorDialogs[ptr]->isVisible();
}

void PluginProcessor::hideTextEditorDialog(uint64_t ptr)
{
    MessageManager::callAsync([this, ptr]() {
        textEditorDialogs.erase(ptr);
    });
}

void PluginProcessor::raiseTextEditorDialog(uint64_t ptr)
{
    if(textEditorDialogs.contains(ptr))
    {
        textEditorDialogs[ptr]->toFront(true);
    }
}

void PluginProcessor::showTextEditorDialog(uint64_t ptr, SmallString const& title, std::function<void(String, uint64_t)> save, std::function<void(uint64_t)> close)
{
    MessageManager::callAsync([this, ptr, weakRef = pd::WeakReference(reinterpret_cast<t_pd*>(ptr), this), title, save, close]() {
        static std::unique_ptr<Dialog> saveDialog = nullptr;
        
        auto onClose = [this, save, weakRef, title, ptr, close](String const& lastText, bool const hasChanged) {
            if (!hasChanged) {
                if (auto lock = weakRef.get<t_pd>()) {
                    close(ptr);
                }
                textEditorDialogs[ptr].reset(nullptr);
                return;
            }
            
            Dialogs::showAskToSaveDialog(
                                         &saveDialog, textEditorDialogs[ptr].get(), "", [this, save, close, ptr, title, weakRef, text = lastText](int const result) mutable {
                                             if (result == 2) {

                                                 if (auto lock = weakRef.get<t_pd>()) {
                                                     save(text, ptr);
                                                     close(ptr);
                                                 }
                                                 textEditorDialogs[ptr].reset(nullptr);
                                             }
                                             if (result == 1) {
                                                 if (auto lock = weakRef.get<t_pd>()) {
                                                     close(ptr);
                                                 }
                                                 textEditorDialogs[ptr].reset(nullptr);
                                             }
                                         },
                                         15, false);
        };
        
        auto onSave = [save, ptr, weakRef](String const& lastText) {
            if (auto lock = weakRef.get<t_pd>()) {
                save(lastText, ptr);
            }
        };
        if(auto* textEditor = Dialogs::showTextEditorDialog("", title.toString(), onClose, onSave)) {
            textEditorDialogs[ptr].reset(textEditor);
        }
    });
}

// set custom plugin latency
void PluginProcessor::performLatencyCompensationChange(float const value)
{
    if (!approximatelyEqual<int>(customLatencySamples, value)) {
        customLatencySamples = value;

        for (auto const& editor : getEditors()) {
            editor->statusbar->setLatencyDisplay(customLatencySamples);
        }

        setLatencySamples(customLatencySamples + Instance::getBlockSize());
    }
}

void PluginProcessor::sendParameterInfoChangeMessage()
{
    hostInfoUpdater.triggerAsyncUpdate();
}

void PluginProcessor::handleParameterMessage(SmallArray<pd::Atom> const& atoms)
{
    auto getEnabledParameter = [this](SmallString const& name) -> PlugDataParameter* {
        for (auto* p : getParameters()) {
            auto* param = dynamic_cast<PlugDataParameter*>(p);
            if (param->isEnabled() && param->getTitle() == name) {
                return param;
            }
        }
        return nullptr;
    };
    
    if (atoms.size() >= 2) {
        auto name = atoms[0].toSmallString();
        auto selector = hash(atoms[1].toSmallString());
        switch(selector)
        {
            case hash("create"): {
                enableAudioParameter(name);
                break;
            }
            case hash("destroy"): {
                disableAudioParameter(name);
                break;
            }
            case hash("float"):
            {
                if (atoms.size() >= 3 && atoms[2].isFloat()) {
                    if(auto* param = getEnabledParameter(name))
                    {
                        float const value = atoms[2].getFloat();
                        param->setUnscaledValueNotifyingHost(value);

                        if (ProjectInfo::isStandalone) {
                            for (auto const* editor : getEditors()) {
                                editor->sidebar->updateAutomationParameterValue(param);
                            }
                        }
                    }
                }
                break;
            }
            case hash("range"): {
                if (atoms.size() >= 4 && atoms[2].isFloat() && atoms[3].isFloat()) {
                    if(auto* param = getEnabledParameter(name))
                    {
                        float min = atoms[2].getFloat();
                        float max = atoms[3].getFloat();
                        max = std::max(max, min + 0.000001f);
                        param->setRange(min, max);
                    }
                }
                break;
            }
            case hash("mode"): {
                if (atoms.size() >= 3 && atoms[2].isFloat()) {
                    if(auto* param = getEnabledParameter(name))
                    {
                        float const mode = atoms[2].getFloat();
                        param->setMode(static_cast<PlugDataParameter::Mode>(std::clamp<int>(mode, 1, 4)));
                    }
                }
                break;
            }
            case hash("change"): {
                if (atoms.size() >= 3 && atoms[2].isFloat()) {
                    if(auto* param = getEnabledParameter(name))
                    {
                        int const state = atoms[2].getFloat() != 0;
                        param->setGestureState(state);
                    }
                }
                break;
            }
            case hash("default"): {
                if (atoms.size() >= 3 && atoms[2].isFloat()) {
                    if(auto* param = getEnabledParameter(name))
                    {
                        param->setDefaultValue(atoms[2].getFloat());
                        if (ProjectInfo::isStandalone) {
                            for (auto const* editor : getEditors()) {
                                editor->sidebar->updateAutomationParameterValue(param);
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}

void PluginProcessor::enableAudioParameter(SmallString const& name)
{
    int numEnabled = 0;
    for (auto* p : getParameters()) {
        auto const* param = dynamic_cast<PlugDataParameter*>(p);
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

    updateEnabledParameters();

    for (auto const* editor : getEditors()) {
        editor->sidebar->updateAutomationParameters();
    }
}

void PluginProcessor::disableAudioParameter(SmallString const& name)
{
    for (auto* p : getParameters()) {
        auto* param = dynamic_cast<PlugDataParameter*>(p);
        if (param->isEnabled() && param->getTitle() == name) {
            param->setEnabled(false);
            param->setDefaultValue(0.0f);
            param->setValue(0.0f);
            param->setRange(0.0f, 1.0f);
            param->setMode(PlugDataParameter::Float);
            
            param->notifyDAW();
            break;
        }
    }

    updateEnabledParameters();

    for (auto const* editor : getEditors()) {
        editor->sidebar->updateAutomationParameters();
    }
}

void PluginProcessor::fillDataBuffer(SmallArray<pd::Atom> const& vec)
{
    if (!vec[0].isSymbol()) {
        logWarning("[daw_storage]: accepts only lists beginning with a Symbol atom");
        return;
    }
    auto const childName = String(vec[0].toString());

    if (!XmlElement::isValidXmlName(childName)) {
        logWarning("[daw_storage]: name must start with alphabetical character");
        return;
    }

    if (extraData) {
        int const numChildren = extraData->getNumChildElements();
        if (numChildren > 0) {
            // Searching if a previously created child element exists, with same name as vec[0]. If true, delete it.
            if (auto* list = extraData->getChildByName(childName))
                extraData->removeChildElement(list, true);
        }
        if (auto list = extraData->createNewChildElement(childName)) {
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
    if (auto const* extra_data = xml.getChildByName(juce::StringRef("ExtraData"))) {
        int const nlists = extra_data->getNumChildElements();
        SmallArray<pd::Atom> vec;
        for (int i = 0; i < nlists; ++i) {
            if (auto const* list = extra_data->getChildElement(i)) {
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

                sendList("__from_daw_databuffer", vec);
                loaded = true;
            }
        }
    }

    if (!loaded) {
        sendBang("__from_daw_databuffer");
    }
}

void PluginProcessor::updateConsole(int const numMessages, bool const newWarning)
{
    for (auto const* editor : getEditors()) {
        editor->sidebar->updateConsole(numMessages, newWarning);
    }
}

SmallArray<PluginEditor*> PluginProcessor::getEditors() const
{
    SmallArray<PluginEditor*> editors;
    if (ProjectInfo::isStandalone) {
        editors.reserve(editors.size());
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

void PluginProcessor::reloadAbstractions(File const changedPatch, t_glist* except)
{
    setThis();

    // Ensure that all messages are dequeued before we start deleting objects
    sendMessagesFromQueue();

    isPerformingGlobalSync = true;

    pd::Patch::reloadPatch(changedPatch, except);

    for (auto* editor : getEditors()) {
        // Synchronising can potentially delete some other canvases, so make sure we use a safepointer
        SmallArray<Component::SafePointer<Canvas>> canvases;
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
