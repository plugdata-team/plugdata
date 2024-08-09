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

#include "Sidebar/Sidebar.h"
#include "Sidebar/Palettes.h"

#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"
#include "Dialogs/AudioOutputSettings.h"
#include "Dialogs/AlignmentTools.h"

#include "Components/ArrowPopupMenu.h"

class LatencyDisplayButton : public Component
    , public MultiTimer
    , public SettableTooltipClient {
    Label latencyValue;
    Label icon;
    bool isHover = false;
    Colour bgColour;
    int currentLatencyValue = 0;

    enum TimerRoutine { Timeout,
        Animate };
    float alpha = 1.0f;
    bool fading = false;

public:
    std::function<void()> onClick = []() {};
    LatencyDisplayButton()
    {
        icon.setFont(Fonts::getIconFont());
        icon.setText(Icons::GlyphDelay, dontSendNotification);

        icon.setJustificationType(Justification::centredLeft);
        latencyValue.setJustificationType(Justification::centredRight);

        setInterceptsMouseClicks(true, false);
        addMouseListener(this, true);

        // we need to specifically turn off mouse intercept for child components for tooltip of parent to work
        // setting child components intercept to false in parent is not enough
        latencyValue.setInterceptsMouseClicks(false, false);
        icon.setInterceptsMouseClicks(false, false);

        setTooltip("Plugin latency, click to reset to 64 samples");

        addAndMakeVisible(latencyValue);
        addAndMakeVisible(icon);

        buttonStateChanged();
    };

    void lookAndFeelChanged() override
    {
        buttonStateChanged();
    }

    void timerCallback(int ID) override
    {
        switch (ID) {
        case Timeout:
            startTimer(Animate, 1000 / 30.0f);
            break;
        case Animate:
            alpha = pow(alpha, 1.0f / 2.2f);
            alpha -= 0.02f;
            alpha = pow(alpha, 2.2f);
            alpha = std::clamp(alpha, 0.0f, 1.0f);
            alpha = std::isfinite(alpha) ? alpha : 0.0f;
            fading = true;
            if (alpha <= 0.01f) {
                alpha = 0.0f;
                stopTimer(Animate);
                setVisible(false);
            }
            buttonStateChanged();
            break;
        default:
            break;
        }
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(1, 6).toFloat();
        g.setColour(bgColour);
        g.fillRoundedRectangle(b, Corners::defaultCornerRadius);
    }

    void setLatencyValue(int const value)
    {
        currentLatencyValue = value;
        updateValue();
        if (value == 0) {
            startTimer(Timeout, 1000 / 3.0f);
        } else {
            stopTimer(Timeout);
            stopTimer(Animate);
            fading = false;
            setVisible(true);
            alpha = 1.0f;
            buttonStateChanged();
        }
    }

    void updateValue()
    {
        if (isHover && !fading) {
            latencyValue.setJustificationType(Justification::centredLeft);
            latencyValue.setText("Reset", dontSendNotification);
        } else {
            latencyValue.setJustificationType(Justification::centredRight);
            latencyValue.setText(String(currentLatencyValue) + " smpl", dontSendNotification);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        onClick();
    }

    void buttonStateChanged()
    {
        bgColour = getLookAndFeel().findColour(isHover ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarActiveColourId).withAlpha(alpha);
        auto textColour = bgColour.contrasting().withAlpha(alpha);
        icon.setColour(Label::textColourId, textColour);
        latencyValue.setColour(Label::textColourId, textColour);

        updateValue();

        repaint();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHover = true;
        buttonStateChanged();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHover = false;
        buttonStateChanged();
    }

    void resized() override
    {
        icon.setBounds(0, 0, getHeight(), getHeight());
        latencyValue.setBounds(getHeight(), 0, getWidth() - getHeight(), getHeight());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyDisplayButton);
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
        g.fillRoundedRectangle(thumb, Corners::defaultCornerRadius * 0.5f);
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

        auto outerBorderWidth = 2.5f;
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
        Fonts::drawIcon(g, Icons::MIDI, getLocalBounds().removeFromLeft(16).withTrimmedTop(1), findColour(ComboBox::textColourId), 13);

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
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuTextColourId));
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

        Fonts::drawIcon(g, Icons::CPU, getLocalBounds().removeFromLeft(16), textColour, 14);
        Fonts::drawFittedText(g, String(cpuUsageToDraw) + "%", getLocalBounds().withTrimmedLeft(22).withTrimmedTop(1), textColour, 1, 0.9f, 13.5, Justification::centredLeft);
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

