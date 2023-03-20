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
        std::map<String, TextButton*> buttons = {
            { "edit", new TextButton("Edit") },
            { "lock", new TextButton("Lock") },
            { "run", new TextButton("Run") },
            { "alt", new TextButton("Alt") }
        };
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

            buttons["edit"]->getProperties().set("Style", "SmallIcon");
            addAndMakeVisible(buttons["edit"]);
            buttons["edit"]->setButtonText(Icons::Edit);
            buttons["edit"]->setClickingTogglesState(true);
            buttons["edit"]->addListener(this);

            buttons["lock"]->getProperties().set("Style", "SmallIcon");
            addAndMakeVisible(buttons["lock"]);
            buttons["lock"]->setButtonText(Icons::Lock);
            buttons["lock"]->setClickingTogglesState(true);
            buttons["lock"]->addListener(this);

            buttons["run"]->getProperties().set("Style", "SmallIcon");
            addAndMakeVisible(buttons["run"]);
            buttons["run"]->setButtonText(Icons::Presentation);
            buttons["run"]->setClickingTogglesState(true);
            buttons["run"]->addListener(this);
            if (settingName == "order" || settingName == "direction" || settingName == "activation_state")
                buttons["run"]->setEnabled(false);

            buttons["alt"]->getProperties().set("Style", "TextIcon");
            addAndMakeVisible(buttons["alt"]);
            buttons["alt"]->setButtonText("Alt");
            buttons["alt"]->setClickingTogglesState(true);
            buttons["alt"]->addListener(this);

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
                buttonBit = overlayState::editDisplay;
            } else if (name == "Lock") {
                buttonBit = overlayState::lockDisplay;
            } else if (name == "Run") {
                buttonBit = overlayState::runDisplay;
            } else if (name == "Alt") {
                buttonBit = overlayState::altDisplay;
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

                for (auto& [settingName, button] : buttons) {
                    button->setToggleState(false, false);
                }

                if (state & overlayState::editDisplay) {
                    buttons["edit"]->setToggleState(true, false);
                }
                if (state & overlayState::lockDisplay) {
                    buttons["lock"]->setToggleState(true, false);
                }
                if (state & overlayState::runDisplay) {
                    buttons["run"]->setToggleState(true, false);
                }
                if (state & overlayState::altDisplay) {
                    buttons["alt"]->setToggleState(true, false);
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
            buttons["edit"]->setBounds(bounds);
            bounds.translate(30,0);
            buttons["lock"]->setBounds(bounds);
            bounds.translate(30,0);
            buttons["run"]->setBounds(bounds);
            bounds.translate(30,0);
            buttons["alt"]->setBounds(bounds.withWidth(35).withHeight(20));
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
        
        for (auto& [label, buttonGroup] : buttonGroups) {
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
        buttonGroups["Origin"]->setBounds(bounds.removeFromTop(30));
        buttonGroups["Border"]->setBounds(bounds.removeFromTop(30));

        objectLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups["Index"]->setBounds(bounds.removeFromTop(30));
        buttonGroups["Coordinate"]->setBounds(bounds.removeFromTop(30));
        buttonGroups["Activation State"]->setBounds(bounds.removeFromTop(30));

        connectionLabel.setBounds(bounds.removeFromTop(20));
        buttonGroups["Order"]->setBounds(bounds.removeFromTop(30));
        buttonGroups["Direction"]->setBounds(bounds.removeFromTop(30));
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

    enum overlayState {
        allOff = 0,
        editDisplay = 1,
        lockDisplay = 2,
        runDisplay = 4,
        altDisplay = 8
    };

    std::map<String, OverlayDisplaySettings::OverlaySelector*> buttonGroups = {
        { "Origin", new OverlaySelector(originValue, "origin", "Origin") },
        { "Border", new OverlaySelector(borderValue, "border", "Border") },
        { "Index", new OverlaySelector(indexValue, "index", "Index") },
        { "Coordinate", new OverlaySelector(coordinateValue, "coordinate", "Coordinate") },
        { "Activation State", new OverlaySelector(activationValue, "activation_state", "Activation state") },
        { "Order", new OverlaySelector(orderValue, "order", "Order") },
        { "Direction", new OverlaySelector(directionValue, "direction", "Direction") }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayDisplaySettings)
};