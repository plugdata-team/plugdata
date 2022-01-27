/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <set>
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
    g.strokePath(roundedPath, PathStrokeType(3.5f, PathStrokeType::mitered, PathStrokeType::rounded));
    
    auto baseColour = Colours::white;
    
    if (isSelected) {
        baseColour = start->isSignal ? Colours::yellow : MainLook::highlightColour;
    }
    
    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(roundedPath, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void Connection::mouseMove(const MouseEvent& e)
{
    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);
    if(curvedConnection) {
        for(int n = 2; n < lastPlan.size() - 1; n++) {
            auto line = Line<int>(lastPlan[n - 1], lastPlan[n]);
            Point<int> nearest;
            if(line.getDistanceFromPoint(e.getPosition(), nearest) < 3) {
                
                setMouseCursor(line.isVertical() ? MouseCursor::LeftRightResizeCursor : MouseCursor::UpDownResizeCursor);
            }
        }
    }
}


void Connection::mouseDrag(const MouseEvent& e)
{
    /* WIP: disabled for now
    auto& first = start->isInput ? start : end;
    auto& last = start->isInput ? end : start;
    
    Point<int> pstart = first->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = last->getCanvasBounds().getCentre() - getPosition();
    
    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);
    if(curvedConnection) {
        for(int n = 2; n < lastPlan.size() - 1; n++) {
            auto line = Line<int>(lastPlan[n - 1], lastPlan[n]);
            Point<int> nearest;
            
            if(line.getDistanceFromPoint(e.getPosition(), nearest) < 5) {
                auto delta = e.getPosition() - mouseDownWithinTarget;
                
                if(line.isVertical()) {
                    lastPlan[n - 1].x += delta.x;
                    lastPlan[n].x += delta.x;
                }
                else {
                    lastPlan[n - 1].y += delta.y;
                    lastPlan[n].y += delta.y;
                }
                
                applyPlan(lastPlan, pstart, pend);
                roundedPath = path.createPathWithRoundedCorners(8.0f);
                repaint();
            }
        }
    } */
    
}

void Connection::mouseUp(const MouseEvent& e)
{
    isDragging = false;
}

void Connection::mouseDown(const MouseEvent& e)
{
    mouseDownWithinTarget = e.getMouseDownPosition();
    
    isSelected = !isSelected;
    repaint();
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    int left = std::min(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) - 4;
    int top = std::min(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) - 4;
    int right = std::max(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) + 4;
    int bottom = std::max(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) + 4;
    
    setBounds(left, top, right - left, bottom - top);
    resized();
}

void Connection::resized()
{
    if(!start || !end) return;
    
    auto& s = start->isInput ? start : end;
    auto& e = start->isInput ? end : start;
    
    Point<int> pstart = s->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = e->getCanvasBounds().getCentre() - getPosition();
    
    Rectangle<int> bounds(pstart, pend);
    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);
    
    auto point = path.getPointAlongPath(0.0f);

    if(!curvedConnection) {
        path.clear();
        path.startNewSubPath(pstart.toFloat());
        path.lineTo(pend.toFloat());
        roundedPath = path.createPathWithRoundedCorners(8.0f);
    }
    else {
        auto scale = path.getTransformToScaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false);
        
        bool finiteScale = std::isfinite(scale.mat00)
        && std::isfinite(scale.mat01)
        && std::isfinite(scale.mat02)
        && std::isfinite(scale.mat10)
        && std::isfinite(scale.mat11);
        
        if(finiteScale) path.applyTransform(scale);
        
        PathFlatteningIterator i (path, AffineTransform(), 1.0f);
        i.next();
        
        bool validStart = Point<int>(i.x1, i.y1).getDistanceFrom(pstart) < 5;
        bool validEnd;
        
        int numTurns = 0;
        bool lastTurn = Line<int>(i.x1, i.y1, i.x2, i.y2).isVertical();
        
        while (i.next()) {
            auto direction = Line<int>(i.x1, i.y1, i.x2, i.y2).isVertical();
            if(direction != lastTurn) {
                lastTurn = direction;
                numTurns++;
            }
            
            if(i.isLastInSubpath()) {
                validEnd  = Point<int>(i.x1, i.y1).getDistanceFrom(pend) < 5;
            }
        }
        
        if(!validStart || !validEnd || numTurns < 3) {
           // applyBestPath();
           // scale = AffineTransform();
        }
        
        applyBestPath();
        scale = AffineTransform();
        
        roundedPath = path.createPathWithRoundedCorners(8.0f);
    }
}

