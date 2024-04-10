/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/CircularBuffer.h"

#include "Statusbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"

#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"
#include "Dialogs/AlignmentTools.h"

#include "Components/ArrowPopupMenu.h"

class OversampleSelector : public TextButton {

    class OversampleSettingsPopup : public Component {
    public:
        std::function<void(int)> onChange = [](int) {};
        std::function<void()> onClose = []() {};

        explicit OversampleSettingsPopup(int currentSelection)
        {
            title.setText("Oversampling factor", dontSendNotification);
            title.setFont(Fonts::getBoldFont().withHeight(14.0f));
            title.setJustificationType(Justification::centred);
            addAndMakeVisible(title);

            one.setConnectedEdges(ConnectedOnRight);
            two.setConnectedEdges(ConnectedOnLeft | ConnectedOnRight);
            four.setConnectedEdges(ConnectedOnLeft | ConnectedOnRight);
            eight.setConnectedEdges(ConnectedOnLeft);

            auto buttons = Array<TextButton*> { &one, &two, &four, &eight };

            int i = 0;
            for (auto* button : buttons) {
                button->setRadioGroupId(hash("oversampling_selector"));
                button->setClickingTogglesState(true);
                button->onClick = [this, i]() {
                    onChange(i);
                };

                button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
                button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuActiveTextColourId));
                button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
                button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
                button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

                addAndMakeVisible(button);
                i++;
            }

            buttons[currentSelection]->setToggleState(true, dontSendNotification);

            setSize(180, 50);
        }

        ~OversampleSettingsPopup() override
        {
            onClose();
        }

    private:
        void resized() override
        {
            auto b = getLocalBounds().reduced(4, 4);
            auto titleBounds = b.removeFromTop(22);

            title.setBounds(titleBounds.translated(0, -2));

            auto buttonWidth = b.getWidth() / 4;

            one.setBounds(b.removeFromLeft(buttonWidth));
            two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
            four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
            eight.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        }

        Label title;
        TextButton one = TextButton("1x");
        TextButton two = TextButton("2x");
        TextButton four = TextButton("4x");
        TextButton eight = TextButton("8x");
    };

public:
    explicit OversampleSelector(PluginProcessor* pd)
    {
        onClick = [this, pd]() {
            auto selection = log2(getButtonText().upToLastOccurrenceOf("x", false, false).getIntValue());

            auto oversampleSettings = std::make_unique<OversampleSettingsPopup>(selection);

            oversampleSettings->onChange = [this, pd](int result) {
                setButtonText(String(1 << result) + "x");
                pd->setOversampling(result);
            };
            oversampleSettings->onClose = [this]() {
                repaint();
            };
            auto* editor = findParentComponentOfClass<PluginEditor>();
            editor->showCalloutBox(std::move(oversampleSettings), getScreenBounds());
        };
    }

private:
    void lookAndFeelChanged() override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto buttonText = getButtonText();
        if (buttonText == "1x") {
            g.setColour(isMouseOverOrDragging() ? findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f) : findColour(PlugDataColour::toolbarTextColourId));
        } else {
            g.setColour(isMouseOverOrDragging() ? findColour(PlugDataColour::toolbarActiveColourId).brighter(0.8f) : findColour(PlugDataColour::toolbarActiveColourId));
        }

        g.setFont(Fonts::getCurrentFont().withHeight(14.0f));
        g.drawText(buttonText, getLocalBounds(), Justification::centred);
    }
};

class VolumeSlider : public Slider {
public:
    VolumeSlider()
        : Slider(Slider::LinearHorizontal, Slider::NoTextBox)
    {
        setSliderSnapsToMousePosition(false);
    }

    void resized() override
    {
        setMouseDragSensitivity(getWidth() - (margin * 2));
    }

    void mouseMove(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseMove(e);
    }

    void mouseUp(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseUp(e);
    }

    void mouseDown(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseDown(e);
    }

