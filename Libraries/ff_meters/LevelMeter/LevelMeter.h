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

    LevelMeter.h
    Created: 5 Apr 2016 9:49:54am
    Author:  Daniel Walz

 ==============================================================================
*/

#pragma once

namespace foleys
{

/** @addtogroup ff_meters */
/*@{*/

class LevelMeterLookAndFeel;

//==============================================================================
/*
 \class LevelMeter
 \brief A component to display live gain and RMS readings

 This class is used to display a level reading. It supports max and RMS levels.
 You can also set a reduction value to display, the definition of that value is up to you.
*/
class LevelMeter    : public juce::Component, private juce::Timer
{
public:

    enum MeterFlags
    {
        Default         = 0x0000, /**< Default is showing all channels in the LevelMeterSource without a border */
        Horizontal      = 0x0001, /**< Displays the level bars horizontally */
        Vintage         = 0x0002, /**< Switches to a special mode of old school meters (to come) */
        SingleChannel   = 0x0004, /**< Display only one channel meter. \see setSelectedChannel */
        HasBorder       = 0x0008, /**< Displays a rounded border around the meter. This is used with the default constructor */
        Reduction       = 0x0010, /**< This turns the bar into a reduction bar.
                                   The additional reduction bar is automatically added, as soon a reduction value < 1.0 is set
                                   in the LevelMeterSource. \see LevelMeterSource::setReductionLevel */
        Minimal         = 0x0020, /**< For a stereo meter, this tries to save space by showing only one line tickmarks in the middle and no max numbers */
        MaxNumber       = 0x0040  /**< To add level meter to Minimal, set this flag */
    };

    enum ColourIds
    {
        lmTextColour = 0x2200001,   /**< Colour for the numbers etc. */
        lmTextDeactiveColour,       /**< Unused, will eventually be removed */
        lmTextClipColour,           /**< Colour to print the max number if it has clipped */
        lmTicksColour,              /**< Colour for the tick marks */
        lmOutlineColour,            /**< Colour for the frame around all */
        lmBackgroundColour,         /**< Background colour */
        lmBackgroundClipColour,     /**< This is the colour of the clip indicator if it has clipped */
        lmMeterForegroundColour,    /**< Unused, will eventually be removed */
        lmMeterOutlineColour,       /**< Colour for the outlines of meter bars etc. */
        lmMeterBackgroundColour,    /**< Background colour for the actual meter bar and the max number */
        lmMeterMaxNormalColour,     /**< Text colour for the max number, if under warn threshold */
        lmMeterMaxWarnColour,       /**< Text colour for the max number, if between warn threshold and clip threshold */
        lmMeterMaxOverColour,       /**< Text colour for the max number, if above the clip threshold */
        lmMeterGradientLowColour,   /**< Colour for the meter bar under the warn threshold */
        lmMeterGradientMidColour,   /**< Colour for the meter bar in the warn area */
        lmMeterGradientMaxColour,   /**< Colour for the meter bar at the clip threshold */
        lmMeterReductionColour      /**< Colour for the reduction meter displayed within the meter */
    };

    /**
     These methods define a interface for the LookAndFeel class of juce.
     The LevelMeter needs a LookAndFeel, that implements these methods.
     There is a default implementation to be included in your custom LookAndFeel class, \see LookAndFeelMethods.h
     */
    class LookAndFeelMethods {
    public:
        virtual ~LookAndFeelMethods() {}

        /** Define your default colours in this callback */
        virtual void setupDefaultMeterColours () = 0;

        /** Call this to create the cached ColourGradients after changing colours of the meter gradients */
        virtual void updateMeterGradients () = 0;

        /** Override this to change the inner rectangle in case you want to paint a border e.g. */
        virtual juce::Rectangle<float> getMeterInnerBounds (juce::Rectangle<float> bounds,
                                                            MeterFlags meterType) const = 0;

        /** Override this callback to define the placement of a meter channel. */
        virtual juce::Rectangle<float> getMeterBounds (juce::Rectangle<float> bounds,
                                                       MeterFlags meterType,
                                                       int numChannels,
                                                       int channel) const = 0;

        /** Override this callback to define the placement of the actual meter bar. */
        virtual juce::Rectangle<float> getMeterBarBounds (juce::Rectangle<float> bounds,
                                                          MeterFlags meterType) const = 0;

        /** Override this callback to define the placement of the tickmarks.
         To disable this feature return an empty rectangle. */
        virtual juce::Rectangle<float> getMeterTickmarksBounds (juce::Rectangle<float> bounds,
                                                                MeterFlags meterType) const = 0;

