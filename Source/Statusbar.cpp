/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

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
        auto height = getHeight() / 2.0f;
        auto width = getWidth() - 8.0f;
        auto x = 4.0f;

        auto outerBorderWidth = 2.0f;
        auto spacingFraction = 0.03f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockWidth = (width - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockHeight = height - doubleOuterBorderWidth;
        auto blockRectWidth = (1.0f - 2.0f * spacingFraction) * blockWidth;
        auto blockRectSpacing = spacingFraction * blockWidth;
        auto blockCornerSize = 0.1f * blockWidth;
        auto c = findColour(PlugDataColour::dataColourId);

        for (int ch = 0; ch < numChannels; ch++)
        {
            auto y = ch * height;

            for (auto i = 0; i < totalBlocks; ++i)
            {
                if (i >= blocks[ch])
                    // TODO: this again but right
//                    g.setColour(findColour(PlugDataColour::meterColourId));
                    g.setColour(findColour(PlugDataColour::signalColourId));
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
        g.setColour(findColour(ComboBox::textColourId));
        g.setFont(Font(11));
        g.drawText("MIDI", getLocalBounds().removeFromLeft(28), Justification::right);

        auto midiInRect = Rectangle<float>(38.0f, 8.0f, 15.0f, 3.0f);
        auto midiOutRect = Rectangle<float>(38.0f, 17.0f, 15.0f, 3.0f);

        // TODO: what happened to meterColourId???
        g.setColour(blinkMidiIn ? findColour(PlugDataColour::dataColourId) : findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRoundedRectangle(midiInRect, 1.0f);

        g.setColour(blinkMidiOut ? findColour(PlugDataColour::dataColourId) : findColour(PlugDataColour::canvasBackgroundColourId));
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

    locked.addListener(this);
    commandLocked.addListener(this);
    
    
    oversampleSelector.setTooltip("Set oversampling");
    oversampleSelector.setName("statusbar:oversample");
    oversampleSelector.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    
    
    oversampleSelector.setButtonText(String(1 << pd.oversampling) + "x");
    
    oversampleSelector.onClick = [this](){
        PopupMenu menu;
        menu.addItem(1, "1x");
        menu.addItem(2, "2x");
        menu.addItem(3, "4x");
        menu.addItem(4, "8x");
        
        auto* editor = pd.getActiveEditor();
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&oversampleSelector).withParentComponent(editor),
                           [this](int result)
                           {
                               if (result != 0)
                               {
                                   oversampleSelector.setButtonText(String(1 << (result - 1)) + "x");
                                   pd.setOversampling(result - 1);
                               }
                           });
    };
    addAndMakeVisible(oversampleSelector);
    
    
    powerButton = std::make_unique<TextButton>(Icons::Power);
    lockButton = std::make_unique<TextButton>(Icons::Lock);
    connectionStyleButton = std::make_unique<TextButton>(Icons::ConnectionStyle);
    connectionPathfind = std::make_unique<TextButton>(Icons::Wand);
    zoomIn = std::make_unique<TextButton>(Icons::ZoomIn);
    zoomOut = std::make_unique<TextButton>(Icons::ZoomOut);
    presentationButton = std::make_unique<TextButton>(Icons::Presentation);
    gridButton = std::make_unique<TextButton>(Icons::Grid);
    themeButton = std::make_unique<TextButton>(Icons::Theme);
    browserButton = std::make_unique<TextButton>(Icons::Documentation);
    automationButton = std::make_unique<TextButton>(Icons::Parameters);

    presentationButton->setTooltip("Presentation Mode");
    presentationButton->setClickingTogglesState(true);
    presentationButton->setConnectedEdges(12);
    presentationButton->setName("statusbar:presentation");
    presentationButton->getToggleStateValue().referTo(presentationMode);

    presentationButton->onClick = [this]()
    {
        // When presenting we are always locked
        // A bit different from Max's presentation mode
        if (presentationButton->getToggleState())
        {
            lastLockMode = static_cast<bool>(locked.getValue());
            if (!lastLockMode)
            {
                locked = var(true);
            }
            lockButton->setEnabled(false);
        }
        else
        {
            locked = var(lastLockMode);
            lockButton->setEnabled(true);
        }
    };

    addAndMakeVisible(presentationButton.get());

    powerButton->setTooltip("Mute");
    powerButton->setClickingTogglesState(true);
    powerButton->setConnectedEdges(12);
    powerButton->setName("statusbar:mute");
    addAndMakeVisible(powerButton.get());

    powerButton->onClick = [this]() { powerButton->getToggleState() ? pd.startDSP() : pd.releaseDSP(); };

    powerButton->setToggleState(pd_getdspstate(), dontSendNotification);

    lockButton->setTooltip("Lock");
    lockButton->setClickingTogglesState(true);
    lockButton->setConnectedEdges(12);
    lockButton->setName("statusbar:lock");
    lockButton->getToggleStateValue().referTo(locked);
    addAndMakeVisible(lockButton.get());
    lockButton->setButtonText(locked == var(true) ? Icons::Lock : Icons::Unlock);

    connectionStyleButton->setTooltip("Enable segmented connections");
    connectionStyleButton->setClickingTogglesState(true);
    connectionStyleButton->setConnectedEdges(12);
    connectionStyleButton->setName("statusbar:connectionstyle");
    connectionStyleButton->onClick = [this]()
    {
        bool segmented = connectionStyleButton->getToggleState();
        auto* editor = dynamic_cast<PlugDataPluginEditor*>(pd.getActiveEditor());
        for (auto& connection : editor->getCurrentCanvas()->getSelectionOfType<Connection>())
        {
            connection->setSegmented(segmented);
        }
        connectionPathfind->setEnabled(segmented);
    };

    addAndMakeVisible(connectionStyleButton.get());

    connectionPathfind->setTooltip("Find best connection path");
    connectionPathfind->setConnectedEdges(12);
    connectionPathfind->setName("statusbar:findpath");
    connectionPathfind->onClick = [this]() { dynamic_cast<ApplicationCommandManager*>(pd.getActiveEditor())->invokeDirectly(CommandIDs::ConnectionPathfind, true); };
    connectionPathfind->setEnabled(connectionStyleButton->getToggleState());
    addAndMakeVisible(connectionPathfind.get());

    addAndMakeVisible(volumeSlider);
    volumeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

    volumeSlider.setValue(0.75);
    volumeSlider.setRange(0.0f, 1.0f);
    volumeSlider.setName("statusbar:meter");

    volumeAttachment = std::make_unique<SliderParameterAttachment>(*pd.parameters.getParameter("volume"), volumeSlider, nullptr);

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(midiBlinker);

    levelMeter->toBehind(&volumeSlider);

    setSize(getWidth(), statusbarHeight);
    
    // Timer to make sure modifier keys are up-to-date...
    // Hoping to find a better solution for this
    startTimer(150);

}

Statusbar::~Statusbar()
{
    delete midiBlinker;
    delete levelMeter;
}

void Statusbar::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(locked))
    {
        lockButton->setButtonText(locked == var(true) ? Icons::Lock : Icons::Unlock);
    }
    if (v.refersToSameSourceAs(commandLocked))
    {
        auto c = static_cast<bool>(commandLocked.getValue()) ? findColour(PlugDataColour::toolbarActiveColourId).brighter(0.2f) : findColour(PlugDataColour::toolbarTextColourId);
        lockButton->setColour(TextButton::textColourOffId, c);
    }
}

