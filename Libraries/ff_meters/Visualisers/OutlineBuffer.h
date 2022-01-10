/*
 ==============================================================================
 Copyright (c) 2017 - 2020 Foleys Finest Audio Ltd. - Daniel Walz
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

 OutlineBuffer.h
 Created: 9 Sep 2017 16:49:54pm
 Author:  Daniel Walz

 ==============================================================================
 */

#pragma once

namespace foleys
{

    /** @addtogroup ff_meters */
    /*@{*/

    /**
     \class OutlineBuffer

     This class implements a circular buffer to store min and max values
     of anaudio signal. The block size can be specified. At any time the
     UI can request an outline of the last n blocks as Path to fill or stroke
     */
    class OutlineBuffer
    {

        class ChannelData
        {
            std::vector<float>           minBuffer;
            std::vector<float>           maxBuffer;
            std::atomic<size_t>          writePointer     {0};
            int                          fraction        = 0;
            int                          samplesPerBlock = 128;

            JUCE_LEAK_DETECTOR (ChannelData)
        public:
            ChannelData ()
            {
                setSize (1024);
            }

            /**
             This copy constructor does not really copy. It is only present to satisfy the vector.
             */
            ChannelData (const ChannelData& other)
            {
                setSize (other.getSize());
            }

            /**
             @return the number of values the buffer will store.
             */
            int getSize () const
            {
                return static_cast<int> (minBuffer.size());
            }

            void setSamplesPerBlock (const int numSamples)
            {
                samplesPerBlock = numSamples;
            }

            /**
             @param numBlocks is the number of values the buffer will store. Allow a little safety buffer, so you
             don't write into the part, where it is currently read
             */
            void setSize (const int numBlocks)
            {
                minBuffer.resize (size_t (numBlocks), 0.0f);
                maxBuffer.resize (size_t (numBlocks), 0.0f);
                writePointer = writePointer % size_t (numBlocks);
            }

            void pushChannelData (const float* input, const int numSamples)
            {
                // create peak values
                int samples = 0;
                juce::Range<float> minMax;
                while (samples < numSamples)
                {
                    auto leftover = numSamples - samples;
                    if (fraction > 0)
                    {
                        minMax = juce::FloatVectorOperations::findMinAndMax (input, samplesPerBlock - fraction);
                        maxBuffer [(size_t) writePointer] = std::max (maxBuffer [(size_t) writePointer], minMax.getEnd());
                        minBuffer [(size_t) writePointer] = std::min (minBuffer [(size_t) writePointer], minMax.getStart());
                        samples += samplesPerBlock - fraction;
                        fraction = 0;
                        writePointer = (writePointer + 1) % maxBuffer.size();
                    }
                    else if (leftover > samplesPerBlock)
                    {
                        minMax = juce::FloatVectorOperations::findMinAndMax (input + samples, samplesPerBlock);
                        maxBuffer [(size_t) writePointer] = minMax.getEnd();
                        minBuffer [(size_t) writePointer] = minMax.getStart();
                        samples += samplesPerBlock;
                        writePointer = (writePointer + 1) % maxBuffer.size();
                    }
                    else
                    {
                        minMax = juce::FloatVectorOperations::findMinAndMax (input + samples, leftover);
                        maxBuffer [(size_t) writePointer] = minMax.getEnd();
                        minBuffer [(size_t) writePointer] = minMax.getStart();
                        samples += samplesPerBlock - fraction;
                        fraction = leftover;
                    }
                    jassert (minMax.getStart() == minMax.getStart() && minMax.getEnd() == minMax.getEnd());
                }
            }

            void getChannelOutline (juce::Path& outline, const juce::Rectangle<float> bounds, const int numSamplesToPlot) const
            {
                auto numSamples = size_t (numSamplesToPlot);
                auto latest = writePointer > 0 ? writePointer - 1 : maxBuffer.size() - 1;
                auto oldest = (latest >= numSamples) ? latest - numSamples : latest + maxBuffer.size() - numSamples;

                const auto dx = bounds.getWidth() / numSamples;
                const auto dy = bounds.getHeight() * 0.35f;
                const auto my = bounds.getCentreY();
                auto  x  = bounds.getX();
                auto  s  = oldest;

                outline.startNewSubPath (x, my);
                for (size_t i=0; i < numSamples; ++i)
                {
                    outline.lineTo (x, my + minBuffer [s] * dy);
                    x += dx;
                    if (s < minBuffer.size() - 1)
                        s += 1;
                    else
                        s = 0;
                }

                for (size_t i=0; i < numSamples; ++i)
                {
                    outline.lineTo (x, my + maxBuffer [s] * dy);
                    x -= dx;
                    if (s > 1)
                        s -= 1;
                    else
                        s = maxBuffer.size() - 1;
                }
            }
        };

        std::vector<ChannelData> channelDatas;
        int                      samplesPerBlock = 128;


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutlineBuffer)
    public:
        OutlineBuffer ()
        {
        }

        /**
         @param numChannels is the number of channels the buffer will store
         @param numBlocks is the number of values the buffer will store. Allow a little safety buffer, so you
         don't write into the part, where it is currently read
         */
        void setSize (const int numChannels, const int numBlocks)
        {
            channelDatas.resize (size_t (numChannels));
            for (auto& data : channelDatas) {
                data.setSize (numBlocks);
                data.setSamplesPerBlock (samplesPerBlock);
            }
        }

        /**
         @param numSamples sets the size of each analysed block
         */
        void setSamplesPerBlock (const int numSamples)
        {
            samplesPerBlock = numSamples;
            for (auto& data : channelDatas)
                data.setSamplesPerBlock (numSamples);
        }

        /**
         Push a block of audio samples into the outline buffer.
         */
        void pushBlock (const juce::AudioBuffer<float>& buffer, const int numSamples)
        {
            for (int i=0; i < buffer.getNumChannels(); ++i) {
                if (i < int (channelDatas.size())) {
                    channelDatas [size_t (i)].pushChannelData (buffer.getReadPointer (i), numSamples);
                }
            }
        }

        /**
         Returns the outline of a specific channel inside the bounds.
         @param path is a Path to be populated
         @param bounds the rectangle to paint within. The result is not clipped, if samples are exceeding 1.0, it may paint outside
         @param channel the index of the channel to paint
         @param numSamples is the number of sample blocks
         @return a path with a single channel outline (min to max)
         */
        void getChannelOutline (juce::Path& path, const juce::Rectangle<float> bounds, const int channel, const int numSamples) const
        {
            if (channel < int (channelDatas.size()))
                return channelDatas [size_t (channel)].getChannelOutline (path, bounds, numSamples);
        }

        /**
         This returns the outlines of each channel, splitting the bounds into equal sized rows
         @param path is a Path to be populated
         @param bounds the rectangle to paint within. The result is not clipped, if samples are exceeding 1.0, it may paint outside
         @param numSamples is the number of sample blocks
         @return a path with a single channel outline (min to max)
         */
        void getChannelOutline (juce::Path& path, const juce::Rectangle<float> bounds, const int numSamples) const
        {
            juce::Rectangle<float>  b (bounds);
            const int   numChannels = static_cast<int> (channelDatas.size());
            const float h           = bounds.getHeight() / numChannels;

            for (int i=0; i < numChannels; ++i) {
                getChannelOutline (path, b.removeFromTop (h) , i, numSamples);
            }
        }


    };
    /*@}*/
}