        /** Override this callback to define the placement of the clip indicator light.
         To disable this feature return an empty rectangle. */
        virtual juce::Rectangle<float> getMeterClipIndicatorBounds (juce::Rectangle<float> bounds,
                                                                    MeterFlags meterType) const = 0;


        /** Override this to draw background and if wanted a frame. If the frame takes space away, 
         it should return the reduced bounds */
        virtual juce::Rectangle<float> drawBackground (juce::Graphics&,
                                                       MeterFlags meterType,
                                                       juce::Rectangle<float> bounds) = 0;

        /** This is called to draw the actual numbers and bars on top of the static background */
        virtual void drawMeterBars (juce::Graphics&,
                                    MeterFlags meterType,
                                    juce::Rectangle<float> bounds,
                                    LevelMeterSource* source,
                                    int fixedNumChannels=-1,
                                    int selectedChannel=-1) = 0;

        /** This draws the static background of the whole level meter group with all channels */
        virtual void drawMeterBarsBackground (juce::Graphics&,
                                              MeterFlags meterType,
                                              juce::Rectangle<float> bounds,
                                              int numChannels,
                                              int fixedNumChannels=-1) = 0;

        /** This draws a group of informations representing one channel */
        virtual void drawMeterChannel (juce::Graphics&,
                                       MeterFlags meterType,
                                       juce::Rectangle<float> bounds,
                                       LevelMeterSource* source,
                                       int selectedChannel) = 0;

        /** This draws the static backgrounds representing one channel */
        virtual void drawMeterChannelBackground (juce::Graphics&,
                                                 MeterFlags meterType,
                                                 juce::Rectangle<float> bounds) = 0;

        /** This callback draws the actual level bar. The background has an extra callback */
        virtual void drawMeterBar (juce::Graphics&,
                                   MeterFlags meterType,
                                   juce::Rectangle<float> bounds,
                                   float rms, const float peak) = 0;
        
        /** This callback draws an reduction from top. Only triggered, if a reduction < 1.0 is set in the LevelMeterSource */
        virtual void drawMeterReduction (juce::Graphics& g,
                                         foleys::LevelMeter::MeterFlags meterType,
                                         juce::Rectangle<float> bounds,
                                         float reduction) = 0;

        /** This draws the background for the actual level bar */
        virtual void drawMeterBarBackground (juce::Graphics&,
                                             MeterFlags meterType,
                                             juce::Rectangle<float> bounds) = 0;

        /** This draws the tickmarks for the level scale. It is painted on the static background */
        virtual void drawTickMarks (juce::Graphics&,
                                    MeterFlags meterType,
                                    juce::Rectangle<float> bounds) = 0;

        /** This callback draws the clip indicator. The background has an extra callback */
        virtual void drawClipIndicator (juce::Graphics&,
                                        MeterFlags meterType,
                                        juce::Rectangle<float> bounds,
                                        bool hasClipped) = 0;

        /** This draws the background for the clip indicator LED */
        virtual void drawClipIndicatorBackground (juce::Graphics&,
                                                  MeterFlags meterType,
                                                  juce::Rectangle<float> bounds) = 0;

        /** Override this callback to define the placement of the max level.
         To disable this feature return an empty rectangle. */
        virtual juce::Rectangle<float> getMeterMaxNumberBounds (juce::Rectangle<float> bounds,
                                                                MeterFlags meterType) const = 0;

        /** This callback draws the number of maximum level. The background has an extra callback */
        virtual void drawMaxNumber (juce::Graphics&,
                                    MeterFlags meterType,
                                    juce::Rectangle<float> bounds,
                                    float maxGain) = 0;

        /** This draws the background for the maximum level display */
        virtual void drawMaxNumberBackground (juce::Graphics&,
                                              MeterFlags meterType,
                                              juce::Rectangle<float> bounds) = 0;

        /** This is called by the frontend to check, if the clip indicator was clicked (e.g. for reset) */
        virtual int hitTestClipIndicator (juce::Point<int> position,
                                          MeterFlags meterType,
                                          juce::Rectangle<float> bounds,
                                          const LevelMeterSource* source) const = 0;

        /** This is called by the frontend to check, if the maximum level number was clicked (e.g. for reset) */
        virtual int hitTestMaxNumber (juce::Point<int> position,
                                      MeterFlags meterType,
                                      juce::Rectangle<float> bounds,
                                      const LevelMeterSource* source) const = 0;
    };

