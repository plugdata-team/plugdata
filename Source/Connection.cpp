/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Connection.h"

#include "Canvas.h"
#include "Edge.h"
#include "LookAndFeel.h"

Connection::Connection(Canvas* parent, Edge* s, Edge* e, bool exists) : cnv(parent), outlet(s->isInlet ? e : s), inlet(s->isInlet ? s : e), outbox(outlet->box), inbox(inlet->box)
{
    
    locked.referTo(parent->locked);
    
    // Make sure it's not 2x the same edge
    if (!outlet || !inlet || outlet->isInlet == inlet->isInlet)
    {
        outlet = nullptr;
        inlet = nullptr;
        jassertfalse;
        return;
    }
    
    inIdx = inlet->edgeIdx;
    outIdx = outlet->edgeIdx;
    
    outlet->repaint();
    inlet->repaint();
    
    // If it doesn't already exist in pd, create connection in pd
    if (!exists)
    {
        bool canConnect = parent->patch.createConnection(outbox->getPointer(), outIdx, inbox->getPointer(), inIdx);
        
        if (!canConnect)
        {
            outlet = nullptr;
            inlet = nullptr;
            
            MessageManager::callAsync([this]() { cnv->connections.removeObject(this); });
            
            return;
        }
    }
    else
    {
        auto info = cnv->storage.getInfo(getId(), "Path");
        if (!info.isEmpty()) setState(info);
    }
    
    // Listen to changes at edges
    outbox->addComponentListener(this);
    inbox->addComponentListener(this);
    outlet->addComponentListener(this);
    inlet->addComponentListener(this);
    
    setInterceptsMouseClicks(true, false);
    
    addMouseListener(cnv, true);
    
    cnv->addAndMakeVisible(this);
    setAlwaysOnTop(true);
    
    // Update position (TODO: don't invoke virtual functions from constructor!)
    componentMovedOrResized(*outlet, true, true);
    componentMovedOrResized(*inlet, true, true);
    
    updatePath();
    repaint();
}

String Connection::getState()
{
    String pathAsString;
    
    MemoryOutputStream stream;
    
    for (auto& point : currentPlan)
    {
        stream.writeInt(point.x - outlet->getCanvasBounds().getCentre().x);
        stream.writeInt(point.y - outlet->getCanvasBounds().getCentre().y);
    }
    
    return stream.getMemoryBlock().toBase64Encoding();
}

void Connection::setState(const String& state)
{
    auto plan = PathPlan();
    
    auto block = MemoryBlock();
    auto succeeded = block.fromBase64Encoding(state);
    
    if (succeeded)
    {
        auto stream = MemoryInputStream(block, false);
        
        while (!stream.isExhausted())
        {
            auto x = stream.readInt();
            auto y = stream.readInt();
            
            plan.emplace_back(x + outlet->getCanvasBounds().getCentreX(), y + outlet->getCanvasBounds().getCentreY());
        }
    }
    currentPlan = plan;
    updatePath();
}

String Connection::getId() const
{
    MemoryOutputStream stream;
    
    // TODO: check if connection is still valid before requesting idx from box
    
    stream.writeInt(cnv->patch.getIndex(inbox->getPointer()));
    stream.writeInt(cnv->patch.getIndex(outbox->getPointer()));
    stream.writeInt(inIdx);
    stream.writeInt(outbox->numInputs + outIdx);
    
    return stream.getMemoryBlock().toBase64Encoding();
}

Connection::~Connection()
{
    if(!cnv->isBeingDeleted) {
        // Ensure there's no pointer to this object in the selection
        cnv->setSelected(this, false);
    }
    
    if (outlet)
    {
        outlet->repaint();
        outlet->removeComponentListener(this);
    }
    if (outbox)
    {
        outbox->removeComponentListener(this);
    }
    
    if (inlet)
    {
        inlet->repaint();
        inlet->removeComponentListener(this);
    }
    if (inbox)
    {
        inbox->removeComponentListener(this);
    }
}