void Connection::applyBestPath() {
    if(!start || !end) return;
    
    auto& s = start->isInput ? start : end;
    auto& e = start->isInput ? end : start;
    
    Point<int> pstart = s->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = e->getCanvasBounds().getCentre() - getPosition();
    
    path.clear();
    
    findPath(pstart, pend);
    roundedPath = path.createPathWithRoundedCorners(8.0f);
    repaint();
}

void Connection::applyPlan(PathPlan plan, Point<int> start, Point<int> end) {
    
    // Function for snapping coordinates to a grid
    auto snap = [](int& coord1, const int& coord2, int diff){
        if(abs(coord1 - coord2) <= (diff * 0.75f)) coord1 = coord2;
    };
    
    // Create path with start point
    Path connectionPath;
    connectionPath.startNewSubPath(start.toFloat());
    
    // Add points inbetween if we've found a path
    if(!plan.empty()) {
        for(int n = 1; n < plan.size(); n++) {
            int incrX = std::max(abs(plan[n].x - plan[n - 1].x), 1);
            int incrY = std::max(abs(plan[n].y - plan[n - 1].y), 1);
            
            if(connectionPath.contains(plan[n].toFloat())) continue;
            snap(plan[n].x, start.x, incrX);
            snap(plan[n].x, end.x, incrX);
            snap(plan[n].y, start.y, incrY);
            snap(plan[n].y, end.y, incrY);
            
            connectionPath.lineTo(plan[n].toFloat());
        }
    }
    
    // End point
    connectionPath.lineTo(end.toFloat());
    
    path = connectionPath;
    lastPlan = plan;
}

void Connection::findPath(Point<int> start, Point<int> end){
    std::vector<PathPlan> alternativePaths;
    PathPlan pathStack;
    PathPlan bestPath;
    
    pathStack.reserve(8);
    
    int numFound = 0;
    int resolution = 3;
    
    int incrementX, incrementY;
    
    // Look for paths at an increasing resolution
    while(!numFound && resolution < 7) {
        // Find paths on an resolution*resolution lattice grid
        incrementX = std::max(abs(start.x - end.x) / resolution, resolution);
        incrementY = std::max(abs(start.y - end.y) / resolution, resolution);
        
        numFound = findLatticePaths(bestPath, alternativePaths, pathStack, start, end, {incrementX, incrementY});
        resolution++;
        pathStack.clear();
    }

    // Check if we've found an optimal path, otherwise use an alternative
    if(bestPath.empty() && !alternativePaths.empty()) {
        bestPath = alternativePaths[0];
    }
    
    PathPlan simplifiedPath = {bestPath[0]};
    
    bool lastDirection = Line<int>(bestPath[0], bestPath[1]).isVertical();
    for(int n = 2; n < bestPath.size(); n++) {
        auto line = Line<int>(bestPath[n - 1], bestPath[n]);
        
        if(line.isVertical() != lastDirection) {
            simplifiedPath.push_back(bestPath[n - 2]);
            simplifiedPath.push_back(bestPath[n - 1]);
            lastDirection = line.isVertical();
        }
    }
    
    simplifiedPath.push_back(bestPath.back());
    
    applyPlan(simplifiedPath, start, end);
}

