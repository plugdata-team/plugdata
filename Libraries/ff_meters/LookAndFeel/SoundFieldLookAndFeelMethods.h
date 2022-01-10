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

    \file SoundFieldLookAndFeelMethods.h
    Author:  Daniel Walz

    To use the default implementation of your SoundField in your LookAndFeel,
    inherit your LookAndFeel class from SoundField::LookAndFeelMethods and
    include this file inside your class declaration in a public section.

    In the Constructor you might want to call setupDefaultMeterColours() to
    setup the default colour scheme.
 ==============================================================================
 */


// include this file inside the implementation of your LookAndFeel to get the default implementation instead of copying it there

void setupDefaultStereoFieldColours () override
{
    setColour (StereoFieldComponent::backgroundColour,    juce::Colour (0xff050a29));
    setColour (StereoFieldComponent::borderColour,        juce::Colours::silver);
    setColour (StereoFieldComponent::outlineColour,       juce::Colours::silver);
    setColour (StereoFieldComponent::gonioColour,         juce::Colours::silver);
    setColour (StereoFieldComponent::currentValuesColour, juce::Colours::silver);
    setColour (StereoFieldComponent::maxValuesColour,     juce::Colours::darkgrey);
}

void drawGonioBackground (juce::Graphics& g, juce::Rectangle<float> bounds, float margin, float border) override
{
    auto colour = findColour (StereoFieldComponent::backgroundColour);
    g.fillAll (colour);
    colour = findColour (StereoFieldComponent::borderColour);
    g.setColour (colour);
    g.drawRoundedRectangle (bounds.reduced (margin * 0.5f), margin * 0.5f, border);

    colour = findColour (StereoFieldComponent::outlineColour);
    g.setColour (colour);
    auto size = std::min (bounds.getWidth(), bounds.getHeight());
    auto oscBounds = bounds.withSizeKeepingCentre (size, size).reduced (10);
    g.drawEllipse (oscBounds.toFloat(), 1.0);
}

void drawGonioMeter (juce::Graphics& g,
                     juce::Rectangle<float> bounds,
                     const StereoFieldBuffer<float>& stereoBuffer,
                     int leftIdx, int rightIdx) override
{
    auto colour = findColour (StereoFieldComponent::gonioColour);
    g.setColour (colour);

    auto size = std::min (bounds.getWidth(), bounds.getHeight());
    auto oscBounds = bounds.withSizeKeepingCentre (size, size);
    auto osciloscope = stereoBuffer.getOscilloscope (512, oscBounds.toFloat(), leftIdx, rightIdx);
    g.strokePath (osciloscope, juce::PathStrokeType (1.0));
}

void drawStereoFieldBackground (juce::Graphics& g, juce::Rectangle<float> bounds, float margin, float border) override
{
    g.drawRoundedRectangle (bounds.reduced (margin * 0.5f), margin * 0.5f, border);
    auto graph = bounds.reduced (margin);
    juce::Path background;
    background.addRectangle (graph);
    background.addCentredArc (graph.getCentreX(), graph.getBottom(),
                              0.5f * graph.getWidth(), 0.5f * graph.getWidth(),
                              0.0f, 1.5f * juce::MathConstants<float>::pi, 2.5f * juce::MathConstants<float>::pi);
    background.addCentredArc (graph.getCentreX(), graph.getBottom(),
                              0.25f * graph.getWidth(), 0.25f * graph.getWidth(),
                              0.0f, 1.5f * juce::MathConstants<float>::pi, 2.5f * juce::MathConstants<float>::pi);
    const auto d = graph.getWidth() * 0.5f / std::sqrt (2.0f);
    background.addLineSegment (juce::Line<float> (graph.getCentreX(), graph.getBottom(),
                                                  graph.getCentreX() - d, graph.getBottom() - d), 1.0f);
    background.addLineSegment (juce::Line<float> (graph.getCentreX(), graph.getBottom(),
                                                  graph.getCentreX(), graph.getBottom() - graph.getWidth() * 0.5f), 1.0f);
    background.addLineSegment (juce::Line<float> (graph.getCentreX(), graph.getBottom(),
                                                  graph.getCentreX() + d, graph.getBottom() - d), 1.0f);
    g.strokePath (background, juce::PathStrokeType (1.0));

}

void drawStereoField ([[maybe_unused]] juce::Graphics& g,
                      [[maybe_unused]] juce::Rectangle<float> bounds,
                      [[maybe_unused]] const StereoFieldBuffer<float>&,
                      [[maybe_unused]] int leftIdx = 0,
                      [[maybe_unused]] int rightIdx = 1) override
{
    juce::ignoreUnused (g);
    juce::ignoreUnused (bounds);
    juce::ignoreUnused (leftIdx, rightIdx);
}

