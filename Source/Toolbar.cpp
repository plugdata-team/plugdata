/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/CircularBuffer.h"

#include "Toolbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"

#include "Components/BouncingViewport.h"
#include "Dialogs/Dialogs.h"
#include "Dialogs/AudioOutputSettings.h"
#include "Utility/MidiDeviceManager.h"

class VolumeComponent final : public Slider
    , public Slider::Listener
    , public ToolbarSource::Listener
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

    class VolumeSliderDecibelPopup final : public Component {
    public:
        VolumeSliderDecibelPopup()
        {
            setAlwaysOnTop(true);
        }

        void paint(Graphics& g) override
        {
            g.fillAll(PlugDataColours::levelMeterBackgroundColour);
            g.setColour(PlugDataColours::toolbarTextColour.withAlpha(0.666f));
            g.drawText(String(decibelValue) + "dB", getLocalBounds(), textJustification);
        }

        void setValue(float const newValue)
        {
            float realGain;
            if (newValue <= 0.8f)
                realGain = pow(jmap(newValue, 0.0f, 0.8f, 0.0f, 1.0f), 2.5f);
            else
                realGain = jmap(newValue, 0.8f, 1.0f, 1.0f, 2.0f);

            decibelValue = std::clamp<int>(Decibels::gainToDecibels(realGain), -96, 6);
            repaint();
        }

        void setJustification(Justification const justification)
        {
            textJustification = justification;
        }

        int decibelValue = 0;
        Justification textJustification = Justification::left;
    };

