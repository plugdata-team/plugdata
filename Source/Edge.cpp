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

    onClick = [this]()
    {
        if (bool(locked.getValue())) return;
        createConnection();
    };

    //setBufferedToImage(true);
}

bool Edge::hasConnection()
{
    // Check if it has any connections
    for (auto* connection : box->cnv->connections)
    {
        if (connection->inlet == this || connection->outlet == this)
        {
            return true;
        }
    }

    return false;
}

Rectangle<int> Edge::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return getBounds() + box->getPosition();
}

void Edge::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    if (!isHovered)
    {
        bounds = bounds.reduced(2);
    }

    bool down = isDown() && !bool(box->locked.getValue());
    bool over = isOver() && !bool(box->locked.getValue());

    auto backgroundColour = isSignal ? Colours::yellow : findColour(Slider::thumbColourId);

    if(!box->edgeHovered) backgroundColour = backgroundColour.withAlpha(0.7f);
    
    if (down || over) backgroundColour = backgroundColour.contrasting(down ? 0.2f : 0.05f);

    Path path;

    // Visual change if it has a connection
    if (hasConnection() && !isHovered)
    {
        if (isInlet)
        {
            path.addPieSegment(bounds, MathConstants<float>::pi + MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
        }
        else
        {
            path.addPieSegment(bounds, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
        }
    }
    else
    {
        path.addEllipse(bounds);
    }

    g.setColour(findColour(ResizableWindow::backgroundColourId));
    g.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY(), 2);

    g.setColour(backgroundColour);
    g.fillPath(path);
    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
    g.strokePath(path, PathStrokeType(1.f));
}

void Edge::resized()
{
}

void Edge::mouseDrag(const MouseEvent& e)
{
    // Ignore when locked
    if (bool(locked.getValue())) return;

    // For dragging to create new connections
    TextButton::mouseDrag(e);
    if (!box->cnv->connectingEdge && e.getLengthOfMousePress() > 300)
    {
        box->cnv->connectingEdge = this;
        auto* cnv = findParentComponentOfClass<Canvas>();
        cnv->connectingWithDrag = true;
    }
}

void Edge::mouseMove(const MouseEvent& e)
{
}

void Edge::mouseEnter(const MouseEvent& e)
{
    // Only show when not locked
    isHovered = !bool(locked.getValue());
    box->edgeHovered = true;
    for(auto& edge : box->edges) edge->repaint();
}

void Edge::mouseExit(const MouseEvent& e)
{
    isHovered = false;
    box->edgeHovered = false;
    for(auto& edge : box->edges) edge->repaint();
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
