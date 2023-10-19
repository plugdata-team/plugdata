/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

class Limiter
{
public:

    Limiter() = default;

    void process (dsp::AudioBlock<float>& block) noexcept
    {
        firstStageCompressor.process(dsp::ProcessContextReplacing<float>(block));
        secondStageCompressor.process(dsp::ProcessContextReplacing<float>(block));

        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            FloatVectorOperations::clip (block.getChannelPointer (channel), block.getChannelPointer (channel), -1.0f, 1.0f, block.getNumSamples());
        }
    }

    void prepare (const dsp::ProcessSpec& spec)
    {
        jassert (spec.sampleRate > 0);
        jassert (spec.numChannels > 0);

        sampleRate = spec.sampleRate;

        firstStageCompressor.prepare (spec);
        secondStageCompressor.prepare (spec);

        update();
        reset();
    }

    void reset()
    {
        firstStageCompressor.reset();
        secondStageCompressor.reset();
    }
    
private:

    void update()
    {
        firstStageCompressor.setThreshold (-8.0f);
        firstStageCompressor.setRatio     (4.0f);
        firstStageCompressor.setAttack    (2.0f);
        firstStageCompressor.setRelease   (200.0f);

        secondStageCompressor.setThreshold (-6.0f);
        secondStageCompressor.setRatio     (1000.0f);
        secondStageCompressor.setAttack    (0.001f);
        secondStageCompressor.setRelease   (releaseTime);
    }

    //==============================================================================
    dsp::Compressor<float> firstStageCompressor, secondStageCompressor;

    double sampleRate = 44100.0;
    float releaseTime = 100.0;
};
