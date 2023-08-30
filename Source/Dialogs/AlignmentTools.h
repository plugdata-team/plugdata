/*
 // Copyright (c) 2023 Timothy Schoen
 // Copyright (c) 2023 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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
        alignButtons.add(new TextButton(Icons::AlignTop));
        alignButtons.add(new TextButton(Icons::AlignHCentre));
        alignButtons.add(new TextButton(Icons::AlignBottom));

        alignButtons[AlignButton::Left]->setTooltip("Align objects to left");
        alignButtons[AlignButton::VCentre]->setTooltip("Align objects to vertical center");
        alignButtons[AlignButton::Right]->setTooltip("Align objects to right");
        alignButtons[AlignButton::Top]->setTooltip("Align objects to top");
        alignButtons[AlignButton::HCentre]->setTooltip("Align objects to horizontal center");
        alignButtons[AlignButton::Bottom]->setTooltip("Align objects to bottom");

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

        for (auto* button : alignButtons) {
            button->getProperties().set("Style", "LargeIcon");
            button->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
            addAndMakeVisible(button);
        }

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
            if (buttonColumn >= 3) {
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

    enum AlignButton { Left,
        VCentre,
        Right,
        Top,
        HCentre,
        Bottom };

    OwnedArray<TextButton> alignButtons;

    Label alignmentLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignmentTools)
};
