#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
#include "LookAndFeel.h"

class SnapSettings : public Component {
public:
    class GridSizeSlider : public Component {
    public:
        GridSizeSlider()
        {
            addAndMakeVisible(slider.get());
            slider->setRange(5, 30, 5);
            slider->setValue(SettingsFile::getInstance()->getProperty<int>("grid_size"));
            slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            slider->setColour(Slider::ColourIds::trackColourId, findColour(PlugDataColour::panelBackgroundColourId));

            slider->onValueChange = [this]() {
                SettingsFile::getInstance()->setProperty("grid_size", slider->getValue());
            };
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

    enum SnapItem {
        Edges = 0,
        Centers,
        Grid,
    };

    enum SnapBitMask {
        GridBit = 1,
        EdgesBit = 2,
        CentersBit = 4
    };

    class SnapSelector : public Component
        , public Value::Listener
        , public Button::Listener
        , public SettableTooltipClient {
    private:
        Label textLabel;

        Value snapValue;

        String property = "grid_type";

        SnapBitMask snapBit;

        SnapSettings* parent;

    public:
        TextButton button;

        String groupName;

        bool dragToggledInteraction = false;

        bool buttonHover = false;

    public:
        SnapSelector(SnapSettings* parent, String icon, String nameOfGroup, SnapBitMask snapBitValue)
            : groupName(nameOfGroup)
            , snapBit(snapBitValue)
            , parent(parent)
        {
            button.getProperties().set("Style", "SmallIcon");
            addAndMakeVisible(button);
            button.setClickingTogglesState(true);
            button.setInterceptsMouseClicks(false, false);
            button.addListener(this);

            button.setButtonText(icon);

            textLabel.setText(groupName, dontSendNotification);
            textLabel.setInterceptsMouseClicks(false, false);
            textLabel.setFont(Font(14));
            addAndMakeVisible(textLabel);

            snapValue = SettingsFile::getInstance()->getProperty<int>(property);
            snapValue.addListener(this);
            valueChanged(snapValue);

            setSize(110, 30);
        }

        void paint(Graphics& g) override
        {
            if (dragToggledInteraction) {
                g.setColour(findColour(PlugDataColour::toolbarHoverColourId));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::defaultCornerRadius);
            }
        }

        void mouseEnter(MouseEvent const& e) override
        {
            if (!dragToggledInteraction) {
                button.setState(Button::ButtonState::buttonOver);
                buttonHover = true;
                repaint();
            }
        }

        void mouseExit(MouseEvent const& e) override
        {
            button.setState(Button::ButtonState::buttonNormal);
            buttonHover = false;
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            // allow mouse click on item label to also toggle state
            button.setToggleState(!button.getToggleState(), dontSendNotification);
            buttonClicked(&button);
            parent->mouseInteraction = button.getToggleState() ? MouseInteraction::ToggledButtonOn : MouseInteraction::ToggledButtonOff;
        }

        void buttonClicked(Button* button) override
        {
            // button->setState(Button::ButtonState::buttonOver);
            int currentBitValue = SettingsFile::getInstance()->getProperty<int>(property);

            if (button->getToggleState()) {
                snapValue = currentBitValue | snapBit;
            } else {
                button->setState(Button::ButtonState::buttonNormal);
                snapValue = currentBitValue & ~snapBit;
            }

            SettingsFile::getInstance()->setProperty(property, snapValue);
            repaint();
        }

        void valueChanged(Value& value) override
        {
            if (value.refersToSameSourceAs(snapValue)) {
                auto state = getValue<int>(snapValue);
                button.setToggleState(state & snapBit, dontSendNotification);
            }
        }

        void resized() override
        {
            auto bounds = Rectangle<int>(0, 0, 30, 30);
            button.setBounds(bounds);
            bounds.translate(25, 0);
            textLabel.setBounds(bounds.withWidth(150));
        }
    };

    SnapSettings()
    {
        for (auto* group : buttonGroups) {
            addAndMakeVisible(group);
            group->addMouseListener(this, true);
        }

        buttonGroups[SnapItem::Grid]->setTooltip("Snap to canvas grid");
        buttonGroups[SnapItem::Edges]->setTooltip("Snap to edges of objects");
        buttonGroups[SnapItem::Centers]->setTooltip("Snap to centers of objects");

        addAndMakeVisible(gridSlider.get());

        setSize(110, 500);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        buttonGroups[SnapItem::Edges]->setBounds(bounds.removeFromTop(30));
        buttonGroups[SnapItem::Centers]->setBounds(bounds.removeFromTop(30));
        buttonGroups[SnapItem::Grid]->setBounds(bounds.removeFromTop(30));
        gridSlider->setBounds(bounds.removeFromTop(30));
        setSize(110, bounds.getY());
    }

    void mouseUp(MouseEvent const& e) override
    {
        for (auto* group : buttonGroups) {
            group->dragToggledInteraction = false;
            // button.button.setState(Button::ButtonState::buttonNormal);
            group->repaint();
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        for (auto* group : buttonGroups) {
            if (group->dragToggledInteraction == false && group->getScreenBounds().contains(e.getScreenPosition()) && e.getDistanceFromDragStart() > 2) {
                // group.button.setState(Button::ButtonState::buttonOver);
                group->dragToggledInteraction = true;
                group->button.setToggleState(mouseInteraction, dontSendNotification);
                group->buttonClicked(&group->button);
            }
        }
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

    enum MouseInteraction {
        ToggledButtonOff = 0,
        ToggledButtonOn = 1
    };

    MouseInteraction mouseInteraction;

private:
    static inline bool isShowing = false;

    std::unique_ptr<GridSizeSlider> gridSlider = std::make_unique<GridSizeSlider>();

    OwnedArray<SnapSettings::SnapSelector> buttonGroups = {
        new SnapSelector(this, Icons::SnapEdges, "Edges", SnapBitMask::EdgesBit),
        new SnapSelector(this, Icons::SnapCenters, "Centers", SnapBitMask::CentersBit),
        new SnapSelector(this, Icons::Grid, "Grid", SnapBitMask::GridBit)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnapSettings)
};
