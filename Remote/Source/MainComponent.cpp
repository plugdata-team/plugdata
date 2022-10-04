#include "MainComponent.h"


//==============================================================================
PureData::PureData(String ID) : message_handler(ID)
{
    channelPointers.reserve(32);

    // Set up midi buffers
    midiBufferIn.ensureSize(2048);
    midiBufferOut.ensureSize(2048);
    midiBufferTemp.ensureSize(2048);
    midiBufferCopy.ensureSize(2048);
    
    libpd_init();
    
    patches.add(new Patch(libpd_openfile("instantosc.pd", "/Users/timschoen")));
    
    // Some platforms require permissions to open input channels so request that here
    if(juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

PureData::~PureData()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void PureData::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    const auto blksize = static_cast<size_t>(libpd_blocksize());
    const auto numIn = std::max(static_cast<size_t>(2), static_cast<size_t>(2));
    const auto nouts = std::max(static_cast<size_t>(2), static_cast<size_t>(2));
    audioBufferIn.resize(numIn * blksize);
    audioBufferOut.resize(nouts * blksize);
    std::fill(audioBufferOut.begin(), audioBufferOut.end(), 0.f);
    std::fill(audioBufferIn.begin(), audioBufferIn.end(), 0.f);

    libpd_init_audio(2, 2, sampleRate);
    libpd_start_message(1); // one entry in list
    libpd_add_float(1.0f);
    libpd_finish_message("pd", "dsp");
}

void PureData::processInternal()
{
    //setThis();

    // clear midi out
    /*
    if (producesMidi())
    {
        midiByteIndex = 0;
        midiByteBuffer[0] = 0;
        midiByteBuffer[1] = 0;
        midiByteBuffer[2] = 0;
        midiBufferOut.clear();
    } */

    // Dequeue messages
    //sendMessagesFromQueue();
    //sendPlayhead();
    //sendMidiBuffer();
    //sendParameters();
    
    
    for(auto* patch : patches) {
        patch->update();
    }
    
    MemoryBlock target;
    while(message_handler.receiveMessages(target)) {
        MemoryInputStream istream(target, false);
        auto type = static_cast<MessageHandler::MessageType>(istream.readInt());
        
        if(type == MessageHandler::tGlobal)
        {
            auto name = istream.readString();
            if(name == "CreateObject") {
                auto cnv_id = istream.readString();
                auto initializer = istream.readString();
                createObject(cnv_id, initializer);
            }
            if(name == "RemoveObject") {
                
            }
            if(name == "CreateConnection") {
                
            }
            if(name == "RemoveConnection") {
                
            }
        }
    }


    // Process audio
    FloatVectorOperations::copy(audioBufferIn.data() + (2 * 64), audioBufferOut.data() + (2 * 64), (minOut - 2) * 64);
    libpd_process_raw(audioBufferIn.data(), audioBufferOut.data());
}


void PureData::createObject(String cnv_id, String initialiser) {
    
    auto* cnv = reinterpret_cast<t_canvas*>(cnv_id.getLargeIntValue());
    
    auto tokens = StringArray::fromTokens(initialiser, true);
    
    auto* sym = gensym(tokens[0].toRawUTF8());
    tokens.remove(0);
    
    int argc = tokens.size();
    auto argv = std::vector<t_atom>(argc);

    // Set position
    //SETFLOAT(argv.data(), static_cast<float>(x));
    //SETFLOAT(argv.data() + 1, static_cast<float>(y));

    for (int i = 0; i < tokens.size(); i++) {
        auto& tok = tokens[i];
        if (tokens[i].containsOnly("0123456789e.-+") && tokens[i] != "-") {
            SETFLOAT(argv.data() + i, tokens[i].getFloatValue());
        } else {
            SETSYMBOL(argv.data() + i, gensym(tokens[i].toRawUTF8()));
        }
    }
    

    
    libpd_createobj(cnv, sym, argv.size(), argv.data());
}

/*
void PureData::removeObject(String cnv_id, String initialiser) {
    
    auto* cnv = reinterpret_cast<t_canvas*>(cnv_id.getLargeIntValue());
    
    
    //libpd_createobj(cnv, t_symbol *s, int argc, t_atom *argv);
} */

void PureData::synchronise() {
    
    for(auto* patch : patches) {
        patch->synchronise();
    }
}

void PureData::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto buffer = dsp::AudioBlock<float>(*bufferToFill.buffer);
    
    ScopedNoDenormals noDenormals;
    const int blockSize = libpd_blocksize();
    const int numSamples = static_cast<int>(buffer.getNumSamples());
    const int adv = audioAdvancement >= 64 ? 0 : audioAdvancement;
    const int numLeft = blockSize - adv;
    const int numIn = 2;
    const int numOut = 2;

    channelPointers.clear();
    for(int ch = 0; ch < std::max(numIn, numOut); ch++) {
        channelPointers.push_back(buffer.getChannelPointer(ch));
    }

    const bool midiConsume = true;
    const bool midiProduce = true;

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
            //midiBufferIn.addEvents(midiMessages, 0, numSamples, adv);
        }
        if (midiProduce)
        {
           //  midiMessages.clear();
           // midiMessages.addEvents(midiBufferOut, adv, numSamples, -adv);
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
        //MidiBuffer const& midiin = midiProduce ? midiBufferTemp : midiMessages;
        if (midiProduce)
        {
            //midiBufferTemp.swapWith(midiMessages);
            //midiMessages.clear();
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
            //midiBufferIn.addEvents(midiin, 0, numLeft, adv);
        }
        if (midiProduce)
        {
            //midiMessages.addEvents(midiBufferOut, adv, numLeft, -adv);
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
                //midiBufferIn.addEvents(midiin, pos, blockSize, 0);
            }
            if (midiProduce)
            {
                //midiMessages.addEvents(midiBufferOut, 0, blockSize, pos);
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
                //midiBufferIn.addEvents(midiin, pos, remaining, 0);
            }
            if (midiProduce)
            {
               //midiMessages.addEvents(midiBufferOut, 0, remaining, pos);
            }
            audioAdvancement = remaining;
        }
    }
}

void PureData::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

void PureData::setAudioChannels(int numInputChannels, int numOutputChannels, const XmlElement* const xml)
{
    String audioError;

    audioError = deviceManager.initialise (numInputChannels, numOutputChannels, xml, true);

    jassert (audioError.isEmpty());

    deviceManager.addAudioCallback (&audioSourcePlayer);
    audioSourcePlayer.setSource (this);
}

void PureData::shutdownAudio()
{
    audioSourcePlayer.setSource (nullptr);
    deviceManager.removeAudioCallback (&audioSourcePlayer);
}
