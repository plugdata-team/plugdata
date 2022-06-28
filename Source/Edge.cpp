/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Edge.h"

#include "Canvas.h"
#include "Connection.h"
#include "LookAndFeel.h"

Edge::Edge(Box* parent, bool inlet) : box(parent)
{
    isInlet = inlet;
    setSize(8, 8);

    parent->addAndMakeVisible(this);

    locked.referTo(parent->cnv->pd->locked);
}

Rectangle<int> Edge::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return getBounds() + box->getPosition();
}

void Edge::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    if (!isTargeted && !isMouseOver())
    {
        bounds = bounds.reduced(2);
    }

    bool down = isMouseButtonDown() && !bool(box->locked.getValue());
    bool over = isMouseOver() && !bool(box->locked.getValue());

    auto backgroundColour = isSignal ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::highlightColourId);

    if (down || over) backgroundColour = backgroundColour.contrasting(down ? 0.2f : 0.05f);

    if (static_cast<bool>(locked.getValue()))
    {
        backgroundColour = findColour(PlugDataColour::textColourId);
    }

    // Instead of drawing pie segments, just clip the graphics region to the visible edges of the box
    // This is much faster!
    bool stateSaved = false;
    if (!(box->isOver() || over || isTargeted || isMouseOver()) || static_cast<bool>(locked.getValue()))
    {
        g.saveState();
        g.reduceClipRegion(getLocalArea(box, box->getLocalBounds().reduced(Box::margin)));
        stateSaved = true;
    }

    g.setColour(backgroundColour);
    g.fillEllipse(bounds);

    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
    g.drawEllipse(bounds, 1.0f);

    if (stateSaved)
    {
        g.restoreState();
    }
}

void Edge::resized()
{
}

void Edge::mouseDrag(const MouseEvent& e)
{
    // Ignore when locked
    if (bool(locked.getValue())) return;

    if (!box->cnv->connectingEdge && e.getLengthOfMousePress() > 300)
    {
        box->cnv->connectingEdge = this;
        auto* cnv = findParentComponentOfClass<Canvas>();
        cnv->connectingWithDrag = true;
    }
}

void Edge::mouseUp(const MouseEvent& e)
{
    if (bool(locked.getValue())) return;

    if (!e.mouseWasDraggedSinceMouseDown())
    {
        createConnection();
    }

    if (box->cnv->nearestEdge)
    {
        box->cnv->nearestEdge->isTargeted = false;
        box->cnv->nearestEdge->repaint();
        box->cnv->nearestEdge = nullptr;
    }
}

void Edge::mouseEnter(const MouseEvent& e)
{
    for (auto& edge : box->edges) edge->repaint();
}

void Edge::mouseExit(const MouseEvent& e)
{
    for (auto& edge : box->edges) edge->repaint();
}


void Edge::createConnection()
{
    // Check if this is the start or end action of connecting
    if (box->cnv->connectingEdge)
    {
        // Check type for input and output
        bool sameDirection = isInlet == box->cnv->connectingEdge->isInlet;

        bool connectionAllowed = box->cnv->connectingEdge->box != box && !sameDirection;

        // Don't create if this is the same edge
        if (box->cnv->connectingEdge == this)
        {
            box->cnv->connectingEdge = nullptr;
        }
        // Create new connection if allowed
        else if (connectionAllowed)
        {
            auto* cnv = findParentComponentOfClass<Canvas>();
            cnv->connections.add(new Connection(cnv, box->cnv->connectingEdge, this));
            box->cnv->connectingEdge = nullptr;
        }
    }
    // Else set this edge as start of a connection
    else
    {
        box->cnv->connectingEdge = this;
    }
}

Edge* Edge::findNearestEdge(Canvas* cnv, Point<int> position, bool inlet, Box* boxToExclude)
{
    // Find all edges
    Array<Edge*> allEdges;
    for (auto* box : cnv->boxes)
    {
        for (auto* edge : box->edges)
        {
            if (edge->isInlet == inlet && edge->box != boxToExclude)
            {
                allEdges.add(edge);
            }
        }
    }

    Edge* nearestEdge = nullptr;

    for (auto& edge : allEdges)
    {
        auto bounds = edge->getCanvasBounds().expanded(150, 150);
        if (bounds.contains(position))
        {
            if (!nearestEdge) nearestEdge = edge;

            auto oldPos = nearestEdge->getCanvasBounds().getCentre();
            auto newPos = bounds.getCentre();
            nearestEdge = newPos.getDistanceFrom(position) < oldPos.getDistanceFrom(position) ? edge : nearestEdge;
        }
    }

    return nearestEdge;
}
