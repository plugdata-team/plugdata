/*
 ==============================================================================
 Copyright (c) 2018-2020 Foleys Finest Audio Ltd. - Daniel Walz
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
 ==============================================================================
 
 StereoFieldBuffer.h
 Created: 29 Dec 2017 16:49:54pm
 Author:  Daniel Walz
 
 ==============================================================================
 */

#pragma once

namespace foleys
{
    
    /** @addtogroup ff_meters */
    /*@{*/
    
    /**
     \class StereoFieldBuffer
     
     This class implements a circular buffer to buffer audio samples.
     At any time the GUI can ask for a stereo field visualisation of
     two neightbouring channels.
     */
    template<typename FloatType>
    class StereoFieldBuffer
    {
        juce::AudioBuffer<FloatType> sampleBuffer;
        std::atomic<int>             writePosition = { 0 };
        std::vector<FloatType>       maxValues     = { 180, 0.0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldBuffer)

        inline void computeDirection (std::vector<float>& directions, const FloatType left, const FloatType right) const
        {
            if (left == 0)
            {
                directions [45] = std::max (directions [45], std::abs (right));
            }
            else if (left * right > 0)
            {

            }

        }

        inline juce::Point<FloatType> computePosition (const juce::Rectangle<FloatType>& b, const FloatType left, const FloatType right) const
        {
            return juce::Point<FloatType> (b.getCentreX() + FloatType (0.5) * b.getWidth() * (right - left),
                                           b.getCentreY() + FloatType (0.5) * b.getHeight() * (left + right));
        }

    public:
        StereoFieldBuffer ()
        {
        }

        void setBufferSize (int newNumChannels, int newNumSamples)
        {
            sampleBuffer.setSize (newNumChannels, newNumSamples);
            sampleBuffer.clear();
            writePosition = 0;
        }

        void pushSampleBlock (juce::AudioBuffer<FloatType> buffer, int numSamples)
        {
            jassert (buffer.getNumChannels() == sampleBuffer.getNumChannels());

            auto pos   = writePosition.load();
            auto space = sampleBuffer.getNumSamples() - pos;
            if (space >= numSamples) {
                for (int c=0; c < sampleBuffer.getNumChannels(); ++c) {
                    sampleBuffer.copyFrom (c, pos, buffer.getReadPointer(c), numSamples);
                }
                writePosition = pos + numSamples;
            }
            else {
                for (int c=0; c < sampleBuffer.getNumChannels(); ++c) {
                    sampleBuffer.copyFrom (c, pos, buffer.getReadPointer(c),        space);
                    sampleBuffer.copyFrom (c, 0,   buffer.getReadPointer(c, space), numSamples - space);
                }
                writePosition = numSamples - space;
            }
        }

        void resetMaxValues ()
        {
            std::fill (maxValues.begin(), maxValues.end(), 0.0);
        }

        //  ==============================================================================

        juce::Path getOscilloscope (const int numSamples, const juce::Rectangle<FloatType> bounds, int leftIdx, int rightIdx) const
        {
            juce::Path curve;
            auto pos = writePosition.load();
            if (pos >= numSamples)
            {
                auto* left  = sampleBuffer.getReadPointer (leftIdx,  pos - numSamples);
                auto* right = sampleBuffer.getReadPointer (rightIdx, pos - numSamples);
                curve.startNewSubPath (computePosition (bounds, *left, *right));
                ++left; ++right;
                for (int i=1; i < numSamples; ++i) {
                    curve.lineTo (computePosition (bounds, *left, *right));
                    ++left; ++right;
                }
            }
            else
            {
                auto  leftover = numSamples - pos;
                auto* left  = sampleBuffer.getReadPointer (leftIdx,  sampleBuffer.getNumSamples() - leftover);
                auto* right = sampleBuffer.getReadPointer (rightIdx, sampleBuffer.getNumSamples() - leftover);
                curve.startNewSubPath (computePosition (bounds, *left, *right));
                ++left; ++right;
                for (int i=1; i < leftover; ++i) {
                    curve.lineTo (computePosition (bounds, *left, *right));
                    ++left; ++right;
                }
                left  = sampleBuffer.getReadPointer (leftIdx);
                right = sampleBuffer.getReadPointer (rightIdx);
                for (int i=0; i < numSamples - leftover; ++i) {
                    curve.lineTo (computePosition (bounds, *left, *right));
                    ++left; ++right;
                }
            }

            return curve;
        }


        //  ==============================================================================

        void getDirections (std::vector<FloatType>& directions, int numSamples, int leftIdx, int rightIdx)
        {
            jassert (directions.size() == 180);
            std::fill (directions.begin(), directions.end(), 0.0);
            auto pos = writePosition.load();

            if (pos >= numSamples)
            {
                auto* left  = sampleBuffer.getReadPointer (leftIdx,  pos - numSamples);
                auto* right = sampleBuffer.getReadPointer (rightIdx, pos - numSamples);
                for (int i=0; i < numSamples; ++i) {
                    computeDirection (directions, *left, *right);
                    ++left; ++right;
                }
            }
            else
            {
                auto  leftover = numSamples - pos;
                auto* left  = sampleBuffer.getReadPointer (leftIdx,  sampleBuffer.getNumSamples() - leftover);
                auto* right = sampleBuffer.getReadPointer (rightIdx, sampleBuffer.getNumSamples() - leftover);
                for (int i=0; i < leftover; ++i) {
                    computeDirection (directions, *left, *right);
                    ++left; ++right;
                }
                left  = sampleBuffer.getReadPointer (leftIdx);
                right = sampleBuffer.getReadPointer (rightIdx);
                for (int i=0; i < numSamples - leftover; ++i) {
                    computeDirection (directions, *left, *right);
                    ++left; ++right;
                }
            }
        }

    };
    /*@}*/
}
