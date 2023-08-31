/*
 // Copyright (c) 2023 Timothy Schoen
 // Copyright (c) 2023 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

//#include <JuceHeader.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "../PluginEditor.h"

class AlignmentTools : public Component {
public:
    AlignmentTools()
    {
        alignmentLabel.setText("Alignment", dontSendNotification);
        alignmentLabel.setFont(Font(14));
        addAndMakeVisible(alignmentLabel);

        alignButtons.add(new TextButton(Icons::AlignLeft));
        alignButtons.add(new TextButton(Icons::AlignVCentre));
        alignButtons.add(new TextButton(Icons::AlignRight));
        alignButtons.add(new TextButton(Icons::AlignHDistribute));
        alignButtons.add(new TextButton(Icons::AlignTop));
        alignButtons.add(new TextButton(Icons::AlignHCentre));
        alignButtons.add(new TextButton(Icons::AlignBottom));
        alignButtons.add(new TextButton(Icons::AlignVDistribute));

        // tooltips
        alignButtons[AlignButton::Left]->setTooltip("Align objects to left");
        alignButtons[AlignButton::VCentre]->setTooltip("Align objects to vertical center");
        alignButtons[AlignButton::Right]->setTooltip("Align objects to right");
        alignButtons[AlignButton::HDistribute]->setTooltip("Distribute objects horizontal");

        alignButtons[AlignButton::Top]->setTooltip("Align objects to top");
        alignButtons[AlignButton::HCentre]->setTooltip("Align objects to horizontal center");
        alignButtons[AlignButton::Bottom]->setTooltip("Align objects to bottom");
        alignButtons[AlignButton::VDistribute]->setTooltip("Distribute objects vertical");

        // connected edges
        alignButtons[AlignButton::Left]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::VCentre]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft | Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::Right]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft | Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::HDistribute]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft);

        alignButtons[AlignButton::Top]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::HCentre]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft | Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::Bottom]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft | Button::ConnectedEdgeFlags::ConnectedOnRight);
        alignButtons[AlignButton::VDistribute]->setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft);

        auto buttonNum = 0;
        for (auto* button : alignButtons) {
            button->getProperties().set("Style", "SmallIcon");
            button->setClickingTogglesState(true);
            button->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
            button->setRadioGroupId(hash("alignment_tools"));
            addAndMakeVisible(button);
        }

        alignButtons[AlignButton::Left]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::Left);
        };

        alignButtons[AlignButton::Right]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::Right);
        };

        alignButtons[AlignButton::VCentre]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::VCenter);
        };

        alignButtons[AlignButton::Top]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::Top);
        };

        alignButtons[AlignButton::Bottom]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::Bottom);
        };

        alignButtons[AlignButton::HCentre]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::HCenter);
        };

        alignButtons[AlignButton::HDistribute]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::HDistribute);
        };

        alignButtons[AlignButton::VDistribute]->onClick = [this]() {
            auto cnv = pluginEditor->getCurrentCanvas();
            if (cnv)
                cnv->alignObjects(Align::VDistribute);
        };

        setSize(110, 110);
    }

    void resized() override
    {
        Rectangle<int> bounds;
        auto size = 32;
        alignmentLabel.setBounds(5, 0, 80, 26);
        bounds = bounds.getUnion(alignmentLabel.getBounds());

        auto buttonSize = Rectangle<int>(0, 0, size, size);
        auto buttonYPos = 26;
        auto buttonColumn = 0;
        for (auto* button : alignButtons) {
            auto buttonRowColumn = Point<int>(buttonColumn * size, buttonYPos);
            auto buttonBounds = buttonSize.withPosition(buttonRowColumn);
            button->setBounds(buttonBounds);
            bounds = bounds.getUnion(buttonBounds);
            buttonColumn++;
            if (buttonColumn >= 4) {
                buttonYPos += size;
                buttonColumn = 0;
            }
        }
        setSize(bounds.getWidth(), bounds.getHeight());
    }

    static void show(Component* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto alignmentTools = std::make_unique<AlignmentTools>();
        alignmentTools->pluginEditor = static_cast<PluginEditor*>(editor);
        CallOutBox::launchAsynchronously(std::move(alignmentTools), bounds, editor);
    }

    ~AlignmentTools() override
    {
        isShowing = false;
    }

private:
    PluginEditor* pluginEditor;

    static inline bool isShowing = false;

    enum AlignButton { 
        Left,
        VCentre,
        Right,
        HDistribute,
        Top,
        HCentre,
        Bottom,
        VDistribute };

    OwnedArray<TextButton> alignButtons;

    Label alignmentLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignmentTools)
};
