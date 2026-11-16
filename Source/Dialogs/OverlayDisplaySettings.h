#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>

#include "Constants.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "DetectiveMode.h"
#include "Components/PropertiesPanel.h"

class OverlayDisplaySettings final : public Component
    , private SettingsFileListener {
public:
    class OverlaySelector final : public Component
        , public Button::Listener {
        enum ButtonType {
            Edit = 0,
            Lock,
            Alt
        };
        OwnedArray<SmallIconButton> buttons { new SmallIconButton("edit"), new SmallIconButton("lock"), new SmallIconButton("alt") };

        Label textLabel;
        String groupName;
        String settingName;
        String toolTip;
        DynamicObject::Ptr overlayState;
        Overlay group;

    public:
        OverlaySelector(DynamicObject::Ptr settings, Overlay const groupType, String nameOfSetting, String nameOfGroup, String toolTipString)
            : groupName(std::move(nameOfGroup))
            , settingName(std::move(nameOfSetting))
            , toolTip(std::move(toolTipString))
            , overlayState(settings)
            , group(groupType)
        {
            auto controlVisibility = [this](String const& mode) {
                if (settingName == "origin" || settingName == "border" || mode == "edit" || mode == "lock" || mode == "alt") {
                    return true;
                }

                return false;
            };

            for (auto* button : buttons) {
                addAndMakeVisible(button);
                button->setVisible(controlVisibility(button->getName()));
                button->addListener(this);
            }

            buttons[Edit]->setButtonText(Icons::Edit);
            buttons[Lock]->setButtonText(Icons::Lock);
            buttons[Alt]->setButtonText(Icons::Eye);

            auto const lowerCaseToolTip = toolTip.toLowerCase();

            buttons[Edit]->setTooltip("Show " + lowerCaseToolTip + " in edit mode");
            buttons[Lock]->setTooltip("Show " + lowerCaseToolTip + " in lock mode");
            buttons[Alt]->setTooltip("Show " + lowerCaseToolTip + " when overlay button is active");

            textLabel.setText(groupName, dontSendNotification);
            textLabel.setTooltip(toolTip);
            textLabel.setFont(Font(FontOptions(14)));
            addAndMakeVisible(textLabel);

            auto const editState = static_cast<int>(settings->getProperty("edit"));
            auto const lockState = static_cast<int>(settings->getProperty("lock"));
            auto const altState = static_cast<int>(settings->getProperty("alt"));

            buttons[Edit]->setToggleState(static_cast<bool>(editState & group), dontSendNotification);
            buttons[Lock]->setToggleState(static_cast<bool>(lockState & group), dontSendNotification);
            buttons[Alt]->setToggleState(static_cast<bool>(altState & group), dontSendNotification);

            setSize(200, 30);
        }

        void buttonClicked(Button* button) override
        {
            auto const name = button->getName();

            int buttonBit = overlayState->getProperty(name);

            button->setToggleState(!button->getToggleState(), dontSendNotification);

            if (button->getToggleState()) {
                buttonBit = buttonBit | group;
            } else {
                buttonBit = buttonBit & ~group;
            }

            overlayState->setProperty(name, buttonBit);
            SettingsFile::getInstance()->triggerSettingsChange("overlays");
        }

        void resized() override
        {
            auto bounds = Rectangle<int>(4, 0, 30, 30);

            textLabel.setBounds(bounds.withWidth(getWidth() / 2.0));
            bounds.translate(getWidth() - 92, 0);

            buttons[Edit]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Lock]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Alt]->setBounds(bounds);
            bounds.translate(25, 0);
        }

        void enablementChanged() override
        {
            textLabel.setAlpha(isEnabled() ? 1.0f : 0.4f);
        }
    };

    // Speed of the message animation, with normal speed in the middle
    class SpeedSlider final : public PropertiesPanelProperty {
    public:
        explicit SpeedSlider(Value const& value)
            : PropertiesPanelProperty("Animation speed")
        {
            slider.setRange(0.1, 5.0, 0.1);
            slider.setSkewFactorFromMidPoint(1.0);
            slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            slider.setDoubleClickReturnValue(true, 1.0);
            slider.getValueObject().referTo(value);
            slider.onValueChange = [this] { repaint(); };
            addAndMakeVisible(slider);
        }

        void paint(Graphics& g) override
        {
            PropertiesPanelProperty::paint(g);

            auto const colour = findColour(PropertyComponent::labelTextColourId).withMultipliedAlpha(isEnabled() ? 0.77f : 0.3f);
            Fonts::drawText(g, String(slider.getValue(), 1) + "x", getLocalBounds().removeFromRight(valueWidth).withTrimmedRight(8), colour, 13, Justification::centredRight);
        }

        void resized() override
        {
            slider.setBounds(getLocalBounds().withTrimmedLeft(getWidth() / 2).withTrimmedRight(valueWidth));
        }

        void enablementChanged() override
        {
            slider.setAlpha(isEnabled() ? 1.0f : 0.4f);
            repaint();
        }

    private:
        static constexpr int valueWidth = 38;
        Slider slider;
    };

    // Plays, pauses and steps through the message animation
    class AnimationButton final : public Button {
    public:
        AnimationButton(String const& text, String const& iconToUse)
            : Button(text)
            , icon(iconToUse)
        {
            setButtonText(text);
        }

        void setIcon(String const& newIcon)
        {
            icon = newIcon;
            repaint();
        }

        void paintButton(Graphics& g, bool const isHighlighted, bool const isDown) override
        {
            auto const& colours = getThemeColours(*this);
            auto const bounds = getLocalBounds().toFloat().reduced(0.5f);

            auto const contrast = !isEnabled() ? 0.035f : isDown ? 0.1f : isHighlighted ? 0.07f : 0.035f;
            g.setColour(colours.popupMenuBackgroundColour.contrasting(contrast));
            g.fillRoundedRectangle(bounds, Corners::largeCornerRadius);
            g.setColour(colours.toolbarOutlineColour);
            g.drawRoundedRectangle(bounds, Corners::largeCornerRadius, 1.0f);

            auto const colour = colours.popupMenuTextColour.withAlpha(isEnabled() ? 1.0f : 0.35f);
            auto const font = Fonts::getDefaultFont().withHeight(14.0f);
            auto const textWidth = static_cast<int>(std::ceil(Fonts::getStringWidth(getButtonText(), font)));
            constexpr int iconSize = 16;
            constexpr int gap = 6;

            auto content = getLocalBounds().withSizeKeepingCentre(iconSize + gap + textWidth, getHeight());
            Fonts::drawIcon(g, icon, content.removeFromLeft(iconSize), colour, 14);
            content.removeFromLeft(gap);
            g.setColour(colour);
            g.setFont(font);
            g.drawText(getButtonText(), content, Justification::centredLeft);
        }

    private:
        String icon;
    };

    explicit OverlayDisplaySettings(PluginEditor* e)
        : editor(e)
    {
        auto overlayTree = SettingsFile::getInstance()->getProperty<DynamicObject>("overlays");

        canvasLabel.setText("Canvas", dontSendNotification);
        canvasLabel.setFont(Fonts::getSemiBoldFont().withHeight(14));
        addAndMakeVisible(canvasLabel);

        objectLabel.setText("Object", dontSendNotification);
        objectLabel.setFont(Fonts::getSemiBoldFont().withHeight(14));
        addAndMakeVisible(objectLabel);

        connectionLabel.setText("Connection", dontSendNotification);
        connectionLabel.setFont(Fonts::getSemiBoldFont().withHeight(14));
        addAndMakeVisible(connectionLabel);

        canvas.add(new OverlaySelector(overlayTree, Origin, "origin", "Origin", "Origin point of canvas"));
        canvas.add(new OverlaySelector(overlayTree, Border, "border", "Border", "Plugin / window workspace size"));

        object.add(new OverlaySelector(overlayTree, Index, "index", "Index", "Object index in patch"));

        connection.add(new OverlaySelector(overlayTree, Direction, "direction", "Direction", "Direction of connections"));
        connection.add(new OverlaySelector(overlayTree, Order, "order", "Order", "Trigger order of multiple outlets"));
        connection.add(new OverlaySelector(overlayTree, Behind, "behind", "Behind", "Connection cables behind objects"));

        // Pd only reports activity and messages while debug visualization is on
        debugOverlays.add(new OverlaySelector(overlayTree, ActivationState, "activation_state", "Object activity", "Object activity"));
        debugOverlays.add(new OverlaySelector(overlayTree, ConnectionActivity, "connection_activity", "Connection activity", "Connection activity"));
        debugOverlays.add(new OverlaySelector(overlayTree, MessageAnimation, "message_animation", "Message animation", "Message animation"));

        groups[0] = &canvas;
        groups[1] = &object;
        groups[2] = &connection;
        groups[3] = &debugOverlays;

        for (auto const& group : groups) {
            for (auto const& item : *group) {
                addAndMakeVisible(item);
            }
        }

        auto* settings = SettingsFile::getInstance();
        visualizationToggle = std::make_unique<PropertiesPanel::BoolComponent>("Debug visualization", settings->getPropertyAsValue("debug_visualization"), StringArray { "Off", "On" });
        visualizationToggle->setHideLabel(true);
        visualizationToggle->setTooltip("Let Pd report what it's doing, for the overlays below and the Debugger panel");
        addAndMakeVisible(*visualizationToggle);

        tooltipsToggle = std::make_unique<PropertiesPanel::BoolComponent>("Connection tooltips", settings->getPropertyAsValue("debug_connections"), StringArray { "Off", "On" });
        tooltipsToggle->setTooltip("Show the last message that went through a connection when you hover over it");
        addAndMakeVisible(*tooltipsToggle);

        speedSlider = std::make_unique<SpeedSlider>(settings->getPropertyAsValue("detective_speed"));
        speedSlider->setTooltip("Speed of the message animation");
        addAndMakeVisible(*speedSlider);

        playButton.onClick = [this] {
            if (auto* detectiveMode = editor->pd->detectiveMode.get())
                detectiveMode->setPaused(!detectiveMode->isPaused());
            updateDebugControls();
        };
        addAndMakeVisible(playButton);

        stepButton.setTooltip("Play the current message to the end of its connection, and stop at the next one");
        stepButton.onClick = [this] {
            if (auto* detectiveMode = editor->pd->detectiveMode.get())
                detectiveMode->step();
            updateDebugControls();
        };
        addAndMakeVisible(stepButton);

        updateDebugControls();
        setSize(335, 454);
    }

    void settingsChanged(String const& name, var const&) override
    {
        if (name == "debug_visualization" || name == "overlays")
            updateDebugControls();
    }

    void updateDebugControls()
    {
        auto const isVisualizing = DetectiveMode::isDebuggingEnabled();
        auto const isAnimating = isVisualizing && DetectiveMode::isAnimationRequested();
        for (auto* item : debugOverlays)
            item->setEnabled(isVisualizing);
        tooltipsToggle->setEnabled(isVisualizing);
        speedSlider->setEnabled(isAnimating);
        playButton.setEnabled(isAnimating);
        stepButton.setEnabled(isAnimating);

        auto const* detectiveMode = editor->pd->detectiveMode.get();
        auto const isPaused = detectiveMode && detectiveMode->isPaused();
        playButton.setButtonText(isPaused ? "Play" : "Pause");
        playButton.setIcon(isPaused ? Icons::PlayTriangle : Icons::Pause);
        playButton.setTooltip(isPaused ? "Play the message animation" : "Pause the message that's being shown");
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 0).withTrimmedTop(24);

        constexpr auto labelHeight = 26;
        constexpr auto itemHeight = 28;

        auto leftSide = bounds.removeFromLeft(bounds.proportionOfWidth(0.5f)).withTrimmedRight(4);
        auto rightSide = bounds.withTrimmedLeft(4);

        canvasLabel.setBounds(leftSide.removeFromTop(labelHeight));
        for (auto const& item : canvas) {
            item->setBounds(leftSide.removeFromTop(itemHeight));
        }

        leftSide.removeFromTop(2);
        objectLabel.setBounds(leftSide.removeFromTop(labelHeight));
        for (auto const& item : object) {
            item->setBounds(leftSide.removeFromTop(itemHeight));
        }

        connectionLabel.setBounds(rightSide.removeFromTop(labelHeight));
        for (auto const& item : connection) {
            item->setBounds(rightSide.removeFromTop(itemHeight));
        }

        // Debug visualization spans both columns, below the taller one
        auto debug = getLocalBounds().reduced(4, 0).withTop(jmax(leftSide.getY(), rightSide.getY()) + 10);
        separatorY = debug.getY();
        debug.removeFromTop(6);

        auto header = debug.removeFromTop(30);
        visualizationToggle->setBounds(header.removeFromRight(110));
        titleArea = header.withTrimmedLeft(6);
        debug.removeFromTop(6);

        for (auto const& item : debugOverlays) {
            item->setBounds(debug.removeFromTop(itemHeight));
        }
        debug.removeFromTop(8);

        tooltipsToggle->setBounds(debug.removeFromTop(itemHeight));
        debug.removeFromTop(8);

        speedSlider->setBounds(debug.removeFromTop(itemHeight));
        debug.removeFromTop(8);

        auto buttons = debug.removeFromTop(30);
        playButton.setBounds(buttons.removeFromLeft((buttons.getWidth() - 8) / 2));
        stepButton.setBounds(buttons.withTrimmedLeft(8));
        debug.removeFromTop(8);

        warningArea = debug.removeFromTop(32);
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        g.setColour(colours.popupMenuTextColour);
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Overlays", 0, 0, getWidth(), 24, Justification::centred);

        g.setColour(colours.toolbarOutlineColour);
        g.drawLine(4, 24, getWidth() - 8, 24);
        g.drawLine(4, separatorY, getWidth() - 8, separatorY);

        auto const drawGroupBackground = [&g, &colours](Rectangle<int> const groupBounds) {
            g.setColour(colours.popupMenuBackgroundColour.contrasting(0.035f));
            g.fillRoundedRectangle(groupBounds.toFloat(), Corners::largeCornerRadius);

            g.setColour(colours.toolbarOutlineColour);
            g.drawRoundedRectangle(groupBounds.toFloat(), Corners::largeCornerRadius, 1.0f);
        };

        for (auto const& group : groups) {
            auto const groupBounds = group->getFirst()->getBounds().getUnion(group->getLast()->getBounds());
            drawGroupBackground(groupBounds);

            // draw lines between items
            for (auto const& item : *group) {
                if (item != group->getLast())
                    g.drawHorizontalLine(item->getBottom(), groupBounds.getX(), groupBounds.getRight());
            }
        }

        Fonts::drawStyledText(g, "Debug visualization", titleArea, colours.popupMenuTextColour, Semibold, 14);
        drawGroupBackground(tooltipsToggle->getBounds());
        drawGroupBackground(speedSlider->getBounds());

        // The cost is easy to miss, since Pd only slows down while this is on
        drawGroupBackground(warningArea);
        auto warning = warningArea.reduced(10, 0);
        Fonts::drawIcon(g, Icons::Info, warning.removeFromLeft(18).withSizeKeepingCentre(18, 18), colours.objectSelectedOutlineColour, 15);
        Fonts::drawFittedText(g, "Pd runs slower while this is on", warning.withTrimmedLeft(8), colours.popupMenuTextColour.withAlpha(0.75f), 1, 1.0f, 13.0f, Justification::centredLeft);
    }

    static void show(PluginEditor* editor, Rectangle<int> const bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>(editor);
        editor->showCalloutBox(std::move(overlayDisplaySettings), bounds);
    }

    ~OverlayDisplaySettings() override
    {
        isShowing = false;
    }

private:
    static inline bool isShowing = false;

    PluginEditor* editor;

    Label canvasLabel, objectLabel, connectionLabel;

    StackArray<OwnedArray<OverlaySelector>*, 4> groups;

    OwnedArray<OverlaySelector> canvas;
    OwnedArray<OverlaySelector> object;
    OwnedArray<OverlaySelector> connection;
    OwnedArray<OverlaySelector> debugOverlays;

    std::unique_ptr<PropertiesPanel::BoolComponent> visualizationToggle;
    std::unique_ptr<PropertiesPanel::BoolComponent> tooltipsToggle;
    std::unique_ptr<SpeedSlider> speedSlider;
    AnimationButton playButton { "Pause", Icons::Pause };
    AnimationButton stepButton { "Step", Icons::Step };

    int separatorY = 0;
    Rectangle<int> titleArea, warningArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};
