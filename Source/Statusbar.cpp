/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Statusbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"

#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"

#include "Utility/ArrowPopupMenu.h"


VolumeSlider::VolumeSlider()
    : Slider(Slider::LinearHorizontal, Slider::NoTextBox)
{
    setSliderSnapsToMousePosition(false);
}

void VolumeSlider::resized()
{
    setMouseDragSensitivity(getWidth() - (margin * 2));
}

void VolumeSlider::paint(Graphics& g)
{
    auto backgroundColour = findColour(PlugDataColour::levelMeterThumbColourId);
    
    auto value = getValue();
    auto thumbSize = getHeight() * 0.7f;
    auto position = Point<float>(margin + (value * (getWidth() - (margin * 2))), getHeight() * 0.5f);
    auto thumb = Rectangle<float>(thumbSize, thumbSize).withCentre(position);
    g.setColour(backgroundColour.withAlpha(0.8f));
    g.fillEllipse(thumb);
}



class LevelMeter : public Component
    , public StatusbarSource::Listener {
    float audioLevel[2] = {0.0f, 0.0f};
    float peekLevel[2] = {0.0f, 0.0f};

    int numChannels = 2;

    bool clipping[2] = { false, false };

public:
    LevelMeter() = default;

    void audioLevelChanged(float level[2], float peak[2]) override
    {
        bool hasChanged = false;
        for (int i = 0; i < 2; i++) {
            if (audioLevel[i] != level[i] || peekLevel[i] != peak[i]) {
                hasChanged = true;
                audioLevel[i] = level[i];
                peekLevel[i] = peak[i];
                if (level[i] >= 1.0f)
                    clipping[i] = true;
                else
                    clipping[i] = false;
            }
        }
    if (isShowing() && hasChanged)
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto height = getHeight() / 4.0f;
        auto barHeight = height * 0.6f;
        auto halfBarHeight = barHeight * 0.5f;
        auto width = getWidth() - 8.0f;
        auto x = 4.0f;

        auto outerBorderWidth = 2.0f;
        auto spacingFraction = 0.08f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;
        auto bgHeight = getHeight() - doubleOuterBorderWidth;
        auto bgWidth = width - doubleOuterBorderWidth;
        auto meterWidth = width - bgHeight;
        auto barWidth = meterWidth - 2;
        auto leftOffset = x + (bgHeight * 0.5f);
        
        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(x + outerBorderWidth, outerBorderWidth, bgWidth, bgHeight, bgHeight * 0.5f);

        for (int ch = 0; ch < numChannels; ch++) {
            auto barYPos = outerBorderWidth + ((ch + 1) * (bgHeight / 3.0f)) - halfBarHeight;
            auto barLength = jmin(audioLevel[ch] * barWidth, barWidth);
            auto peekPos = jmin(peekLevel[ch] * barWidth, barWidth);

            if (peekPos > 1) {
                g.setColour(clipping[ch] ? Colours::red : findColour(PlugDataColour::levelMeterActiveColourId));
                g.fillRect(leftOffset, barYPos, barLength, barHeight);
                g.fillRect(leftOffset + peekPos + 1, barYPos, 1.0f, barHeight);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

class MidiBlinker : public Component
    , public StatusbarSource::Listener {

public:
    void paint(Graphics& g) override
    {
        Fonts::drawText(g, "MIDI", getLocalBounds().removeFromLeft(28), findColour(ComboBox::textColourId), 11, Justification::centredRight);

        auto midiInRect = Rectangle<float>(38.0f, 8.0f, 15.0f, 3.0f);
        auto midiOutRect = Rectangle<float>(38.0f, 17.0f, 15.0f, 3.0f);

        g.setColour(blinkMidiIn ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(midiInRect, 1.0f);

        g.setColour(blinkMidiOut ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(midiOutRect, 1.0f);
    }

    void midiReceivedChanged(bool midiReceived) override
    {
        blinkMidiIn = midiReceived;
        repaint();
    };

    void midiSentChanged(bool midiSent) override
    {
        blinkMidiOut = midiSent;
        repaint();
    };

    bool blinkMidiIn = false;
    bool blinkMidiOut = false;
};

Statusbar::Statusbar(PluginProcessor* processor)
    : pd(processor)
{
    levelMeter = new LevelMeter();
    midiBlinker = new MidiBlinker();

    pd->statusbarSource->addListener(levelMeter);
    pd->statusbarSource->addListener(midiBlinker);
    pd->statusbarSource->addListener(this);

    setWantsKeyboardFocus(true);

    oversampleSelector.setTooltip("Set oversampling");
    oversampleSelector.getProperties().set("FontScale", 0.5f);
    oversampleSelector.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

    oversampleSelector.setButtonText(String(1 << pd->oversampling) + "x");

    oversampleSelector.onClick = [this]() {
        PopupMenu menu;
        menu.addItem(1, "1x");
        menu.addItem(2, "2x");
        menu.addItem(3, "4x");
        menu.addItem(4, "8x");

        auto* editor = pd->getActiveEditor();
        ArrowPopupMenu::showMenuAsync(&menu, PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&oversampleSelector).withParentComponent(editor),
            [this](int result) {
                if (result != 0) {
                    oversampleSelector.setButtonText(String(1 << (result - 1)) + "x");
                    pd->setOversampling(result - 1);
                }
            });
    };
    addAndMakeVisible(oversampleSelector);

    powerButton.setButtonText(Icons::Power);
    connectionStyleButton.setButtonText(Icons::ConnectionStyle);
    connectionPathfind.setButtonText(Icons::Wand);
    protectButton.setButtonText(Icons::Protection);
    centreButton.setButtonText(Icons::Centre);
    fitAllButton.setButtonText(Icons::FitAll);

    powerButton.setTooltip("Enable/disable DSP");
    powerButton.setClickingTogglesState(true);
    powerButton.getProperties().set("Style", "SmallIcon");
    addAndMakeVisible(powerButton);

    powerButton.onClick = [this]() { powerButton.getToggleState() ? pd->startDSP() : pd->releaseDSP(); };

    powerButton.setToggleState(pd_getdspstate(), dontSendNotification);

    centreButton.setTooltip("Move view to origin");
    centreButton.getProperties().set("Style", "SmallIcon");
    centreButton.onClick = [this]() {
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->jumpToOrigin();
        }
    };

    addAndMakeVisible(centreButton);

    fitAllButton.setTooltip("Zoom to fit all");
    fitAllButton.getProperties().set("Style", "SmallIcon");
    fitAllButton.onClick = [this]() {
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->zoomToFitAll();
        }
    };

    addAndMakeVisible(fitAllButton);

    connectionStyleButton.setTooltip("Enable segmented connections");
    connectionStyleButton.setClickingTogglesState(true);
    connectionStyleButton.getProperties().set("Style", "SmallIcon");
    connectionStyleButton.onClick = [this]() {
        bool segmented = connectionStyleButton.getToggleState();
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());

        auto* cnv = editor->getCurrentCanvas();

        // cnv->patch.startUndoSequence("ChangeSegmentedPaths");

        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            connection->setSegmented(segmented);
        }

        // cnv->patch.endUndoSequence("ChangeSegmentedPaths");
    };
    addAndMakeVisible(connectionStyleButton);

    connectionPathfind.setTooltip("Find best connection path");
    connectionPathfind.getProperties().set("Style", "SmallIcon");
    connectionPathfind.onClick = [this]() { dynamic_cast<ApplicationCommandManager*>(pd->getActiveEditor())->invokeDirectly(CommandIDs::ConnectionPathfind, true); };
    addAndMakeVisible(connectionPathfind);

    protectButton.setTooltip("Clip output signal and filter non-finite values");
    protectButton.getProperties().set("Style", "SmallIcon");
    protectButton.setClickingTogglesState(true);
    protectButton.setToggleState(SettingsFile::getInstance()->getProperty<int>("protected"), dontSendNotification);
    protectButton.onClick = [this]() {
        int state = protectButton.getToggleState();
        pd->setProtectedMode(state);
        SettingsFile::getInstance()->setProperty("protected", state);
    };
    addAndMakeVisible(protectButton);

    addAndMakeVisible(volumeSlider);

    volumeAttachment = std::make_unique<SliderParameterAttachment>(*dynamic_cast<RangedAudioParameter*>(pd->getParameters()[0]), volumeSlider, nullptr);
    
    volumeSlider.setRange(0.0f, 1.0f);
    volumeSlider.setValue(0.8f);
    volumeSlider.setDoubleClickReturnValue(true, 0.8f);

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(midiBlinker);

    levelMeter->toBehind(&volumeSlider);

    overlayButton.setButtonText(Icons::Eye);
    overlaySettingsButton.setButtonText(Icons::ThinDown);

    overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>();
    overlaySettingsButton.onClick = [this]() {
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());
        overlayDisplaySettings->show(editor, editor->getLocalArea(this, overlaySettingsButton.getBounds()));
    };

    snapEnableButton.setButtonText(Icons::Magnet);
    snapSettingsButton.setButtonText(Icons::ThinDown);

    snapEnableButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getPropertyAsValue("grid_enabled"));

    snapSettings = std::make_unique<SnapSettings>();

    snapSettingsButton.onClick = [this]() {
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());
        snapSettings->show(editor, editor->getLocalArea(this, snapSettingsButton.getBounds()));
    };

    // overlay button
    overlayButton.getProperties().set("Style", "SmallIcon");
    overlaySettingsButton.getProperties().set("Style", "SmallIcon");

    overlayButton.setClickingTogglesState(true);
    overlaySettingsButton.setClickingTogglesState(false);

    addAndMakeVisible(overlayButton);
    addAndMakeVisible(overlaySettingsButton);

    overlayButton.setConnectedEdges(Button::ConnectedOnRight);
    overlaySettingsButton.setConnectedEdges(Button::ConnectedOnLeft);

    overlayButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").getPropertyAsValue("alt_mode", nullptr));
    overlayButton.setTooltip(String("Show overlays"));
    overlaySettingsButton.setTooltip(String("Overlay settings"));

    // snapping button
    snapEnableButton.getProperties().set("Style", "SmallIcon");
    snapSettingsButton.getProperties().set("Style", "SmallIcon");

    snapEnableButton.setClickingTogglesState(true);
    snapSettingsButton.setClickingTogglesState(false);

    addAndMakeVisible(snapEnableButton);
    addAndMakeVisible(snapSettingsButton);

    snapEnableButton.setConnectedEdges(Button::ConnectedOnRight);
    snapSettingsButton.setConnectedEdges(Button::ConnectedOnLeft);

    snapEnableButton.setTooltip(String("Enable snapping"));
    snapSettingsButton.setTooltip(String("Snap settings"));

    setSize(getWidth(), statusbarHeight);
}