public:
    VolumeComponent() : Slider(Slider::LinearHorizontal, Slider::NoTextBox)
    {
        setSliderSnapsToMousePosition(false);
        addListener(this);
        decibelPopup.setAlpha(0.0f);
        addChildComponent(decibelPopup);
        updater.addAnimator(animator, [this]() {
            decibelPopup.setVisible(animationFadeIn);
        });
    }
    void audioLevelChanged(SmallArray<float> peak) override
    {
        bool needsRepaint = false;
        for (int i = 0; i < std::min<int>(peak.size(), 2); i++) {
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

    void timerCallback(int const timerID) override
    {
        peakBarsFade[timerID] = true;
    }

    void paint(Graphics& g) override
    {
        auto const height = getHeight() / 4.0f;
        auto const barHeight = height * 0.6f;
        auto const halfBarHeight = barHeight * 0.5f;
        auto const width = getWidth() - 12.0f;
        constexpr auto x = 6.0f;

        constexpr auto outerBorderWidth = 2.5f;
        auto constexpr doubleOuterBorderWidth = 2.0f * outerBorderWidth;
        auto const bgHeight = getHeight() - doubleOuterBorderWidth;
        auto const bgWidth = width - doubleOuterBorderWidth;
        auto const meterWidth = width - bgHeight;
        auto const barWidth = meterWidth - 2;
        auto const leftOffset = x + bgHeight * 0.5f;

        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRoundedRectangle(x + outerBorderWidth + 4, outerBorderWidth, bgWidth - 8, bgHeight, Corners::defaultCornerRadius);

        for (int ch = 0; ch < numChannels; ch++) {
            auto const barYPos = outerBorderWidth + (ch + 1) * (bgHeight / 3.0f) - halfBarHeight;
            auto const barLength = jmin(audioLevel[ch] * barWidth, barWidth);
            auto const peekPos = jmin(peakLevel[ch] * barWidth, barWidth);

            if (peekPos > 1) {
                g.setColour(clipping[ch] ? Colours::red : PlugDataColours::levelMeterActiveColour);
                g.fillRect(leftOffset, barYPos, barLength, barHeight);
                g.fillRect(leftOffset + peekPos, barYPos, 1.0f, barHeight);
            }
        }

        auto const backgroundColour = PlugDataColours::levelMeterThumbColour;

        auto const value = getValue();
        auto const thumbSize = getHeight() * 0.66f;
        auto const position = Point<float>(margin + value * (getWidth() - margin * 2), getHeight() * 0.5f);
        auto thumb = Rectangle<float>(thumbSize, thumbSize).withCentre(position);
        thumb = thumb.withSizeKeepingCentre(thumb.getWidth() - 12, thumb.getHeight());
        g.setColour(backgroundColour.darker(thumb.contains(getMouseXYRelative().toFloat()) ? 0.3f : 0.0f).withAlpha(0.8f));
        g.fillRoundedRectangle(thumb, Corners::defaultCornerRadius * 0.5f);
    }

    void resized() override
    {
        setMouseDragSensitivity(getWidth() - margin * 2);
    }

    void sliderValueChanged(Slider*) override
    {
        updatePopup(getMouseXYRelative(), isMouseButtonDown());
    }

    void updatePopup(Point<int> const mousePosition, bool isDragging)
    {
        auto const value = getValue();
        auto const thumbSize = getHeight() * 0.7f;
        auto const sliderPosition = Point<int>(margin + value * (getWidth() - margin * 2), getHeight() * 0.5f);
        auto const thumb = Rectangle<int>(thumbSize, thumbSize).withCentre(sliderPosition);

        decibelPopup.setValue(value);

        if (auto const shouldBeVisible = (thumb.contains(mousePosition) || isDragging)) {
            if (value > 0.5f) {
                decibelPopup.setBounds(Rectangle<int>(18, 2, 40, getHeight() - 4));
                decibelPopup.setJustification(Justification::left);
            } else {
                decibelPopup.setBounds(Rectangle<int>(getWidth() - 50, 2, 40, getHeight() - 4));
                decibelPopup.setJustification(Justification::right);
            }

            if (shouldBeVisible != decibelPopup.isVisible()) {
                animationFadeIn = true;
                decibelPopup.setVisible(true);
                animator.start();
            }
        } else {
            if (shouldBeVisible != decibelPopup.isVisible()) {
                animationFadeIn = false;
                animator.start();
            }
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseEnter(e);
        updatePopup(e.getPosition(), false);
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseExit(e);
        updatePopup(e.getPosition(), false);
    }

    void mouseMove(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseMove(e);
        updatePopup(e.getPosition(), false);
    }

    void mouseUp(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseUp(e);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        repaint();
        Slider::mouseDown(e);
    }


private:
    VolumeSliderDecibelPopup decibelPopup;
    int margin = 18;

    bool animationFadeIn = false;
    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder { }
                            .withDurationMs(270)
                            .withEasing(Easings::createEaseInOutCubic())
                            .withValueChangedCallback([this](float v) {
                                decibelPopup.setAlpha(animationFadeIn ? v : (1.0f - v));
                            })
                            .build();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeComponent)
};

// Stores the last N messages. Safe to access from the message thread only.
class MIDIListModel {
public:
    void addMessage(MidiMessage const& message, bool isInput)
    {
        messages.add({ isInput, message });

        if (messages.size() > 1000) {
            messages.erase(messages.begin(), messages.begin() + (messages.size() - 1000));
        }

        NullCheckedInvocation::invoke(onChange);
    }

    void clear()
    {
        messages.clear();

        NullCheckedInvocation::invoke(onChange);
    }

    std::pair<bool, MidiMessage> const& operator[](size_t const ind) const { return messages[messages.size() - ind - 1]; }

    size_t size() const { return messages.size(); }

    std::function<void()> onChange;

private:
    static constexpr auto numToStore = 1000;
    HeapArray<std::pair<bool, MidiMessage>> messages;
};

class MIDIHistory final : public Component
    , private TableListBoxModel {
    enum {
        messageColumn = 1,
        channelColumn,
        dataColumn
    };

public:
    explicit MIDIHistory(MIDIListModel& model)
        : messages(model)
        , bouncer(table.getViewport())
    {
        addAndMakeVisible(table);
        table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        table.setModel(this);
        table.setClickingTogglesRowSelection(false);
        table.setHeader([&] {
            auto header = std::make_unique<TableHeaderComponent>();
            header->addColumn("Type", messageColumn, 110, 30, -1, TableHeaderComponent::visible | TableHeaderComponent::appearsOnColumnMenu);
            header->addColumn("Ch.", channelColumn, 45, 30, -1, TableHeaderComponent::visible | TableHeaderComponent::appearsOnColumnMenu);
            header->addColumn("Message", dataColumn, 125, 30, -1, TableHeaderComponent::visible | TableHeaderComponent::appearsOnColumnMenu);
            return header;
        }());
        table.getViewport()->setScrollBarsShown(true, false, false, false);
        table.getViewport()->setViewPositionProportionately(0.0f, 1.0f);

        midiHistoryTitle.setText("MIDI history", dontSendNotification);
        midiHistoryTitle.setFont(Fonts::getBoldFont().withHeight(14.0f));
        midiHistoryTitle.setJustificationType(Justification::centred);
        addAndMakeVisible(midiHistoryTitle);

        messages.onChange = [&] { table.updateContent(); };
        setSize(278, 178);
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().withTrimmedTop(32).toFloat(), Corners::defaultCornerRadius);

        g.setColour(PlugDataColours::outlineColour);
        g.drawLine(0, 58, getWidth(), 58);
    }

    ~MIDIHistory() override { messages.onChange = nullptr; }

    void resized() override
    {
        midiHistoryTitle.setBounds(0, 6, getWidth(), 20);
        table.setBounds(getLocalBounds().withTrimmedTop(32));
    }

private:
    int getNumRows() override { return static_cast<int>(messages.size()); }

    void paintRowBackground(Graphics&, int, int, int, bool) override { }
    void paintCell(Graphics&, int, int, int, int, bool) override { }

    Component* refreshComponentForCell(int const rowNumber,
        int const columnId,
        bool,
        Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;

        auto const index = static_cast<int>(messages.size()) - 1 - rowNumber;
        auto const message = messages[static_cast<size_t>(index)];

        auto* label = new Label({ }, [&] {
            auto const direction = message.first ? "In: " : "Out: ";
            switch (columnId) {
            case messageColumn:
                return direction + getEventString(message.second);
            // case timeColumn:    return String (message.getTimeStamp());
            case channelColumn:
                return String(message.second.getChannel());
            case dataColumn:
                return getDataString(message.second);
            default:
                break;
            }

            jassertfalse;
            return String();
        }());

        label->setFont(Fonts::getDefaultFont().withHeight(14.0f));
        return label;
    }

    static String getEventString(MidiMessage const& m)
    {
        if (m.isNoteOn())
            return "Note on";
        if (m.isNoteOff())
            return "Note off";
        if (m.isProgramChange())
            return "Pgm. change";
        if (m.isPitchWheel())
            return "Pitch wheel";
        if (m.isAftertouch())
            return "Aftertouch";
        if (m.isChannelPressure())
            return "Ch. pressure";
        if (m.isAllNotesOff())
            return "All notes off";
        if (m.isAllSoundOff())
            return "All sound off";
        if (m.isMetaEvent())
            return "Meta event";

        if (m.isController()) {
            return "Ctl. " + String(m.getControllerNumber());
        }

        return String::toHexString(m.getRawData(), m.getRawDataSize());
    }

    static String getDataString(MidiMessage const& m)
    {
        if (m.isNoteOn())
            return MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3) + " Velocity " + String(m.getVelocity());
        if (m.isNoteOff())
            return MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3) + " Velocity " + String(m.getVelocity());
        if (m.isProgramChange())
            return String(m.getProgramChangeNumber());
        if (m.isPitchWheel())
            return String(m.getPitchWheelValue());
        if (m.isAftertouch())
            return MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3) + ": " + String(m.getAfterTouchValue());
        if (m.isChannelPressure())
            return String(m.getChannelPressureValue());
        if (m.isController())
            return String(m.getControllerValue());

        return { };
    }

    MIDIListModel& messages;
    TableListBox table;
    Label midiHistoryTitle;
    BouncingViewportAttachment bouncer;
};