    void paint(Graphics& g) override
    {
        auto backgroundColour = findColour(PlugDataColour::levelMeterThumbColourId);

        auto value = getValue();
        auto thumbSize = getHeight() * 0.7f;
        auto position = Point<float>(margin + (value * (getWidth() - (margin * 2))), getHeight() * 0.5f);
        auto thumb = Rectangle<float>(thumbSize, thumbSize).withCentre(position);
        thumb = thumb.withSizeKeepingCentre(thumb.getWidth() - 12, thumb.getHeight());
        g.setColour(backgroundColour.darker(thumb.contains(getMouseXYRelative().toFloat()) ? 0.3f : 0.0f).withAlpha(0.8f));
        PlugDataLook::fillSmoothedRectangle(g, thumb, Corners::defaultCornerRadius * 0.5f);
    }

private:
    int margin = 18;
};

class LevelMeter : public Component
    , public StatusbarSource::Listener
    , public MultiTimer {
    float audioLevel[2] = { 0.0f, 0.0f };
    float peakLevel[2] = { 0.0f, 0.0f };

    int numChannels = 2;

    bool clipping[2] = { false, false };

    bool peakBarsFade[2] = { true, true };

    float fadeFactor = 0.98f;

    float lastPeak[2] = { 0.0f };
    float lastLevel[2] = { 0.0f };
    float repaintTheshold = 0.01f;

public:
    LevelMeter() = default;

    void audioLevelChanged(Array<float> peak) override
    {
        bool needsRepaint = false;
        for (int i = 0; i < 2; i++) {
            audioLevel[i] *= fadeFactor;
            if (peakBarsFade[i])
                peakLevel[i] *= fadeFactor;

            if (peak[i] > audioLevel[i]) {
                audioLevel[i] = peak[i];
                if (peak[i] >= 1.0f)
                    clipping[i] = true;
                else
                    clipping[i] = false;
            }
            if (peak[i] > peakLevel[i]) {
                peakLevel[i] = peak[i];
                peakBarsFade[i] = false;
                startTimer(i, 1700);
            }

            if (std::abs(peakLevel[i] - lastPeak[i]) > repaintTheshold
                || std::abs(audioLevel[i] - lastLevel[i]) > repaintTheshold
                || (peakLevel[i] == 0.0f && lastPeak[i] != 0.0f)
                || (audioLevel[i] == 0.0f && lastLevel[i] != 0.0f)) {
                lastPeak[i] = peakLevel[i];
                lastLevel[i] = audioLevel[i];

                needsRepaint = true;
            }
        }

        if (needsRepaint)
            repaint();
    }

    void timerCallback(int timerID) override
    {
        peakBarsFade[timerID] = true;
    }

    void paint(Graphics& g) override
    {
        auto height = getHeight() / 4.0f;
        auto barHeight = height * 0.6f;
        auto halfBarHeight = barHeight * 0.5f;
        auto width = getWidth() - 12.0f;
        auto x = 6.0f;

        auto outerBorderWidth = 2.0f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;
        auto bgHeight = getHeight() - doubleOuterBorderWidth;
        auto bgWidth = width - doubleOuterBorderWidth;
        auto meterWidth = width - bgHeight;
        auto barWidth = meterWidth - 2;
        auto leftOffset = x + (bgHeight * 0.5f);

        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(x + outerBorderWidth + 4, outerBorderWidth, bgWidth - 8, bgHeight, Corners::defaultCornerRadius);

        for (int ch = 0; ch < numChannels; ch++) {
            auto barYPos = outerBorderWidth + ((ch + 1) * (bgHeight / 3.0f)) - halfBarHeight;
            auto barLength = jmin(audioLevel[ch] * barWidth, barWidth);
            auto peekPos = jmin(peakLevel[ch] * barWidth, barWidth);

            if (peekPos > 1) {
                g.setColour(clipping[ch] ? Colours::red : findColour(PlugDataColour::levelMeterActiveColourId));
                g.fillRect(leftOffset, barYPos, barLength, barHeight);
                g.fillRect(leftOffset + peekPos, barYPos, 1.0f, barHeight);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

class MIDIBlinker : public Component
    , public StatusbarSource::Listener
    , public SettableTooltipClient {

public:
    MIDIBlinker()
    {
        setTooltip("MIDI activity");
    }

    void paint(Graphics& g) override
    {
        Fonts::drawIcon(g, Icons::MIDI, getLocalBounds().removeFromLeft(20).withTrimmedTop(1), findColour(ComboBox::textColourId), 13);

        auto midiInRect = Rectangle<float>(27.5f, 9.5f, 15.0f, 3.0f);
        auto midiOutRect = Rectangle<float>(27.5f, 18.5f, 15.0f, 3.0f);

        g.setColour(blinkMidiIn ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(midiInRect, 1.0f);

        g.setColour(blinkMidiOut ? findColour(PlugDataColour::levelMeterActiveColourId) : findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(midiOutRect, 1.0f);
    }

    void midiReceivedChanged(bool midiReceived) override
    {
        blinkMidiIn = midiReceived;
        repaint();
    }

    void midiSentChanged(bool midiSent) override
    {
        blinkMidiOut = midiSent;
        repaint();
    }

    bool blinkMidiIn = false;
    bool blinkMidiOut = false;
};

class CPUHistoryGraph : public Component {
public:
    CPUHistoryGraph(CircularBuffer<float>& history, int length)
        : historyLength(length)
        , historyGraph(history)
    {
        mappingMode = SettingsFile::getInstance()->getPropertyAsValue("cpu_meter_mapping_mode").getValue();
    }

    void resized() override
    {
        bounds = getLocalBounds().reduced(6);
        roundedClip.addRoundedRectangle(bounds, Corners::defaultCornerRadius * 0.75f);
    }

    void paint(Graphics& g) override
    {
        // clip the rectangle to rounded corners
        g.saveState();
        g.reduceClipRegion(roundedClip);

        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRect(bounds);

        auto bottom = bounds.getBottom();
        auto height = bounds.getHeight();
        auto points = historyLength;
        auto distribute = static_cast<float>(bounds.getWidth()) / points;
        Path graphTopLine;

        auto getCPUScaledY = [this, bottom, height](float value) -> float {
            float graphValue;
            switch (mappingMode) {
            case 1:
                graphValue = pow(value, 1.0f / 1.5f);
                break;
            case 2:
                graphValue = pow(value, 1.0f / 3.5f);
                break;
            default:
                graphValue = value;
                break;
            }
            return bottom - height * graphValue;
        };

        auto startPoint = Point<float>(bounds.getTopLeft().getX(), getCPUScaledY(0));
        graphTopLine.startNewSubPath(startPoint);

        auto lastValues = historyGraph.last(points);

        for (int i = 0; i < points; i++) {
            auto xPos = (i * distribute) + bounds.getTopLeft().getX() + distribute;
            auto newPoint = Point<float>(xPos, getCPUScaledY(lastValues[i] * 0.01f));
            graphTopLine.lineTo(newPoint);
        }
        Path graphFilled = graphTopLine;

        graphFilled.lineTo(bounds.getBottomRight().toFloat());
        graphFilled.lineTo(bounds.getBottomLeft().toFloat());
        graphFilled.closeSubPath();
        g.setColour(findColour(PlugDataColour::levelMeterActiveColourId).withAlpha(0.3f));
        g.fillPath(graphFilled);

        g.setColour(findColour(PlugDataColour::levelMeterActiveColourId));
        g.strokePath(graphTopLine, PathStrokeType(1.0f));

        g.restoreState();
    }

    void updateMapping(int mapping)
    {
        if (mappingMode != mapping) {
            mappingMode = mapping;
            repaint();
        }
    }

private:
    int historyLength;
    CircularBuffer<float>& historyGraph;
    Rectangle<int> bounds;
    Path roundedClip;
    int mappingMode;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUHistoryGraph);
};

class CPUMeterPopup : public Component {
public:
    CPUMeterPopup(CircularBuffer<float>& history, CircularBuffer<float>& longHistory)
    {
        cpuGraph = std::make_unique<CPUHistoryGraph>(history, 200);
        cpuGraphLongHistory = std::make_unique<CPUHistoryGraph>(longHistory, 300);
        addAndMakeVisible(cpuGraph.get());
        addAndMakeVisible(cpuGraphLongHistory.get());

        fastGraphTitle.setText("CPU usage recent", dontSendNotification);
        fastGraphTitle.setFont(Fonts::getBoldFont().withHeight(14.0f));
        fastGraphTitle.setJustificationType(Justification::centred);
        addAndMakeVisible(fastGraphTitle);

        slowGraphTitle.setText("CPU usage last 5 minutes", dontSendNotification);
        slowGraphTitle.setFont(Fonts::getBoldFont().withHeight(14.0f));
        slowGraphTitle.setJustificationType(Justification::centred);
        addAndMakeVisible(slowGraphTitle);

        linear.setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnRight);
        logA.setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft | TextButton::ConnectedEdgeFlags::ConnectedOnRight);
        logB.setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

        auto buttons = Array<TextButton*> { &linear, &logA, &logB };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("cpu_meter_mapping_mode"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i]() {
                SettingsFile::getInstance()->setProperty("cpu_meter_mapping_mode", i);
                cpuGraph->updateMapping(i);
                cpuGraphLongHistory->updateMapping(i);
            };
            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuActiveTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        auto currentMappingMode = SettingsFile::getInstance()->getPropertyAsValue("cpu_meter_mapping_mode").getValue();
        buttons[currentMappingMode]->setToggleState(true, dontSendNotification);

        setSize(212, 177);
    }

    ~CPUMeterPopup() override
    {
        onClose();
    }

    void resized() override
    {
        fastGraphTitle.setBounds(0, 6, getWidth(), 20);
        cpuGraph->setBounds(0, fastGraphTitle.getBottom(), getWidth(), 50);
        slowGraphTitle.setBounds(0, cpuGraph->getBottom(), getWidth(), 20);
        cpuGraphLongHistory->setBounds(0, slowGraphTitle.getBottom(), getWidth(), 50);

        auto b = getLocalBounds().withTop(cpuGraphLongHistory->getBottom() + 5).reduced(6, 0).withHeight(20);
        auto buttonWidth = getWidth() / 3;
        linear.setBounds(b.removeFromLeft(buttonWidth));
        logA.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        logB.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    std::function<void()> getUpdateFunc()
    {
        return [this]() {
            this->update();
        };
    }

    std::function<void()> getUpdateFuncLongHistory()
    {
        return [this]() {
            this->updateLong();
        };
    }

    std::function<void()> onClose = []() {};

private:
    void update()
    {
        cpuGraph->repaint();
    }

    void updateLong()
    {
        cpuGraphLongHistory->repaint();
    }

    Label fastGraphTitle;
    Label slowGraphTitle;
    std::unique_ptr<CPUHistoryGraph> cpuGraph;
    std::unique_ptr<CPUHistoryGraph> cpuGraphLongHistory;

    TextButton linear = TextButton("Linear");
    TextButton logA = TextButton("Log A");
    TextButton logB = TextButton("Log B");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUMeterPopup);
};

class CPUMeter : public Component
    , public StatusbarSource::Listener
    , public Timer
    , public SettableTooltipClient {

public:
    CPUMeter()
    {
        startTimer(1000);
        setTooltip("CPU usage");
    }

    void paint(Graphics& g) override
    {
        Colour textColour;
        if (isMouseOver() || currentCalloutBox)
            textColour = findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f);
        else
            textColour = findColour(PlugDataColour::toolbarTextColourId);

        Fonts::drawIcon(g, Icons::CPU, getLocalBounds().removeFromLeft(20), textColour, 14);
        Fonts::drawText(g, String(cpuUsageToDraw) + "%", getLocalBounds().withTrimmedLeft(26).withTrimmedTop(1), textColour, 13.5, Justification::centredLeft);
    }

    void timerCallback() override
    {
        CriticalSection::ScopedLockType lock(cpuMeterMutex);
        auto lastCpuUsage = cpuUsage.last();
        cpuUsageToDraw = round(lastCpuUsage);
        cpuUsageLongHistory.push(lastCpuUsage);
        updateCPUGraphLong();
        repaint();
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().contains(x, y);
    }

    void mouseDown(MouseEvent const& e) override
    {
        // check if the callout is active, otherwise mouse down / up will trigger callout box again
        if (isCallOutBoxActive) {
            isCallOutBoxActive = false;
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!isCallOutBoxActive) {
            auto cpuHistory = std::make_unique<CPUMeterPopup>(cpuUsage, cpuUsageLongHistory);
            updateCPUGraph = cpuHistory->getUpdateFunc();
            updateCPUGraphLong = cpuHistory->getUpdateFuncLongHistory();

            cpuHistory->onClose = [this]() {
                updateCPUGraph = []() { return; };
                updateCPUGraphLong = []() { return; };
                repaint();
            };

            auto* editor = findParentComponentOfClass<PluginEditor>();
            currentCalloutBox = &editor->showCalloutBox(std::move(cpuHistory), getScreenBounds());
            isCallOutBoxActive = true;
        } else {
            isCallOutBoxActive = false;
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void cpuUsageChanged(float newCpuUsage) override
    {
        ScopedLock lock(cpuMeterMutex);
        cpuUsage.push(newCpuUsage); // FIXME: we should use a circular buffer instead
        updateCPUGraph();
    }

    std::function<void()> updateCPUGraph = []() { return; };
    std::function<void()> updateCPUGraphLong = []() { return; };

    static inline SafePointer<CallOutBox> currentCalloutBox = nullptr;
    bool isCallOutBoxActive = false;
    CriticalSection cpuMeterMutex;

    CircularBuffer<float> cpuUsage = CircularBuffer<float>(256);
    CircularBuffer<float> cpuUsageLongHistory = CircularBuffer<float>(512);
    int cpuUsageToDraw = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUMeter);
};

Statusbar::Statusbar(PluginProcessor* processor)
    : pd(processor)
{
    levelMeter = std::make_unique<LevelMeter>();
    cpuMeter = std::make_unique<CPUMeter>();
    midiBlinker = std::make_unique<MIDIBlinker>();
    volumeSlider = std::make_unique<VolumeSlider>();
    oversampleSelector = std::make_unique<OversampleSelector>(processor);

    pd->statusbarSource->addListener(levelMeter.get());
    pd->statusbarSource->addListener(midiBlinker.get());
    pd->statusbarSource->addListener(cpuMeter.get());
    pd->statusbarSource->addListener(this);

    setWantsKeyboardFocus(true);

    oversampleSelector->setTooltip("Set oversampling");
    oversampleSelector->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

    oversampleSelector->setButtonText(String(1 << pd->oversampling) + "x");
    addAndMakeVisible(*oversampleSelector);

    powerButton.setButtonText(Icons::Power);
    protectButton.setButtonText(Icons::Protection);
    centreButton.setButtonText(Icons::Centre);
    fitAllButton.setButtonText(Icons::FitAll);
    debugButton.setButtonText(Icons::Debug);

    debugButton.setTooltip("Enable/disable debugging tooltips");
    debugButton.setClickingTogglesState(true);
    debugButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getPropertyAsValue("debug_connections"));
    debugButton.onClick = [this]() {
        set_plugdata_debugging_enabled(debugButton.getToggleState());
        // Recreate the DSP graph with the new optimisations
        pd->lockAudioThread();
        canvas_update_dsp();
        pd->unlockAudioThread();
    };
    set_plugdata_debugging_enabled(debugButton.getToggleState());
    addAndMakeVisible(debugButton);

    powerButton.setTooltip("Enable/disable DSP");
    powerButton.setClickingTogglesState(true);
    addAndMakeVisible(powerButton);

    powerButton.onClick = [this]() { powerButton.getToggleState() ? pd->startDSP() : pd->releaseDSP(); };

    powerButton.setToggleState(pd_getdspstate(), dontSendNotification);

    centreButton.setTooltip("Move view to origin");
    centreButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->jumpToOrigin();
        }
    };

    addAndMakeVisible(centreButton);

    fitAllButton.setTooltip("Zoom to fit all");
    fitAllButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->zoomToFitAll();
        }
    };

    addAndMakeVisible(fitAllButton);

    protectButton.setTooltip("Clip output signal and filter non-finite values");
    protectButton.setClickingTogglesState(true);
    protectButton.setToggleState(SettingsFile::getInstance()->getProperty<int>("protected"), dontSendNotification);
    protectButton.onClick = [this]() {
        int state = protectButton.getToggleState();
        pd->setProtectedMode(state);
        SettingsFile::getInstance()->setProperty("protected", state);
    };
    addAndMakeVisible(protectButton);

    volumeSlider->setRange(0.0f, 1.0f);
    volumeSlider->setValue(0.8f);
    volumeSlider->setDoubleClickReturnValue(true, 0.8f);
    addAndMakeVisible(*volumeSlider);

    if (ProjectInfo::isStandalone) {
        volumeSlider->onValueChange = [this]() {
            pd->volume->store(volumeSlider->getValue());
        };
    } else {
        volumeAttachment = std::make_unique<SliderParameterAttachment>(*dynamic_cast<RangedAudioParameter*>(pd->getParameters()[0]), *volumeSlider, nullptr);
    }

    addAndMakeVisible(*levelMeter);
    addAndMakeVisible(*midiBlinker);
    addAndMakeVisible(*cpuMeter);

    levelMeter->toBehind(volumeSlider.get());

    overlayButton.setButtonText(Icons::Eye);
    overlaySettingsButton.setButtonText(Icons::ThinDown);

    overlaySettingsButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        OverlayDisplaySettings::show(editor, overlaySettingsButton.getScreenBounds());
    };

    snapEnableButton.setButtonText(Icons::Magnet);
    snapSettingsButton.setButtonText(Icons::ThinDown);

    snapEnableButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getPropertyAsValue("grid_enabled"));

    snapSettingsButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        SnapSettings::show(editor, snapSettingsButton.getScreenBounds());
    };

    alignmentButton.setButtonText(Icons::AlignLeft);
    alignmentButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        AlignmentTools::show(editor, alignmentButton.getScreenBounds());
    };

    overlayButton.setClickingTogglesState(true);
    overlaySettingsButton.setClickingTogglesState(false);

    addAndMakeVisible(overlayButton);
    addAndMakeVisible(overlaySettingsButton);

    overlayButton.setConnectedEdges(Button::ConnectedOnRight);
    overlaySettingsButton.setConnectedEdges(Button::ConnectedOnLeft);

    overlayButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").getPropertyAsValue("alt_mode", nullptr));
    overlayButton.setTooltip(String("Show overlays"));
    overlaySettingsButton.setTooltip(String("Overlay settings"));

    snapEnableButton.setClickingTogglesState(true);
    snapSettingsButton.setClickingTogglesState(false);

    addAndMakeVisible(snapEnableButton);
    addAndMakeVisible(snapSettingsButton);

    snapEnableButton.setConnectedEdges(Button::ConnectedOnRight);
    snapSettingsButton.setConnectedEdges(Button::ConnectedOnLeft);

    snapEnableButton.setTooltip(String("Enable snapping"));
    snapSettingsButton.setTooltip(String("Snap settings"));

    addAndMakeVisible(alignmentButton);

    alignmentButton.setTooltip(String("Alignment tools"));

    setSize(getWidth(), statusbarHeight);
}

