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

using PathPlan = std::vector<Point<int>>;

class Canvas;
class Connection : public Component, public ComponentListener
{
   public:
    int inIdx;
    int outIdx;

    SafePointer<Edge> inlet, outlet;
    SafePointer<Box> inbox, outbox;

    Path toDraw;
    String lastId;

    Connection(Canvas* parent, Edge* start, Edge* end, bool exists = false);
    ~Connection() override;

    void paint(Graphics&) override;

    bool isSegmented();
    void setSegmented(bool segmented);

    void updatePath();

    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    
    Point<int> getStartPoint();
    Point<int> getEndPoint();

    void reconnect(Edge* target, bool dragged);

    bool intersects(Rectangle<float> toCheck, int accuracy = 4) const;
    int getClosestLineIdx(const Point<int>& position, const PathPlan& plan);

    String getId() const;

    String getState();
    void setState(const String& block);

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<int> start, Point<int> end, Point<int> increment);

    void findPath();

    bool intersectsObject(Box* object);
    bool straightLineIntersectsObject(Line<int> first);

   private:
    bool wasSelected = false;
    bool segmented = false;
    
    Array<SafePointer<Connection>> reconnecting;

    Rectangle<float> startReconnectHandle, endReconnectHandle;

    PathPlan currentPlan;

    Value locked;

    Canvas* cnv;

    Point<int> origin, offset;

    int dragIdx = -1;

    float mouseDownPosition = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};