class MIDIBlinker final : public Component
    , public ToolbarSource::Listener
    , public SettableTooltipClient {

public:
    MIDIBlinker()
    {
        setTooltip("MIDI activity");
        setRepaintsOnMouseActivity(true);

        lookAndFeelChanged();
    }

    void lookAndFeelChanged() override
    {
        activeColour = PlugDataColours::levelMeterActiveColour;
        bgColour = PlugDataColours::levelMeterBackgroundColour;
        textColour = PlugDataColours::toolbarTextColour;
    }

    void paint(Graphics& g) override
    {
        auto const isHovered = isMouseOver() || currentCalloutBox;

        Fonts::drawIcon(g, Icons::MIDI, getLocalBounds().removeFromLeft(16).withTrimmedTop(1), textColour.brighter(isHovered ? 0.8f : 0.0f), 13);

        auto const offsetY = getHeight() / 4.0f;
        constexpr auto offsetX = 20.0f;

        auto const midiInPos = Point<float>(offsetX, offsetY);
        auto const midiOutPos = Point<float>(offsetX, offsetY * 2.4f);

        g.setColour(blinkMidiIn ? activeColour : bgColour.brighter(isHovered ? 0.2f : 0.0f));
        g.fillEllipse(midiInPos.x, midiInPos.y, 5, 5);

        g.setColour(blinkMidiOut ? activeColour : bgColour.brighter(isHovered ? 0.2f : 0.0f));
        g.fillEllipse(midiOutPos.x, midiOutPos.y, 5, 5);
    }

    void midiReceivedChanged(bool const midiReceived) override
    {
        blinkMidiIn = midiReceived;
        repaint();
    }

    void midiSentChanged(bool const midiSent) override
    {
        blinkMidiOut = midiSent;
        repaint();
    }

    void midiMessageReceived(MidiMessage const& midiReceived) override
    {
        messages.addMessage(midiReceived, true);
    }

    void midiMessageSent(MidiMessage const& midiSent) override
    {
        messages.addMessage(midiSent, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        // check if the callout is active, otherwise mouse down / up will trigger callout box again
        if (isCallOutBoxActive) {
            isCallOutBoxActive = false;
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (!isCallOutBoxActive) {
            auto midiLogger = std::make_unique<MIDIHistory>(messages);
            auto* editor = findParentComponentOfClass<PluginEditor>();
            currentCalloutBox = &editor->showCalloutBox(std::move(midiLogger), getScreenBounds());
            isCallOutBoxActive = true;
        } else {
            isCallOutBoxActive = false;
        }
    }

    bool blinkMidiIn = false;
    bool blinkMidiOut = false;
    bool isCallOutBoxActive = false;
    MIDIListModel messages;

    Colour activeColour;
    Colour bgColour;
    Colour textColour;

    static inline SafePointer<CallOutBox> currentCalloutBox = nullptr;
};

class CPUHistoryGraph final : public Component {
public:
    CPUHistoryGraph(CircularBuffer<float>& history, int const length)
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

        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRect(bounds);

        auto bottom = bounds.getBottom();
        auto height = bounds.getHeight();
        auto const points = historyLength;
        auto const distribute = static_cast<float>(bounds.getWidth()) / points;
        Path graphTopLine;

        auto getCPUScaledY = [this, bottom, height](float const value) -> float {
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

        auto const startPoint = Point<float>(bounds.getTopLeft().getX(), getCPUScaledY(0));
        graphTopLine.startNewSubPath(startPoint);

        auto lastValues = historyGraph.last(points);

        for (int i = 0; i < points; i++) {
            auto const xPos = i * distribute + bounds.getTopLeft().getX() + distribute;
            auto const newPoint = Point<float>(xPos, getCPUScaledY(lastValues[i] * 0.01f));
            graphTopLine.lineTo(newPoint);
        }
        Path graphFilled = graphTopLine;

        graphFilled.lineTo(bounds.getBottomRight().toFloat());
        graphFilled.lineTo(bounds.getBottomLeft().toFloat());
        graphFilled.closeSubPath();
        g.setColour(PlugDataColours::levelMeterActiveColour.withAlpha(0.3f));
        g.fillPath(graphFilled);

        g.setColour(PlugDataColours::levelMeterActiveColour);
        g.strokePath(graphTopLine, PathStrokeType(1.0f));

        g.restoreState();
    }

    void updateMapping(int const mapping)
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

class CPUMeterPopup final : public Component {
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

        auto buttons = SmallArray<TextButton*> { &linear, &logA, &logB };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("cpu_meter_mapping_mode"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i] {
                SettingsFile::getInstance()->setProperty("cpu_meter_mapping_mode", i);
                cpuGraph->updateMapping(i);
                cpuGraphLongHistory->updateMapping(i);
            };
            button->setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        auto const currentMappingMode = SettingsFile::getInstance()->getProperty<int>("cpu_meter_mapping_mode");
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
        auto const buttonWidth = getWidth() / 3;
        linear.setBounds(b.removeFromLeft(buttonWidth));
        logA.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        logB.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    std::function<void()> getUpdateFunc()
    {
        return [this] {
            this->update();
        };
    }

    std::function<void()> getUpdateFuncLongHistory()
    {
        return [this] {
            this->updateLong();
        };
    }

    std::function<void()> onClose = [] { };

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

class CPUMeter final : public Component
    , public ToolbarSource::Listener
    , public Timer
    , public SettableTooltipClient {

public:
    CPUMeter()
    {
        startTimer(1000);
        setTooltip("CPU usage");
        setRepaintsOnMouseActivity(true);
    }

    void paint(Graphics& g) override
    {
        Colour textColour;
        if (isMouseOver() || currentCalloutBox)
            textColour = PlugDataColours::toolbarTextColour.brighter(0.8f);
        else
            textColour = PlugDataColours::toolbarTextColour;

        Fonts::drawIcon(g, Icons::CPU, getLocalBounds().removeFromLeft(16), textColour, 14);
        Fonts::drawFittedText(g, String(cpuUsageToDraw) + "%", getLocalBounds().withTrimmedLeft(22).withTrimmedTop(1), textColour, 1, 0.9f, 13.5, Justification::centredLeft);
    }

    void timerCallback() override
    {
        auto const lastCpuUsage = cpuUsage.last();
        auto const oldCpuUsage = cpuUsageToDraw;
        cpuUsageToDraw = round(lastCpuUsage);
        cpuUsageLongHistory.push(lastCpuUsage);
        updateCPUGraphLong();
        if (oldCpuUsage != cpuUsageToDraw)
            repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        // check if the callout is active, otherwise mouse down / up will trigger callout box again
        if (isCallOutBoxActive) {
            isCallOutBoxActive = false;
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (!isCallOutBoxActive) {
            auto cpuHistory = std::make_unique<CPUMeterPopup>(cpuUsage, cpuUsageLongHistory);
            updateCPUGraph = cpuHistory->getUpdateFunc();
            updateCPUGraphLong = cpuHistory->getUpdateFuncLongHistory();

            cpuHistory->onClose = [this] {
                updateCPUGraph = [] { };
                updateCPUGraphLong = [] { };
                repaint();
            };

            auto* editor = findParentComponentOfClass<PluginEditor>();
            currentCalloutBox = &editor->showCalloutBox(std::move(cpuHistory), getScreenBounds());
            isCallOutBoxActive = true;
        } else {
            isCallOutBoxActive = false;
        }
    }

    void cpuUsageChanged(float const newCpuUsage) override
    {
        cpuUsage.push(newCpuUsage);
        updateCPUGraph();
    }

    std::function<void()> updateCPUGraph = [] { };
    std::function<void()> updateCPUGraphLong = [] { };

    static inline SafePointer<CallOutBox> currentCalloutBox = nullptr;
    bool isCallOutBoxActive = false;

    CircularBuffer<float> cpuUsage = CircularBuffer<float>(256);
    CircularBuffer<float> cpuUsageLongHistory = CircularBuffer<float>(512);
    int cpuUsageToDraw = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUMeter);
};

ToolbarSource::ToolbarSource()
{
    startTimerHz(30);
}

void ToolbarSource::setSampleRate(double const newSampleRate)
{
    sampleRate = static_cast<int>(newSampleRate);
}

void ToolbarSource::setBufferSize(int const bufferSize)
{
    this->bufferSize = bufferSize;
}

void ToolbarSource::process(MidiBuffer const& midiInput, MidiBuffer const& midiOutput)
{
    for (auto event : midiOutput)
        lastMidiSent.enqueue(event.getMessage());
    for (auto event : midiInput)
        lastMidiReceived.enqueue(event.getMessage());

    auto hasRealEvents = [](MidiBuffer const& buffer) {
        return std::ranges::any_of(buffer,
            [](auto const& event) {
                return !event.getMessage().isSysEx();
            });
    };

    auto const nowInMs = Time::getMillisecondCounter();
    if (hasRealEvents(midiOutput))
        lastMidiSentTime.store(nowInMs);
    if (hasRealEvents(midiInput))
        lastMidiReceivedTime.store(nowInMs);
}

void ToolbarSource::prepareToPlay(int const nChannels)
{
    peakBuffer.reset(sampleRate, nChannels);
}

void ToolbarSource::timerCallback()
{
    auto const currentTime = Time::getMillisecondCounter();

    auto const hasReceivedMidi = currentTime - lastMidiReceivedTime.load() < 700;
    auto const hasSentMidi = currentTime - lastMidiSentTime.load() < 700;
    auto const hasProcessedAudio = currentTime - lastAudioProcessedTime.load() < 700;

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

    MidiMessage message;
    while (lastMidiSent.try_dequeue(message)) {
        for (auto* listener : listeners)
            listener->midiMessageSent(message);
    }
    while (lastMidiReceived.try_dequeue(message)) {
        for (auto* listener : listeners)
            listener->midiMessageReceived(message);
    }

    if (hasProcessedAudio != audioProcessedState) {
        audioProcessedState = hasProcessedAudio;
        for (auto* listener : listeners)
            listener->audioProcessedChanged(hasProcessedAudio);
    }

    auto const peak = peakBuffer.getPeak();

    for (auto* listener : listeners) {
        listener->audioLevelChanged(peak);
        listener->cpuUsageChanged(cpuUsage.load());
    }
}

void ToolbarSource::addListener(Listener* l)
{
    listeners.add(l);
}

void ToolbarSource::removeListener(Listener* l)
{
    listeners.remove_one(l);
}

void ToolbarSource::setCPUUsage(float const cpu)
{
    cpuUsage.store(cpu);
}

class PowerButton final : public Component
    , public ToolbarSource::Listener {
public:
    explicit PowerButton(PluginProcessor* processor)
        : pd(processor)
    {
        toggle.setButtonText(Icons::Power);
        toggle.setTooltip("Enable/disable DSP");
        toggle.setClickingTogglesState(true);
        toggle.setToggleState(pd_getdspstate(), dontSendNotification);
        toggle.onClick = [this] { toggle.getToggleState() ? pd->startDSP() : pd->releaseDSP(); };

        chevron.setButtonText(Icons::ThinDown);
        chevron.onClick = [this] { showCallout(); };

        addAndMakeVisible(toggle);
        addAndMakeVisible(chevron);

        setRepaintsOnMouseActivity(true);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        constexpr int chevronWidth = 14;
        toggle.setBounds(b.removeFromLeft(b.getWidth() - chevronWidth));
        chevron.setBounds(b);
    }

    void showDSPState(bool const dspState)
    {
        toggle.setToggleState(dspState, dontSendNotification);
    }

    void audioProcessedChanged(bool const audioProcessed) override
    {
        auto const colour = audioProcessed
            ? PlugDataColours::levelMeterActiveColour
            : PlugDataColours::signalColour;
        toggle.setColour(TextButton::textColourOnId, colour);
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0, 4.5f), Corners::defaultCornerRadius);

        g.setColour(PlugDataColours::levelMeterBackgroundColour.contrasting(0.1f));
        auto const x = getWidth() - 15;
        g.drawLine(x, 4.5f, x, getHeight() - 4.5f);
    }

private:
    void showCallout()
    {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        auto content = std::make_unique<AudioSettingsCallout>(editor);
        editor->showCalloutBox(std::move(content), chevron.getScreenBounds());
    }

    PluginProcessor* pd;
    SmallIconButton toggle;
    SmallIconButton chevron;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PowerButton)
};

// Used to display status icons for DAW latency, oversampling, recording
class StatusBadge final : public Component {
public:
    StatusBadge()
    {
        updater.addAnimator(animator);
        setVisible(false);
    }

    void setBadgeColour(Colour const c)
    {
        badgeColour = c;
        repaint();
    }

    void setIcon(String const& iconChar) { icon = iconChar; }
    void setHoverText(String const& text) { hoverText = text; }

    // Sets the idle text. An empty string hides the badge.
    void setText(String const& text)
    {
        if (text == idleText)
            return;
        idleText = text;
        if (!showingHover)
            animateToWidth(idleText.isEmpty() ? 0 : calcWidth(idleText));
    }

    int getDesiredWidth() const { return currentWidth; }

    void mouseEnter(MouseEvent const&) override
    {
        if (idleText.isEmpty() || hoverText.isEmpty())
            return;
        showingHover = true;
        animateToWidth(calcWidth(hoverText));
        repaint();
    }

    void mouseExit(MouseEvent const&) override
    {
        if (!showingHover)
            return;
        showingHover = false;
        animateToWidth(idleText.isEmpty() ? 0 : calcWidth(idleText));
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        if (showingHover && onClick)
            onClick();
    }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat().reduced(0, 4);
        if (bounds.getWidth() < 4)
            return;

        g.setColour(badgeColour);
        g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius);

        auto attr = makeAttributedString((getWidth() >= calcWidth(hoverText)) ? hoverText : idleText);
        attr.draw(g, bounds.reduced(4, 0));
    }

    std::function<void()> onClick;

