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
    std::unique_ptr<pd::Object>* inObj;
    std::unique_ptr<pd::Object>* outObj;

    SafePointer<Edge> start, end;

    PathPlan currentPlan;
    Path toDraw;
    
    Value locked;
    Value connectionStyle;

    Canvas* cnv;

    String lastId;

    int dragIdx = -1;

    bool isSelected = false;
    int mouseDownPosition = 0;

    Connection(Canvas* parent, Edge* start, Edge* end, bool exists = false);
    ~Connection() override;

    void paint(Graphics&) override;
    void resized() override;

    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    String getId() const;

    MemoryBlock getState();
    void setState(MemoryBlock& block);

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<int> start, Point<int> end, Point<int> increment);

    void applyPath(const PathPlan& plan, bool updateState = true);

    PathPlan findPath();

    PathPlan scalePath(const PathPlan& plan);

    bool straightLineIntersectsObject(Line<int> first);

   private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};
