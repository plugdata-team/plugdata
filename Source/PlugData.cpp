#include "PlugData.h"

void PlugData::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
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

void PlugData::processInternal()
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
    
    
}


void PlugData::prepareToPlay(double sampleRate, int samplesPerBlock, int oversamp)
{
    oversample = oversamp;
    osfactor = pow(2, oversamp);
    samplerate = sampleRate * osfactor;
    sampsperblock = samplesPerBlock * osfactor;
    
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
}