bool Connection::hitTest(int x, int y)
{
    if (locked == var(true) || !cnv->connectingEdges.isEmpty()) return false;
    
    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));
    
    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);
    
    // Get outlet and inlet point
    auto pstart = getStartPoint().toFloat();
    auto pend = getEndPoint().toFloat();
    
    if (cnv->isSelected(this) && (startReconnectHandle.contains(position) || endReconnectHandle.contains(position)))
        return true;
    
    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position + getPosition().toFloat()) < 8.0f || pend.getDistanceFrom(position + getPosition().toFloat()) < 8.0f)
        return false;
    
    
    return nearestPoint.getDistanceFrom(position) < 3;
}

bool Connection::intersects(Rectangle<float> toCheck, int accuracy) const
{
    PathFlatteningIterator i(toDraw);
    
    while (i.next())
    {
        auto point1 = Point<float>(i.x1, i.y1);
        
        // Skip points to reduce accuracy a bit for better performance
        for (int n = 0; n < accuracy; n++)
        {
            auto next = i.next();
            if (!next) break;
        }
        
        auto point2 = Point<float>(i.x2, i.y2);
        auto currentLine = Line<float>(point1, point2);
        
        if (toCheck.intersects(currentLine))
        {
            return true;
        }
    }
    
    return false;
}
void Connection::paint(Graphics& g)
{
    auto baseColour = findColour(PlugDataColour::connectionColourId);
    auto dataColour = findColour(PlugDataColour::highlightColourId);
    auto signalColour = findColour(PlugDataColour::signalColourId);
    auto handleColour = outlet->isSignal ? dataColour : signalColour;
    
    if (cnv->isSelected(this))
    {
        baseColour = outlet->isSignal ? signalColour : dataColour;
    }
    else if (isMouseOver())
    {
        baseColour = outlet->isSignal ? signalColour : dataColour;
        baseColour = baseColour.brighter(0.6f);
    }
    
    g.setColour(baseColour.darker(0.1));
    g.strokePath(toDraw, PathStrokeType(2.5f, PathStrokeType::mitered, PathStrokeType::square));
    
    g.setColour(baseColour.darker(0.2));
    g.strokePath(toDraw, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::square));
    
    g.setColour(baseColour);
    g.strokePath(toDraw, PathStrokeType(0.5f, PathStrokeType::mitered, PathStrokeType::square));
    
    if (cnv->isSelected(this))
    {
        auto mousePos = getMouseXYRelative();
        
        bool overStart = startReconnectHandle.contains(mousePos.toFloat());
        bool overEnd = endReconnectHandle.contains(mousePos.toFloat());
        
        g.setColour(handleColour);
        
        g.fillEllipse(startReconnectHandle.expanded(overStart ? 3.0f : 0.0f));
        g.fillEllipse(endReconnectHandle.expanded(overEnd ? 3.0f : 0.0f));
        
        g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
        g.drawEllipse(startReconnectHandle.expanded(overStart ? 3.0f : 0.0f), 0.5f);
        g.drawEllipse(endReconnectHandle.expanded(overEnd ? 3.0f : 0.0f), 0.5f);
    }
}

bool Connection::isSegmented()
{
    return segmented;
}
void Connection::setSegmented(bool isSegmented)
{
    segmented = isSegmented;
    cnv->storage.setInfo(getId(), "Segmented", segmented ? "1" : "0");
    updatePath();
    repaint();
}

