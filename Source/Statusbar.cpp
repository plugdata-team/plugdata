/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Statusbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"

class LevelMeter : public Component
    , public StatusbarSource::Listener {
    int totalBlocks = 15;
    int blocks[2] = { 0 };

    int numChannels = 2;

public:
    LevelMeter() {};

    void audioLevelChanged(float level[2]) override
    {

        bool needsRepaint = false;

        for (int ch = 0; ch < numChannels; ch++) {
            auto chLevel = level[ch];

            if (!std::isfinite(chLevel)) {
                blocks[ch] = 0;
                return;
            }

            auto lvl = static_cast<float>(std::exp(std::log(chLevel) / 3.0) * (chLevel > 0.002));
            auto numBlocks = roundToInt(totalBlocks * lvl);

            if (blocks[ch] != numBlocks) {
                blocks[ch] = numBlocks;
                needsRepaint = true;
            }
        }

        if (needsRepaint && isShowing())
            repaint();
    }

    void paint(Graphics& g) override
    {
        auto height = getHeight() / 2.0f;
        auto width = getWidth() - 8.0f;
        auto x = 4.0f;

        auto outerBorderWidth = 2.0f;
        auto spacingFraction = 0.08f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockWidth = (width - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockHeight = height - doubleOuterBorderWidth;
        auto blockRectWidth = (1.0f - 2.0f * spacingFraction) * blockWidth;
        auto blockRectSpacing = spacingFraction * blockWidth;
        auto c = findColour(PlugDataColour::levelMeterActiveColourId);

        for (int ch = 0; ch < numChannels; ch++) {
            auto y = ch * height;

            for (auto i = 0; i < totalBlocks; ++i) {
                if (i >= blocks[ch])
                    g.setColour(findColour(PlugDataColour::levelMeterInactiveColourId));
                else
                    g.setColour(i < totalBlocks - 1 ? c : Colours::red);

                if (i == 0 || i == totalBlocks - 1) {
                    bool curveTop = ch == 0;
                    bool curveLeft = i == 0;

                    auto roundedBlockPath = Path();
                    roundedBlockPath.addRoundedRectangle(x + outerBorderWidth + (i * blockWidth) + blockRectSpacing, y + outerBorderWidth, blockRectWidth, blockHeight, 4.0f, 4.0f, curveTop && curveLeft, curveTop && !curveLeft, !curveTop && curveLeft, !curveTop && !curveLeft);
                    g.fillPath(roundedBlockPath);
                } else {
                    g.fillRect(x + outerBorderWidth + (i * blockWidth) + blockRectSpacing, y + outerBorderWidth, blockRectWidth, blockHeight);
                }
            }
        }

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(x + outerBorderWidth, outerBorderWidth, width - doubleOuterBorderWidth, getHeight() - doubleOuterBorderWidth, 4.0f, 1.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

class MidiBlinker : public Component
    , public StatusbarSource::Listener {

public:
    void paint(Graphics& g) override
    {
        PlugDataLook::drawText(g, "MIDI", getLocalBounds().removeFromLeft(28), findColour(ComboBox::textColourId), 11, Justification::centredRight);

        auto midiInRect = Rectangle<float>(38.0f, 8.0f, 15.0f, 3.0f);
        auto midiOutRect = Rectangle<float>(38.0f, 17.0f, 15.0f, 3.0f);

        g.setColour(blinkMidiIn ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterInactiveColourId));
        g.fillRoundedRectangle(midiInRect, 1.0f);

        g.setColour(blinkMidiOut ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterInactiveColourId));
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

    pd->statusbarSource.addListener(levelMeter);
    pd->statusbarSource.addListener(midiBlinker);
    pd->statusbarSource.addListener(this);

    setWantsKeyboardFocus(true);

    commandLocked.referTo(pd->commandLocked);

    locked.addListener(this);
    commandLocked.addListener(this);

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
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&oversampleSelector).withParentComponent(editor),
            [this](int result) {
                if (result != 0) {
                    oversampleSelector.setButtonText(String(1 << (result - 1)) + "x");
                    pd->setOversampling(result - 1);
                }
            });
    };
    addAndMakeVisible(oversampleSelector);

    powerButton = std::make_unique<TextButton>(Icons::Power);
    lockButton = std::make_unique<TextButton>(Icons::Lock);
    connectionStyleButton = std::make_unique<TextButton>(Icons::ConnectionStyle);
    connectionPathfind = std::make_unique<TextButton>(Icons::Wand);
    presentationButton = std::make_unique<TextButton>(Icons::Presentation);
    gridButton = std::make_unique<TextButton>(Icons::Grid);

    presentationButton->setTooltip("Presentation Mode");
    presentationButton->setClickingTogglesState(true);
    presentationButton->setConnectedEdges(12);
    presentationButton->getProperties().set("Style", "SmallIcon");
    presentationButton->getToggleStateValue().referTo(presentationMode);

    presentationButton->onClick = [this]() {
        // When presenting we are always locked
        // A bit different from Max's presentation mode
        if (presentationButton->getToggleState()) {
            locked = var(true);
        }
    };

    addAndMakeVisible(presentationButton.get());

    powerButton->setTooltip("Mute");
    powerButton->setClickingTogglesState(true);
    powerButton->setConnectedEdges(12);
    powerButton->getProperties().set("Style", "SmallIcon");
    addAndMakeVisible(powerButton.get());

    gridButton->setTooltip("Enable grid");
    gridButton->setConnectedEdges(12);
    gridButton->getProperties().set("Style", "SmallIcon");

    gridButton->onClick = [this]() {
        PopupMenu gridSelector;
        int gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
        gridSelector.addItem("Absolute grid", true, gridEnabled == 2, [this]() {
            gridButton->setColour(TextButton::textColourOffId, Colours::orange);
            SettingsFile::getInstance()->setProperty("grid_enabled", 2);
        });
        gridSelector.addItem("Relative grid", true, gridEnabled == 1, [this]() {
            gridButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::gridLineColourId));
            SettingsFile::getInstance()->setProperty("grid_enabled", 1);
        });
        gridSelector.addItem("No grid", true, gridEnabled == 0, [this]() {
            gridButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
            SettingsFile::getInstance()->setProperty("grid_enabled", 0);
        });

        gridSelector.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withTargetComponent(gridButton.get()).withParentComponent(pd->getActiveEditor()));
    };

    // Initialise grid state
    propertyChanged("grid_enabled", SettingsFile::getInstance()->getProperty<int>("grid_enabled"));

    addAndMakeVisible(gridButton.get());

    powerButton->onClick = [this]() { powerButton->getToggleState() ? pd->startDSP() : pd->releaseDSP(); };

    powerButton->setToggleState(pd_getdspstate(), dontSendNotification);

    lockButton->setTooltip("Edit Mode");
    lockButton->setClickingTogglesState(true);
    lockButton->setConnectedEdges(12);
    lockButton->getProperties().set("Style", "SmallIcon");
    lockButton->getToggleStateValue().referTo(locked);
    addAndMakeVisible(lockButton.get());
    lockButton->setButtonText(locked == var(true) ? Icons::Lock : Icons::Unlock);
    lockButton->onClick = [this]() {
        if (static_cast<bool>(presentationMode.getValue())) {
            presentationMode = false;
        }
    };

    connectionStyleButton->setTooltip("Enable segmented connections");
    connectionStyleButton->setClickingTogglesState(true);
    connectionStyleButton->setConnectedEdges(12);
    connectionStyleButton->getProperties().set("Style", "SmallIcon");
    connectionStyleButton->onClick = [this]() {
        bool segmented = connectionStyleButton->getToggleState();
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());

        auto* cnv = editor->getCurrentCanvas();

        // cnv->patch.startUndoSequence("ChangeSegmentedPaths");

        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            connection->setSegmented(segmented);
        }

        // cnv->patch.endUndoSequence("ChangeSegmentedPaths");
    };

    addAndMakeVisible(connectionStyleButton.get());

    connectionPathfind->setTooltip("Find best connection path");
    connectionPathfind->setConnectedEdges(12);
    connectionPathfind->getProperties().set("Style", "SmallIcon");
    connectionPathfind->onClick = [this]() { dynamic_cast<ApplicationCommandManager*>(pd->getActiveEditor())->invokeDirectly(CommandIDs::ConnectionPathfind, true); };
    addAndMakeVisible(connectionPathfind.get());

    addAndMakeVisible(volumeSlider);
    volumeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

    volumeSlider.setValue(0.75);
    volumeSlider.setRange(0.0f, 1.0f);
    volumeSlider.getProperties().set("Style", "VolumeSlider");

    volumeAttachment = std::make_unique<SliderParameterAttachment>(*dynamic_cast<RangedAudioParameter*>(pd->getParameters()[0]), volumeSlider, nullptr);

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(midiBlinker);

    levelMeter->toBehind(&volumeSlider);

    setSize(getWidth(), statusbarHeight);
}