void Statusbar::paint(Graphics &g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, 0.5f, static_cast<float>(getWidth()), 0.5f);
}

void Statusbar::resized()
{
    int pos = 0;
    auto position = [this, &pos](int width, bool inverse = false) -> int
    {
        int result = 8 + pos;
        pos += width + 2;
        return inverse ? getWidth() - pos : result;
    };

    lockButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    position(5);  // Seperator

    connectionStyleButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());
    connectionPathfind->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    position(5);  // Seperator
    
    presentationButton->setBounds(position(getHeight()), 0, getHeight(), getHeight());

    pos = 0;  // reset position for elements on the left

    powerButton->setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    int levelMeterPosition = position(100, true);
    levelMeter->setBounds(levelMeterPosition, 0, 100, getHeight());
    volumeSlider.setBounds(levelMeterPosition, 0, 100, getHeight());
    
    // Offset to make text look centred
    oversampleSelector.setBounds(position(getHeight(), true) + 3, 0, getHeight(), getHeight());

    midiBlinker->setBounds(position(55, true), 0, 55, getHeight());
}

void Statusbar::modifierKeysChanged(const ModifierKeys& modifiers)
{
    if(auto* editor = dynamic_cast<PlugDataPluginEditor*>(pd.getActiveEditor()))
    {
        auto* cnv = editor->getCurrentCanvas();
        if(cnv && (cnv->didStartDragging || cnv->isDraggingLasso)) {
            return;
        }
    }
    commandLocked = modifiers.isCommandDown() && locked.getValue() == var(false);
}

void Statusbar::timerCallback()
{
    modifierKeysChanged(ModifierKeys::getCurrentModifiers());
}

StatusbarSource::StatusbarSource()
{
    level[0] = 0.0f;
    level[1] = 0.0f;
}

static bool hasRealEvents(MidiBuffer& buffer)
{
    return std::any_of(buffer.begin(), buffer.end(),
    [](const auto& event){
        return !event.getMessage().isSysEx();
    });
}

void StatusbarSource::processBlock(const AudioBuffer<float>& buffer, MidiBuffer& midiIn, MidiBuffer& midiOut, int channels)
{
    auto** channelData = buffer.getArrayOfReadPointers();

    if (channels == 1)
    {
        level[1] = 0;
    }
    else if (channels == 0)
    {
        level[0] = 0;
        level[1] = 0;
    }

    for (int ch = 0; ch < channels; ch++)
    {
        // TODO: this logic for > 2 channels makes no sense!!
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