class ZoomLabel : public Component {
public:
    ZoomLabel(Statusbar* parent)
        : statusbar(parent)
    {
        setRepaintsOnMouseActivity(true);
    }

private:
    void paint(Graphics& g) override
    {
        // We can use a tabular numbers font here, but I'm not sure it really looks better that way
        // g.setFont(Fonts::getTabularNumbersFont().withHeight(14));
        if (isEnabled()) {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId).contrasting(isMouseOver() ? 0.35f : 0.0f));
        } else {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId).withAlpha(0.65f));
        }
        g.drawFittedText(String(int(statusbar->currentZoomLevel)) + "%", 0, 0, getWidth() - 2, getHeight(), Justification::centredRight, 1, 0.95f);
    }

    void enablementChanged() override
    {
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!isEnabled())
            return;

        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            auto defaultZoom = SettingsFile::getInstance()->getProperty<float>("default_zoom") / 100.0f;
            cnv->zoomScale.setValue(defaultZoom);
            cnv->setTransform(AffineTransform().scaled(defaultZoom));
            if (cnv->viewport)
                cnv->viewport->resized();
        }
    }

    Statusbar* statusbar;
};

Statusbar::Statusbar(PluginProcessor* processor)
    : pd(processor)
{
    levelMeter = std::make_unique<LevelMeter>();
    cpuMeter = std::make_unique<CPUMeter>();
    midiBlinker = std::make_unique<MIDIBlinker>();
    volumeSlider = std::make_unique<VolumeSlider>();
    zoomLabel = std::make_unique<ZoomLabel>(this);

    pd->statusbarSource->addListener(levelMeter.get());
    pd->statusbarSource->addListener(midiBlinker.get());
    pd->statusbarSource->addListener(cpuMeter.get());
    pd->statusbarSource->addListener(this);

    latencyDisplayButton = std::make_unique<LatencyDisplayButton>();
    addChildComponent(latencyDisplayButton.get());
    latencyDisplayButton->onClick = [this]() {
        pd->performLatencyCompensationChange(0);
    };

    powerButton.setButtonText(Icons::Power);
    centreButton.setButtonText(Icons::Centre);

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
    addAndMakeVisible(*zoomLabel);

    levelMeter->toBehind(volumeSlider.get());

    zoomComboButton.setButtonText(Icons::ThinDown);

    zoomComboButton.onClick = [this]() {
        PopupMenu zoomMenu;
        auto zoomOptions = StringArray { "25%", "50%", "75%", "100%", "125%", "150%", "175%", "200%", "250%", "300%" };
        for (auto zoomOption : zoomOptions) {
            auto scale = zoomOption.upToFirstOccurrenceOf("%", false, false).getIntValue() / 100.0f;
            zoomMenu.addItem(zoomOption, [this, scale]() {
                auto* editor = findParentComponentOfClass<PluginEditor>();
                if (auto* cnv = editor->getCurrentCanvas()) {
                    cnv->zoomScale.setValue(scale);
                    cnv->setTransform(AffineTransform().scaled(scale));
                    if (cnv->viewport)
                        cnv->viewport->resized();
                }
            });
        }

        zoomMenu.addSeparator();
        zoomMenu.addItem("Zoom to fit content", [this]() {
            auto* editor = findParentComponentOfClass<PluginEditor>();
            if (auto* cnv = editor->getCurrentCanvas()) {
                cnv->zoomToFitAll();
            }
        });
        zoomMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withTargetComponent(&zoomComboButton));
    };

    snapEnableButton.setButtonText(Icons::Magnet);
    snapSettingsButton.setButtonText(Icons::ThinDown);

    audioSettingsButton.setButtonText(Icons::ThinDown);
    audioSettingsButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        AudioOutputSettings::show(editor, audioSettingsButton.getScreenBounds());
    };

    snapEnableButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getPropertyAsValue("grid_enabled"));
    snapEnableButton.setClickingTogglesState(true);
    addAndMakeVisible(snapEnableButton);

    snapSettingsButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        SnapSettings::show(editor, snapSettingsButton.getScreenBounds());
    };
    addAndMakeVisible(snapSettingsButton);

    addAndMakeVisible(zoomComboButton);

    overlayButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").getPropertyAsValue("alt_mode", nullptr));
    overlayButton.setTooltip(String("Show overlays"));
    overlayButton.setButtonText(Icons::Eye);
    overlayButton.setClickingTogglesState(true);
    overlaySettingsButton.setButtonText(Icons::ThinDown);
    addAndMakeVisible(overlayButton);

    overlaySettingsButton.onClick = [this]() {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        OverlayDisplaySettings::show(editor, overlaySettingsButton.getScreenBounds());
    };
    addAndMakeVisible(overlaySettingsButton);

    limiterButton.getProperties().set("bold_text", true);
    limiterButton.setClickingTogglesState(true);
    limiterButton.setToggleState(SettingsFile::getInstance()->getProperty<bool>("protected"), dontSendNotification);

    limiterButton.onStateChange = [this]() {
        limiterButton.setTooltip(limiterButton.getToggleState() ? "Turn off limiter" : "Turn on limiter");
    };

    limiterButton.onClick = [this]() {
        auto state = limiterButton.getToggleState();
        pd->setProtectedMode(state);
        SettingsFile::getInstance()->setProperty("protected", state);
    };
    addAndMakeVisible(limiterButton);

    zoomComboButton.setTooltip(String("Select zoom"));

    addAndMakeVisible(audioSettingsButton);

    audioSettingsButton.setTooltip(String("Audio settings"));
    snapSettingsButton.setTooltip(String("Snap settings"));

    setLatencyDisplay(pd->getLatencySamples() - pd::Instance::getBlockSize());

    setSize(getWidth(), statusbarHeight);

    lookAndFeelChanged();
}