Statusbar::~Statusbar()
{
    pd->statusbarSource.removeListener(levelMeter);
    pd->statusbarSource.removeListener(midiBlinker);
    pd->statusbarSource.removeListener(this);

    delete midiBlinker;
    delete levelMeter;
}

void Statusbar::attachToCanvas(Canvas* cnv)
{
    locked.referTo(cnv->locked);
    lockButton->getToggleStateValue().referTo(cnv->locked);
}

void Statusbar::propertyChanged(String name, var value)
{
    if (name == "grid_enabled") {
        int gridEnabled = static_cast<int>(value);
        if (gridEnabled == 0) {
            gridButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
            gridButton->setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
        }
        if (gridEnabled == 1) {
            gridButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::gridLineColourId));
            gridButton->setColour(TextButton::textColourOnId, findColour(PlugDataColour::gridLineColourId).brighter(0.4f));
        } else if (gridEnabled == 2) {
            gridButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::signalColourId));
            // TODO: fix weird colour id usage
            gridButton->setColour(TextButton::textColourOnId, findColour(PlugDataColour::signalColourId).brighter(0.4f));
        }
    }
}

void Statusbar::valueChanged(Value& v)
{
    bool lockIcon = locked == var(true) || commandLocked == var(true);
    lockButton->setButtonText(lockIcon ? Icons::Lock : Icons::Unlock);

    if (v.refersToSameSourceAs(commandLocked)) {
        auto c = static_cast<bool>(commandLocked.getValue()) ? findColour(PlugDataColour::toolbarActiveColourId) : findColour(PlugDataColour::toolbarTextColourId);
        lockButton->setColour(PlugDataColour::toolbarTextColourId, c);
    }
}

