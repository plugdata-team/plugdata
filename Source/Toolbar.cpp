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
#include "Components/DraggableNumber.h"
#include "Dialogs/Dialogs.h"
#include "Utility/MidiDeviceManager.h"

class IconTextButton final : public TextButton {
public:
    IconTextButton(String const& icon, String const& text)
        : icon(icon)
    {
        setButtonText(text);
    }

    void setActiveColour(Colour const& background, Colour const& text)
    {
        activeBackground = background;
        activeText = text;
        repaint();
    }

private:
    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().reduced(4, 0);
        bool const on = getToggleState();

        // Background
        Colour background = on && activeBackground.has_value() ? *activeBackground : PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            background = background.contrasting(0.04f);

        g.setColour(background);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Text + icon colour
        Colour const foreground = on && activeBackground.has_value() ? activeText : PlugDataColours::popupMenuTextColour;

        g.setColour(foreground);

        g.setFont(Fonts::getCurrentFont().withHeight(14.f));
        g.drawText(getButtonText(), bounds, Justification::centred, true);

        // Icon on the left
        auto iconBounds = bounds.removeFromLeft(getHeight());
        Fonts::drawIcon(g, icon, iconBounds, foreground, 14);
    }

    String icon;
    std::optional<Colour> activeBackground;
    Colour activeText;
};

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
    VolumeComponent()
        : Slider(Slider::LinearHorizontal, Slider::NoTextBox)
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
    explicit MIDIHistory(PluginEditor* editor, MIDIListModel& model)
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

        midiSettingsButton.onClick = [this, editor] {
            Dialogs::showSettingsDialog(editor, 1);
            closeCalloutBox();
        };
        addAndMakeVisible(midiSettingsButton);

        messages.onChange = [&] { table.updateContent(); };
        setSize(278, 200);
    }

    void closeCalloutBox()
    {
        MessageManager::callAsync([_callout = SafePointer(findParentComponentOfClass<CallOutBox>())]() {
            if (_callout)
                _callout->dismiss();
        });
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(6, 32).toFloat(), Corners::defaultCornerRadius);

        g.setColour(PlugDataColours::outlineColour);
        g.drawLine(6, 58, getWidth() - 6, 58);
    }

    ~MIDIHistory() override { messages.onChange = nullptr; }

    void resized() override
    {
        auto bounds = getLocalBounds();
        midiHistoryTitle.setBounds(0, 6, getWidth(), 20);

        midiSettingsButton.setBounds(bounds.removeFromBottom(32).reduced(2, 4).withTrimmedTop(1));
        table.setBounds(bounds.withTrimmedTop(32).reduced(6, 0));
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
    IconTextButton midiSettingsButton = IconTextButton(Icons::MIDI, "MIDI Settings...");
    ;
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
            auto* editor = findParentComponentOfClass<PluginEditor>();
            auto midiLogger = std::make_unique<MIDIHistory>(editor, messages);
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
        listener->recordingTimeChanged(recorderTime.load());
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

void ToolbarSource::setRecorderTime(float const time)
{
    recorderTime.store(time);
}

// Used to display status icons for DAW latency, oversampling, recording
class StatusBadge final : public Component {
public:
    StatusBadge()
    {
        updater.addAnimator(animator);
        setVisible(false);
    }

    void setBadgeColour(Colour const badge, Colour const text)
    {
        badgeColour = badge;
        textColour = text;
        repaint();
    }

    void setIcon(String const& iconChar) { icon = iconChar; }
    void setHoverText(String const& text)
    {
        hoverText = text;
        hoverWidth = calcWidth(hoverText);
    }

    void setText(String const& text)
    {
        if (text == idleText)
            return;
        idleText = text;
        idleWidth = calcWidth(idleText);
        if (!showingHover)
            animateToWidth(idleText.isEmpty() ? 0 : idleWidth);
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

        auto hover = isMouseOver() ? hoverText : idleText;
        auto const longerFits = getWidth() >= std::max(idleWidth, hoverWidth);
        auto const& text = longerFits ? (idleWidth >= hoverWidth ? idleText : hover)
                                      : (idleWidth < hoverWidth ? idleText : hover);

        TextLayout layout;
        layout.createLayout(makeAttributedString(text), 10000, 23);
        layout.draw(g, bounds.reduced(4, 0));
    }

    std::function<void()> onClick;

private:
    AttributedString makeAttributedString(String const& text) const
    {
        AttributedString attr;
        attr.setJustification(Justification::centred);

        if (icon.isNotEmpty()) {
            attr.append(icon, Fonts::getIconFont().withHeight(11.0f), textColour);
        }

        attr.append(icon.isNotEmpty() ? " " + text : text, Fonts::getSemiBoldFont().withHeight(12.0f), textColour);

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

    Colour badgeColour, textColour;
    String idleText;
    String hoverText;
    String icon;
    bool showingHover = false;
    int startWidth = 0;
    int endWidth = 0;
    int currentWidth = 0;
    int hoverWidth = 0;
    int idleWidth = 0;

    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder { }
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
    LimiterButton() = default;

    void paint(Graphics& g) override
    {
        auto const inactiveColour = PlugDataColours::levelMeterBackgroundColour;
        auto const activeColour = PlugDataColours::toolbarActiveColour.interpolatedWith(PlugDataColours::toolbarBackgroundColour, 0.8f);

        constexpr float cornerRadius = Corners::defaultCornerRadius;

        auto const textSegment = getLocalBounds().withWidth(getWidth());
        auto const iconSegment = getLocalBounds().withLeft(getWidth());

        auto textColour = getToggleState() ? activeColour : inactiveColour;
        if (isMouseOver() && !iconSegment.contains(getMouseXYRelative())) {
            textColour = textColour.contrasting(0.2f);
        }

        g.setColour(textColour);
        Path textPath;
        textPath.addRoundedRectangle(textSegment.getX() + 0.5f, textSegment.getY() + 0.5f, textSegment.getWidth() - 1.0f, textSegment.getHeight() - 1.0f, cornerRadius, cornerRadius, true, true, true, true);
        g.fillPath(textPath);

        auto iconColour = inactiveColour;
        if (isMouseOver() && iconSegment.contains(getMouseXYRelative())) {
            iconColour = iconColour.contrasting(0.2f);
        }

        g.setColour(PlugDataColours::toolbarTextColour);
        g.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        g.drawText(getButtonText(), 0, 0, getWidth(), getHeight(), Justification::centred);
    }
};

class OversampleSettings final : public Component {
public:
    std::function<void(int)> onChange = [](int) { };

    explicit OversampleSettings(int const currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        eight.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = Array<TextButton*> { &one, &two, &four, &eight };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i] {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        buttons[currentSelection]->setToggleState(true, dontSendNotification);

        setSize(180, 50);
    }

private:
    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 4);
        auto const buttonWidth = b.getWidth() / 4;

        one.setBounds(b.removeFromLeft(buttonWidth));
        two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        eight.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    TextButton one = TextButton("1x");
    TextButton two = TextButton("2x");
    TextButton four = TextButton("4x");
    TextButton eight = TextButton("8x");
};

class LimiterSettings final : public Component {
public:
    std::function<void(int)> onChange = [](int) { };

    explicit LimiterSettings(int const currentSelection)
    {
        one.setConnectedEdges(Button::ConnectedOnRight);
        two.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        three.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        four.setConnectedEdges(Button::ConnectedOnLeft);

        auto buttons = SmallArray<TextButton*> { &one, &two, &three, &four };

        int i = 0;
        for (auto* button : buttons) {
            button->setRadioGroupId(hash("oversampling_selector"));
            button->setClickingTogglesState(true);
            button->onClick = [this, i] {
                onChange(i);
            };

            button->setColour(TextButton::textColourOffId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::textColourOnId, PlugDataColours::popupMenuTextColour);
            button->setColour(TextButton::buttonColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::popupMenuBackgroundColour.contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            i++;
        }

        buttons[currentSelection]->setToggleState(true, dontSendNotification);

        setSize(180, 50);
    }

private:
    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 4);
        auto const buttonWidth = b.getWidth() / 4;

        one.setBounds(b.removeFromLeft(buttonWidth));
        two.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        three.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
        four.setBounds(b.removeFromLeft(buttonWidth).expanded(1, 0));
    }

    TextButton one = TextButton("-12db");
    TextButton two = TextButton("-6db");
    TextButton three = TextButton("0db");
    TextButton four = TextButton("3db");
};

