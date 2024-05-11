//
// Created by alexmitchell on 17/01/24.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"

class ValueTreeNodeComponent;
class ValueTreeNodeBranchLine : public Component, public SettableTooltipClient
{
public:
    explicit ValueTreeNodeBranchLine(ValueTreeNodeComponent* node) : node(node)
    {
    }

    void paint(Graphics& g) override;

    void mouseEnter(const MouseEvent& e) override
    {
        isHover = true;
        repaint();
    }

    void mouseExit(const MouseEvent& e) override
    {
        isHover = false;
        repaint();
    }

    void mouseMove(const MouseEvent& e) override
    {
        if (!isHover) {
            isHover = true;
            repaint();
        }
    }

    void mouseUp(const MouseEvent& e) override;

    void resized() override
    {
        treeLine.clear();

        if (getParentComponent()->isVisible()) {
            // create a line to show the current branch
            auto b = getLocalBounds();
            auto lineEnd = Point<float>(4.0f, b.getHeight() - 3.0f);
            treeLine.startNewSubPath(4.0f, 0.0f);
            treeLine.lineTo(lineEnd);
            
            if(!b.isEmpty()) {
                treeLineImage = Image(Image::PixelFormat::ARGB, b.getWidth(), b.getHeight(), true);
                Graphics treeLineG(treeLineImage);
                treeLineG.setColour(Colours::white);
                treeLineG.strokePath(treeLine, PathStrokeType(1.0f));
                auto ballEnd = Rectangle<float>(0, 0, 5, 5).withCentre(lineEnd);
                treeLineG.fillEllipse(ballEnd);
            }
        }
    }

private:
    ValueTreeNodeComponent* node;
    Path treeLine;
    Image treeLineImage;
    bool isHover = false;
};
