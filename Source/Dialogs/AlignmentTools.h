/*
 // Copyright (c) 2023 Timothy Schoen
 // Copyright (c) 2023 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "PluginEditor.h"

class AlignmentButton : public TextButton {
public:
    AlignmentButton(String const& icon, String const& text)
        : titleText(text)
        , iconText(icon)
    {
    }

    void paint(Graphics& g) override
    {
        auto iconBounds = getLocalBounds().reduced(9).translated(0, 0);
        auto textBounds = getLocalBounds().removeFromBottom(14);

        auto textColour = findColour(PlugDataColour::popupMenuTextColourId);
        if (isHovering)
            textColour = textColour.contrasting(0.3f);

        Fonts::drawText(g, titleText, textBounds, textColour, 13.0f, Justification::centred);
        Fonts::drawIcon(g, iconText, iconBounds, textColour, 15.0f);
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().reduced(4).contains(x, y);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

private:
    String titleText;
    String iconText;
    bool isHovering = false;
};

class AlignmentTools : public Component {
public:
    AlignmentTools()
    {
        verticalAlignmentLabel.setText("Vertical alignment", dontSendNotification);
        verticalAlignmentLabel.setFont(Fonts::getBoldFont().withHeight(14));
        verticalAlignmentLabel.setJustificationType(Justification::centred);
        addAndMakeVisible(verticalAlignmentLabel);

        horizontalAlignmentLabel.setText("Horizontal alignment", dontSendNotification);
        horizontalAlignmentLabel.setFont(Fonts::getBoldFont().withHeight(14));
        horizontalAlignmentLabel.setJustificationType(Justification::centred);
        addAndMakeVisible(horizontalAlignmentLabel);

        alignButtons.add(new AlignmentButton(Icons::AlignLeft, "Left"));
        alignButtons.add(new AlignmentButton(Icons::AlignVCentre, "Center"));
        alignButtons.add(new AlignmentButton(Icons::AlignRight, "Right"));
        alignButtons.add(new AlignmentButton(Icons::AlignHDistribute, "Distribute"));
        alignButtons.add(new AlignmentButton(Icons::AlignTop, "Top"));
        alignButtons.add(new AlignmentButton(Icons::AlignHCentre, "Center"));
        alignButtons.add(new AlignmentButton(Icons::AlignBottom, "Bottom"));
        alignButtons.add(new AlignmentButton(Icons::AlignVDistribute, "Distribute"));

        // tooltips
        alignButtons[AlignButton::Left]->setTooltip("Align selected objects to left");
        alignButtons[AlignButton::VCentre]->setTooltip("Align selected objects to vertical center");
        alignButtons[AlignButton::Right]->setTooltip("Align selected objects to right");
        alignButtons[AlignButton::HDistribute]->setTooltip("Distribute selected objects horizontal");

        alignButtons[AlignButton::Top]->setTooltip("Align selected objects to top");
        alignButtons[AlignButton::HCentre]->setTooltip("Align selected objects to horizontal center");
        alignButtons[AlignButton::Bottom]->setTooltip("Align selected objects to bottom");
        alignButtons[AlignButton::VDistribute]->setTooltip("Distribute selected objects vertical");

        for (auto* button : alignButtons) {
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
                cnv->alignObjects(Align::VCentre);
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
                cnv->alignObjects(Align::HCentre);
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

        setSize(210, 180);
    }

    void resized() override
    {
        auto horizontalButtonBounds = getLocalBounds().reduced(4);
        auto verticalButtonBounds = horizontalButtonBounds.removeFromBottom(horizontalButtonBounds.getHeight() / 2).withTrimmedTop(8);

        verticalAlignmentLabel.setBounds(verticalButtonBounds.removeFromTop(18));
        horizontalAlignmentLabel.setBounds(horizontalButtonBounds.removeFromTop(18));

        auto buttonWidth = verticalButtonBounds.getWidth() / 4;

        for (int i = 0; i < 4; i++) {
            alignButtons[i]->setBounds(horizontalButtonBounds.removeFromLeft(buttonWidth).translated(0, -8));
        }

        for (int i = 4; i < 8; i++) {
            alignButtons[i]->setBounds(verticalButtonBounds.removeFromLeft(buttonWidth).translated(0, -8));
        }
    }

    static void show(Component* editor, Rectangle<int> bounds)
    {
        if (isShowing)
            return;

        isShowing = true;

        auto alignmentTools = std::make_unique<AlignmentTools>();
        auto* pluginEditor = dynamic_cast<PluginEditor*>(editor);
        alignmentTools->pluginEditor = pluginEditor;
        pluginEditor->showCalloutBox(std::move(alignmentTools), bounds);
    }

    ~AlignmentTools() override
    {
        isShowing = false;
    }

private:
    PluginEditor* pluginEditor = nullptr;

    static inline bool isShowing = false;

    enum AlignButton {
        Left,
        VCentre,
        Right,
        HDistribute,
        Top,
        HCentre,
        Bottom,
        VDistribute
    };

    OwnedArray<TextButton> alignButtons;

    Label verticalAlignmentLabel;
    Label horizontalAlignmentLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignmentTools)
};
