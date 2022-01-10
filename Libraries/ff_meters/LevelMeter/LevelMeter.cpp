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

    LevelMeter.cpp
    Created: 5 Apr 2016 9:49:54am
    Author:  Daniel Walz

 ==============================================================================
*/

namespace foleys
{

LevelMeter::LevelMeter (MeterFlags type)
  : meterType       (type)
{
    lookAndFeelChanged();

    onMaxLevelClicked = [](FFAU::LevelMeter& meter, [[maybe_unused]] int channel, [[maybe_unused]] juce::ModifierKeys mods)
    {
        // default clear all indicators. Overwrite this lambda to change the behaviour
        meter.clearMaxLevelDisplay();
        meter.clearClipIndicator();
    };

    onClipLightClicked = [](FFAU::LevelMeter& meter, [[maybe_unused]] int channel, [[maybe_unused]] juce::ModifierKeys mods)
    {
        // default clear all indicators. Overwrite this lambda to change the behaviour
        meter.clearMaxLevelDisplay();
        meter.clearClipIndicator();
    };

    startTimerHz (refreshRate);
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

void LevelMeter::setMeterFlags (MeterFlags type)
{
    meterType = type;
}

void LevelMeter::setMeterSource (LevelMeterSource* src)
{
    source = src;
    repaint();
}

void LevelMeter::setSelectedChannel (int c)
{
    selectedChannel = c;
}

void LevelMeter::setFixedNumChannels (int numChannels)
{
    fixedNumChannels = numChannels;
}

void LevelMeter::setRefreshRateHz (int newRefreshRate)
{
    refreshRate = newRefreshRate;
    startTimerHz (refreshRate);
}

void LevelMeter::paint (juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState saved (g);

    const juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    int numChannels = source ? source->getNumChannels() : 1;
    if (useBackgroundImage)
    {
        // This seems to only speed up, if you use complex drawings on the background. For
        // "normal" vector graphics the image approach seems actually slower
        if (backgroundNeedsRepaint)
        {
            backgroundImage = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
            juce::Graphics backGraphics (backgroundImage);
            lmLookAndFeel->drawBackground (backGraphics, meterType, bounds);
            lmLookAndFeel->drawMeterBarsBackground (backGraphics, meterType, bounds, numChannels, fixedNumChannels);
            backgroundNeedsRepaint = false;
        }
        g.drawImageAt (backgroundImage, 0, 0);
        lmLookAndFeel->drawMeterBars (g, meterType, bounds, source, fixedNumChannels, selectedChannel);
    }
    else
    {
        lmLookAndFeel->drawBackground (g, meterType, bounds);
        lmLookAndFeel->drawMeterBarsBackground (g, meterType, bounds, numChannels, fixedNumChannels);
        lmLookAndFeel->drawMeterBars (g, meterType, bounds, source, fixedNumChannels, selectedChannel);
    }

    if (source)
    {
        source->decayIfNeeded();
    }
}

void LevelMeter::resized ()
{
    lmLookAndFeel->updateMeterGradients();
    backgroundNeedsRepaint = true;
}

void LevelMeter::visibilityChanged ()
{
    backgroundNeedsRepaint = true;
}

void LevelMeter::timerCallback ()
{
    if ((source && source->checkNewDataFlag()) || backgroundNeedsRepaint)
    {
        if (source)
            source->resetNewDataFlag();

        repaint();
    }
}

void LevelMeter::clearClipIndicator (int channel)
{
    if (source == nullptr)
        return;

    if (channel < 0)
        source->clearAllClipFlags();
    else
        source->clearClipFlag (channel);
}

void LevelMeter::clearMaxLevelDisplay (int channel)
{
    if (source == nullptr)
        return;

    if (channel < 0)
        source->clearAllMaxNums();
    else
        source->clearMaxNum (channel);
}

void LevelMeter::mouseDown (const juce::MouseEvent &event)
{
    if (source == nullptr)
        return;

    const juce::Rectangle<float> innerBounds = lmLookAndFeel->getMeterInnerBounds (getLocalBounds().toFloat(),
                                                                                   meterType);
    if (event.mods.isLeftButtonDown())
    {
        auto channel = lmLookAndFeel->hitTestClipIndicator (event.getPosition(),
                                                            meterType,
                                                            innerBounds,
                                                            source);
        if (channel >= 0)
        {
            listeners.call (&LevelMeter::Listener::clipLightClicked, this, channel, event.mods);
            if (onClipLightClicked)
                onClipLightClicked (*this, channel, event.mods);
        }

        channel = lmLookAndFeel->hitTestMaxNumber (event.getPosition(),
                                                   meterType,
                                                   innerBounds,
                                                   source);
        if (channel >= 0)
        {
            listeners.call (&LevelMeter::Listener::maxLevelClicked, this, channel, event.mods);
            if (onMaxLevelClicked)
                onMaxLevelClicked (*this, channel, event.mods);
        }
    }
}

void LevelMeter::addListener (LevelMeter::Listener* listener)
{
    listeners.add (listener);
}

void LevelMeter::removeListener (LevelMeter::Listener* listener)
{
    listeners.remove (listener);
}

void LevelMeter::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void LevelMeter::lookAndFeelChanged()
{
    if (auto* lnf = dynamic_cast<LookAndFeelMethods*> (&getLookAndFeel()))
    {
        lmLookAndFeel = lnf;
        fallbackLookAndFeel.reset();
    }
    else
    {
        if (fallbackLookAndFeel.get() == nullptr)
            fallbackLookAndFeel = std::make_unique<LevelMeterLookAndFeel>();

        lmLookAndFeel = fallbackLookAndFeel.get();
    }
}

} // namespace foleys