    LevelMeter (MeterFlags type = HasBorder);
    ~LevelMeter () override;

    /**
     Allows to change the meter's configuration by setting a combination of MeterFlags
     */
    void setMeterFlags (MeterFlags type);

    void paint (juce::Graphics&) override;

    void resized () override;

    void visibilityChanged () override;

    void timerCallback () override;

    /**
     Set a LevelMeterSource to display. This separation is used, so the source can work in the processing and the 
     GUI can display the values.
     */
    void setMeterSource (foleys::LevelMeterSource* source);

    /**
     Set a specific channel to display. This is only useful, if MeterFlags::SingleChannel is set.
     */
    void setSelectedChannel (int c);

    /**
     If you don't know, how many channels will be in the processblock, you can set this number to avoid stretching
     the width of the channels.
     */
    void setFixedNumChannels (int numChannels);

    void setRefreshRateHz (int newRefreshRate);

    /**
     Unset the clip indicator flag for a channel. Use -1 to reset all clip indicators.
     */
    void clearClipIndicator (int channel=-1);

    /**
     Set the max level display back to -inf for a channel. Use -1 to reset all max levels.
     */
    void clearMaxLevelDisplay (int channel=-1);

    /**
     This lambda is called when the user clicks on a clip light. It is initially set to clear all clip lights
     and max level numbers.
     */
    std::function<void(foleys::LevelMeter& meter, int channel, juce::ModifierKeys mods)> onClipLightClicked;

    /**
     This lambda is called when the user clicks on a max level display. It is initially set to clear all clip lights
     and max level numbers.
     */
    std::function<void(foleys::LevelMeter& meter, int channel, juce::ModifierKeys mods)> onMaxLevelClicked;

    /**
     \internal
    */
    void mouseDown (const juce::MouseEvent& event) override;
    
    void parentHierarchyChanged() override;
    void lookAndFeelChanged() override;

    /**
     DEPRECATED: Instead of using the Listener, use the new lambdas:
     \see onMaxLevelClicked, onClipLightClicked

     This Listener interface is meant to implement behaviour if either the clip indicator or the max level text
     is clicked.

     An example implementation could look like this (+alt means clear all, else clear the clicked number):
     \code{.cpp}
     void clipLightClicked (LevelMeter* clickedMeter, const int channel, ModifierKeys mods) override
     {
         clickedMeter->clearClipIndicator (mods.isAltDown() ? -1 : channel);
     }
     
     void maxLevelClicked (LevelMeter* clickedMeter, const int channel, ModifierKeys mods) override
     {
         clickedMeter->clearMaxLevelDisplay (mods.isAltDown() ? -1 : channel);
     }
     \endcode
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        /**
         This is called, when the user clicks a clip indicator. It can be used to reset the clip indicator.
         To allow different behaviour, e.g. resetting only one indicator or even all meters spread over the UI.
         \see clearClipIndicator, maxLevelClicked
         */
        virtual void clipLightClicked (foleys::LevelMeter* meter, int channel, juce::ModifierKeys mods) = 0;
        /**
         This is called, when the user clicks a max level text. It can be used to reset the max number.
         \see clearMaxLevelDisplay, clipLightClicked
         */
        virtual void maxLevelClicked (foleys::LevelMeter* meter, int channel, juce::ModifierKeys mods)  = 0;
    };

    void addListener (foleys::LevelMeter::Listener*);

    void removeListener (foleys::LevelMeter::Listener*);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
    
    juce::WeakReference<foleys::LevelMeterSource> source;

    int                                   selectedChannel  = -1;
    int                                   fixedNumChannels = -1;
    MeterFlags                            meterType = HasBorder;
    int                                   refreshRate = 30;
    bool                                  useBackgroundImage = false;
    juce::Image                           backgroundImage;
    bool                                  backgroundNeedsRepaint = true;

    std::unique_ptr<LevelMeterLookAndFeel> fallbackLookAndFeel;
    LevelMeter::LookAndFeelMethods*        lmLookAndFeel = nullptr;

    juce::ListenerList<foleys::LevelMeter::Listener> listeners;
};

inline LevelMeter::MeterFlags operator|(LevelMeter::MeterFlags a, LevelMeter::MeterFlags b)
{return static_cast<LevelMeter::MeterFlags>(static_cast<int>(a) | static_cast<int>(b));}

/*@}*/

} // end namespace foleys
