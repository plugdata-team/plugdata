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
 
 StereoFieldComponent.h
 Created: 2 Jan 2018 00:16:54pm
 Author:  Daniel Walz
 
 ==============================================================================
 */

#pragma once


namespace foleys
{
    
    /** @addtogroup ff_meters */
    /*@{*/
    
    /**
     \class StereoFieldComponent
     
     This class implements a circular buffer to buffer audio samples.
     At any time the GUI can ask for a stereo field visualisation of
     two neightbouring channels.
     */
    class StereoFieldComponent : public juce::Component
    {
    public:
        enum
        {
            GonioMeter = 0,
            StereoField
        };

        enum ColourIds
        {
            backgroundColour    = 0x2200101, /**< Colour for the numbers etc. */
            borderColour,                    /**< Colour for the numbers etc. */
            outlineColour,                   /**< Colour for the numbers etc. */
            gonioColour,                     /**< Colour for the numbers etc. */
            currentValuesColour,             /**< Colour for the numbers etc. */
            maxValuesColour                  /**< Colour for the numbers etc. */
        };

        class LookAndFeelMethods
        {
        public:
            virtual ~LookAndFeelMethods() {}

            virtual void setupDefaultStereoFieldColours () = 0;

            virtual void drawGonioBackground (juce::Graphics& g, juce::Rectangle<float> bounds, float margin, float border) = 0;

            virtual void drawGonioMeter (juce::Graphics& g,
                                         juce::Rectangle<float> bounds,
                                         const StereoFieldBuffer<float>& stereoBuffer,
                                         int leftIdx, int rightIdx) = 0;

            virtual void drawStereoFieldBackground (juce::Graphics& g, juce::Rectangle<float> bounds, float margin, float border) = 0;

            virtual void drawStereoField (juce::Graphics& g,
                                          juce::Rectangle<float> bounds,
                                          const StereoFieldBuffer<float>&,
                                          int leftIdx = 0, int rightIdx = 1) = 0;
        };

        StereoFieldComponent (StereoFieldBuffer<float>& stereo)
        : stereoBuffer (stereo)
        {
        }

        void paint (juce::Graphics& g) override
        {
            juce::Graphics::ScopedSaveState saved (g);

            if (auto* lnf = dynamic_cast<StereoFieldComponent::LookAndFeelMethods*> (&getLookAndFeel()))
            {
                if (type == GonioMeter)
                {
                    auto bounds = getLocalBounds().toFloat();
                    lnf->drawGonioBackground (g, bounds, margin, border);
                    lnf->drawGonioMeter (g, bounds.reduced (margin), stereoBuffer, 0, 1);
                }
                else if (type == StereoField)
                {
                    auto bounds = getLocalBounds().toFloat();
                    lnf->drawStereoFieldBackground (g, bounds, margin, border);
                    lnf->drawStereoField (g, bounds.reduced (margin), stereoBuffer, 0, 1);
                }
            }
            else
            {
                // This LookAndFeel is missing the StereoFieldComponent::LookAndFeelMethods.
                // If you work with the default LookAndFeel, set an instance of
                // LevelMeterLookAndFeel as LookAndFeel of this component.
                //
                // If you write a LookAndFeel from scratch, inherit also
                // StereoFieldComponent::LookAndFeelMethods
                jassertfalse;
            }

        }

    private:
        StereoFieldBuffer<float>& stereoBuffer;
        int                       type = GonioMeter;

        float margin = 5.0f;
        float border = 2.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldComponent)


    };
    /*@}*/
}