void Connection::mouseMove(const MouseEvent& e)
{
    int n = getClosestLineIdx(e.getPosition() + origin, currentPlan);
    
    if (isSegmented() && currentPlan.size() > 2 && n > 0)
    {
        auto line = Line<int>(currentPlan[n - 1], currentPlan[n]);
        
        if (line.isVertical())
        {
            setMouseCursor(MouseCursor::LeftRightResizeCursor);
        }
        else if (line.isHorizontal())
        {
            setMouseCursor(MouseCursor::UpDownResizeCursor);
        }
        else
        {
            setMouseCursor(MouseCursor::NormalCursor);
        }
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
    
    repaint();
}

void Connection::mouseExit(const MouseEvent& e)
{
    repaint();
}

void Connection::mouseDown(const MouseEvent& e)
{
    wasSelected = cnv->isSelected(this);
    
    // Deselect all other connection if shift or command is not down
    if (!ModifierKeys::getCurrentModifiers().isCommandDown() && !ModifierKeys::getCurrentModifiers().isShiftDown())
    {
        cnv->deselectAll();
    }
    
    cnv->setSelected(this, true);
    repaint();
    
    if (currentPlan.size() <= 2) return;
    
    int n = getClosestLineIdx(e.getPosition() + origin, currentPlan);
    if (n < 0) return;
    
    if (Line<int>(currentPlan[n - 1], currentPlan[n]).isVertical())
    {
        mouseDownPosition = currentPlan[n].x;
    }
    else
    {
        mouseDownPosition = currentPlan[n].y;
    }
    
    dragIdx = n;
}

void Connection::mouseDrag(const MouseEvent& e)
{
    if (wasSelected && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6)
    {
        reconnect(inlet, true);
    }
    if (wasSelected && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6)
    {
        reconnect(outlet, true);
    }
    
    if (currentPlan.empty()) return;
    
    if (isSegmented() && dragIdx != -1)
    {
        auto n = dragIdx;
        auto delta = e.getPosition() - e.getMouseDownPosition();
        auto line = Line<int>(currentPlan[n - 1], currentPlan[n]);
        
        if (line.isVertical())
        {
            currentPlan[n - 1].x = mouseDownPosition + delta.x;
            currentPlan[n].x = mouseDownPosition + delta.x;
        }
        else
        {
            currentPlan[n - 1].y = mouseDownPosition + delta.y;
            currentPlan[n].y = mouseDownPosition + delta.y;
        }
        
        updatePath();
        repaint();
    }
}

void Connection::mouseUp(const MouseEvent& e)
{
    if (dragIdx != -1)
    {
        auto state = getState();
        lastId = getId();
        
        cnv->storage.setInfo(lastId, "Path", state);
        dragIdx = -1;
    }
    
    if (wasSelected && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && startReconnectHandle.contains(e.getPosition().toFloat()))
    {
        reconnect(inlet, false);
    }
    if (wasSelected && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && endReconnectHandle.contains(e.getPosition().toFloat()))
    {
        reconnect(outlet, false);
    }
    if (reconnecting.size())
    {
        // Async to safely self-destruct
        MessageManager::callAsync([canvas = cnv, r = reconnecting]() mutable {
            for(auto& c : r) {
                if(c) {
                    canvas->connections.removeObject(c.getComponent());
                }
            }
        });
        
        reconnecting.clear();
    }
    
}

int Connection::getClosestLineIdx(const Point<int>& position, const PathPlan& plan)
{
    if (plan.size() < 2) return -1;
    
    for (int n = 2; n < plan.size() - 1; n++)
    {
        auto line = Line<int>(plan[n - 1], plan[n]);
        Point<int> nearest;
        if (line.getDistanceFromPoint(position - offset, nearest) < 3)
        {
            return n;
        }
    }
    
    return -1;
}

void Connection::reconnect(Edge* target, bool dragged)
{
    if(!reconnecting.isEmpty()) return;
    
    auto& otherEdge = target == inlet ? outlet : inlet;
    
    Array<Connection*> connections = {this};
    
    if(Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
        for(auto* c : otherEdge->box->getConnections()) {
            if(c == this || !cnv->isSelected(c)) continue;
            
            connections.add(c);
        }
    }
    
    for(auto* c : connections) {
        
        if (cnv->patch.hasConnection(c->outbox->getPointer(), c->outIdx, c->inbox->getPointer(), c->inIdx))
        {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(c->outbox->getPointer(), c->outIdx, c->inbox->getPointer(), c->inIdx);
        }
        
        // Create new connection
        cnv->connectingEdges.add(target->isInlet ? c->inlet : c->outlet);
        
        cnv->connectingWithDrag = true;
        c->setVisible(false);
        
        reconnecting.add(SafePointer(c));
        
        // Make sure we're deselected and remove object
        cnv->setSelected(c, false);
    }
    
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    if (!inlet || !outlet) return;
    
    auto pstart = getStartPoint();
    auto pend = getEndPoint();
    
    if (currentPlan.size() <= 2 || cnv->storage.getInfo(getId(), "Style") == "0")
    {
        updatePath();
        return;
    }
    
    // If both inlet and outlet are selected we can just move the connection cord
    if ((cnv->isSelected(outbox) && cnv->isSelected(inbox)) || cnv->updatingBounds)
    {
        auto offset = pstart - currentPlan[0];
        for (auto& point : currentPlan) point += offset;
        updatePath();
        return;
    }
    
    int idx1 = 0;
    int idx2 = 1;
    
    auto& position = pstart;
    if (&component == inlet || &component == inbox)
    {
        idx1 = static_cast<int>(currentPlan.size() - 1);
        idx2 = static_cast<int>(currentPlan.size() - 2);
        position = pend;
    }
    
    if (Line<int>(currentPlan[idx1], currentPlan[idx2]).isVertical())
    {
        currentPlan[idx2].x = position.x;
    }
    else
    {
        currentPlan[idx2].y = position.y;
    }
    
    currentPlan[idx1] = position;
    updatePath();
}



Point<int> Connection::getStartPoint()
{
    return {outlet->getCanvasBounds().getCentreX(), outlet->getCanvasBounds().getCentreY()};
}

Point<int> Connection::getEndPoint()
{
    
    return  {inlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreY()};
}

void Connection::updatePath()
{
    if (!outlet || !inlet) return;
    
    int left = std::min(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) - 4;
    int top = std::min(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) - 4;
    int right = std::max(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) + 4;
    int bottom = std::max(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) + 4;
    
    origin = Rectangle<int>(left, top, right - left, bottom - top).getPosition();
    
    auto pstart = getStartPoint() - origin;
    auto pend = getEndPoint() - origin;
    
    if(lastId.isEmpty()) lastId = getId();
    
    segmented = cnv->storage.getInfo(lastId, "Segmented") == "1";
    
    if (!segmented)
    {
        toDraw.clear();
        
        const float width = std::max(pstart.x, pend.x) - std::min(pstart.x, pend.x);
        const float height = std::max(pstart.y, pend.y) - std::min(pstart.y, pend.y);
        
        const float min = std::min<float>(width, height);
        const float max = std::max<float>(width, height);
        
        const float maxShiftY = 20.f;
        const float maxShiftX = 20.f;
        
        const float shiftY = std::min<float>(maxShiftY, max * 0.5);
        
        const float shiftX = ((pstart.y >= pend.y) ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * ((pstart.x < pend.x) ? -1. : 1.);
        
        const Point<float> ctrlPoint1{pstart.x - shiftX, pstart.y + shiftY};
        const Point<float> ctrlPoint2{pend.x + shiftX, pend.y - shiftY};
        
        toDraw.startNewSubPath(pstart.toFloat());
        toDraw.cubicTo(ctrlPoint1, ctrlPoint2, pend.toFloat());
        
        currentPlan.clear();
    }
    else
    {
        if (currentPlan.empty())
        {
            findPath();
        }
        
        auto snap = [this](Point<int> point, int idx1, int idx2)
        {
            if (Line<int>(currentPlan[idx1], currentPlan[idx2]).isVertical())
            {
                currentPlan[idx2].x = point.x + origin.x;
            }
            else
            {
                currentPlan[idx2].y = point.y + origin.y;
            }
            
            currentPlan[idx1] = point + origin;
        };
        
        snap(pstart, 0, 1);
        snap(pend, static_cast<int>(currentPlan.size() - 1), static_cast<int>(currentPlan.size() - 2));
        
        Path connectionPath;
        connectionPath.startNewSubPath(pstart.toFloat());
        
        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++)
        {
            if (connectionPath.contains(currentPlan[n].toFloat())) continue;  // ??
            
            connectionPath.lineTo(currentPlan[n].toFloat() - origin.toFloat());
        }
        
        connectionPath.lineTo(pend.toFloat());
        toDraw = connectionPath.createPathWithRoundedCorners(8.0f);
    }
    
    repaint();
    
    auto bounds = toDraw.getBounds().toNearestInt().expanded(8);
    setBounds(bounds + origin);
    
    if (bounds.getX() < 0 || bounds.getY() < 0)
    {
        toDraw.applyTransform(AffineTransform::translation(-bounds.getX(), -bounds.getY()));
    }
    
    offset = {-bounds.getX(), -bounds.getY()};
    
    startReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(8.5f));
    endReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(std::max(toDraw.getLength() - 8.5f, 9.5f)));
}