int Connection::findLatticePaths(PathPlan& bestPath, std::vector<PathPlan>& altPaths, PathPlan& pathStack, Point<int> start, Point<int> end, Point<int> increment){
    

    
    // Stop after we've found a path
    if(!bestPath.empty())
        return 0;
    
    // Add point to path
    pathStack.push_back(start);
    
    // Check if it intersects any box
    if(pathStack.size() > 1 && straightLineIntersectsObject(Line<int>(pathStack.back(), *(pathStack.end() - 2)))) {
        return 0;
    }
    
    bool endVertically = pathStack[0].y > end.y;
    

    
    // Check if we've reached the destination
    if(abs(start.x - end.x) < (increment.x * 0.5) && abs(start.y - end.y) < (increment.y * 0.5)) {
        bestPath = pathStack;
        return 1;
    }
    
    // Count the number of found paths
    int count = 0;
    
    // Get current stack to revert to after each trial
    auto pathCopy = pathStack;
    
    auto followLine = [this, &count, &pathCopy, &bestPath, &altPaths, &pathStack, &increment](Point<int> start, Point<int> end, bool isX) {
        
        auto& coord1 = isX ? start.x : start.y;
        auto& coord2 = isX ? end.x : end.y;
        auto& incr =   isX ? increment.x : increment.y;
       
        if(abs(coord1 - coord2) >= incr) {
            coord1 > coord2 ? coord1 -= incr : coord1 += incr;
            count += findLatticePaths(bestPath, altPaths, pathStack, start, end, increment);
            pathStack = pathCopy;
        }
    };
    
    // If we're halfway on the axis, change preferred direction by inverting search order
    // This will make it do a staircase effect
    if(endVertically) {
        if(abs(start.y - end.y) >= abs(pathStack[0].y - end.y) * 0.5) {
            followLine(start, end, false);
            followLine(start, end, true);
        }
        else {
            followLine(start, end, true);
            followLine(start, end, false);
        }
    }
    else {
        if(abs(start.x - end.x) >= abs(pathStack[0].x - end.x) * 0.5) {
            followLine(start, end, true);
            followLine(start, end, false);
        }
        else {
            followLine(start, end, false);
            followLine(start, end, true);
        }
    }

    return count;
}


bool Connection::straightLineIntersectsObject(Line<int> toCheck) {
    bool result = false;
    for(const auto& box : cnv->boxes) {
        
        auto bounds = box->getBounds().expanded(3) - getPosition();
        
        if(box->graphics)
            bounds.removeFromTop(25);
        
        if(box == start->box || box == end->box) continue;
        
        auto intersectV = [](Line<int> first, Line<int> second) {
            if(first.getStartY() > first.getEndY()) {
                first = {first.getEnd(), first.getStart()};
            }
            
            return first.getStartX() > second.getStartX() && first.getStartX() < second.getEndX() && second.getStartY() > first.getStartY() && second.getStartY() < first.getEndY();
        };
        
        auto intersectH = [](Line<int> first, Line<int> second) {
            if(first.getStartX() > first.getEndX()) {
                first = {first.getEnd(), first.getStart()};
            }
            
            return first.getStartY() > second.getStartY() && first.getStartY() < second.getEndY() && second.getStartX() > first.getStartX() && second.getStartX() < first.getEndX();
        };
        
        
        if(toCheck.isVertical() &&
           (intersectV(toCheck, Line<int>(bounds.getTopLeft(), bounds.getTopRight())) ||
            intersectV(toCheck, Line<int>(bounds.getBottomRight(), bounds.getBottomLeft())))) {
            result = true;
            break;
        }
        else if(toCheck.isHorizontal() &&
                (intersectH(toCheck, Line<int>(bounds.getTopRight(), bounds.getBottomRight())) ||
                 intersectH(toCheck, Line<int>(bounds.getTopLeft(), bounds.getBottomLeft()))))
        {
            result = true;
            break;
        }
    }
    
    return result;
}