class AudioSettingsCallout final : public Component
    , public ToolbarSource::Listener
    , public Value::Listener {
public:
    explicit AudioSettingsCallout(PluginEditor* editor)
        : pd(editor->pd)
        , limiterSettings(SettingsFile::getInstance()->getProperty<int>("limiter_threshold"))
        , oversampleSettings(std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3))
    {
        limiterLabel.setText("Limiter", dontSendNotification);
        limiterLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        addAndMakeVisible(limiterLabel);
        limiterSettings.onChange = [this](int const value) {
            pd->setLimiterThreshold(value);
        };
        addAndMakeVisible(limiterSettings);

        oversamplingLabel.setText("Oversampling", dontSendNotification);
        oversamplingLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        addAndMakeVisible(oversamplingLabel);
        oversampleSettings.onChange = [this, editor](int const value) {
            pd->setOversampling(value);
            editor->audioToolbar->updateOversampling();
        };
        addAndMakeVisible(oversampleSettings);

        recordButton.setActiveColour(Colour(245, 62, 62), Colours::white);
        recordButton.onClick = [this, editor]() {
            bool recording = editor->pd->toggleRecording(editor);
            if (!recording)
                closeCalloutBox();
        };
        addAndMakeVisible(recordButton);

        audioSettingsButton.onClick = [this, editor] {
            Dialogs::showSettingsDialog(editor, 0);
            closeCalloutBox();
        };
        addAndMakeVisible(audioSettingsButton);

        pd->toolbarSource->addListener(this);

        if (ProjectInfo::isStandalone) {
            setSize(200, 180);
        } else {
            latencyLabel.setText("Latency compensation (samples)", dontSendNotification);
            latencyLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
            addAndMakeVisible(latencyLabel);

            latencyValue.addListener(this);
            latencyValue = pd->getLatencySamples() - pd::Instance::getBlockSize();

            latencyNumber.setEditableOnClick(true);
            latencyNumber.setMinimum(0);
            latencyNumber.setMaximum(1 << 30);
            latencyNumber.setFont(latencyNumber.getFont().withHeight(14.0f));
            latencyNumber.setText(latencyValue.toString(), dontSendNotification);
            latencyNumber.onValueChange = [this](double const v) {
                latencyValue = static_cast<int>(v);
            };
            latencyNumber.onReturnKey = [this](double const v) {
                latencyValue = static_cast<int>(v);
            };
            addAndMakeVisible(latencyNumber);

            // Tail length (float seconds)
            tailLengthLabel.setText("Tail length (seconds)", dontSendNotification);
            tailLengthLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
            addAndMakeVisible(tailLengthLabel);

            tailLengthValue.referTo(pd->tailLength);

            tailLengthNumber.setEditableOnClick(true);
            tailLengthNumber.setPrecision(3);
            tailLengthNumber.setFont(tailLengthNumber.getFont().withHeight(14.0f));
            tailLengthNumber.setText(tailLengthValue.toString(), dontSendNotification);
            tailLengthNumber.onValueChange = [this](double const v) {
                tailLengthValue = static_cast<float>(v);
            };
            tailLengthNumber.onReturnKey = [this](double const v) {
                tailLengthValue = static_cast<float>(v);
            };
            addAndMakeVisible(tailLengthNumber);

            setSize(200, 290);
        }
    }

    ~AudioSettingsCallout() override
    {
        pd->toolbarSource->removeListener(this);
    }

    void paint(Graphics& g) override
    {
        if (!ProjectInfo::isStandalone) {
            g.setColour(PlugDataColours::popupMenuBackgroundColour.contrasting(0.04f));
            g.fillRoundedRectangle(tailLengthNumber.getBounds().reduced(0, 2).toFloat(), Corners::defaultCornerRadius);
            g.fillRoundedRectangle(latencyNumber.getBounds().reduced(0, 2).toFloat(), Corners::defaultCornerRadius);
        }
    }

    void recordingTimeChanged(float const time) override
    {
        if (time > 0) {
            int const secs = static_cast<int>(time);
            recordButton.setButtonText(String::formatted("Record (%d:%02d)", secs / 60, secs % 60));
            recordButton.setToggleState(true, dontSendNotification);
        } else {
            recordButton.setButtonText("Record");
            recordButton.setToggleState(false, dontSendNotification);
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(latencyValue)) {
            pd->performLatencyCompensationChange(getValue<int>(latencyValue));
            latencyNumber.setValue(getValue<int>(latencyValue), dontSendNotification);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8);
        constexpr int labelHeight = 18;
        constexpr int selectorHeight = 28;
        constexpr int gap = 8;

        limiterLabel.setBounds(b.removeFromTop(labelHeight));
        limiterSettings.setBounds(b.removeFromTop(selectorHeight));
        b.removeFromTop(gap);

        oversamplingLabel.setBounds(b.removeFromTop(labelHeight));
        oversampleSettings.setBounds(b.removeFromTop(selectorHeight));
        b.removeFromTop(gap);

        if (!ProjectInfo::isStandalone) {
            latencyLabel.setBounds(b.removeFromTop(labelHeight));
            latencyNumber.setBounds(b.removeFromTop(selectorHeight));
            b.removeFromTop(gap);

            tailLengthLabel.setBounds(b.removeFromTop(labelHeight));
            tailLengthNumber.setBounds(b.removeFromTop(selectorHeight));
            b.removeFromTop(gap + 2);
        }

        recordButton.setBounds(b.removeFromTop(selectorHeight - 4));
        b.removeFromTop(gap);
        audioSettingsButton.setBounds(b.removeFromTop(selectorHeight - 4));
    }

    void closeCalloutBox()
    {
        MessageManager::callAsync([_callout = SafePointer(findParentComponentOfClass<CallOutBox>())]() {
            if (_callout)
                _callout->dismiss();
        });
    }