Statusbar::~Statusbar()
{
    pd->statusbarSource->removeListener(levelMeter.get());
    pd->statusbarSource->removeListener(midiBlinker.get());
    pd->statusbarSource->removeListener(cpuMeter.get());
    pd->statusbarSource->removeListener(this);
}

void Statusbar::handleAsyncUpdate()
{
    auto* editor = findParentComponentOfClass<PluginEditor>();
    if (auto* cnv = editor->getCurrentCanvas()) {
        currentZoomLevel = getValue<float>(cnv->zoomScale) * 100;
    } else {
        currentZoomLevel = 100.0f;
    }
    repaint();
}

void Statusbar::updateZoomLevel()
{
    triggerAsyncUpdate();
}

void Statusbar::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));

    auto* editor = findParentComponentOfClass<PluginEditor>();
    auto start = !editor->palettes->isExpanded() ? 29.0f : 0.0f;
    auto end = editor->sidebar->isHidden() ? 29.0f : 0.0f;
    g.drawLine(start, 0.5f, static_cast<float>(getWidth()) - end, 0.5f);

    g.drawLine(firstSeparatorPosition, 6.0f, firstSeparatorPosition, getHeight() - 6.0f);
    g.drawLine(secondSeparatorPosition, 6.0f, secondSeparatorPosition, getHeight() - 6.0f);
}

void Statusbar::resized()
{
    int pos = 0;
    auto position = [this, &pos](int width, bool inverse = false) -> int {
        int result = 8 + pos;
        pos += width + 3;
        return inverse ? getWidth() - pos : result;
    };

    auto spacing = getHeight();

    // Some newer iPhone models have a very large corner radius
#if JUCE_IOS
    position(22);
#endif

    zoomLabel->setBounds(position(34), 0, 34, getHeight());
    zoomComboButton.setBounds(position(8) - 12, 0, getHeight(), getHeight());

    firstSeparatorPosition = position(4) + 3.f; // First seperator

    centreButton.setBounds(position(spacing), 0, getHeight(), getHeight());

    secondSeparatorPosition = position(4) + 0.5f; // Second seperator

    pos -= 3;
    
    snapEnableButton.setBounds(position(12), 0, getHeight(), getHeight());
    snapSettingsButton.setBounds(position(spacing - 4), 0, getHeight(), getHeight());

    overlayButton.setBounds(position(12), 0, getHeight(), getHeight());
    overlaySettingsButton.setBounds(position(spacing - 4), 0, getHeight(), getHeight());

    pos = 4; // reset position for elements on the right

#if JUCE_IOS
    position(22, true);
#endif

    audioSettingsButton.setBounds(position(getHeight(), true), 0, getHeight(), getHeight());
    powerButton.setBounds(position(getHeight() - 6, true), 0, getHeight(), getHeight());

    limiterButton.setBounds(position(44, true), 4, 44, getHeight() - 8);

    // TODO: combine these both into one
    int levelMeterPosition = position(112, true);
    levelMeter->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);
    volumeSlider->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);

    // Hide these if there isn't enough space
    midiBlinker->setVisible(getWidth() > 500);
    cpuMeter->setVisible(getWidth() > 500);

    midiBlinker->setBounds(position(55, true) + 10, 0, 55, getHeight());
    cpuMeter->setBounds(position(45, true), 0, 50, getHeight());
    latencyDisplayButton->setBounds(position(104, true), 0, 100, getHeight());
}

