#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
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
        Value overlayValue;
    public:
        OverlaySelector(Value setting, String nameOfSetting, String nameOfGroup)
        : overlayValue(setting)
        , groupName(nameOfGroup)
        , settingName(nameOfSetting)
        {
            setSize(230, 30);
            
            for(auto& button : buttons)
            {
                button.getProperties().set("Style", &button == &buttons[Alt] ? "TextIcon" : "SmallIcon");
                addAndMakeVisible(button);
                button.setClickingTogglesState(true);
                button.addListener(this);
            }
            
            buttons[Edit].setButtonText(Icons::Edit);
            buttons[Lock].setButtonText(Icons::Lock);
            buttons[Run].setButtonText(Icons::Presentation);
            buttons[Alt].setButtonText("Alt");
            
            addAndMakeVisible(textLabel);
            textLabel.setJustificationType(Justification::centredLeft);
            
            textLabel.setText(groupName, dontSendNotification);
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
        
        void paintOverChildren(Graphics& g) override
        {
            // debugging
            if (false) {
                g.setColour(Colours::red);
                g.drawRect(getLocalBounds(), 1.0f);
            }
        }
        
        void resized() override
        {
            auto bounds = Rectangle<int>(0,0,30,30);
            buttons[Edit].setBounds(bounds);
            bounds.translate(30,0);
            buttons[Lock].setBounds(bounds);
            bounds.translate(30,0);
            buttons[Run].setBounds(bounds);
            bounds.translate(30,0);
            buttons[Alt].setBounds(bounds.withWidth(35).withHeight(20));
            bounds.translate(35,0);
            textLabel.setBounds(bounds.withWidth(150));
        }
    };
    
    OverlayDisplaySettings()
    {
        setSize(225, 280);
        
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
    
    void paintOverChildren(Graphics& g) override
    {
        // debugging
        if (false) {
            g.setColour(Colours::red);
            g.drawRect(getLocalBounds(), 1.0f);
        }
    }
    
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        canvasLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Origin].setBounds(bounds.removeFromTop(30));
        buttonGroups[Border].setBounds(bounds.removeFromTop(30));
        
        objectLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Index].setBounds(bounds.removeFromTop(30));
        buttonGroups[Coordinate].setBounds(bounds.removeFromTop(30));
        buttonGroups[ActivationState].setBounds(bounds.removeFromTop(30));
        
        connectionLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups[Order].setBounds(bounds.removeFromTop(30));
        buttonGroups[Direction].setBounds(bounds.removeFromTop(30));
    }
    
    static void show(Rectangle<int> bounds)
    {
        if (isShowing)
            return;
        
        isShowing = true;
        
        auto overlayDisplaySettings = std::make_unique<OverlayDisplaySettings>();
        CallOutBox::launchAsynchronously(std::move(overlayDisplaySettings), bounds, nullptr);
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
        OverlaySelector(originValue, "origin", "Origin"),
        OverlaySelector(borderValue, "border", "Border"),
        OverlaySelector(indexValue, "index", "Index"),
        OverlaySelector(coordinateValue, "coordinate", "Coordinate"),
        OverlaySelector(activationValue, "activation_state", "Activation state"),
        OverlaySelector(orderValue, "order", "Order"),
        OverlaySelector(directionValue, "direction", "Direction")
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};
