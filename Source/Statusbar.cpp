/*
 // Copyright (c) 2021-2025 Timothy Schoen.
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
#include "Utility/MidiDeviceManager.h"

#include "Sidebar/CommandInput.h"

class CommandButton final : public Component
    , public MultiTimer {
    Label leftText;
    Component hitArea;

    Colour bgCol;
    Colour textCol;

    float tW, tH;
    float cW, cH;
    float alpha = 0.0f;
    static constexpr int textHeight = 14;
    int textWidth = 0;

public:
    CommandButton()
    {
        addAndMakeVisible(leftText);
        addAndMakeVisible(hitArea);

        hitArea.addMouseListener(this, false);

        lookAndFeelChanged();

        setCommandButtonText();

        setSize(25, 20);
    }

    ~CommandButton() override
    {
        hitArea.removeMouseListener(this);
    }

    void lookAndFeelChanged() override
    {
        leftText.setFont(Fonts::getDefaultFont().withHeight(textHeight));
        textCol = findColour(PlugDataColour::toolbarTextColourId).withAlpha(0.5f);
        bgCol = findColour(PlugDataColour::panelBackgroundColourId).contrasting();

        leftText.setColour(Label::textColourId, textCol.withMultipliedAlpha(alpha));

        repaint();
    }

    void paint(Graphics& g) override
    {
        auto const bgColour = isMouseOver(true) ? bgCol.withAlpha(0.15f) : bgCol.withAlpha(0.05f);
        g.setColour(bgColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarTextColourId));
        g.drawText(">", getWidth() - 22, 0, 20, 20, Justification::centred);
    }

    void mouseDown(MouseEvent const& e) override
    {
        onClick();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void setCommandButtonText()
    {
        auto empty = String();
        setCommandButtonText(empty);
    }

    void setCommandButtonText(String& text)
    {
        // Don't display text for selection that is empty
        if (text == "empty")
            text = "";

        auto const prevText = leftText.getText();

        if (prevText != text || text.isEmpty()) {
            leftText.setText(text, dontSendNotification);
            Font const font = Fonts::getDefaultFont().withHeight(textHeight);
            textWidth = ceil(font.getStringWidthFloat(text));

            animateTo(textWidth + 25, 20);

            // We dont want to flash the alpha animation when the text is going to be very similar
            if (!(text.containsWholeWord("selected)") && prevText.containsWholeWord("selected)"))) {
                alpha = 0.0f;
                startTimer(1, 1000 / 60);
            }
        }
    }

    void animateTo(int const w, int const h)
    {
        tW = w;
        tH = h;

        if (getWidth() != w || getHeight() != h) {
            cW = getWidth();
            cH = getHeight();

            startTimer(0, 1000 / 60);
        }
    }

    void timerCallback(int const ID) override
    {
        if (ID == 0) {
            auto const heightSpeed = abs(cH - tH) / 5;
            auto const widthSpeed = abs(cW - tW) / 5;

            // Smoothly adjust the width towards the target width (tW)
            if (std::abs(cW - tW) > 1.0f) {
                if (cW < tW)
                    cW += widthSpeed;
                else
                    cW -= widthSpeed;
            } else {
                cW = tW;
            }

            // Smoothly adjust the height towards the target height (tH)
            if (std::abs(cH - tH) > 1.0f) {
                if (cH < tH)
                    cH += heightSpeed;
                else
                    cH -= heightSpeed;
            } else {
                cH = tH;
            }

            setSize(cW, cH);

            if (std::abs(cW - tW) <= 1.0f && std::abs(cH - tH) <= 1.0f)
                stopTimer(0);
        } else if (ID == 1) {
            if (alpha < 1.0f) {
                alpha += 1.0f / 45;
                leftText.setColour(Label::textColourId, textCol.withMultipliedAlpha(alpha));
            } else {
                alpha = 1.0f;
                stopTimer(1);
            }
        }
    }

    void resized() override
    {
        auto const b = getLocalBounds().removeFromLeft(textWidth + 12);
        leftText.setBounds(b);

        hitArea.setBounds(getLocalBounds());

        if (getParentComponent())
            getParentComponent()->resized();
    }

    std::function<void()> onClick = [] { };
};

class StatusbarTextButton final : public TextButton {
public:
    std::function<void()> openMenu;

    StatusbarTextButton() = default;

    void paint(Graphics& g) override
    {
        auto const inactiveColour = findColour(PlugDataColour::levelMeterBackgroundColourId);
        auto const activeColour = findColour(PlugDataColour::toolbarActiveColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.8f);

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

        g.setColour(findColour(PlugDataColour::toolbarTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(13.5f));
        g.drawText(getButtonText(), 0, 0, getWidth() - iconWidth, getHeight(), Justification::centred);

        if (iconWidth) {
            g.setColour(iconColour);
            Path iconPath;
            iconPath.addRoundedRectangle(iconSegment.getX() + 0.5f, iconSegment.getY() + 0.5f, iconSegment.getWidth() - 1.0f, iconSegment.getHeight() - 1.0f, cornerRadius, cornerRadius, false, true, false, true);
            g.fillPath(iconPath);

            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
            g.setFont(Fonts::getIconFont().withHeight(11.5f));
            g.drawText(Icons::ThinDown, getWidth() - iconWidth, 0, iconWidth, getHeight(), Justification::centred);

            g.setColour(findColour(PlugDataColour::outlineColourId));
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

class LatencyDisplayButton final : public Component
    , public SettableTooltipClient {
    Label latencyValue;
    Label icon;
    bool isHovered = false;
    Colour bgColour;
    int currentLatencyValue = 0;

public:
    std::function<void()> onClick = [] { };
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
    }

    void lookAndFeelChanged() override
    {
        buttonStateChanged();
    }

    void paint(Graphics& g) override
    {
        auto const b = getLocalBounds().reduced(1, 6).toFloat();
        g.setColour(bgColour);
        g.fillRoundedRectangle(b, Corners::defaultCornerRadius);
    }

    void setLatencyValue(int const value)
    {
        currentLatencyValue = value;
        setVisible(value != 0);
        buttonStateChanged();

        if (auto* parent = getParentComponent()) {
            parent->resized();
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        onClick();
    }

    void buttonStateChanged()
    {
        bgColour = getLookAndFeel().findColour(isHovered ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarActiveColourId);
        auto const textColour = bgColour.contrasting();
        icon.setColour(Label::textColourId, textColour);
        latencyValue.setColour(Label::textColourId, textColour);

        if (isHovered) {
            latencyValue.setJustificationType(Justification::centredLeft);
            latencyValue.setText("Reset", dontSendNotification);
        } else {
            latencyValue.setJustificationType(Justification::centredRight);
            latencyValue.setText(String(currentLatencyValue) + " smpl", dontSendNotification);
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovered = true;
        buttonStateChanged();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovered = false;
        buttonStateChanged();
    }

    void resized() override
    {
        icon.setBounds(0, 0, getHeight(), getHeight());
        latencyValue.setBounds(getHeight(), 0, getWidth() - getHeight(), getHeight());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyDisplayButton);
};

class VolumeSlider final : public Slider
    , public Slider::Listener {

    class VolumeSliderDecibelPopup final : public Component {
    public:
        VolumeSliderDecibelPopup()
        {
            setAlwaysOnTop(true);
        }

        void paint(Graphics& g) override
        {
            g.fillAll(findColour(PlugDataColour::levelMeterBackgroundColourId));
            g.setColour(findColour(PlugDataColour::toolbarTextColourId).withAlpha(0.666f));
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
    VolumeSlider()
        : Slider(Slider::LinearHorizontal, Slider::NoTextBox)
    {
        setSliderSnapsToMousePosition(false);
        addListener(this);
        addChildComponent(decibelPopup);
    }

    void resized() override
    {
        setMouseDragSensitivity(getWidth() - margin * 2);
    }

    void sliderValueChanged(Slider*) override
    {
        updatePopup(getMouseXYRelative());
    }

    void updatePopup(Point<int> const mousePosition)
    {
        auto const value = getValue();
        auto const thumbSize = getHeight() * 0.7f;
        auto const sliderPosition = Point<int>(margin + value * (getWidth() - margin * 2), getHeight() * 0.5f);
        auto const thumb = Rectangle<int>(thumbSize, thumbSize).withCentre(sliderPosition);

        decibelPopup.setValue(value);

        if (auto const shouldBeVisible = thumb.contains(mousePosition)) {
            if (value > 0.5f) {
                decibelPopup.setBounds(Rectangle<int>(18, 2, 34, getHeight() - 4));
                decibelPopup.setJustification(Justification::left);
            } else {
                decibelPopup.setBounds(Rectangle<int>(getWidth() - 50, 2, 34, getHeight() - 4));
                decibelPopup.setJustification(Justification::right);
            }

            if (shouldBeVisible != decibelPopup.isVisible()) {
                Desktop::getInstance().getAnimator().fadeIn(&decibelPopup, 200);
            }
        } else {
            if (shouldBeVisible != decibelPopup.isVisible()) {
                Desktop::getInstance().getAnimator().fadeOut(&decibelPopup, 200);
            }
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseEnter(e);
        updatePopup(e.getPosition());
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseExit(e);
        updatePopup(e.getPosition());
    }

    void mouseMove(MouseEvent const& e) override
    {
        repaint();
        Slider::mouseMove(e);
        updatePopup(e.getPosition());
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

    void paint(Graphics& g) override
    {
        auto const backgroundColour = findColour(PlugDataColour::levelMeterThumbColourId);

        auto const value = getValue();
        auto const thumbSize = getHeight() * 0.7f;
        auto const position = Point<float>(margin + value * (getWidth() - margin * 2), getHeight() * 0.5f);
        auto thumb = Rectangle<float>(thumbSize, thumbSize).withCentre(position);
        thumb = thumb.withSizeKeepingCentre(thumb.getWidth() - 12, thumb.getHeight());
        g.setColour(backgroundColour.darker(thumb.contains(getMouseXYRelative().toFloat()) ? 0.3f : 0.0f).withAlpha(0.8f));
        g.fillRoundedRectangle(thumb, Corners::defaultCornerRadius * 0.5f);
    }

private:
    VolumeSliderDecibelPopup decibelPopup;
    int margin = 18;
};

class LevelMeter final : public Component
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

        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(x + outerBorderWidth + 4, outerBorderWidth, bgWidth - 8, bgHeight, Corners::defaultCornerRadius);

        for (int ch = 0; ch < numChannels; ch++) {
            auto const barYPos = outerBorderWidth + (ch + 1) * (bgHeight / 3.0f) - halfBarHeight;
            auto const barLength = jmin(audioLevel[ch] * barWidth, barWidth);
            auto const peekPos = jmin(peakLevel[ch] * barWidth, barWidth);

            if (peekPos > 1) {
                g.setColour(clipping[ch] ? Colours::red : findColour(PlugDataColour::levelMeterActiveColourId));
                g.fillRect(leftOffset, barYPos, barLength, barHeight);
                g.fillRect(leftOffset + peekPos, barYPos, 1.0f, barHeight);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
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
        g.setColour(getLookAndFeel().findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().withTrimmedTop(32).toFloat(), Corners::defaultCornerRadius);

        g.setColour(findColour(PlugDataColour::outlineColourId));
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

        auto* label = new Label({}, [&] {
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

        return {};
    }

    MIDIListModel& messages;
    TableListBox table;
    Label midiHistoryTitle;
    BouncingViewportAttachment bouncer;
};

class MIDIBlinker final : public Component
    , public StatusbarSource::Listener
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
        activeColour = findColour(PlugDataColour::levelMeterActiveColourId);
        bgColour = findColour(PlugDataColour::levelMeterBackgroundColourId);
        textColour = findColour(PlugDataColour::toolbarTextColourId);
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
        if (!ModifierKeys::getCurrentModifiers().isLeftButtonDown())
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

        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
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
        g.setColour(findColour(PlugDataColour::levelMeterActiveColourId).withAlpha(0.3f));
        g.fillPath(graphFilled);

        g.setColour(findColour(PlugDataColour::levelMeterActiveColourId));
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
            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.04f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
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
    , public StatusbarSource::Listener
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
            textColour = findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f);
        else
            textColour = findColour(PlugDataColour::toolbarTextColourId);

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
        if (!ModifierKeys::getCurrentModifiers().isLeftButtonDown())
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

class ZoomLabel final : public Component {
public:
    explicit ZoomLabel(Statusbar* parent)
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
        g.drawFittedText(String(static_cast<int>(statusbar->currentZoomLevel)) + "%", 0, 0, getWidth() - 2, getHeight(), Justification::centredRight, 1, 0.95f);
    }

    void enablementChanged() override
    {
        repaint();
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            float const newScale = std::clamp(getValue<float>(cnv->zoomScale) + wheel.deltaY, 0.25f, 3.0f);
            cnv->zoomScale.setValue(newScale);
            cnv->setTransform(AffineTransform().scaled(newScale));
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!isEnabled() || !e.mods.isLeftButtonDown())
            return;

        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            auto const defaultZoom = SettingsFile::getInstance()->getProperty<float>("default_zoom") / 100.0f;
            cnv->zoomScale.setValue(defaultZoom);
            cnv->setTransform(AffineTransform().scaled(defaultZoom));
            if (cnv->viewport)
                cnv->viewport->resized();
        }
    }

    Statusbar* statusbar;
};

Statusbar::Statusbar(PluginProcessor* processor, PluginEditor* e)
    : pd(processor)
    , editor(e)
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
    latencyDisplayButton->onClick = [this] {
        pd->performLatencyCompensationChange(0);
    };

    powerButton.setButtonText(Icons::Power);
    centreButton.setButtonText(Icons::Centre);

    commandInputButton = std::make_unique<CommandButton>();
    addChildComponent(commandInputButton.get());

    commandInputButton->onClick = [this] {
        showCommandInput();
    };

    powerButton.setTooltip("Enable/disable DSP");
    powerButton.setClickingTogglesState(true);
    addAndMakeVisible(powerButton);

    powerButton.onClick = [this] { powerButton.getToggleState() ? pd->startDSP() : pd->releaseDSP(); };

    powerButton.setToggleState(pd_getdspstate(), dontSendNotification);

    centreButton.setTooltip("Move view to origin");
    centreButton.onClick = [this] {
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->jumpToOrigin();
        }
    };

    addChildComponent(centreButton);

    volumeSlider->setRange(0.0f, 1.0f);
    volumeSlider->setValue(0.8f);
    volumeSlider->setDoubleClickReturnValue(true, 0.8f);
    addAndMakeVisible(*volumeSlider);

    if (ProjectInfo::isStandalone) {
        volumeSlider->onValueChange = [this] {
            pd->volume->store(volumeSlider->getValue());
        };
    } else {
        volumeAttachment = std::make_unique<SliderParameterAttachment>(*dynamic_cast<RangedAudioParameter*>(pd->getParameters()[0]), *volumeSlider, nullptr);
    }

    addAndMakeVisible(*levelMeter);
    addAndMakeVisible(*midiBlinker);
    addChildComponent(*cpuMeter);
    addChildComponent(*zoomLabel);

    levelMeter->toBehind(volumeSlider.get());

    zoomComboButton.setButtonText(Icons::ThinDown);

    zoomComboButton.onClick = [this] {
        PopupMenu zoomMenu;
        auto zoomOptions = StringArray { "25%", "50%", "75%", "100%", "125%", "150%", "175%", "200%", "250%", "300%" };
        for (auto zoomOption : zoomOptions) {
            auto scale = zoomOption.upToFirstOccurrenceOf("%", false, false).getIntValue() / 100.0f;
            zoomMenu.addItem(zoomOption, [this, scale] {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    cnv->zoomScale.setValue(scale);
                    cnv->setTransform(AffineTransform().scaled(scale));
                    if (cnv->viewport)
                        cnv->viewport->resized();
                }
            });
        }

        zoomMenu.addSeparator();
        zoomMenu.addItem("Zoom to fit content", [this] {
            if (auto* cnv = editor->getCurrentCanvas()) {
                cnv->zoomToFitAll();
            }
        });
        zoomMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withTargetComponent(&zoomComboButton));
    };

    snapEnableButton.setButtonText(Icons::Magnet);
    snapSettingsButton.setButtonText(Icons::ThinDown);

    if (String(PLUGDATA_GIT_HASH).isNotEmpty()) {
        plugdataString.setText(String("Nightly build: ") + String(PLUGDATA_GIT_HASH), dontSendNotification);
        addAndMakeVisible(plugdataString);
        plugdataString.setTooltip("Click to copy hash to clipboard");
        plugdataString.addMouseListener(this, false);
    }

    helpButton.setButtonText(Icons::Help);
    helpButton.onClick = [] {
        URL("https://plugdata.org/documentation.html").launchInDefaultBrowser();
    };

    sidebarExpandButton.setToggleState(false, dontSendNotification);
    sidebarExpandButton.setClickingTogglesState(true);
    sidebarExpandButton.setButtonText(Icons::PanelExpand);
    sidebarExpandButton.onClick = [this] {
        if (sidebarExpandButton.getToggleState()) {
            editor->sidebar->setVisible(true);
            editor->sidebar->showSidebar(true);
        } else {
            editor->sidebar->setVisible(false);
            editor->sidebar->showSidebar(false);
        }
    };

    snapEnableButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getPropertyAsValue("grid_enabled"));
    snapEnableButton.setClickingTogglesState(true);
    addChildComponent(snapEnableButton);

    snapSettingsButton.onClick = [this] {
        SnapSettings::show(editor, snapSettingsButton.getScreenBounds());
    };
    addChildComponent(snapSettingsButton);

    addChildComponent(zoomComboButton);

    overlayButton.getToggleStateValue().referTo(SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").getPropertyAsValue("alt_mode", nullptr));
    overlayButton.setTooltip(String("Show overlays"));
    overlayButton.setButtonText(Icons::Eye);
    overlayButton.setClickingTogglesState(true);
    overlaySettingsButton.setButtonText(Icons::ThinDown);
    addChildComponent(overlayButton);

    overlaySettingsButton.onClick = [this] {
        OverlayDisplaySettings::show(editor, overlaySettingsButton.getScreenBounds());
    };
    addChildComponent(overlaySettingsButton);

    limiterButton = std::make_unique<StatusbarTextButton>();
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
    limiterButton->openMenu = [this] {
        AudioOutputSettings::show(editor, limiterButton->getScreenBounds().removeFromRight(14), AudioOutputSettings::Limiter);
    };
    addAndMakeVisible(*limiterButton);

    oversampleButton = std::make_unique<StatusbarTextButton>();
    auto updateOversampleText = [this] {
        int const oversampling = std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3);
        StringArray const factors = { "1x", "2x", "4x", "8x" };
        oversampleButton->setButtonText(factors[oversampling]);
        oversampleButton->setToggleState(oversampling, dontSendNotification);
    };

    updateOversampleText();
    oversampleButton->setToggleState(SettingsFile::getInstance()->getProperty<int>("oversampling"), dontSendNotification);

    oversampleButton->onStateChange = [this] {
        oversampleButton->setTooltip(oversampleButton->getToggleState() ? "Disable oversampling" : "Enable oversampling");
    };

    oversampleButton->onClick = [this, updateOversampleText] {
        AudioOutputSettings::show(editor, oversampleButton->getScreenBounds().removeFromRight(14), AudioOutputSettings::Oversampling, [this, updateOversampleText] {
            updateOversampleText();
            pd->setOversampling(std::clamp(SettingsFile::getInstance()->getProperty<int>("oversampling"), 0, 3));
        });
    };

    addAndMakeVisible(*oversampleButton);

    zoomComboButton.setTooltip(String("Select zoom"));

    addAndMakeVisible(helpButton);
    addAndMakeVisible(sidebarExpandButton);

    helpButton.setTooltip(String("View online documentation"));
    sidebarExpandButton.setTooltip(String("Expand sidebar"));
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

void Statusbar::showCommandInput()
{
    if (commandInputCallout) {
        commandInputCallout->dismiss();
        return;
    }

    auto commandInput = std::make_unique<CommandInput>(editor);
    auto const rawCommandInput = commandInput.get();
    auto& callout = editor->showCalloutBox(std::move(commandInput), commandInputButton->getScreenBounds().removeFromRight(22));

    commandInputCallout = &callout;

    rawCommandInput->onDismiss = [this] {
        // If the mouse is not over the button when callout is closed, the button doesn't know it needs to repaint
        // This can cause paint glitches on the button if the cursor is not over the button when callout is closed
        // So we call repaint on the button when the callout is destroyed
        commandInputButton->repaint();
    };
}

void Statusbar::setCommandButtonText(String& text)
{
    commandInputButton->setCommandButtonText(text);
    resized();
}

void Statusbar::handleAsyncUpdate()
{
    if (auto const* cnv = editor->getCurrentCanvas()) {
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

    auto const start = !editor->palettes->isExpanded() && editor->palettes->isVisible() ? 29.0f : 0.0f;
    auto const end = editor->sidebar->isHidden() ? 29.0f : 0.0f;
    g.drawLine(start, 0.5f, static_cast<float>(getWidth()) - end, 0.5f);

    if (!welcomePanelIsShown) {
        g.drawLine(firstSeparatorPosition, 6.0f, firstSeparatorPosition, getHeight() - 6.0f);
        g.drawLine(secondSeparatorPosition, 6.0f, secondSeparatorPosition, getHeight() - 6.0f);
    }
}

void Statusbar::mouseDown(MouseEvent const& e)
{
    if (e.originalComponent == &plugdataString)
        SystemClipboard::copyTextToClipboard(String(PLUGDATA_GIT_HASH));
}

void Statusbar::resized()
{
    int pos = 0;
    auto position = [this, &pos](int const width, bool const inverse = false) -> int {
        int const result = 8 + pos;
        pos += width + 3;
        return inverse ? getWidth() - pos : result;
    };

    auto const spacing = getHeight();

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

    auto const lastButtonPosition = position(getHeight(), true);
    helpButton.setBounds(4, 0, 34, getHeight());
    if (plugdataString.isVisible())
        plugdataString.setBounds(helpButton.getRight() + 4, 0, 200, getHeight());

    if (welcomePanelIsShown) {
        sidebarExpandButton.setBounds(lastButtonPosition, 0, getHeight(), getHeight());
        powerButton.setBounds(position(getHeight() - 6, true), 0, getHeight(), getHeight());
    } else {
        powerButton.setBounds(lastButtonPosition, 0, getHeight(), getHeight());
    }

    oversampleButton->setBounds(position(26, true) + 2, 4, 26, getHeight() - 8);
    limiterButton->setBounds(position(56, true) - 1, 4, 56, getHeight() - 8);

    // TODO: combine these both into one
    int const levelMeterPosition = position(112, true);
    levelMeter->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);
    volumeSlider->setBounds(levelMeterPosition, 2, 120, getHeight() - 4);

    midiBlinker->setBounds(position(33, true) + 10, 0, 33, getHeight());
    cpuMeter->setBounds(position(40, true), 0, 50, getHeight());

    if (latencyDisplayButton->isVisible()) {
        latencyDisplayButton->setBounds(position(104, true), 0, 100, getHeight());
    }

    commandInputButton->setTopRightPosition(position(10, true), getHeight() * 0.5f - commandInputButton->getHeight() * 0.5f);
}

void Statusbar::setLatencyDisplay(int const value)
{
    if (!ProjectInfo::isStandalone) {
        latencyDisplayButton->setLatencyValue(value);
    }
}

void Statusbar::showDSPState(bool const dspState)
{
    powerButton.setToggleState(dspState, dontSendNotification);
}

void Statusbar::showLimiterState(bool const limiterState)
{
    limiterButton->setToggleState(limiterState, dontSendNotification);
}

void Statusbar::setHasActiveCanvas(bool const hasActiveCanvas)
{
    centreButton.setEnabled(hasActiveCanvas);
    zoomComboButton.setEnabled(hasActiveCanvas);
    zoomLabel->setEnabled(hasActiveCanvas);
    if (!hasActiveCanvas) {
        commandInputButton->setCommandButtonText();
    }
}

void Statusbar::audioProcessedChanged(bool const audioProcessed)
{
    auto const colour = findColour(audioProcessed ? PlugDataColour::levelMeterActiveColourId : PlugDataColour::signalColourId);
    powerButton.setColour(TextButton::textColourOnId, colour);
}

void Statusbar::setWelcomePanelShown(bool const isShowing)
{
    welcomePanelIsShown = isShowing;
    cpuMeter->setVisible(!isShowing);
    zoomLabel->setVisible(!isShowing);
    commandInputButton->setVisible(!isShowing);
    zoomComboButton.setVisible(!isShowing);
    centreButton.setVisible(!isShowing);
    overlayButton.setVisible(!isShowing);
    overlaySettingsButton.setVisible(!isShowing);
    snapEnableButton.setVisible(!isShowing);
    snapSettingsButton.setVisible(!isShowing);
    sidebarExpandButton.setVisible(isShowing);
    helpButton.setVisible(isShowing);
    plugdataString.setVisible(isShowing);
    if (!isShowing)
        sidebarExpandButton.setToggleState(false, dontSendNotification);
    resized();
}

StatusbarSource::StatusbarSource()
{
    startTimerHz(30);
}

void StatusbarSource::setSampleRate(double const newSampleRate)
{
    sampleRate = static_cast<int>(newSampleRate);
}

void StatusbarSource::setBufferSize(int const bufferSize)
{
    this->bufferSize = bufferSize;
}

void StatusbarSource::process(MidiBuffer const& midiInput, MidiBuffer const& midiOutput)
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

void StatusbarSource::prepareToPlay(int const nChannels)
{
    peakBuffer.reset(sampleRate, nChannels);
}

void StatusbarSource::timerCallback()
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

void StatusbarSource::addListener(Listener* l)
{
    listeners.add(l);
}

void StatusbarSource::removeListener(Listener* l)
{
    listeners.remove_one(l);
}

void StatusbarSource::setCPUUsage(float const cpu)
{
    cpuUsage.store(cpu);
}
