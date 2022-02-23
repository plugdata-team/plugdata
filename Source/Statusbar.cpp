/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <memory>

#include "Statusbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"

struct LevelMeter : public Component, public Timer
{
    int numChannels = 2;
    StatusbarSource& source;

    explicit LevelMeter(StatusbarSource& statusbarSource) : source(statusbarSource)
    {
        startTimerHz(20);
    }

    void timerCallback() override
    {
        if (isShowing())
        {
            bool needsRepaint = false;
            for (int ch = 0; ch < numChannels; ch++)
            {
                auto newLevel = source.level[ch].load();

                if (!std::isfinite(newLevel))
                {
                    source.level[ch] = 0;
                    blocks[ch] = 0;
                    return;
                }

                float lvl = (float)std::exp(std::log(newLevel) / 3.0) * (newLevel > 0.002);
                auto numBlocks = roundToInt(totalBlocks * lvl);

                if (blocks[ch] != numBlocks)
                {
                    blocks[ch] = numBlocks;
                    needsRepaint = true;
                }
            }

            if (needsRepaint) repaint();
        }
    }

    void paint(Graphics& g) override
    {
        int height = getHeight() / 2;
        int width = getWidth() - 8;
        int x = 4;

        auto outerBorderWidth = 2.0f;
        auto spacingFraction = 0.03f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockWidth = (width - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockHeight = height - doubleOuterBorderWidth;
        auto blockRectWidth = (1.0f - 2.0f * spacingFraction) * blockWidth;
        auto blockRectSpacing = spacingFraction * blockWidth;
        auto blockCornerSize = 0.1f * blockWidth;
        auto c = findColour(Slider::thumbColourId);

        for (int ch = 0; ch < numChannels; ch++)
        {
            int y = ch * height;

            for (auto i = 0; i < totalBlocks; ++i)
            {
                if (i >= blocks[ch])
                    g.setColour(Colours::darkgrey);
                else
                    g.setColour(i < totalBlocks - 1 ? c : Colours::red);

                g.fillRoundedRectangle(x + outerBorderWidth + (i * blockWidth) + blockRectSpacing, y + outerBorderWidth, blockRectWidth, blockHeight, blockCornerSize);
            }
        }
    }

    int totalBlocks = 15;
    int blocks[2] = {0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

struct MidiBlinker : public Component, public Timer
{
    StatusbarSource& source;

    explicit MidiBlinker(StatusbarSource& statusbarSource) : source(statusbarSource)
    {
        startTimer(200);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colours::white);
        g.setFont(Font(11));
        g.drawText("MIDI", getLocalBounds().removeFromLeft(28), Justification::right);

        auto midiInRect = Rectangle<float>(38.0f, 7.0f, 15.0f, 3.0f);
        auto midiOutRect = Rectangle<float>(38.0f, 15.0f, 15.0f, 3.0f);

        g.setColour(findColour(ComboBox::outlineColourId));

        g.setColour(blinkMidiIn ? findColour(Slider::thumbColourId) : Colours::darkgrey);
        g.fillRoundedRectangle(midiInRect, 1.0f);

        g.setColour(blinkMidiOut ? findColour(Slider::thumbColourId) : Colours::darkgrey);
        g.fillRoundedRectangle(midiOutRect, 1.0f);
    }

    void timerCallback() override
    {
        if (source.midiReceived != blinkMidiIn)
        {
            blinkMidiIn = source.midiReceived;
            repaint();
        }
        if (source.midiSent != blinkMidiOut)
        {
            blinkMidiOut = source.midiSent;
            repaint();
        }
    }

    bool blinkMidiIn = false;
    bool blinkMidiOut = false;
};

Statusbar::Statusbar(PlugDataAudioProcessor& processor) : pd(processor)
{
    levelMeter = new LevelMeter(processor.statusbarSource);
    midiBlinker = new MidiBlinker(processor.statusbarSource);

    setWantsKeyboardFocus(true);

    locked.referTo(pd.locked);
    commandLocked.referTo(pd.commandLocked);
    zoomScale.referTo(pd.zoomScale);

    bypassButton = std::make_unique<TextButton>(Icons::Power);
    lockButton = std::make_unique<TextButton>(Icons::Lock);
    connectionStyleButton = std::make_unique<TextButton>(Icons::ConnectionStyle);
    connectionPathfind = std::make_unique<TextButton>(Icons::Wand);
    zoomIn = std::make_unique<TextButton>(Icons::ZoomIn);
    zoomOut = std::make_unique<TextButton>(Icons::ZoomOut);
    presentationButton = std::make_unique<TextButton>(Icons::Presentation);

    presentationButton->setTooltip("Presentation Mode");
    presentationButton->setClickingTogglesState(true);
    presentationButton->setConnectedEdges(12);
    presentationButton->setName("statusbar:presentation");
    presentationButton->getToggleStateValue().referTo(presentationMode);
    addAndMakeVisible(presentationButton.get());
    
    bypassButton->setTooltip("Bypass");
    bypassButton->setClickingTogglesState(true);
    bypassButton->setConnectedEdges(12);
    bypassButton->setName("statusbar:bypass");
    addAndMakeVisible(bypassButton.get());

    bypassButton->setToggleState(true, dontSendNotification);

    lockButton->setTooltip("Lock");
    lockButton->setClickingTogglesState(true);
    lockButton->setConnectedEdges(12);
    lockButton->setName("statusbar:lock");
    lockButton->getToggleStateValue().referTo(locked);
    lockButton->onClick = [this]() { lockButton->setButtonText(locked == true ? Icons::Lock : Icons::Unlock); };
    addAndMakeVisible(lockButton.get());

    lockButton->setButtonText(locked == true ? Icons::Lock : Icons::Unlock);

    connectionStyle.referTo(pd.settingsTree.getPropertyAsValue("ConnectionStyle", nullptr));

    connectionStyleButton->setTooltip("Enable segmented connections");
    connectionStyleButton->setClickingTogglesState(true);
    connectionStyleButton->setConnectedEdges(12);
    connectionStyleButton->setName("statusbar:connectionstyle");
    connectionStyleButton->getToggleStateValue().referTo(connectionStyle);

    connectionStyleButton->onClick = [this]() { connectionPathfind->setEnabled(connectionStyle == true); };

    addAndMakeVisible(connectionStyleButton.get());

    connectionPathfind->setTooltip("Find best connection path");
    connectionPathfind->setConnectedEdges(12);
    connectionPathfind->setName("statusbar:findpath");
    connectionPathfind->onClick = [this]()
    {
        dynamic_cast<ApplicationCommandManager*>(pd.getActiveEditor())->invokeDirectly(CommandIDs::ConnectionPathfind, true);
    };
    addAndMakeVisible(connectionPathfind.get());

    addAndMakeVisible(zoomLabel);
    zoomLabel.setText("100%", dontSendNotification);
    zoomLabel.setFont(Font(11));

    zoomIn->setTooltip("Zoom In");
    zoomIn->setConnectedEdges(12);
    zoomIn->setName("statusbar:zoomin");
    zoomIn->onClick = [this]() { zoom(true); };
    addAndMakeVisible(zoomIn.get());

    zoomOut->setTooltip("Zoom Out");
    zoomOut->setConnectedEdges(12);
    zoomOut->setName("statusbar:zoomout");
    zoomOut->onClick = [this]() { zoom(false); };

    addAndMakeVisible(zoomOut.get());

    addAndMakeVisible(volumeSlider);
    volumeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

    volumeSlider.setValue(0.75);
    volumeSlider.setRange(0.0f, 1.0f);
    volumeSlider.setName("statusbar:meter");

    volumeAttachment = std::make_unique<SliderParameterAttachment>(*pd.parameters.getParameter("volume"), volumeSlider, nullptr);

    enableAttachment = std::make_unique<ButtonParameterAttachment>(*pd.parameters.getParameter("enabled"), *bypassButton, nullptr);

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(midiBlinker);

    levelMeter->toBehind(&volumeSlider);

    setSize(getWidth(), statusbarHeight);

#if JUCE_LINUX
    startTimer(50);
#endif
}

Statusbar::~Statusbar()
{
    delete midiBlinker;
    delete levelMeter;

#if JUCE_LINUX
    stopTimer();
#endif
}

void Statusbar::resized()
{
    lockButton->setBounds(8, 0, getHeight(), getHeight());

    connectionStyleButton->setBounds(43, 0, getHeight(), getHeight());
    connectionPathfind->setBounds(70, 0, getHeight(), getHeight());

    zoomLabel.setBounds(110, 0, getHeight() * 2, getHeight());

    zoomIn->setBounds(146, 0, getHeight(), getHeight());
    zoomOut->setBounds(174, 0, getHeight(), getHeight());

    presentationButton->setBounds(215, 0, getHeight(), getHeight());
    
    bypassButton->setBounds(getWidth() - 30, 0, getHeight(), getHeight());

    levelMeter->setBounds(getWidth() - 133, 1, 100, getHeight());
    midiBlinker->setBounds(getWidth() - 190, 0, 70, getHeight());

    volumeSlider.setBounds(getWidth() - 133, 0, 100, getHeight());
}

// We don't get callbacks for the ctrl/command key on Linux, so we have to check it with a timer...
// This timer is only started on Linux
void Statusbar::timerCallback()
{
    if (ModifierKeys::getCurrentModifiers().isCommandDown() && locked == false)
    {
        commandLocked = true;
    }

    if (!ModifierKeys::getCurrentModifiers().isCommandDown() && commandLocked == true)
    {
        commandLocked = false;
    }
}

bool Statusbar::keyStateChanged(bool isKeyDown, Component*)
{
    // Lock when command is down
    auto mod = ComponentPeer::getCurrentModifiersRealtime();

    if (isKeyDown && mod.isCommandDown() && !lockButton->getToggleState())
    {
        commandLocked = true;
    }

    if (!mod.isCommandDown() && pd.commandLocked == true)
    {
        commandLocked = false;
    }

    return false;  //  Never claim this event!
}

void Statusbar::zoom(bool zoomIn)
{
    float value = static_cast<float>(zoomScale.getValue());

    // Zoom limits
    value = std::clamp(zoomIn ? value + 0.1f : value - 0.1f, 0.5f, 2.0f);

    // Round in case we zoomed with scrolling
    value = static_cast<float>(static_cast<int>(value * 10.)) / 10.;

    zoomScale = value;

    zoomLabel.setText(String(value * 100.0f) + "%", dontSendNotification);
}

void Statusbar::zoom(float zoomAmount)
{
    float value = static_cast<float>(zoomScale.getValue());
    value *= zoomAmount;

    // Zoom limits
    value = std::clamp(value, 0.5f, 2.0f);

    zoomScale = value;

    zoomLabel.setText(String(value * 100.0f, 1) + "%", dontSendNotification);
}

void Statusbar::defaultZoom()
{
    zoomScale = 1.0;

    zoomLabel.setText("100%", dontSendNotification);
}

StatusbarSource::StatusbarSource()
{
    level[0] = 0.0f;
    level[1] = 0.0f;
}

static bool hasRealEvents(MidiBuffer& buffer)
{
    for (auto event : buffer)
    {
        auto m = event.getMessage();
        if (!m.isSysEx())
        {
            return true;
        }
    }

    return false;
}

void StatusbarSource::processBlock(const AudioBuffer<float>& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut)
{
    auto** channelData = buffer.getArrayOfReadPointers();

    for (int ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        auto localLevel = level[ch & 1].load();

        for (int n = 0; n < buffer.getNumSamples(); n++)
        {
            float s = std::abs(channelData[ch][n]);

            const float decayFactor = 0.99992f;

            if (s > localLevel)
                localLevel = s;
            else if (localLevel > 0.001f)
                localLevel *= decayFactor;
            else
                localLevel = 0;
        }

        level[ch & 1] = localLevel;
    }

    auto now = Time::getCurrentTime();

    auto hasInEvents = hasRealEvents(midiIn);
    auto hasOutEvents = hasRealEvents(midiOut);

    if (!hasInEvents && (now - lastMidiIn).inMilliseconds() > 700)
    {
        midiReceived = false;
    }
    else if (hasInEvents)
    {
        midiReceived = true;
        lastMidiIn = now;
    }

    if (!hasOutEvents && (now - lastMidiOut).inMilliseconds() > 700)
    {
        midiSent = false;
    }
    else if (hasOutEvents)
    {
        midiSent = true;
        lastMidiOut = now;
    }
}

void StatusbarSource::prepareToPlay(int nChannels)
{
    numChannels = nChannels;
}
