/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Connection.h"
#include "Box.h"
#include "Canvas.h"
#include "Edge.h"

//==============================================================================
Connection::Connection(Canvas* parent, Edge* s, Edge* e, bool exists)
    : start(s), end(e)
{
    
    // Should improve performance
    setBufferedToImage(true);
    
    cnv = parent;

    // Receive mouse events on canvas
    addMouseListener(cnv, true);

    // Make sure it's not 2x the same edge
    if (!start || !end || start->isInput == end->isInput) {
        start = nullptr;
        end = nullptr;
        parent->connections.removeObject(this);
        return;
    }

    // check which is the input
    if (start->isInput) {
        inIdx = start->edgeIdx;
        outIdx = end->edgeIdx;
        inObj = &start->box->pdObject;
        outObj = &end->box->pdObject;
    } else {
        inIdx = end->edgeIdx;
        outIdx = start->edgeIdx;
        inObj = &end->box->pdObject;
        outObj = &start->box->pdObject;
    }

    // If it doesn't already exist in pd, create connection in pd
    if (!exists) {
        bool canConnect = parent->patch.createConnection(outObj->get(), outIdx, inObj->get(), inIdx);
        
        
        if (!canConnect) {
            start = nullptr;
            end = nullptr;
            parent->connections.removeObject(this);
            return;
        }
    }
    
    // Listen to changes at edges
    start->box->addComponentListener(this);
    end->box->addComponentListener(this);
    start->addComponentListener(this);
    end->addComponentListener(this);

    // Don't need mouse clicks
    setInterceptsMouseClicks(true, false);

    cnv->addAndMakeVisible(this);

    setSize(600, 400);

    // Update position
    componentMovedOrResized(*start, true, true);
    componentMovedOrResized(*end, true, true);

    resized();
}

Connection::~Connection()
{
    if (start && start->box) {
        start->box->removeComponentListener(this);
        start->removeComponentListener(this);
    }
    if (end && end->box) {
        end->box->removeComponentListener(this);
        end->removeComponentListener(this);
    }
}

bool Connection::hitTest(int x, int y) {
    
    if(start->box->locked || end->box->locked) return false;
    
    Point<float> position = Point<float>(x, y);
    
    Point<float> nearestPoint;
    path.getNearestPoint(position, nearestPoint);

    // Get start and end point
    Point<float> pstart = start->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    Point<float> pend = end->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    
    // If we click too close to the end, don't register the click on the connection
    if(pstart.getDistanceFrom(position) < 10.0f) return false;
    if(pend.getDistanceFrom(position)   < 10.0f) return false;
    
    if(nearestPoint.getDistanceFrom(position) < 5) {
        return true;
    }
    
    return false;
}

//==============================================================================
void Connection::paint(Graphics& g)
{
    g.setColour(Colours::grey);
    g.strokePath(path, PathStrokeType(3.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    auto baseColour = Colours::white;

    if (isSelected) {
        baseColour = start->isSignal ? Colours::yellow : MainLook::highlightColour;
    }

    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(path, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void Connection::mouseDown(const MouseEvent& e)
{
    isSelected = !isSelected;
    repaint();
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    int left = std::min(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) - 10;
    int top = std::min(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) - 10;
    int right = std::max(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) + 10;
    int bottom = std::max(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) + 10;

    setBounds(left, top, right - left, bottom - top);
    resized();
}

void Connection::resized()
{
    // Get start and end point
    Point<float> pstart = start->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    Point<float> pend = end->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    
    path.clear();
    path.startNewSubPath(pstart.x, pstart.y);

    // Check cureved connection setting
    bool curvedConnection = !cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);

    // Calculate optimal curve type
    int curvetype = fabs(pstart.x - pend.x) < (fabs(pstart.y - pend.y) * 5.0f) ? 1 : 2;

    curvetype *= !(fabs(pstart.x - pend.x) < 2 || fabs(pstart.y - pend.y) < 2);

    // Don't curve if it's very short
    if (pstart.getDistanceFrom(pend) < 35)
        curvedConnection = false;

    if (curvetype == 1 && curvedConnection) // smooth vertical lines
        path.cubicTo(pstart.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, pend.y);
    else if (curvetype == 2 && curvedConnection) // smooth horizontal lines
        path.cubicTo(fabs(pstart.x - pend.x) * 0.5f, pstart.y, fabs(pstart.x - pend.x) * 0.5f, pend.y, pend.x, pend.y);
    else // Dont smooth when almost straight
        path.lineTo(pend.x, pend.y);

    repaint();
}