private:
    AttributedString makeAttributedString(String const& text) const
    {
        AttributedString attr;
        attr.setJustification(Justification::centred);

        auto const textColour = badgeColour.contrasting();

        if (icon.isNotEmpty())
            attr.append(icon + " ", Fonts::getIconFont().withHeight(11.0f), textColour);

        attr.append(text, Fonts::getSemiBoldFont().withHeight(12.0f), textColour);

        return attr;
    }

    int calcWidth(String const& text) const
    {
        auto const attr = makeAttributedString(text);
        TextLayout layout;
        layout.createLayout(attr, 10000.0f);
        return static_cast<int>(std::ceil(layout.getWidth())) + 16;
    }

    void animateToWidth(int const target)
    {
        animator.complete();
        startWidth = currentWidth;
        endWidth = target;
        animator.start();
    }

    Colour badgeColour;
    String idleText;
    String hoverText;
    String icon;
    bool showingHover = false;
    int startWidth = 0;
    int endWidth = 0;
    int currentWidth = 0;

    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder {}
                            .withDurationMs(320)
                            .withEasing(Easings::createEaseInOut())
                            .withValueChangedCallback([this](float const v) {
                                currentWidth = makeAnimationLimits(startWidth, endWidth).lerp(v);
                                if (v >= 0.999f && endWidth == 0)
                                    setVisible(false);
                                if (auto* parent = getParentComponent())
                                    parent->resized();
                                repaint();
                            })
                            .build();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBadge)
};