void Connection::findPath()
{
    if (!outlet || !inlet) return;
    
    auto pstart = getStartPoint();
    auto pend = getEndPoint();
    
    auto pathStack = PathPlan();
    auto bestPath = PathPlan();
    
    pathStack.reserve(8);
    
    auto numFound = 0;
    
    int incrementX, incrementY;
    
    auto distance = pstart.getDistanceFrom(pend);
    auto distanceX = std::abs(pstart.x - pend.x);
    auto distanceY = std::abs(pstart.y - pend.y);
    
    int maxXResolution = std::min(distanceX / 10, 16);
    int maxYResolution = std::min(distanceY / 10, 16);
    
    int resolutionX = 9;
    int resolutionY = 9;
    
    auto obstacles = Array<Rectangle<int>>();
    auto searchBounds = Rectangle<int>(pstart, pend);
    
    for(auto* box : cnv->boxes) {
        if(box->getBounds().intersects(searchBounds)) {
            obstacles.add(box->getBounds());
        }
    }
    
    // Look for paths at an increasing resolution
    while (!numFound && resolutionX < maxXResolution && distance > 40)
    {
        
        // Find paths on a resolution*resolution lattice ObjectGrid
        incrementX = distanceX / resolutionX;
        incrementY = distanceY / resolutionY;
        
        numFound = findLatticePaths(bestPath, pathStack, pend, pstart, {incrementX, incrementY});
        
        if(resolutionX < maxXResolution) resolutionX++;
        if(resolutionY < maxXResolution) resolutionY++;
        
        if(resolutionX > maxXResolution || resolutionY > maxYResolution) break;
        
        pathStack.clear();
    }
    
    PathPlan simplifiedPath;
    
    bool direction;
    if (!bestPath.empty())
    {
        simplifiedPath.push_back(bestPath.front());
        
        direction = bestPath[0].x == bestPath[1].x;
        
        if (!direction) simplifiedPath.push_back(bestPath.front());
        
        for (int n = 1; n < bestPath.size(); n++)
        {
            if ((bestPath[n].x != bestPath[n - 1].x && direction) || (bestPath[n].y != bestPath[n - 1].y && !direction))
            {
                simplifiedPath.push_back(bestPath[n - 1]);
                direction = !direction;
            }
        }
        
        simplifiedPath.push_back(bestPath.back());
        
        if (!direction) simplifiedPath.push_back(bestPath.back());
    }
    else
    {
        if (pend.y < pstart.y)
        {
            int xHalfDistance = (pstart.x - pend.x) / 2;
            
            simplifiedPath.push_back(pend);  // double to make it draggable
            simplifiedPath.push_back(pend);
            simplifiedPath.push_back({pend.x + xHalfDistance, pend.y});
            simplifiedPath.push_back({pend.x + xHalfDistance, pstart.y});
            simplifiedPath.push_back(pstart);
            simplifiedPath.push_back(pstart);
        }
        else
        {
            int yHalfDistance = (pstart.y - pend.y) / 2;
            simplifiedPath.push_back(pend);
            simplifiedPath.push_back({pend.x, pend.y + yHalfDistance});
            simplifiedPath.push_back({pstart.x, pend.y + yHalfDistance});
            simplifiedPath.push_back(pstart);
        }
    }
    std::reverse(simplifiedPath.begin(), simplifiedPath.end());
    
    currentPlan = simplifiedPath;
    
    auto state = getState();
    lastId = getId();
    cnv->storage.setInfo(lastId, "Path", state);
}

