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
        OwnedArray<TextButton> buttons { new TextButton("edit"), new TextButton("lock"), new TextButton("run"), new TextButton("alt") };

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
                } else {
                    return false;
                }
            };

            for (auto* button : buttons) {
                button->getProperties().set("Style", "SmallIcon");
                addAndMakeVisible(button);
                button->setVisible(controlVisibility(button->getName()));
                button->addListener(this);
            }

            buttons[Edit]->setButtonText(Icons::Edit);
            buttons[Lock]->setButtonText(Icons::Lock);
            buttons[Run]->setButtonText(Icons::Presentation);
            buttons[Alt]->setButtonText(Icons::Eye);

            buttons[Edit]->setTooltip("Show " + groupName.toLowerCase() + " in edit mode");
            buttons[Lock]->setTooltip("Show " + groupName.toLowerCase() + " in run mode");
            buttons[Run]->setTooltip("Show " + groupName.toLowerCase() + " in presentation mode");
            buttons[Alt]->setTooltip("Show " + groupName.toLowerCase() + " when overlay button is active");

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

            setSize(230, 30);
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
            auto bounds = Rectangle<int>(0, 0, 30, 30);
            buttons[Edit]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Lock]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Run]->setBounds(bounds);
            bounds.translate(25, 0);
            buttons[Alt]->setBounds(bounds);
            bounds.translate(25, 0);

            textLabel.setBounds(bounds.withWidth(150));
        }
    };

    OverlayDisplaySettings()
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();

        auto overlayTree = settingsTree.getChildWithName("Overlays");

        canvasLabel.setText("Canvas", dontSendNotification);
        canvasLabel.setFont(Font(14));
        addAndMakeVisible(canvasLabel);

        objectLabel.setText("Object", dontSendNotification);
        objectLabel.setFont(Font(14));
        addAndMakeVisible(objectLabel);

        connectionLabel.setText("Connection", dontSendNotification);
        connectionLabel.setFont(Font(14));
        addAndMakeVisible(connectionLabel);

        buttonGroups.add(new OverlaySelector(overlayTree, Origin, "origin", "Origin", "0,0 point of canvas"));
        buttonGroups.add(new OverlaySelector(overlayTree, Border, "border", "Border", "Plugin / window workspace size"));
        buttonGroups.add(new OverlaySelector(overlayTree, Index, "index", "Index", "Object index in patch"));
        // buttonGroups.add(new OverlaySelector(overlayTree, Coordinate, "coordinate", "Coordinate", "Object coordinate in patch"));
        buttonGroups.add(new OverlaySelector(overlayTree, ActivationState, "activation_state", "Activity", "Show object activity"));
        buttonGroups.add(new OverlaySelector(overlayTree, Direction, "direction", "Direction", "Direction of connection"));
        buttonGroups.add(new OverlaySelector(overlayTree, Order, "order", "Order", "Trigger order of multiple outlets"));

        for (auto* buttonGroup : buttonGroups) {
            addAndMakeVisible(buttonGroup);
        }
        setSize(170, 500);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto const labelHeight = 26;
        auto const itemHeight = 28;
        auto const spacing = 2;

        canvasLabel.setBounds(bounds.removeFromTop(labelHeight));
        buttonGroups[OverlayOrigin]->setBounds(bounds.removeFromTop(itemHeight));
        buttonGroups[OverlayBorder]->setBounds(bounds.removeFromTop(itemHeight));

        bounds.removeFromTop(spacing);
        objectLabel.setBounds(bounds.removeFromTop(labelHeight));
        buttonGroups[OverlayIndex]->setBounds(bounds.removeFromTop(itemHeight));

        // doesn't exist yet
        // buttonGroups[OverlayCoordinate].setBounds(bounds.removeFromTop(28));
        buttonGroups[OverlayActivationState]->setBounds(bounds.removeFromTop(itemHeight));

        bounds.removeFromTop(spacing);
        connectionLabel.setBounds(bounds.removeFromTop(labelHeight));
        buttonGroups[OverlayDirection]->setBounds(bounds.removeFromTop(itemHeight));
        buttonGroups[OverlayOrder]->setBounds(bounds.removeFromTop(itemHeight));
        setSize(170, bounds.getY());
    }

    static void show(Component* parent, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>();
        CallOutBox::launchAsynchronously(std::move(overlayDisplaySettings), bounds, parent);
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

    OwnedArray<OverlayDisplaySettings::OverlaySelector> buttonGroups;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};