private:
    PluginProcessor* pd;

    Label limiterLabel;
    LimiterSettings limiterSettings;

    Label oversamplingLabel;
    OversampleSettings oversampleSettings;

    Label latencyLabel;
    Value latencyValue;
    DraggableNumber latencyNumber { true };

    Label tailLengthLabel;
    Value tailLengthValue;
    DraggableNumber tailLengthNumber { false };

    IconTextButton recordButton = IconTextButton(Icons::Record, "Record");
    IconTextButton audioSettingsButton = IconTextButton(Icons::AudioSettings, "Audio Settings...");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsCallout)
};

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

        chevron.setTooltip("DSP options");
        chevron.setButtonText(Icons::ThinDown);
        chevron.onClick = [this] {
            showCallout();
        };

        toggle.addMouseListener(this, false);
        chevron.addMouseListener(this, false);

        addAndMakeVisible(toggle);
        addAndMakeVisible(chevron);
        setRepaintsOnMouseActivity(true);
    }

    ~PowerButton() override
    {
        toggle.removeMouseListener(this);
        chevron.removeMouseListener(this);
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

    void mouseEnter(MouseEvent const& e) override { updateHover(); }
    void mouseExit(MouseEvent const& e) override { updateHover(); }
    void mouseMove(MouseEvent const& e) override { updateHover(); }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat().reduced(0, 4.5f);
        constexpr float cornerRadius = Corners::defaultCornerRadius;
        auto const chevronWidth = 14.0f;

        auto const togglePart = bounds.withWidth(bounds.getWidth() - chevronWidth);
        auto const chevronPart = bounds.withLeft(bounds.getRight() - chevronWidth);

        auto const baseColour = PlugDataColours::levelMeterBackgroundColour;
        auto const hoverColour = baseColour.contrasting(0.06f);

        {
            Path p;
            p.addRoundedRectangle(togglePart.getX(), togglePart.getY(),
                togglePart.getWidth(), togglePart.getHeight(),
                cornerRadius, cornerRadius,
                true, false, true, false);
            g.setColour(toggleHovered ? hoverColour : baseColour);
            g.fillPath(p);
        }
        {
            Path p;
            p.addRoundedRectangle(chevronPart.getX(), chevronPart.getY(),
                chevronPart.getWidth(), chevronPart.getHeight(),
                cornerRadius, cornerRadius,
                false, true, false, true);
            g.setColour(chevronHovered ? hoverColour : baseColour);
            g.fillPath(p);
        }

        g.setColour(baseColour.contrasting(0.06f));
        auto const x = getWidth() - 15.0f;
        g.drawLine(x, 4.5f, x, getHeight() - 4.5f);
    }

