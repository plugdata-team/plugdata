/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

class Connection;
class Box;
class Canvas;

class Edge : public Component, public SettableTooltipClient
{
public:
    Box* box;

    Edge (Box* parent, bool isInlet);

    void paint (Graphics&) override;
    void resized() override;

    void mouseMove (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

    void mouseEnter (const MouseEvent& e) override;
    void mouseExit (const MouseEvent& e) override;

    static Edge* findNearestEdge (Canvas* cnv, Point<int> position, bool inlet, Box* boxToExclude = nullptr);

    void createConnection();

    Rectangle<int> getCanvasBounds();

    int edgeIdx;
    bool isInlet;
    bool isSignal;

    Value locked;

    bool isHovered = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Edge)
};