Statusbar::~Statusbar()
{
    pd->statusbarSource->removeListener(levelMeter.get());
    pd->statusbarSource->removeListener(midiBlinker.get());
    pd->statusbarSource->removeListener(cpuMeter.get());
    pd->statusbarSource->removeListener(this);
}

void Statusbar::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(29.f, 0.5f, static_cast<float>(getWidth() - 29.5f), 0.5f);

    g.drawLine(firstSeparatorPosition, 6.0f, firstSeparatorPosition, getHeight() - 6.0f);
    g.drawLine(secondSeparatorPosition, 6.0f, secondSeparatorPosition, getHeight() - 6.0f);
    g.drawLine(thirdSeparatorPosition, 6.0f, thirdSeparatorPosition, getHeight() - 6.0f);
    if (getWidth() > 500) {
        g.drawLine(fourthSeparatorPosition, 6.0f, fourthSeparatorPosition, getHeight() - 6.0f);
    }
}

void Statusbar::resized()
{
    int pos = 1;
    auto position = [this, &pos](int width, bool inverse = false) -> int {
        int result = 8 + pos;
        pos += width + 3;
        return inverse ? getWidth() - pos : result;
    };

    auto spacing = getHeight() + 4;

    // Some newer iPhone models have a very large corner radius
#if JUCE_IOS
    position(22);
#endif

    centreButton.setBounds(position(spacing), 0, getHeight(), getHeight());
    fitAllButton.setBounds(position(spacing), 0, getHeight(), getHeight());

    firstSeparatorPosition = position(7) + 3.5f; // Second seperator

    overlayButton.setBounds(position(spacing), 0, getHeight(), getHeight());
    overlaySettingsButton.setBounds(overlayButton.getBounds().translated(getHeight() - 3, 0).withTrimmedRight(8));
    position(10);

    snapEnableButton.setBounds(position(spacing), 0, getHeight(), getHeight());
    snapSettingsButton.setBounds(snapEnableButton.getBounds().translated(getHeight() - 3, 0).withTrimmedRight(8));
    position(10);

    alignmentButton.setBounds(position(spacing), 0, getHeight(), getHeight());
    debugButton.setBounds(position(spacing), 0, getHeight(), getHeight());

    pos = 4; // reset position for elements on the right

#if JUCE_IOS
    position(22, true);
#endif

    protectButton.setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    powerButton.setBounds(position(getHeight(), true), 0, getHeight(), getHeight());

    // TODO: combine these both into one
    int levelMeterPosition = position(110, true);
    levelMeter->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);
    volumeSlider->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);

    secondSeparatorPosition = position(5, true) + 5.0f; // Third seperator

    // Offset to make text look centred
    oversampleSelector->setBounds(position(spacing - 8, true), 1, getHeight() - 2, getHeight() - 2);

    thirdSeparatorPosition = position(5, true) + 2.5f; // Fourth seperator

    // Hide these if there isn't enough space
    midiBlinker->setVisible(getWidth() > 500);
    cpuMeter->setVisible(getWidth() > 500);

    midiBlinker->setBounds(position(40, true) - 8, 0, 55, getHeight());
    fourthSeparatorPosition = position(10, true);
    cpuMeter->setBounds(position(48, true), 0, 50, getHeight());
}