void Statusbar::setLatencyDisplay(int value)
{
    if (!ProjectInfo::isStandalone) {
        latencyDisplayButton->setLatencyValue(value);
    }
}

void Statusbar::showDSPState(bool dspState)
{
    powerButton.setToggleState(dspState, dontSendNotification);
}

void Statusbar::setHasActiveCanvas(bool hasActiveCanvas)
{
    centreButton.setEnabled(hasActiveCanvas);
    zoomComboButton.setEnabled(hasActiveCanvas);
    zoomLabel->setEnabled(hasActiveCanvas);
}

void Statusbar::audioProcessedChanged(bool audioProcessed)
{
    auto colour = findColour(audioProcessed ? PlugDataColour::levelMeterActiveColourId : PlugDataColour::signalColourId);

    powerButton.setColour(TextButton::textColourOnId, colour);
}

void Statusbar::lookAndFeelChanged()
{
    limiterButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    limiterButton.setColour(TextButton::buttonColourId, findColour(PlugDataColour::levelMeterBackgroundColourId));
    auto limiterButtonActiveColour = findColour(PlugDataColour::toolbarActiveColourId).withAlpha(0.3f);
    limiterButton.setColour(TextButton::buttonOnColourId, limiterButtonActiveColour);

    auto blendColours = [](juce::Colour const& bottomColour, juce::Colour const& topColour) -> Colour {
        float alpha = topColour.getFloatAlpha();

        float r = alpha * topColour.getFloatRed() + (1 - alpha) * bottomColour.getFloatRed();
        float g = alpha * topColour.getFloatGreen() + (1 - alpha) * bottomColour.getFloatGreen();
        float b = alpha * topColour.getFloatBlue() + (1 - alpha) * bottomColour.getFloatBlue();

        return Colour::fromFloatRGBA(r, g, b, 1.0f);
    };

    // Blend the button colour & toolbar background colour to make sure that the button's 'on' text is visible
    // as we are using the active colour with alpha to reduce how distracting the limiter button active state is.
    auto blendedButtonColour = blendColours(findColour(PlugDataColour::toolbarBackgroundColourId), limiterButtonActiveColour);

    limiterButton.setColour(TextButton::textColourOffId, findColour(PlugDataColour::panelTextColourId));
    limiterButton.setColour(TextButton::textColourOnId, blendedButtonColour.contrasting());
}

StatusbarSource::StatusbarSource()
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
    /*
    if (channels == 1) {
        level[1].store(0, std::memory_order_relaxed) = 0;
    } else if (channels == 0) {
        level[0].store(0, std::memory_order_relaxed);
        level[1].store(0, std::memory_order_relaxed);
    } */

    auto nowInMs = Time::getMillisecondCounter();

    lastAudioProcessedTime.store(nowInMs, std::memory_order_relaxed);

    if (hasMidiOutput)
        lastMidiSentTime.store(nowInMs, std::memory_order_relaxed);
    if (hasMidiInput)
        lastMidiReceivedTime.store(nowInMs, std::memory_order_relaxed);
}

void StatusbarSource::prepareToPlay(int nChannels)
{
    peakBuffer.reset(sampleRate, bufferSize, nChannels);
}

void StatusbarSource::timerCallback()
{
    auto currentTime = Time::getMillisecondCounter();

    auto hasReceivedMidi = currentTime - lastMidiReceivedTime.load(std::memory_order_relaxed) < 700;
    auto hasSentMidi = currentTime - lastMidiSentTime.load(std::memory_order_relaxed) < 700;
    auto hasProcessedAudio = currentTime - lastAudioProcessedTime.load(std::memory_order_relaxed) < 700;

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
        listener->cpuUsageChanged(cpuUsage.load(std::memory_order_relaxed));
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
    cpuUsage.store(cpu, std::memory_order_relaxed);
}
