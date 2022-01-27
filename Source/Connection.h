/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Edge.h"
#include "Pd/PdObject.h"
#include <JuceHeader.h>
#include <m_pd.h>

using PathPlan = std::vector<Point<int>>;

class Canvas;
class Connection : public Component, public ComponentListener {
public:
    
    SafePointer<Edge> start, end;
    Path path;
    Path roundedPath;
    PathPlan lastPlan;

    Canvas* cnv;
    
    Point<int> mouseDownWithinTarget;
    bool isDragging = false;

    int inIdx;
    int outIdx;
    std::unique_ptr<pd::Object>* inObj;
    std::unique_ptr<pd::Object>* outObj;

    bool isSelected = false;

    //==============================================================================
    Connection(Canvas* parent, Edge* start, Edge* end, bool exists = false);
    ~Connection() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    
    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    void applyBestPath();
    
    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, std::vector<PathPlan>& altPaths, PathPlan& pathStack, Point<int> start, Point<int> end, Point<int> increment);

    void findPath(Point<int> start, Point<int> end);
    void applyPlan(PathPlan plan, Point<int> start, Point<int> end);
    
    bool straightLineIntersectsObject(Line<int> first);
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};