void Statusbar::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, 0.5f, static_cast<float>(getWidth()), 0.5f);

    // Makes sure it gets updated on theme change
    auto c = static_cast<bool>(commandLocked.getValue()) ? findColour(PlugDataColour::toolbarActiveColourId) : findColour(PlugDataColour::toolbarTextColourId);
    lockButton->setColour(PlugDataColour::toolbarTextColourId, c);
}

void Statusbar::resized()
{
    int pos = 0;
    auto position = [this, &pos](int width, bool inverse = false) -> int {
        int result = 8 + pos;
        pos += width + 3;
        return inverse ? getWidth() - pos : result;
    };

    lockButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());
    presentationButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    position(3); // Seperator

    connectionStyleButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());
    connectionPathfind->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    position(3); // Seperator

    gridButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    pos = 0; // reset position for elements on the left

    powerButton->setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    int levelMeterPosition = position(100, true);
    levelMeter->setBounds(levelMeterPosition, 2, 100, getHeight() - 4);
    volumeSlider.setBounds(levelMeterPosition, 2, 100, getHeight() - 4);

    // Offset to make text look centred
    oversampleSelector.setBounds(position(getHeight(), true) + 3, 0, getHeight(), getHeight());

    midiBlinker->setBounds(position(55, true), 0, 55, getHeight());
}

void Statusbar::modifierKeysChanged(ModifierKeys const& modifiers)
{
    auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());

    commandLocked = modifiers.isCommandDown() && locked.getValue() == var(false);

    if (auto* cnv = editor->getCurrentCanvas()) {
        if (cnv->didStartDragging || cnv->isDraggingLasso || static_cast<bool>(cnv->presentationMode.getValue())) {
            return;
        }

        for (auto* object : cnv->objects) {
            object->showIndex(modifiers.isAltDown());
        }
    }
}

void Statusbar::timerCallback()
{
    modifierKeysChanged(ModifierKeys::getCurrentModifiersRealtime());
}

void Statusbar::audioProcessedChanged(bool audioProcessed)
{
    auto colour = findColour(audioProcessed ? PlugDataColour::levelMeterActiveColourId : PlugDataColour::signalColourId);

    powerButton->setColour(TextButton::textColourOnId, colour);
}

StatusbarSource::StatusbarSource()
{
    level[0] = 0.0f;
    level[1] = 0.0f;

    startTimer(100);
}

static bool hasRealEvents(MidiBuffer& buffer)
{
    return std::any_of(buffer.begin(), buffer.end(),
        [](auto const& event) {
            return !event.getMessage().isSysEx();
        });
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

    for (int ch = 0; ch < channels; ch++) {
        // TODO: this logic for > 2 channels makes no sense!!
        auto localLevel = level[ch & 1].load();

        for (int n = 0; n < buffer.getNumSamples(); n++) {
            float s = std::abs(channelData[ch][n]);

            float const decayFactor = 0.99992f;

            if (s > localLevel)
                localLevel = s;
            else if (localLevel > 0.001f)
                localLevel *= decayFactor;
            else
                localLevel = 0;
        }

        level[ch & 1] = localLevel;
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
    for (auto* listener : listeners) {
        listener->audioLevelChanged(currentLevel);
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
