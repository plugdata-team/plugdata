#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
#include "LookAndFeel.h"


class SnapSettings : public Component
{
public:
    class GridSizeSlider : public Component {
    public:
        GridSizeSlider(/*Canvas* leftCnv, Canvas* rightCnv*/)
        {
            addAndMakeVisible(slider.get());
            slider->setRange(5, 30, 5);
            slider->setValue(SettingsFile::getInstance()->getProperty<int>("grid_size"));
            slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            slider->setColour(Slider::ColourIds::trackColourId, findColour(PlugDataColour::panelBackgroundColourId));

            //slider->onValueChange = [this, leftCnv, rightCnv]() {
            //    SettingsFile::getInstance()->setProperty("grid_size", slider->getValue());
            //    if (leftCnv)
            //        leftCnv->repaint();
            //    if (rightCnv)
            //        rightCnv->repaint();
            //};
        }

        void paint(Graphics& g) override
        {
            auto b = getLocalBounds().reduced(5, 0);
            int x = b.getX();
            int spacing = b.getWidth() / 6;

            for (int i = 5; i <= 30; i += 5) {
                auto textBounds = Rectangle<int>(x, b.getY(), spacing, b.getHeight());
                Fonts::drawStyledText(g, String(i), textBounds, findColour(PlugDataColour::toolbarTextColourId), Monospace, 10, Justification::centredTop);
                x += spacing;
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.reduce(1, 0);
            slider->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() + 5);
        }

    private:
        std::unique_ptr<Slider> slider = std::make_unique<Slider>();
    };

    enum SnapItem
    {
        Grid = 0,
        Edges,
        Corners,
        Centers
    };
    
    enum SnapBitMask
    {
        GridBit = 1,
        EdgesBit = 2,
        CornersBit = 4,
        CentersBit = 8
    };

    class SnapSelector : public Component, public Value::Listener, public Button::Listener
    {
    private:
        TextButton button;
        Label textLabel;

        String groupName;

        Value snapValue;

        String property = "grid_enabled";

        SnapBitMask snapBit;
    public:
        SnapSelector(String icon, String nameOfGroup, SnapBitMask snapBitValue)
        : groupName(nameOfGroup)
        , snapBit(snapBitValue)
        {
            setSize(230, 30);
            
            button.getProperties().set("Style", "SmallIcon");
            addAndMakeVisible(button);
            button.setClickingTogglesState(true);
            button.addListener(this);

            button.setButtonText(icon);

            textLabel.setText(groupName, dontSendNotification);
            addAndMakeVisible(textLabel);

            snapValue = SettingsFile::getInstance()->getProperty<int>(property);
            snapValue.addListener(this);
            valueChanged(snapValue);
        }

        void buttonClicked(Button* button) override
        {
            int currentBitValue = SettingsFile::getInstance()->getProperty<int>(property);

            if (button->getToggleState()) {
                snapValue = currentBitValue | snapBit;
            } else {
                snapValue = currentBitValue &~ snapBit;
            }

            SettingsFile::getInstance()->setProperty(property, snapValue);
        }

        void valueChanged(Value& value) override
        {
            if (value.refersToSameSourceAs(snapValue)) {
                auto state = static_cast<int>(snapValue.getValue());
                button.setToggleState(state & snapBit, dontSendNotification);
            }
        }

        void resized() override
        {
            auto bounds = Rectangle<int>(0,0,30,30);
            button.setBounds(bounds);
            bounds.translate(25,0);
            textLabel.setBounds(bounds.withWidth(150));
        }
    };
    
    SnapSettings()
    {
        setSize(110, 155);

        for (auto& buttonGroup : buttonGroups) {
            addAndMakeVisible(buttonGroup);
        }

        //auto* leftCanvas = editor->splitView.getLeftTabbar()->getCurrentCanvas();
        //auto* rightCanvas = editor->splitView.getRightTabbar()->getCurrentCanvas();
        addAndMakeVisible(gridSlider.get());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(5);

        buttonGroups[SnapItem::Grid].setBounds(bounds.removeFromTop(30));
        buttonGroups[SnapItem::Edges].setBounds(bounds.removeFromTop(30));
        buttonGroups[SnapItem::Corners].setBounds(bounds.removeFromTop(30));
        buttonGroups[SnapItem::Centers].setBounds(bounds.removeFromTop(35));

        gridSlider->setBounds(bounds);
    }

    static void show(Component* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto snapSettings = std::make_unique<SnapSettings>();
        CallOutBox::launchAsynchronously(std::move(snapSettings), bounds, editor);
    }

    ~SnapSettings()
    {
        isShowing = false;
    }

private:
    static inline bool isShowing = false;

    std::unique_ptr<GridSizeSlider> gridSlider = std::make_unique<GridSizeSlider>();

    SnapSettings::SnapSelector buttonGroups[4] = {
        SnapSelector(Icons::Grid, "Grid", SnapBitMask::GridBit),
        SnapSelector(Icons::SnapEdges, "Edges", SnapBitMask::EdgesBit),
        SnapSelector(Icons::SnapCorners, "Corners", SnapBitMask::CornersBit),
        SnapSelector(Icons::SnapCenters, "Centers", SnapBitMask::CentersBit)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnapSettings)
};
