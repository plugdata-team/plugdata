/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PluginProcessor.h"
#include "Canvas.h"
#include "PluginEditor.h"

// Print std::cout and std::cerr to console when in debug mode
#if JUCE_DEBUG
#define LOG_STDOUT true
#else
#define LOG_STDOUT false
#endif

//==============================================================================
PlugDataAudioProcessor::PlugDataAudioProcessor(Console* externalConsole)
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
            )
    , pd::Instance("PlugData")
    ,
#endif
    numin(2)
    , numout(2)
    , m_name("PlugData")
    , m_accepts_midi(true)
    , m_produces_midi(true)
    , m_is_midi_effect(false)
    ,

    parameters(*this, nullptr, juce::Identifier("PlugData"),
        { std::make_unique<juce::AudioParameterFloat>("volume", "Volume", 0.0f, 1.0f, 0.75f),
            std::make_unique<juce::AudioParameterBool>("enabled", "Enabled", true),

            std::make_unique<juce::AudioParameterFloat>("param1", "Parameter 1", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param2", "Parameter 2", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param3", "Parameter 3", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param4", "Parameter 4", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param5", "Parameter 5", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param6", "Parameter 6", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param7", "Parameter 7", 0.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("param8", "Parameter 8", 0.0f, 1.0f, 0.0f) })
{

    volume = parameters.getRawParameterValue("volume");
    enabled = parameters.getRawParameterValue("enabled");

    
    // 8 general purpose automation parameters you can get by using "receive param1" etc.
    for (int n = 0; n < 8; n++) {
        parameterValues[n] = parameters.getRawParameterValue("param" + String(n + 1));
        lastParameters[n] = 0;
    }

    // On first startup, initialise abstractions and settings
    initialiseFilesystem();
    
    // Update pd search paths for abstractions
    updateSearchPaths();

    // Initialise library for text autocompletion
    objectLibrary.initialiseLibrary(settingsTree.getChildWithName("Paths"));

    // Set up midi buffers
    m_midi_buffer_in.ensureSize(2048);
    m_midi_buffer_out.ensureSize(2048);
    m_midi_buffer_temp.ensureSize(2048);

    setCallbackLock(&AudioProcessor::getCallbackLock());

    // Help patches have to run on their own instance, but not have their own console
    // This is a woraround that could be solved in a nicer way
    if (externalConsole) {
        console = externalConsole;
        ownsConsole = false;
    } else {
        LookAndFeel::setDefaultLookAndFeel(&mainLook);
        console = new Console(LOG_STDOUT, LOG_STDOUT);
        ownsConsole = true;
    }

    dequeueMessages();
    processMessages();

    startTimer(80);
}

PlugDataAudioProcessor::~PlugDataAudioProcessor()
{
    // Save current settings before quitting
    saveSettings();

    // Delete console if we own it
    if (ownsConsole) {
        delete console;
    }
}

void PlugDataAudioProcessor::initialiseFilesystem()
{
    // Check if the abstractions directory exists, if not, unzip it from binaryData
    if (!appDir.exists() || !abstractions.exists()) {
        appDir.createDirectory();

        MemoryInputStream binaryAbstractions(BinaryData::Abstractions_zip, BinaryData::Abstractions_zipSize, false);
        auto file = ZipFile(binaryAbstractions);
        file.uncompressTo(appDir);
    }

    // Check if settings file exists, if not, create the default
    if (!settingsFile.existsAsFile()) {
        settingsFile.create();

        // Add default settings
        settingsTree.setProperty("ConnectionStyle", false, nullptr);

        auto pathTree = ValueTree("Paths");

        auto defaultPath = ValueTree("Path");
        defaultPath.setProperty("Path", abstractions.getFullPathName(), nullptr);

        pathTree.appendChild(defaultPath, nullptr);
        settingsTree.appendChild(pathTree, nullptr);

        saveSettings();
    } else {
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
    for (auto child : pathTree) {
        auto path = child.getProperty("Path").toString();
        libpd_add_to_search_path(path.toRawUTF8());
    }

    objectLibrary.initialiseLibrary(pathTree);
}
//==============================================================================
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
    return 0.0;
}

int PlugDataAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
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

//==============================================================================
void PlugDataAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    samplerate = sampleRate;
    sampsperblock = samplesPerBlock;

    const int nins = std::max(numin, 2);
    const int nouts = std::max(numout, 2);

    bufferout.resize(nouts);
    bufferin.resize(nins);

    for (int i = 2; i < nins; i++) {
        bufferin[i] = new const float[sampsperblock]();
    }

    for (int i = 2; i < nouts; i++) {
        bufferout[i] = new float[sampsperblock]();
    }

    prepareDSP(nins, nouts, samplerate);
    // sendCurrentBusesLayoutInformation();
    m_audio_advancement = 0;
    const size_t blksize = static_cast<size_t>(Instance::getBlockSize());

    m_audio_buffer_in.resize(nins * blksize);
    m_audio_buffer_out.resize(nouts * blksize);
    std::fill(m_audio_buffer_out.begin(), m_audio_buffer_out.end(), 0.f);
    std::fill(m_audio_buffer_in.begin(), m_audio_buffer_in.end(), 0.f);
    m_midi_buffer_in.clear();
    m_midi_buffer_out.clear();
    m_midi_buffer_temp.clear();

    startDSP();
    processMessages();
    processPrints();

    processingBuffer.setSize(2, samplesPerBlock);

    meterSource.resize(numout, 50.0f * 0.001f * sampleRate / samplesPerBlock);
}

