#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "PluginEditor.h"

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
            int x = b.getX() - 1;
            int spacing = (b.getWidth() / 6) + 1;

            for (int i = 5; i <= 30; i += 5) {
                auto textBounds = Rectangle<int>(x, b.getY() + 4, spacing, b.getHeight());
                Fonts::drawStyledText(g, String(i), textBounds, findColour(PlugDataColour::toolbarTextColourId), Monospace, 10, Justification::centredTop);
                x += spacing;
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.reduce(1, 0);
            slider->setBounds(bounds.getX(), bounds.getY() + 5, bounds.getWidth(), bounds.getHeight());
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

    static void show(PluginEditor* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto snapSettings = std::make_unique<SnapSettings>();
        editor->showCalloutBox(std::move(snapSettings), bounds);
    }

    class SnapSelector : public Component
        , public Value::Listener
        , public SettableTooltipClient {
    private:
        String const property = "grid_type";

        SnapBitMask snapBit;

        SnapSettings* parent;

    public:
        Value snapValue;
        String icon;
        String groupName;

        bool dragToggledInteraction = false;

    public:
        SnapSelector(SnapSettings* parent, String const& iconText, String nameOfGroup, SnapBitMask snapBitValue)
            : snapBit(snapBitValue)
            , parent(parent)
            , icon(iconText)
            , groupName(std::move(nameOfGroup))
        {
            snapValue.referTo(SettingsFile::getInstance()->getPropertyAsValue(property));
            snapValue.addListener(this);
            valueChanged(snapValue);
        }

        void paint(Graphics& g) override
        {
            if (dragToggledInteraction) {
                g.setColour(findColour(PlugDataColour::toolbarHoverColourId));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::defaultCornerRadius);
            }

            auto iconColour = getToggleState() ? findColour(PlugDataColour::toolbarActiveColourId) : findColour(PlugDataColour::toolbarTextColourId);
            auto textColour = findColour(PlugDataColour::toolbarTextColourId);

            if (isMouseOver()) {
                iconColour = iconColour.contrasting(0.3f);
                textColour = textColour.contrasting(0.3f);
            }

            Fonts::drawIcon(g, icon, Rectangle<int>(0, 0, 30, getHeight()), iconColour, 14);
            Fonts::drawText(g, groupName, Rectangle<int>(30, 0, getWidth(), getHeight()), textColour, 14);
        }

        bool getToggleState()
        {
            return getValue<int>(snapValue) & snapBit;
        }

        void setToggleState(bool state)
        {
            auto currentBitValue = getValue<int>(snapValue);

            if (state) {
                snapValue = currentBitValue | snapBit;
            } else {
                snapValue = currentBitValue & ~snapBit;
            }

            SettingsFile::getInstance()->setProperty(property, snapValue);

            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            auto newState = !getToggleState();
            setToggleState(newState);
            parent->mouseInteraction = newState ? MouseInteraction::ToggledButtonOn : MouseInteraction::ToggledButtonOff;
        }

        void valueChanged(Value& value) override
        {
            if (value.refersToSameSourceAs(snapValue)) {
                repaint();
            }
        }
    };

    SnapSettings()
    {
        snapLabel.setText("Snap", dontSendNotification);
        snapLabel.setFont(Fonts::getSemiBoldFont().withHeight(14));
        addAndMakeVisible(snapLabel);

        gridSizeLabel.setText("Grid Size", dontSendNotification);
        gridSizeLabel.setFont(Fonts::getSemiBoldFont().withHeight(14));
        addAndMakeVisible(gridSizeLabel);

        for (auto* group : buttonGroups) {
            addAndMakeVisible(group);
            group->addMouseListener(this, true);
        }

        buttonGroups[SnapItem::Grid]->setTooltip("Snap to canvas grid");
        buttonGroups[SnapItem::Edges]->setTooltip("Snap to edges of objects");
        buttonGroups[SnapItem::Centers]->setTooltip("Snap to centers of objects");

        addAndMakeVisible(gridSlider.get());
        setSize(140, 182);
    }

    ~SnapSettings()
    {
        isShowing = false;
    }

    void resized() override
    {
        auto bounds = getLocalBounds().withTrimmedTop(24);
        snapLabel.setBounds(bounds.removeFromTop(24));
        buttonGroups[SnapItem::Edges]->setBounds(bounds.removeFromTop(26));
        buttonGroups[SnapItem::Centers]->setBounds(bounds.removeFromTop(26));
        buttonGroups[SnapItem::Grid]->setBounds(bounds.removeFromTop(26));

        gridSizeLabel.setBounds(bounds.removeFromTop(24));
        gridSlider->setBounds(bounds.removeFromTop(34));
    }

    void mouseUp(MouseEvent const& e) override
    {
        for (auto* group : buttonGroups) {
            group->dragToggledInteraction = false;
            group->repaint();
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        for (auto* group : buttonGroups) {
            if (!group->dragToggledInteraction && group->getScreenBounds().contains(e.getScreenPosition()) && e.getDistanceFromDragStart() > 2) {
                group->dragToggledInteraction = true;
                group->setToggleState(mouseInteraction);
            }
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(Fonts::getBoldFont().withHeight(15));
        g.drawText("Grid", 0, 0, getWidth(), 24, Justification::centred);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(4, 24, getWidth() - 8, 24);
    }

    enum MouseInteraction {
        ToggledButtonOff = 0,
        ToggledButtonOn = 1
    };

    MouseInteraction mouseInteraction = ToggledButtonOff;

private:
    static inline bool isShowing = false;
    Label snapLabel, gridSizeLabel;
    std::unique_ptr<GridSizeSlider> gridSlider = std::make_unique<GridSizeSlider>();

    OwnedArray<SnapSettings::SnapSelector> buttonGroups = {
        new SnapSelector(this, Icons::SnapEdges, "Edges", SnapBitMask::EdgesBit),
        new SnapSelector(this, Icons::SnapCenters, "Centers", SnapBitMask::CentersBit),
        new SnapSelector(this, Icons::Grid, "Grid", SnapBitMask::GridBit)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnapSettings)
};