private:
    void updateHover()
    {
        auto const mousePos = getMouseXYRelative();
        bool const inToggle = toggle.getBounds().contains(mousePos);
        bool const inChevron = chevron.getBounds().contains(mousePos);

        if (inToggle != toggleHovered || inChevron != chevronHovered) {
            toggleHovered = inToggle;
            chevronHovered = inChevron;
            repaint();
        }
    }

    void showCallout()
    {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        auto content = std::make_unique<AudioSettingsCallout>(editor);
        editor->showCalloutBox(std::move(content), chevron.getScreenBounds().translated(-26, 0));
        chevronHovered = false;
        repaint();
    }

    PluginProcessor* pd;
    SmallIconButton toggle;
    SmallIconButton chevron;
    bool toggleHovered = false;
    bool chevronHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PowerButton)
};

AudioToolbar::AudioToolbar(PluginProcessor* processor, PluginEditor* editor)
    : pd(processor)
{
    cpuMeter = std::make_unique<CPUMeter>();
    midiBlinker = std::make_unique<MIDIBlinker>();
    volumeComponent = std::make_unique<VolumeComponent>();
    powerButton = std::make_unique<PowerButton>(processor);

    pd->toolbarSource->addListener(cpuMeter.get());
    pd->toolbarSource->addListener(midiBlinker.get());
    pd->toolbarSource->addListener(volumeComponent.get());
    pd->toolbarSource->addListener(powerButton.get());
    pd->toolbarSource->addListener(this);

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

    recordingBadge = std::make_unique<StatusBadge>();
    recordingBadge->setIcon(Icons::Record);
    recordingBadge->setHoverText("Stop recording");
    addChildComponent(recordingBadge.get());
    recordingBadge->onClick = [this, editor] {
        pd->toggleRecording(editor);
    };

    updateOversampling();

    setLatencyDisplay(pd->getLatencySamples() - pd::Instance::getBlockSize());

    addAndMakeVisible(*limiterButton);

    addAndMakeVisible(*cpuMeter);
    addAndMakeVisible(*midiBlinker);
    addAndMakeVisible(*volumeComponent);
    addAndMakeVisible(*powerButton);

    setInterceptsMouseClicks(false, true);

    lookAndFeelChanged();
}