Statusbar::~Statusbar()
{
    pd->statusbarSource->removeListener(levelMeter);
    pd->statusbarSource->removeListener(midiBlinker);
    pd->statusbarSource->removeListener(this);

    delete midiBlinker;
    delete levelMeter;
}

void Statusbar::propertyChanged(String name, var value)
{
}

void Statusbar::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, 0.5f, static_cast<float>(getWidth()), 0.5f);
}

void Statusbar::resized()
{
    int pos = 0;
    auto position = [this, &pos](int width, bool inverse = false) -> int {
        int result = 8 + pos;
        pos += width + 3;
        return inverse ? getWidth() - pos : result;
    };

    connectionStyleButton.setBounds(position(getHeight()), 0, getHeight(), getHeight());
    connectionPathfind.setBounds(position(getHeight()), 0, getHeight(), getHeight());

    position(5); // Seperator

    centreButton.setBounds(position(getHeight()), 0, getHeight(), getHeight());
    fitAllButton.setBounds(position(getHeight()), 0, getHeight(), getHeight());
    position(7); // Seperator

    overlayButton.setBounds(position(getHeight()), 0, getHeight(), getHeight());
    overlaySettingsButton.setBounds(overlayButton.getBounds().translated(overlayButton.getWidth() - 1, 0).withTrimmedRight(8));

    position(getHeight() - 8);

    snapEnableButton.setBounds(position(getHeight()), 0, getHeight(), getHeight());
    snapSettingsButton.setBounds(snapEnableButton.getBounds().translated(snapEnableButton.getWidth() - 1, 0).withTrimmedRight(8));

    pos = 5; // reset position for elements on the right

    protectButton.setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    powerButton.setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    // TODO: combine these both into one
    int levelMeterPosition = position(120, true);
    levelMeter->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);
    volumeSlider.setBounds(levelMeterPosition, 2, 120, getHeight() - 4);

    // Offset to make text look centred
    oversampleSelector.setBounds(position(getHeight(), true) + 3, 1, getHeight() - 2, getHeight() - 2);

    midiBlinker->setBounds(position(55, true), 0, 55, getHeight());
}

