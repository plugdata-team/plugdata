#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

class OverlayDisplaySettings : public Component
{
public:
    class OverlaySelector : public Component, public Value::Listener, public Button::Listener
    {
    private:
        
        enum ButtonType
        {
            Edit = 0,
            Lock,
            Run,
            Alt
        };
        TextButton buttons[4] = {TextButton("Edit"), TextButton("Lock"), TextButton("Run"), TextButton("Alt")};
        
        Label textLabel;
        String groupName;
        String settingName;
        String toolTip;
        Value overlayValue;
    public:
        OverlaySelector(Value setting, String nameOfSetting, String nameOfGroup, String toolTipString)
        : overlayValue(setting)
        , groupName(nameOfGroup)
        , settingName(nameOfSetting)
        , toolTip(toolTipString)
        {
            setSize(230, 30);

            auto controlVisibility = [this](String mode) {
                if (settingName == "origin" || settingName == "border") {
                    return true;
                } else if (mode == "Edit" || mode == "Lock" || mode == "Alt")
                    return true;
                else
                    return false;
            };

            for(auto& button : buttons)
            {
                button.getProperties().set("Style", "SmallIcon");
                addAndMakeVisible(button);
                button.setVisible(controlVisibility(button.getName()));
                button.setClickingTogglesState(true);
                button.addListener(this);
            }
            
            buttons[Edit].setButtonText(Icons::Edit);
            buttons[Lock].setButtonText(Icons::Lock);
            buttons[Run].setButtonText(Icons::Presentation);
            buttons[Alt].setButtonText(Icons::Eye);

            buttons[Edit].setTooltip("Show " + groupName.toLowerCase() + " in edit mode");
            buttons[Lock].setTooltip("Show " + groupName.toLowerCase() + " in run mode");
            buttons[Run].setTooltip("Show " + groupName.toLowerCase() + " in presentation mode");
            buttons[Alt].setTooltip("Show " + groupName.toLowerCase() + " when overlay button is active");

            textLabel.setText(groupName, dontSendNotification);
            textLabel.setTooltip(toolTip);
            addAndMakeVisible(textLabel);

            overlayValue = SettingsFile::getInstance()->getProperty<int>(settingName);
            overlayValue.addListener(this);
            valueChanged(overlayValue);
        }

        void buttonClicked(Button* button) override
        {
            int currentBitValue = SettingsFile::getInstance()->getProperty<int>(settingName);
            int buttonBit = 0;
            auto name = button->getName();
            
            if (name == "Edit") {
                buttonBit = OverlayState::EditDisplay;
            } else if (name == "Lock") {
                buttonBit = OverlayState::LockDisplay;
            } else if (name == "Run") {
                buttonBit = OverlayState::RunDisplay;
            } else if (name == "Alt") {
                buttonBit = OverlayState::AltDisplay;
            }
            
            if (button->getToggleState()) {
                overlayValue = currentBitValue | buttonBit;
            } else {
                overlayValue = currentBitValue &~ buttonBit;
            }
            
            SettingsFile::getInstance()->setProperty(settingName, overlayValue);
        }
        
        void valueChanged(Value& value) override
        {
            if (value.refersToSameSourceAs(overlayValue)) {
                auto state = static_cast<int>(overlayValue.getValue());
                
                for (auto& button : buttons) {
                    button.setToggleState(false, dontSendNotification);
                }
                
                if (state & OverlayState::EditDisplay) {
                    buttons[Edit].setToggleState(true, dontSendNotification);
                }
                if (state & OverlayState::LockDisplay) {
                    buttons[Lock].setToggleState(true, dontSendNotification);
                }
                if (state & OverlayState::RunDisplay) {
                    buttons[Run].setToggleState(true, dontSendNotification);
                }
                if (state & OverlayState::AltDisplay) {
                    buttons[Alt].setToggleState(true, dontSendNotification);
                }
            }
        }

        void resized() override
        {
            auto bounds = Rectangle<int>(0,0,30,30);
            buttons[Edit].setBounds(bounds);
            bounds.translate(25,0);
            buttons[Lock].setBounds(bounds);
            bounds.translate(25,0);
            buttons[Run].setBounds(bounds);
            bounds.translate(25,0);
            buttons[Alt].setBounds(bounds);
            bounds.translate(25,0);

            textLabel.setBounds(bounds.withWidth(150));
        }
    };
    
    OverlayDisplaySettings()
    {
        setSize(200, 275);

        addAndMakeVisible(canvasLabel);
        canvasLabel.setText("Canvas", dontSendNotification);
        canvasLabel.setJustificationType(Justification::topLeft);
        
        addAndMakeVisible(objectLabel);
        objectLabel.setText("Object", dontSendNotification);
        objectLabel.setJustificationType(Justification::topLeft);
        
        addAndMakeVisible(connectionLabel);
        connectionLabel.setText("Connection", dontSendNotification);
        connectionLabel.setJustificationType(Justification::topLeft);
        
        for (auto& buttonGroup : buttonGroups) {
            addAndMakeVisible(buttonGroup);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(5);

        canvasLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Origin].setBounds(bounds.removeFromTop(30));
        buttonGroups[Border].setBounds(bounds.removeFromTop(30));
        
        objectLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Index].setBounds(bounds.removeFromTop(30));
        buttonGroups[Coordinate].setBounds(bounds.removeFromTop(30));
        buttonGroups[ActivationState].setBounds(bounds.removeFromTop(30));
        
        connectionLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Order].setBounds(bounds.removeFromTop(30));
        buttonGroups[Direction].setBounds(bounds);
    }
    
    static void show(Component* parent, Rectangle<int> bounds)
    {
        if (isShowing)
            return;
        
        isShowing = true;
        
        auto overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>();
        CallOutBox::launchAsynchronously(std::move(overlayDisplaySettings), bounds, parent);
    }

    ~OverlayDisplaySettings()
    {
        isShowing = false;
    }

    Value originValue, borderValue, indexValue, coordinateValue, activationValue, orderValue, directionValue;

private:
    static inline bool isShowing = false;

    Label canvasLabel, objectLabel, connectionLabel;
    
    enum OverlayState {
        AllOff = 0,
        EditDisplay = 1,
        LockDisplay = 2,
        RunDisplay = 4,
        AltDisplay = 8
    };

    enum OverlayGroups
    {
        Origin = 0,
        Border,
        Index,
        Coordinate,
        ActivationState,
        Order,
        Direction
    };

    OverlayDisplaySettings::OverlaySelector buttonGroups[7] = {
        OverlaySelector(originValue, "origin", "Origin", "0,0 point of canvas"),
        OverlaySelector(borderValue, "border", "Border", "Plugin / window workspace size"),
        OverlaySelector(indexValue, "index", "Index", "Object index in patch"),
        OverlaySelector(coordinateValue, "coordinate", "Coordinate", "Object coordinate in patch"),
        OverlaySelector(activationValue, "activation_state", "Activation state", "Data flow display"),
        OverlaySelector(orderValue, "order", "Order", "Trigger order of multiple outlets"),
        OverlaySelector(directionValue, "direction", "Direction", "Direction of connection")
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};