int Connection::findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<int> pstart, Point<int> pend, Point<int> increment)
{
    
    auto obstacles = Array<Box*>();
    auto searchBounds = Rectangle<int>(pstart, pend);
    
    for(auto* box : cnv->boxes) {
        if(box->getBounds().intersects(searchBounds)) {
            obstacles.add(box);
        }
    }
    
    // Stop after we've found a path
    if (!bestPath.empty()) return 0;
    
    // Add point to path
    pathStack.push_back(pstart);
    
    // Check if it intersects any box
    if (pathStack.size() > 1 && straightLineIntersectsObject(Line<int>(pathStack.back(), *(pathStack.end() - 2)), obstacles))
    {
        return 0;
    }
    
    bool endVertically = pathStack[0].y > pend.y;
    
    // Check if we've reached the destination
    if (std::abs(pstart.x - pend.x) < (increment.x * 0.5) && std::abs(pstart.y - pend.y) < (increment.y * 0.5))
    {
        bestPath = pathStack;
        return 1;
    }
    
    // Count the number of found paths
    int count = 0;
    
    // Get current stack to revert to after each trial
    auto pathCopy = pathStack;
    
    auto followLine = [this, &count, &pathCopy, &bestPath, &pathStack, &increment](Point<int> currentOutlet, Point<int> currentInlet, bool isX)
    {
        auto& coord1 = isX ? currentOutlet.x : currentOutlet.y;
        auto& coord2 = isX ? currentInlet.x : currentInlet.y;
        auto& incr = isX ? increment.x : increment.y;
        
        if (std::abs(coord1 - coord2) >= incr)
        {
            coord1 > coord2 ? coord1 -= incr : coord1 += incr;
            count += findLatticePaths(bestPath, pathStack, currentOutlet, currentInlet, increment);
            pathStack = pathCopy;
        }
    };
    
    // If we're halfway on the axis, change preferred direction by inverting search order
    // This will make it do a staircase effect
    if (endVertically)
    {
        if (std::abs(pstart.y - pend.y) >= std::abs(pathStack[0].y - pend.y) * 0.5)
        {
            followLine(pstart, pend, false);
            followLine(pstart, pend, true);
        }
        else
        {
            followLine(pstart, pend, true);
            followLine(pstart, pend, false);
        }
    }
    else
    {
        if (std::abs(pstart.x - pend.x) >= std::abs(pathStack[0].x - pend.x) * 0.5)
        {
            followLine(pstart, pend, true);
            followLine(pstart, pend, false);
        }
        else
        {
            followLine(pstart, pend, false);
            followLine(pstart, pend, true);
        }
    }
    
    return count;
}