void Statusbar::audioProcessedChanged(bool audioProcessed)
{
    auto colour = findColour(audioProcessed ? PlugDataColour::levelMeterActiveColourId : PlugDataColour::signalColourId);

    powerButton.setColour(TextButton::textColourOnId, colour);
}

StatusbarSource::StatusbarSource() : numChannels(0)
{
    startTimerHz(30);
}

static bool hasRealEvents(MidiBuffer& buffer)
{
    return std::any_of(buffer.begin(), buffer.end(),
        [](auto const& event) {
            return !event.getMessage().isSysEx();
        });
}

void StatusbarSource::setSampleRate(const double newSampleRate)
{
    sampleRate = static_cast<int>(newSampleRate);
}

void StatusbarSource::processBlock(AudioBuffer<float> const& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut, int channels)
{
    auto const* const* channelData = buffer.getArrayOfReadPointers();

    if (channels == 1) {
        level[1] = 0;
    } else if (channels == 0) {
        level[0] = 0;
        level[1] = 0;
    }

    int delay = sampleRate * 1.7;

    for (int ch = 0; ch < 2; ch++) {
        auto localLevel = level[ch].load();
        auto localPeakHold = peakHold[ch].load();
        float peak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());

        for (int n = 0; n < buffer.getNumSamples(); n++) {
            float const decayFactor = 0.99996f;

            if (peak > localLevel) {
                localLevel = peak;
            }

            if (peak > localPeakHold) {
                localPeakHold = peak;
                peakHoldDelay[ch] = delay;
            }

            if (localLevel > 0.001f) {
                localLevel *= decayFactor;
            } else {
                localLevel = 0;
            }

            if (peakHoldDelay[ch] >= 0) {
                peakHoldDelay[ch]--;
            } else {
                localPeakHold *= decayFactor;
            }
        }

        level[ch] = localLevel;
        peakHold[ch] = localPeakHold;
    }

    auto nowInMs = Time::getCurrentTime().getMillisecondCounter();
    auto hasInEvents = hasRealEvents(midiIn);
    auto hasOutEvents = hasRealEvents(midiOut);

    lastAudioProcessedTime = nowInMs;

    if (hasOutEvents)
        lastMidiSentTime = nowInMs;
    if (hasInEvents)
        lastMidiReceivedTime = nowInMs;
}