class LimiterButton final : public TextButton {
public:
    std::function<void()> openMenu;

    LimiterButton() = default;

    void paint(Graphics& g) override
    {
        auto const inactiveColour = PlugDataColours::levelMeterBackgroundColour;
        auto const activeColour = PlugDataColours::toolbarActiveColour.interpolatedWith(PlugDataColours::toolbarBackgroundColour, 0.8f);

        constexpr float cornerRadius = Corners::defaultCornerRadius;

        auto const iconWidth = openMenu ? 14 : 0;
        auto const textSegment = getLocalBounds().withWidth(getWidth() - iconWidth);
        auto const iconSegment = getLocalBounds().withLeft(getWidth() - iconWidth);

        auto textColour = getToggleState() ? activeColour : inactiveColour;
        if (isMouseOver() && !iconSegment.contains(getMouseXYRelative())) {
            textColour = textColour.contrasting(0.2f);
        }

        g.setColour(textColour);
        Path textPath;
        textPath.addRoundedRectangle(textSegment.getX() + 0.5f, textSegment.getY() + 0.5f, textSegment.getWidth() - 1.0f, textSegment.getHeight() - 1.0f, cornerRadius, cornerRadius, true, openMenu ? false : true, true, openMenu ? false : true);
        g.fillPath(textPath);

        auto iconColour = inactiveColour;
        if (isMouseOver() && iconSegment.contains(getMouseXYRelative())) {
            iconColour = iconColour.contrasting(0.2f);
        }

        g.setColour(PlugDataColours::toolbarTextColour);
        g.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        g.drawText(getButtonText(), 0, 0, getWidth() - iconWidth, getHeight(), Justification::centred);

        if (iconWidth) {
            g.setColour(iconColour);
            Path iconPath;
            iconPath.addRoundedRectangle(iconSegment.getX() + 0.5f, iconSegment.getY() + 0.5f, iconSegment.getWidth() - 1.0f, iconSegment.getHeight() - 1.0f, cornerRadius, cornerRadius, false, true, false, true);
            g.fillPath(iconPath);

            g.setColour(PlugDataColours::toolbarTextColour);
            g.setFont(Fonts::getIconFont().withHeight(11.5f));
            g.drawText(Icons::ThinDown, getWidth() - iconWidth, 0, iconWidth, getHeight(), Justification::centred);

            g.setColour(PlugDataColours::outlineColour);
            g.drawLine(getWidth() - iconWidth, 0, getWidth() - iconWidth, getHeight());
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (openMenu && e.x > getWidth() - 14) // Icon segment
        {
            openMenu();
        } else // Text segment
        {
            TextButton::mouseDown(e);
        }
    }
};

AudioToolbar::AudioToolbar(PluginProcessor* processor, PluginEditor* editor)
    : pd(processor)
{
    cpuMeter = std::make_unique<CPUMeter>();
    midiBlinker = std::make_unique<MIDIBlinker>();
    volumeComponent = std::make_unique<VolumeComponent>();
    powerButton = std::make_unique<PowerButton>(processor);

    pd->statusbarSource->addListener(cpuMeter.get());
    pd->statusbarSource->addListener(midiBlinker.get());
    pd->statusbarSource->addListener(volumeComponent.get());
    pd->statusbarSource->addListener(powerButton.get());

    volumeComponent->setRange(0.0f, 1.0f);
    volumeComponent->setValue(0.8f);
    volumeComponent->setDoubleClickReturnValue(true, 0.8f);

    if (ProjectInfo::isStandalone) {
        volumeComponent->onValueChange = [this] {
            pd->volume->store(volumeComponent->getValue());
        };
    } else {
        volumeAttachment = std::make_unique<SliderParameterAttachment>(
            *dynamic_cast<RangedAudioParameter*>(pd->getParameters()[0]),
            *volumeComponent, nullptr);
    }

    limiterButton = std::make_unique<LimiterButton>();
    limiterButton->setButtonText("Limit");
    limiterButton->setToggleState(pd->getEnableLimiter(), dontSendNotification);
    limiterButton->setClickingTogglesState(true);

    limiterButton->onStateChange = [this] {
        limiterButton->setTooltip(limiterButton->getToggleState() ? "Disable limiter" : "Enable limiter");
    };

    limiterButton->onClick = [this] {
        auto const state = limiterButton->getToggleState();
        pd->setEnableLimiter(state);
        SettingsFile::getInstance()->setProperty("protected", state);
    };

    dawLatencyBadge = std::make_unique<StatusBadge>();
    dawLatencyBadge->setIcon(Icons::GlyphDelay);
    dawLatencyBadge->setHoverText("Reset latency");
    addChildComponent(dawLatencyBadge.get());
    dawLatencyBadge->onClick = [this] {
        pd->performLatencyCompensationChange(0);
    };

    oversamplingBadge = std::make_unique<StatusBadge>();
    oversamplingBadge->setHoverText("Reset oversampling");

    addChildComponent(oversamplingBadge.get());
    oversamplingBadge->onClick = [this] {
        pd->setOversampling(0);
        updateOversampling();
    };
    
    updateOversampling();

    setLatencyDisplay(pd->getLatencySamples() - pd::Instance::getBlockSize());

    addAndMakeVisible(*limiterButton);

    addAndMakeVisible(*cpuMeter);
    addAndMakeVisible(*midiBlinker);
    addAndMakeVisible(*volumeComponent);
    addAndMakeVisible(*powerButton);

    lookAndFeelChanged();
}

AudioToolbar::~AudioToolbar()
{
    pd->statusbarSource->removeListener(cpuMeter.get());
    pd->statusbarSource->removeListener(midiBlinker.get());
    pd->statusbarSource->removeListener(volumeComponent.get());
    pd->statusbarSource->removeListener(powerButton.get());
}

void AudioToolbar::showDSPState(bool const dspState) {
    powerButton->showDSPState(dspState);
}

void AudioToolbar::updateOversampling()
{
    int const oversampling = std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3);