void PlugDataAudioProcessor::releaseResources()
{
    audio_started = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PlugDataAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void PlugDataAudioProcessor::hiResTimerCallback()
{
    // Hack to make sure DAW will keep dequeuing messages from pd to the gui when bypassed
    // Should only start running when audio is bypassed
    // ScopedLock lock(*getCallbackLock());

    bool oldEnabled = enabled;
    enabled->store(0);
    dequeueMessages();
    enabled->store(oldEnabled);
}

void PlugDataAudioProcessor::processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Run help files (without audio)
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {

        for (int c = 0; c < editor->canvases.size(); c++) {
            auto* cnv = editor->canvases[c];
            if (cnv->aux_instance) {
                cnv->aux_instance->enabled->store(0);
                cnv->aux_instance->process(processingBuffer, midiMessages);
            }
        }
    }

    processingBuffer.setSize(2, buffer.getNumSamples());

    processingBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    processingBuffer.copyFrom(1, 0, buffer, totalNumInputChannels == 2 ? 1 : 0, 0, buffer.getNumSamples());

    bool oldEnabled = enabled;
    enabled->store(0);
    dequeueMessages();
    enabled->store(oldEnabled);
}

void PlugDataAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{

    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto const maxOuts = std::max(numout, buffer.getNumChannels());
    for (int i = numin; i < maxOuts; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // midiCollector.removeNextBlockOfMessages(midiMessages, 512);

    for (int n = 0; n < 8; n++) {
        if (parameterValues[n]->load() != lastParameters[n]) {
            lastParameters[n] = parameterValues[n]->load();

            parameterAtom[0] = { pd::Atom(lastParameters[n]) };

            sendList(("param" + String(n + 1)).toRawUTF8(), parameterAtom);
        }
    }

    // Run help files (without audio)
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {

        for (int c = 0; c < editor->canvases.size(); c++) {
            auto* cnv = editor->canvases[c];
            if (cnv->aux_instance) {
                cnv->aux_instance->enabled->store(0);
                cnv->aux_instance->process(processingBuffer, midiMessages);
            }
        }
    }

    processingBuffer.setSize(2, buffer.getNumSamples());

    processingBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    processingBuffer.copyFrom(1, 0, buffer, totalNumInputChannels == 2 ? 1 : 0, 0, buffer.getNumSamples());

    process(processingBuffer, midiMessages);

    buffer.copyFrom(0, 0, processingBuffer, 0, 0, buffer.getNumSamples());
    if (totalNumOutputChannels == 2) {
        buffer.copyFrom(1, 0, processingBuffer, 1, 0, buffer.getNumSamples());
    }

    float avg = 0.0f;
    for (int ch = 0; ch < numout; ch++) {
        avg += buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
    }
    avg /= numout;

    buffer.applyGain(getParameters()[0]->getValue());

    meterSource.measureBlock(buffer);
}

void PlugDataAudioProcessor::process(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    stopTimer();

    const int blocksize = Instance::getBlockSize();
    const int nsamples = buffer.getNumSamples();
    const int adv = m_audio_advancement >= 64 ? 0 : m_audio_advancement;
    const int nleft = blocksize - adv;

    const int nins = numin;
    const int nouts = numout;

    const bool midi_consume = m_accepts_midi;
    const bool midi_produce = m_produces_midi;

    bufferin[0] = buffer.getReadPointer(0);
    bufferin[1] = buffer.getReadPointer(1);

    bufferout[0] = buffer.getWritePointer(0);
    bufferout[1] = buffer.getWritePointer(1);

    //////////////////////////////////////////////////////////////////////////////////////////

    // If the current number of samples in this block
    // is inferior to the number of samples required
    if (nsamples < nleft) {
        // we save the input samples and we output
        // the missing samples of the previous tick.
        for (int j = 0; j < nins; ++j) {
            const int index = j * blocksize + adv;
            std::copy_n(bufferin[j], nsamples, m_audio_buffer_in.data() + index);
        }
        for (int j = 0; j < nouts; ++j) {
            const int index = j * blocksize + adv;
            std::copy_n(m_audio_buffer_out.data() + index, nsamples, bufferout[j]);
        }
        if (midi_consume) {
            m_midi_buffer_in.addEvents(midiMessages, 0, nsamples, adv);
        }
        if (midi_produce) {
            midiMessages.clear();
            midiMessages.addEvents(m_midi_buffer_out, adv, nsamples, -adv);
        }
        m_audio_advancement += nsamples;
    }
    // If the current number of samples in this block
    // is superior to the number of samples required
    else {
        //////////////////////////////////////////////////////////////////////////////////////

        // we save the missing input samples, we output
        // the missing samples of the previous tick and
        // we call DSP perform method.
        MidiBuffer const& midiin = midi_produce ? m_midi_buffer_temp : midiMessages;
        if (midi_produce) {
            m_midi_buffer_temp.swapWith(midiMessages);
            midiMessages.clear();
        }

        for (int j = 0; j < nins; ++j) {
            const int index = j * blocksize + adv;
            std::copy_n(bufferin[j], nleft, m_audio_buffer_in.data() + index);
        }
        for (int j = 0; j < nouts; ++j) {
            const int index = j * blocksize + adv;
            std::copy_n(m_audio_buffer_out.data() + index, nleft, bufferout[j]);
        }
        if (midi_consume) {
            m_midi_buffer_in.addEvents(midiin, 0, nleft, adv);
        }
        if (midi_produce) {
            midiMessages.addEvents(m_midi_buffer_out, adv, nleft, -adv);
        }
        m_audio_advancement = 0;
        processInternal();

        //////////////////////////////////////////////////////////////////////////////////////

        // If there are other DSP ticks that can be
        // performed, then we do it now.
        int pos = nleft;
        while ((pos + blocksize) <= nsamples) {
            for (int j = 0; j < nins; ++j) {
                const int index = j * blocksize;
                std::copy_n(bufferin[j] + pos, blocksize, m_audio_buffer_in.data() + index);
            }
            for (int j = 0; j < nouts; ++j) {
                const int index = j * blocksize;
                std::copy_n(m_audio_buffer_out.data() + index, blocksize, bufferout[j] + pos);
            }
            if (midi_consume) {
                m_midi_buffer_in.addEvents(midiin, pos, blocksize, 0);
            }
            if (midi_produce) {
                midiMessages.addEvents(m_midi_buffer_out, 0, blocksize, pos);
            }
            processInternal();
            pos += blocksize;
        }

        //////////////////////////////////////////////////////////////////////////////////////

        // If there are samples that can't be
        // processed, then save them for later
        // and outputs the remaining samples
        const int remaining = nsamples - pos;
        if (remaining > 0) {
            for (int j = 0; j < nins; ++j) {
                const int index = j * blocksize;
                std::copy_n(bufferin[j] + pos, remaining, m_audio_buffer_in.data() + index);
            }
            for (int j = 0; j < nouts; ++j) {
                const int index = j * blocksize;
                std::copy_n(m_audio_buffer_out.data() + index, remaining, bufferout[j] + pos);
            }
            if (midi_consume) {
                m_midi_buffer_in.addEvents(midiin, pos, remaining, 0);
            }
            if (midi_produce) {
                midiMessages.addEvents(m_midi_buffer_out, 0, remaining, pos);
            }
            m_audio_advancement = remaining;
        }
    }

    // Start timer:
    // if we don't start bypassing, this timer will be stopped before it calls
    // If we do start bypassing, this will ensure gui messages will still be processed
    startTimer(150);
}

void PlugDataAudioProcessor::processInternal()
{
    //////////////////////////////////////////////////////////////////////////////////////////
    //                                     DEQUEUE MESSAGES                                 //
    //////////////////////////////////////////////////////////////////////////////////////////
    dequeueMessages();
    processMessages();
    processPrints();
    processMidi();

    //////////////////////////////////////////////////////////////////////////////////////////
    //                                          MIDI IN                                     //
    //////////////////////////////////////////////////////////////////////////////////////////
    if (m_accepts_midi) {
        for (auto it = m_midi_buffer_in.cbegin(); it != m_midi_buffer_in.cend(); ++it) {
            auto const message = (*it).getMessage();
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
        m_midi_buffer_in.clear();
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //                                  RETRIEVE MESSAGES                                   //
    //////////////////////////////////////////////////////////////////////////////////////////
    processMessages();

    //////////////////////////////////////////////////////////////////////////////////////////
    //                                          AUDIO                                       //
    //////////////////////////////////////////////////////////////////////////////////////////

    if (enabled->load()) {
        // Copy circuitlab's output to Pure data to Pd input channels
        std::copy_n(m_audio_buffer_out.data() + (2 * 64), (numout - 2) * 64, m_audio_buffer_in.data() + (2 * 64));

        performDSP(m_audio_buffer_in.data(), m_audio_buffer_out.data());
    }

    else {
        std::fill(m_audio_buffer_in.begin(), m_audio_buffer_in.end(), 0.f);

        performDSP(m_audio_buffer_in.data(), m_audio_buffer_out.data());

        std::fill(m_audio_buffer_out.begin(), m_audio_buffer_out.end(), 0.f);
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //                                          MIDI OUT                                    //
    //////////////////////////////////////////////////////////////////////////////////////////

    if (m_produces_midi) {
        m_midibyte_index = 0;
        m_midibyte_buffer[0] = 0;
        m_midibyte_buffer[1] = 0;
        m_midibyte_buffer[2] = 0;
        m_midi_buffer_out.clear();
        processMidi();
    }
}

//==============================================================================
bool PlugDataAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PlugDataAudioProcessor::createEditor()
{
    auto* editor = new PlugDataPluginEditor(*this, console);
    auto* cnv = editor->canvases.add(new Canvas(*editor, false));
    cnv->title = "Untitled Patcher";
    editor->mainCanvas = cnv;

    auto patch = getPatch();
    if (!patch.getPointer()) {
        editor->mainCanvas->createPatch();
    } else {
        editor->getMainCanvas()->loadPatch(patch);
    }

    editor->addTab(cnv);

    return editor;
}

//==============================================================================
void PlugDataAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryBlock xmlBlock;

    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, xmlBlock);

    // Store pure-data state
    MemoryOutputStream ostream(destData, false);

    ostream.writeString(getCanvasContent());
    ostream.writeInt(getLatencySamples());
    ostream.writeInt(xmlBlock.getSize());
    ostream.write(xmlBlock.getData(), xmlBlock.getSize());
}

void PlugDataAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{

    MemoryInputStream istream(data, sizeInBytes, false);
    String state = istream.readString();
    int latency = istream.readInt();
    int xmlSize = istream.readInt();

    void* xmlData = (void*)new char[xmlSize];
    istream.read(xmlData, xmlSize);

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(xmlData, xmlSize));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

    loadPatch(state);
    setLatencySamples(latency);
}

void PlugDataAudioProcessor::loadPatch(String patch)
{

    // String extra_info = patch.fromFirstOccurrenceOf("#X text plugdata_info:",false, false).upToFirstOccurrenceOf(";", false, false);

    // Load from content or location
    File patchFile;
    if (!patch.startsWith("#") && patch.endsWith(".pd") && File(patch).existsAsFile()) {
        patchFile = File(patch);
    } else {
        patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(patch);
    }

    const CriticalSection* lock = getCallbackLock();

    lock->enter();
    openPatch(patchFile.getParentDirectory().getFullPathName().toStdString(), patchFile.getFileName().toStdString());
    lock->exit();

    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {
        auto* cnv = editor->canvases.add(new Canvas(*editor, false));
        cnv->title = "Untitled Patcher";

        editor->mainCanvas = cnv;
        cnv->patch = getPatch();
        cnv->synchronise();
        editor->addTab(cnv);
    }
}

void PlugDataAudioProcessor::receiveNoteOn(const int channel, const int pitch, const int velocity)
{
    if (velocity == 0) {
        m_midi_buffer_out.addEvent(MidiMessage::noteOff(channel, pitch, uint8(0)), m_audio_advancement);
    } else {
        m_midi_buffer_out.addEvent(MidiMessage::noteOn(channel, pitch, static_cast<uint8>(velocity)), m_audio_advancement);
    }
}

void PlugDataAudioProcessor::receiveControlChange(const int channel, const int controller, const int value)
{
    m_midi_buffer_out.addEvent(MidiMessage::controllerEvent(channel, controller, value), m_audio_advancement);
}

void PlugDataAudioProcessor::receiveProgramChange(const int channel, const int value)
{
    m_midi_buffer_out.addEvent(MidiMessage::programChange(channel, value), m_audio_advancement);
}

void PlugDataAudioProcessor::receivePitchBend(const int channel, const int value)
{
    m_midi_buffer_out.addEvent(MidiMessage::pitchWheel(channel, value + 8192), m_audio_advancement);
}

void PlugDataAudioProcessor::receiveAftertouch(const int channel, const int value)
{
    m_midi_buffer_out.addEvent(MidiMessage::channelPressureChange(channel, value), m_audio_advancement);
}

void PlugDataAudioProcessor::receivePolyAftertouch(const int channel, const int pitch, const int value)
{
    m_midi_buffer_out.addEvent(MidiMessage::aftertouchChange(channel, pitch, value), m_audio_advancement);
}

void PlugDataAudioProcessor::receiveMidiByte(const int port, const int byte)
{
    if (m_midibyte_issysex) {
        if (byte == 0xf7) {
            m_midi_buffer_out.addEvent(MidiMessage::createSysExMessage(m_midibyte_buffer, static_cast<int>(m_midibyte_index)), m_audio_advancement);
            m_midibyte_index = 0;
            m_midibyte_issysex = false;
        } else {
            m_midibyte_buffer[m_midibyte_index++] = static_cast<uint8>(byte);
            if (m_midibyte_index == 512) {
                m_midibyte_index = 511;
            }
        }
    } else if (m_midibyte_index == 0 && byte == 0xf0) {
        m_midibyte_issysex = true;
    } else {
        m_midibyte_buffer[m_midibyte_index++] = static_cast<uint8>(byte);
        if (m_midibyte_index >= 3) {
            m_midi_buffer_out.addEvent(MidiMessage(m_midibyte_buffer, 3), m_audio_advancement);
            m_midibyte_index = 0;
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugDataAudioProcessor();
}
