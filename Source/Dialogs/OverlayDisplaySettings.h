#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>

#include "Constants.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

class OverlayDisplaySettings : public Component {
public:
    class OverlaySelector : public Component
        , public Button::Listener {
    private:
        enum ButtonType {
            Edit = 0,
            Lock,
            Run,
            Alt
        };
        OwnedArray<SmallIconButton> buttons { new SmallIconButton("edit"), new SmallIconButton("lock"), new SmallIconButton("run"), new SmallIconButton("alt") };

        Label textLabel;
        String groupName;
        String settingName;
        String toolTip;
        ValueTree overlayState;
        Overlay group;

    public:

        OverlaySelector(ValueTree const& settings, Overlay groupType, String nameOfSetting, String nameOfGroup, String toolTipString)
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
            buttons[Run]->setButtonText(Icons::Presentation);
            buttons[Alt]->setButtonText(Icons::Eye);

            auto lowerCaseToolTip = toolTip.toLowerCase();

            buttons[Edit]->setTooltip("Show " + lowerCaseToolTip + " in edit mode");
            buttons[Lock]->setTooltip("Show " + lowerCaseToolTip + " in run mode");
            buttons[Run]->setTooltip("Show " + lowerCaseToolTip + " in presentation mode");
            buttons[Alt]->setTooltip("Show " + lowerCaseToolTip + " when overlay button is active");

            textLabel.setText(groupName, dontSendNotification);
            textLabel.setTooltip(toolTip);
            textLabel.setFont(Font(14));
            addAndMakeVisible(textLabel);

            auto editState = static_cast<int>(settings.getProperty("edit"));
            auto lockState = static_cast<int>(settings.getProperty("lock"));
            auto runState = static_cast<int>(settings.getProperty("run"));
            auto altState = static_cast<int>(settings.getProperty("alt"));

            buttons[Edit]->setToggleState(static_cast<bool>(editState & group), dontSendNotification);
            buttons[Lock]->setToggleState(static_cast<bool>(lockState & group), dontSendNotification);
            buttons[Run]->setToggleState(static_cast<bool>(runState & group), dontSendNotification);
            buttons[Alt]->setToggleState(static_cast<bool>(altState & group), dontSendNotification);

            setSize(200, 30);
        }

        void buttonClicked(Button* button) override
        {
            auto name = button->getName();

            int buttonBit = overlayState.getProperty(name);

            button->setToggleState(!button->getToggleState(), dontSendNotification);

            if (button->getToggleState()) {
                buttonBit = buttonBit | group;
            } else {
                buttonBit = buttonBit & ~group;
            }

            overlayState.setProperty(name, buttonBit, nullptr);
        }

        void resized() override
        {
            auto bounds = Rectangle<int>(4, 0, 30, 30);

            textLabel.setBounds(bounds.withWidth(getWidth() / 2.0));
            bounds.translate((getWidth() / 2.0) - 12, 0);

            buttons[Edit]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Lock]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Run]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Alt]->setBounds(bounds);
            bounds.translate(25, 0);
        }
    };

    OverlayDisplaySettings()
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();

        auto overlayTree = settingsTree.getChildWithName("Overlays");

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
        object.add(new OverlaySelector(overlayTree, ActivationState, "activation_state", "Activity", "Object activity"));

        connection.add(new OverlaySelector(overlayTree, Direction, "direction", "Direction", "Direction of connections"));
        connection.add(new OverlaySelector(overlayTree, Order, "order", "Order", "Trigger order of multiple outlets"));
        connection.add(new OverlaySelector(overlayTree, Behind, "behind", "Behind", "Connection cables behind objects"));

        groups.add(&canvas);
        groups.add(&object);
        groups.add(&connection);

        for (auto& group : groups) {
            for (auto& item : *group) {
                addAndMakeVisible(item);
            }
        }
        setSize(200, 505);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 0);

        auto const labelHeight = 26;
        auto const itemHeight = 28;
        auto const spacing = 2;

        canvasLabel.setBounds(bounds.removeFromTop(labelHeight));
        for (auto& item : canvas) {
            item->setBounds(bounds.removeFromTop(itemHeight));
        }

        bounds.removeFromTop(spacing);
        objectLabel.setBounds(bounds.removeFromTop(labelHeight));
        for (auto& item : object) {
            item->setBounds(bounds.removeFromTop(itemHeight));
        }

        bounds.removeFromTop(spacing);
        connectionLabel.setBounds(bounds.removeFromTop(labelHeight));
        for (auto& item : connection) {
            item->setBounds(bounds.removeFromTop(itemHeight));
        }
        setSize(200, bounds.getY() + 5);
    }

    void paint(Graphics& g) override
    {
        for (auto& group : groups) {
            auto groupBounds = group->getFirst()->getBounds().getUnion(group->getLast()->getBounds());

            // draw background rectangle
            g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.035f));
            g.fillRoundedRectangle(groupBounds.toFloat(), Corners::largeCornerRadius);

            // draw outline rectangle
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.drawRoundedRectangle(groupBounds.toFloat(), Corners::largeCornerRadius, 1.0f);

            // draw lines between items
            for (auto& item : *group){
                if ((group->size() >= 2) && (item != group->getLast()))
                    g.drawHorizontalLine(item->getBottom(), groupBounds.getX(), groupBounds.getRight());
            }
        }
    }

    static void show(PluginEditor* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>();
        editor->showCalloutBox(std::move(overlayDisplaySettings), bounds);
    }

    ~OverlayDisplaySettings() override
    {
        isShowing = false;
    }

private:
    static inline bool isShowing = false;

    Label canvasLabel, objectLabel, connectionLabel;

    enum OverlayState {
        AllOff = 0,
        EditDisplay,
        LockDisplay,
        RunDisplay,
        AltDisplay
    };

    Array<OwnedArray<OverlayDisplaySettings::OverlaySelector>*> groups;

    OwnedArray<OverlayDisplaySettings::OverlaySelector> canvas;
    OwnedArray<OverlayDisplaySettings::OverlaySelector> object;
    OwnedArray<OverlayDisplaySettings::OverlaySelector> connection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};