    SmallArray<String, 4> const factors = { "1x", "2x", "4x", "8x" };
    oversamplingBadge->setText(factors[oversampling]);
    oversamplingBadge->setVisible(oversampling);
}

void AudioToolbar::lookAndFeelChanged()
{
    oversamplingBadge->setBadgeColour(PlugDataColours::levelMeterActiveColour);
    dawLatencyBadge->setBadgeColour(PlugDataColours::levelMeterActiveColour);
}

void AudioToolbar::resized()
{
    auto b = getLocalBounds().reduced(4, 0);

    // Power button on the right
    auto powerBounds = b.removeFromRight(38).translated(-3, 0);
    powerButton->setBounds(powerBounds);

    auto limiterBounds = b.removeFromRight(50).translated(-7, 0).reduced(3, 4);
    limiterButton->setBounds(limiterBounds);

    // Volume gets the remaining space
    volumeComponent->setBounds(b.removeFromRight(120).reduced(0, 2));

    midiBlinker->setBounds(b.removeFromRight(26));
    cpuMeter->setBounds(b.removeFromRight(54));

    b.removeFromRight(8);

    if (dawLatencyBadge->isVisible()) {
        dawLatencyBadge->setBounds(b.removeFromRight(dawLatencyBadge->getDesiredWidth()).reduced(0, 1));
    }
    if (oversamplingBadge->isVisible()) {
        oversamplingBadge->setBounds(b.removeFromRight(oversamplingBadge->getDesiredWidth()).reduced(0, 1));
    }
}

void AudioToolbar::showLimiterState(bool enabled)
{
    limiterButton->setToggleState(enabled, dontSendNotification);
}

void AudioToolbar::setLatencyDisplay(int samples)
{
    if (!ProjectInfo::isStandalone) {
        dawLatencyBadge->setVisible(samples);
        dawLatencyBadge->setText(String("Latency: " + samples));
    }
}

#if JUCE_MAC
void AudioToolbar::ToolbarDragListener::mouseEnter(MouseEvent const& e)
{
    if (parent->isMouseOverOrDragging(true)) {
        if (auto const* topLevel = parent->getTopLevelComponent()) {
            if (auto* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer, false);
            }
        }
    }
}

void AudioToolbar::ToolbarDragListener::mouseExit(MouseEvent const& e)
{
    if (!parent->isMouseOverOrDragging(true)) {
        if (auto const* topLevel = parent->getTopLevelComponent()) {
            if (auto* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer, true);
            }
        }
    }
}
#endif
