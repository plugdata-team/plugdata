/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"

// Print std::cout and std::cerr to console when in debug mode
#if JUCE_DEBUG
#define LOG_STDOUT true
#else
#define LOG_STDOUT false
#endif


//==============================================================================
PlugDataAudioProcessor::PlugDataAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),  pd::Instance("PlugData"),
#endif
    numin(2), numout(2),
    m_name("PlugData"),
    m_accepts_midi(true),
    m_produces_midi(false),
    m_is_midi_effect(false),
    m_bypass(false){
    
    if(!appDir.exists() || !abstractions.exists()) {
            appDir.createDirectory();

            MemoryInputStream binaryAbstractions(BinaryData::Abstractions_zip, BinaryData::Abstractions_zipSize, false);
            auto file = ZipFile(binaryAbstractions);
            file.uncompressTo(appDir);
    }
        
    libpd_add_to_search_path(abstractions.getFullPathName().toRawUTF8());
    
    objectLibrary.initialiseLibrary();
        
    m_midi_buffer_in.ensureSize(2048);
    m_midi_buffer_out.ensureSize(2048);
    m_midi_buffer_temp.ensureSize(2048);
        
    setCallbackLock(&AudioProcessor::getCallbackLock());
        
    LookAndFeel::setDefaultLookAndFeel(&mainLook);
    
        
    console.reset(new Console(LOG_STDOUT, LOG_STDOUT));
    dequeueMessages();

    processMessages();
        
        
    
}

PlugDataAudioProcessor::~PlugDataAudioProcessor()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PlugDataAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PlugDataAudioProcessor::setCurrentProgram (int index)
{
}

const String PlugDataAudioProcessor::getProgramName (int index)
{
    return {};
}

void PlugDataAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void PlugDataAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerate = sampleRate;
    sampsperblock = samplesPerBlock;
    
    const int nins      = std::max(numin, 2);
    const int nouts     = std::max(numout, 2);

    
    bufferout.resize(nouts);
    bufferin.resize(nins);
    
    
    for(int i = 2; i < nins; i++) {
        bufferin[i] = new const float[sampsperblock]();
    }
    
    for(int i = 2; i < nouts; i++) {
        bufferout[i] = new float[sampsperblock]();
    }

    prepareDSP(nins, nouts, samplerate);
    //sendCurrentBusesLayoutInformation();
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
}

void PlugDataAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PlugDataAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
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
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PlugDataAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

   //midiCollector.removeNextBlockOfMessages(midiMessages, 512);
    
    processingBuffer.setSize(2, buffer.getNumSamples());
    
    processingBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    processingBuffer.copyFrom(1, 0, buffer, totalNumInputChannels == 2 ? 1 : 0, 0, buffer.getNumSamples());
    
    process(processingBuffer, midiMessages);
    
    buffer.copyFrom(0, 0, processingBuffer, 0, 0, buffer.getNumSamples());
    if(totalNumOutputChannels == 2) {
        buffer.copyFrom(1, 0, processingBuffer, 1, 0, buffer.getNumSamples());
    }
   
    
    // ..do something to the data...
}