void Statusbar::audioProcessedChanged(bool audioProcessed)
{
    auto colour = findColour(audioProcessed ? PlugDataColour::levelMeterActiveColourId : PlugDataColour::signalColourId);

    powerButton.setColour(TextButton::textColourOnId, colour);
}

StatusbarSource::StatusbarSource()
    : numChannels(0)
{
    startTimerHz(30);
}

void StatusbarSource::setSampleRate(double const newSampleRate)
{
    sampleRate = static_cast<int>(newSampleRate);
}

void StatusbarSource::setBufferSize(int bufferSize)
{
    this->bufferSize = bufferSize;
}

void StatusbarSource::process(bool hasMidiInput, bool hasMidiOutput, int channels)
{
    if (channels == 1) {
        level[1] = 0;
    } else if (channels == 0) {
        level[0] = 0;
        level[1] = 0;
    }

    auto nowInMs = Time::getCurrentTime().getMillisecondCounter();

    lastAudioProcessedTime = nowInMs;

    if (hasMidiOutput)
        lastMidiSentTime = nowInMs;
    if (hasMidiInput)
        lastMidiReceivedTime = nowInMs;
}

void StatusbarSource::prepareToPlay(int nChannels)
{
    numChannels = nChannels;
    peakBuffer.reset(sampleRate, bufferSize, nChannels);
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

    auto peak = peakBuffer.getPeak();

    for (auto* listener : listeners) {
        listener->audioLevelChanged(peak);
        listener->cpuUsageChanged(cpuUsage);
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

void StatusbarSource::setCPUUsage(float cpu)
{
    cpuUsage = cpu;
}