void StatusbarSource::prepareToPlay(int nChannels)
{
    numChannels = nChannels;
}

void StatusbarSource::timerCallback()
{
    auto currentTime = Time::getCurrentTime().getMillisecondCounter();

    auto hasReceivedMidi = currentTime - lastMidiReceivedTime < 700;
    auto hasSentMidi = currentTime - lastMidiSentTime < 700;
    auto hasProcessedAudio = currentTime - lastAudioProcessedTime < 700;

    if (hasReceivedMidi != midiReceivedState) {
        midiReceivedState = hasReceivedMidi;
        for (auto* listener : listeners)
            listener->midiReceivedChanged(hasReceivedMidi);
    }
    if (hasSentMidi != midiSentState) {
        midiSentState = hasSentMidi;
        for (auto* listener : listeners)
            listener->midiSentChanged(hasSentMidi);
    }
    if (hasProcessedAudio != audioProcessedState) {
        audioProcessedState = hasProcessedAudio;
        for (auto* listener : listeners)
            listener->audioProcessedChanged(hasProcessedAudio);
    }

    float currentLevel[2] = { level[0].load(), level[1].load() };
    float currentPeak[2] = { peakHold[0].load(), peakHold[1].load() };
    for (auto* listener : listeners) {
        listener->audioLevelChanged(currentLevel, currentPeak);
        listener->timerCallback();
    }
}

void StatusbarSource::addListener(Listener* l)
{
    listeners.push_back(l);
}

void StatusbarSource::removeListener(Listener* l)
{
    listeners.erase(std::remove(listeners.begin(), listeners.end(), l), listeners.end());
}
