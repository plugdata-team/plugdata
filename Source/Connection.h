/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

extern "C"
{
#include <m_pd.h>
}

#include "Edge.h"
#include "Pd/PdObject.h"

using PathPlan = std::vector<Point<int>>;

class Canvas;
class Connection : public Component, public ComponentListener
{
   public:
    int inIdx;
    int outIdx;

    SafePointer<Edge> inlet, outlet;

    PathPlan currentPlan;
    Path toDraw;

    Value locked;
    Value connectionStyle;

    Canvas* cnv;

    String lastId;

    Point<int> origin, offset;

    int dragIdx = -1;

    float mouseDownPosition = 0;

    Connection(Canvas* parent, Edge* start, Edge* end, bool exists = false);
    ~Connection() override;

    void paint(Graphics&) override;

    void updatePath();

    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    

    int getClosestLineIdx(const Point<int>& position, const PathPlan& plan);

    String getId() const;

    MemoryBlock getState();
    void setState(MemoryBlock& block);

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<int> start, Point<int> end, Point<int> increment);

    void applyPath(const PathPlan& plan, bool updateState = true);

    void findPath();

    bool straightLineIntersectsObject(Line<int> first);

   private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};