bool Connection::intersectsObject(Box* object)
{
    auto b = (object->getBounds() - getPosition()).toFloat();
    return toDraw.intersectsLine({b.getTopLeft(), b.getTopRight()})
    || toDraw.intersectsLine({b.getTopLeft(), b.getBottomLeft()})
    || toDraw.intersectsLine({b.getBottomRight(), b.getBottomLeft()})
    || toDraw.intersectsLine({b.getBottomRight(), b.getTopRight()});
}


bool Connection::straightLineIntersectsObject(Line<int> toCheck, Array<Box*>& boxes)
{
    
    for (const auto& box : boxes)
    {
        auto bounds = box->getBounds().expanded(1);
        
        if (box == outbox || box == inbox || !bounds.intersects(getBounds())) continue;
        
        auto intersectV = [](Line<int> first, Line<int> second)
        {
            if (first.getStartY() > first.getEndY())
            {
                first = {first.getEnd(), first.getStart()};
            }
            
            return first.getStartX() > second.getStartX() && first.getStartX() < second.getEndX() && second.getStartY() > first.getStartY() && second.getStartY() < first.getEndY();
        };
        
        auto intersectH = [](Line<int> first, Line<int> second)
        {
            if (first.getStartX() > first.getEndX())
            {
                first = {first.getEnd(), first.getStart()};
            }
            
            return first.getStartY() > second.getStartY() && first.getStartY() < second.getEndY() && second.getStartX() > first.getStartX() && second.getStartX() < first.getEndX();
        };
        
        bool intersectsV = toCheck.isVertical() && (intersectV(toCheck, Line<int>(bounds.getTopLeft(), bounds.getTopRight())) || intersectV(toCheck, Line<int>(bounds.getBottomRight(), bounds.getBottomLeft())));
        
        bool intersectsH = toCheck.isHorizontal() && (intersectH(toCheck, Line<int>(bounds.getTopRight(), bounds.getBottomRight())) || intersectH(toCheck, Line<int>(bounds.getTopLeft(), bounds.getBottomLeft())));
        if (intersectsV || intersectsH)
        {
            return true;
        }
        
        /*
         if(bounds.toFloat().intersects(toCheck.toFloat())) {
         return true;
         } TODO: benchmark these two options */
        // TODO: possible mark areas that have already been visited?
    }
    
    return false;
}