void PlugDataAudioProcessor::process(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    //ScopedNoDenormals noDenormals;
    const int blocksize = Instance::getBlockSize();
    const int nsamples  = buffer.getNumSamples();
    const int adv       = m_audio_advancement >= 64 ? 0 : m_audio_advancement;
    const int nleft     = blocksize - adv;

    const int nins      = numin;
    const int nouts     = numout;
    
    const bool midi_consume = m_accepts_midi;
    const bool midi_produce = m_produces_midi;
    
    bufferin[0] = buffer.getReadPointer(0);
    bufferin[1] = buffer.getReadPointer(1);
    
    bufferout[0] = buffer.getWritePointer(0);
    bufferout[1] = buffer.getWritePointer(1);
    
                    
    auto const maxOuts = std::max(nouts, buffer.getNumChannels());
    for(int i = nins; i < maxOuts; ++i)
    {
        buffer.clear(i, 0, nsamples);
    }
    
    
    //////////////////////////////////////////////////////////////////////////////////////////
    
    // If the current number of samples in this block
    // is inferior to the number of samples required
    if(nsamples < nleft)
    {
        // we save the input samples and we output
        // the missing samples of the previous tick.
        for(int j = 0; j < nins; ++j)
        {
            const int index = j*blocksize+adv;
            std::copy_n(bufferin[j], nsamples, m_audio_buffer_in.data()+index);
        }
        for(int j = 0; j < nouts; ++j)
        {
            const int index = j*blocksize+adv;
            std::copy_n(m_audio_buffer_out.data()+index, nsamples, bufferout[j]);
        }
        if(midi_consume)
        {
            m_midi_buffer_in.addEvents(midiMessages, 0, nsamples, adv);
        }
        if(midi_produce)
        {
            midiMessages.clear();
            midiMessages.addEvents(m_midi_buffer_out, adv, nsamples, -adv);
        }
        m_audio_advancement += nsamples;
    }
    // If the current number of samples in this block
    // is superior to the number of samples required
    else
    {
        //////////////////////////////////////////////////////////////////////////////////////
        
        // we save the missing input samples, we output
        // the missing samples of the previous tick and
        // we call DSP perform method.
        MidiBuffer const& midiin = midi_produce ? m_midi_buffer_temp : midiMessages;
        if(midi_produce)
        {
            m_midi_buffer_temp.swapWith(midiMessages);
            midiMessages.clear();
        }
        
        for(int j = 0; j < nins; ++j)
        {
            const int index = j*blocksize+adv;
            std::copy_n(bufferin[j], nleft, m_audio_buffer_in.data()+index);
        }
        for(int j = 0; j < nouts; ++j)
        {
            const int index = j*blocksize+adv;
            std::copy_n(m_audio_buffer_out.data()+index, nleft, bufferout[j]);
        }
        if(midi_consume)
        {
            m_midi_buffer_in.addEvents(midiin, 0, nleft, adv);
        }
        if(midi_produce)
        {
            midiMessages.addEvents(m_midi_buffer_out, adv, nleft, -adv);
        }
        m_audio_advancement = 0;
        processInternal();
        
        //////////////////////////////////////////////////////////////////////////////////////
        
        // If there are other DSP ticks that can be
        // performed, then we do it now.
        int pos = nleft;
        while((pos + blocksize) <= nsamples)
        {
            for(int j = 0; j < nins; ++j)
            {
                const int index = j*blocksize;
                std::copy_n(bufferin[j]+pos, blocksize, m_audio_buffer_in.data()+index);
            }
            for(int j = 0; j < nouts; ++j)
            {
                const int index = j*blocksize;
                std::copy_n(m_audio_buffer_out.data()+index, blocksize, bufferout[j]+pos);
            }
            if(midi_consume)
            {
                m_midi_buffer_in.addEvents(midiin, pos, blocksize, 0);
            }
            if(midi_produce)
            {
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
        if(remaining > 0)
        {
            for(int j = 0; j < nins; ++j)
            {
                const int index = j*blocksize;
                std::copy_n(bufferin[j]+pos, remaining, m_audio_buffer_in.data()+index);
            }
            for(int j = 0; j < nouts; ++j)
            {
                const int index = j*blocksize;
                std::copy_n(m_audio_buffer_out.data()+index, remaining, bufferout[j]+pos);
            }
            if(midi_consume)
            {
                m_midi_buffer_in.addEvents(midiin, pos, remaining, 0);
            }
            if(midi_produce)
            {
                midiMessages.addEvents(m_midi_buffer_out, 0, remaining, pos);
            }
            m_audio_advancement = remaining;
        }
    }
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
    if(m_accepts_midi)
    {
        for(auto it = m_midi_buffer_in.cbegin(); it != m_midi_buffer_in.cend(); ++it) {
            auto const message = (*it).getMessage();
            if(message.isNoteOn()) {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), message.getVelocity()); }
            else if(message.isNoteOff()) {
                sendNoteOn(message.getChannel(), message.getNoteNumber(), 0); }
            else if(message.isController()) {
                sendControlChange(message.getChannel(), message.getControllerNumber(), message.getControllerValue()); }
            else if(message.isPitchWheel()) {
                sendPitchBend(message.getChannel(), message.getPitchWheelValue() - 8192); }
            else if(message.isChannelPressure()) {
                sendAfterTouch(message.getChannel(), message.getChannelPressureValue()); }
            else if(message.isAftertouch()) {
                sendPolyAfterTouch(message.getChannel(), message.getNoteNumber(), message.getAfterTouchValue()); }
            else if(message.isProgramChange()) {
                sendProgramChange(message.getChannel(), message.getProgramChangeNumber()); }
            else if(message.isSysEx()) {
                for(int i = 0; i < message.getSysExDataSize(); ++i)  {
                    sendSysEx(0, static_cast<int>(message.getSysExData()[i]));
                }
            }
            else if(message.isMidiClock() || message.isMidiStart() || message.isMidiStop() || message.isMidiContinue() ||
                    message.isActiveSense() || (message.getRawDataSize() == 1 && message.getRawData()[0] == 0xff)) {
                for(int i = 0; i < message.getRawDataSize(); ++i)  {
                    sendSysRealTime(0, static_cast<int>(message.getRawData()[i]));
                }
            }
            
            for(int i = 0; i < message.getRawDataSize(); i++)  {
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
    
    if(!m_bypass) {
    // Copy circuitlab's output to Pure data to Pd input channels
    std::copy_n(m_audio_buffer_out.data() + (2 * 64), (numout-2) * 64, m_audio_buffer_in.data() + (2 * 64));
    
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
   /*
    if(m_produces_midi)
    {
        m_midibyte_index = 0;
        m_midibyte_buffer[0] = 0;
        m_midibyte_buffer[1] = 0;
        m_midibyte_buffer[2] = 0;
        m_midi_buffer_out.clear();
        processMidi();
    }*/
    
    audio_started = true;
}


//==============================================================================
bool PlugDataAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PlugDataAudioProcessor::createEditor()
{
    auto* editor = new PlugDataPluginEditor(*this, console.get(), mainTree);
    auto patch = getPatch();
    editor->getMainCanvas()->loadPatch(patch);
    return editor;
}

//==============================================================================
void PlugDataAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    
    // Store pure-data state
    MemoryOutputStream ostream(destData, false);
    ostream.writeString(getCanvasContent());
}

void PlugDataAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    
    MemoryInputStream istream(data, sizeInBytes, false);
    String state = istream.readString();
    
    loadPatch(state);
}

void PlugDataAudioProcessor::loadPatch(String patch) {
    
    String extra_info = patch.fromFirstOccurrenceOf("#X text plugdata_info:",false, false).upToFirstOccurrenceOf(";", false, false);
    
    // Create the pd save file
    auto temp_patch = File::createTempFile(".pd");
    temp_patch.replaceWithText(patch);
    
    const CriticalSection* lock = getCallbackLock();
    
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to create the gui
    
    lock->enter();
    
   openPatch(temp_patch.getParentDirectory().getFullPathName().toStdString(), temp_patch.getFileName().toStdString());
    
    lock->exit();
    
    
    if(auto* editor = dynamic_cast<PlugDataPluginEditor*>(getActiveEditor())) {
        
        auto canvas = ValueTree(Identifiers::canvas);
        canvas.setProperty("Title", "Untitled Patcher", nullptr);
        canvas.setProperty(Identifiers::isGraph, false, nullptr);
        editor->mainCanvas = editor->appendChild<Canvas>(canvas);
        editor->mainCanvas->patch = getPatch();
        editor->mainCanvas->synchronise();
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugDataAudioProcessor();
}