AudioToolbar::~AudioToolbar()
{
    pd->toolbarSource->removeListener(cpuMeter.get());
    pd->toolbarSource->removeListener(midiBlinker.get());
    pd->toolbarSource->removeListener(volumeComponent.get());
    pd->toolbarSource->removeListener(powerButton.get());
    pd->toolbarSource->removeListener(this);
}

void AudioToolbar::showDSPState(bool const dspState)
{
    powerButton->showDSPState(dspState);
}

void AudioToolbar::recordingTimeChanged(float const time)
{
    bool wasVisible = recordingBadge->isVisible();
    recordingBadge->setVisible(time > 0);
    int const secs = static_cast<int>(time);
    recordingBadge->setText(String::formatted("Recording (%d:%02d)", secs / 60, secs % 60));
    if (wasVisible != recordingBadge->isVisible())
        resized();
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
    oversamplingBadge->setBadgeColour(PlugDataColours::levelMeterActiveColour, PlugDataColours::levelMeterActiveColour.contrasting());
    dawLatencyBadge->setBadgeColour(PlugDataColours::levelMeterActiveColour, PlugDataColours::levelMeterActiveColour.contrasting());
    recordingBadge->setBadgeColour(Colour(245, 62, 62), Colours::white);
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
        b.removeFromRight(8);
    }
    if (oversamplingBadge->isVisible()) {
        oversamplingBadge->setBounds(b.removeFromRight(oversamplingBadge->getDesiredWidth()).reduced(0, 1));
        b.removeFromRight(8);
    }
    if (recordingBadge->isVisible()) {
        recordingBadge->setBounds(b.removeFromRight(recordingBadge->getDesiredWidth()).reduced(0, 1));
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
        dawLatencyBadge->setText("Latency: " + String(samples));
    }
}

#if JUCE_MAC
static void setWindowMovable(Component* parent, bool movable)
{
    if (auto const* topLevel = parent->getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, movable);
        }
    }
}

void AudioToolbar::ToolbarDragListener::mouseEnter(MouseEvent const& e)
{
    setWindowMovable(parent, false);
}

void AudioToolbar::ToolbarDragListener::mouseExit(MouseEvent const& e)
{
    setWindowMovable(parent, true);
}
#endif
