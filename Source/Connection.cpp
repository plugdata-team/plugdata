/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Connection.h"

#include "Canvas.h"
#include "Edge.h"

Connection::Connection(Canvas* parent, Edge* s, Edge* e, bool exists) : cnv(parent), outlet(s->isInlet ? e : s), inlet(s->isInlet ? s : e)
{
    // Should improve performance
    setBufferedToImage(true);

    locked.referTo(parent->locked);
    connectionStyle.referTo(parent->connectionStyle);

    // Receive mouse events on canvas: maybe not needed because we test for hitbox now?
    // addMouseListener(cnv, true);

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
        bool canConnect = parent->patch.createConnection(outlet->box->pdObject.get(), outIdx, inlet->box->pdObject.get(), inIdx);

        if (!canConnect)
        {
            outlet = nullptr;
            inlet = nullptr;

            MessageManager::callAsync([this](){
                cnv->connections.removeObject(this);
            });

            return;
        }
    }
    else
    {
        auto info = cnv->patch.getExtraInfo(getId());
        if (!info.isEmpty()) setState(info);
    }

    // Listen to changes at edges
    outlet->box->addComponentListener(this);
    inlet->box->addComponentListener(this);
    outlet->addComponentListener(this);
    inlet->addComponentListener(this);

    setInterceptsMouseClicks(true, false);

    cnv->addAndMakeVisible(this);

    setAlwaysOnTop(true);

    // Update position
    componentMovedOrResized(*outlet, true, true);
    componentMovedOrResized(*inlet, true, true);

    updatePath();
    repaint();
}

MemoryBlock Connection::getState()
{
    MemoryOutputStream info;

    String pathAsString;
    for (auto& point : currentPlan)
    {
        pathAsString += String(point.x - outlet->getPosition().x) + "*" + String(point.y - outlet->getPosition().y) + ",";
    }

    info.writeString(pathAsString);

    return info.getMemoryBlock();
}

void Connection::setState(MemoryBlock& block)
{
    auto info = MemoryInputStream(cnv->patch.getExtraInfo(getId()));

    auto pathAsString = info.readString();

    auto plan = PathPlan();

    for (auto& point : StringArray::fromTokens(pathAsString, ",", ""))
    {
        if (point.length() < 1) continue;

        auto x = point.upToFirstOccurrenceOf("*", false, false).getFloatValue();
        auto y = point.fromFirstOccurrenceOf("*", false, false).getFloatValue();

        plan.emplace_back(x + outlet->getPosition().x, y + outlet->getPosition().y);
    }
    currentPlan = plan;
    updatePath();
}

String Connection::getId() const
{
    int idx1 = cnv->patch.getIndex(inlet->box->pdObject->getPointer());
    int idx2 = cnv->patch.getIndex(outlet->box->pdObject->getPointer());
    return "c" + String(idx1) + String(idx2) + String(inIdx) + String(outIdx);
}

Connection::~Connection()
{
    if (outlet && outlet->box)
    {
        outlet->repaint();
        outlet->box->removeComponentListener(this);
        outlet->removeComponentListener(this);
    }
    if (inlet && inlet->box)
    {
        inlet->repaint();
        inlet->box->removeComponentListener(this);
        inlet->removeComponentListener(this);
    }
}

bool Connection::hitTest(int x, int y)
{
    if (locked == var(true)) return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));

    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto pstart = outlet->getCanvasBounds().getCentre().toFloat() - origin.toFloat();
    auto pend = inlet->getCanvasBounds().getCentre().toFloat() - origin.toFloat();

    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position) < 10.0f) return false;
    if (pend.getDistanceFrom(position) < 10.0f) return false;

    if (nearestPoint.getDistanceFrom(position) < 3)
    {
        return true;
    }

    return false;
}

void Connection::paint(Graphics& g)
{
    g.setColour(Colours::grey);
    g.strokePath(toDraw, PathStrokeType(2.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    auto baseColour = cnv->backgroundColour.contrasting();

    if (cnv->isSelected(this))
    {
        baseColour = outlet->isSignal ? Colours::yellow : findColour(Slider::thumbColourId);
    }
    else if (isMouseOver())
    {
        baseColour = outlet->isSignal ? Colours::yellow : findColour(Slider::thumbColourId);
        baseColour = baseColour.brighter(0.6f);
    }

    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(toDraw, PathStrokeType(2.0f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void Connection::mouseMove(const MouseEvent& e)
{
    bool segmentedConnection = connectionStyle == var(true);
    int n = getClosestLineIdx(e.getPosition() + origin, currentPlan);

    if (segmentedConnection && currentPlan.size() > 2 && n > 0)
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
    if (currentPlan.empty()) return;

    auto pstart = outlet->getCanvasBounds().getCentre() - origin;
    auto pend = inlet->getCanvasBounds().getCentre() - origin;

    bool curvedConnection = cnv->pd->settingsTree.getProperty("ConnectionStyle");
    if (curvedConnection && dragIdx != -1)
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

void Connection::mouseUp(const MouseEvent& e)
{
    if (dragIdx != -1)
    {
        auto state = getState();
        lastId = getId();
        cnv->patch.setExtraInfo(lastId, state);
        dragIdx = -1;
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    auto pstart = outlet->getCanvasBounds().getCentre();
    auto pend = inlet->getCanvasBounds().getCentre();

    if (currentPlan.size() <= 2 || connectionStyle == var(false))
    {
        updatePath();
        return;
    }

    // If both inlet and outlet are selected we can just move the connection cord
    if (cnv->isSelected(outlet->box) && cnv->isSelected(inlet->box))
    {
        auto offset = pstart - currentPlan[0];
        for (auto& point : currentPlan) point += offset;
        updatePath();
        return;
    }

    int idx1 = 0;
    int idx2 = 1;

    auto& position = pstart;
    if (&component == inlet || &component == inlet->box)
    {
        idx1 = currentPlan.size() - 1;
        idx2 = currentPlan.size() - 2;
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

void Connection::updatePath()
{
    if (!outlet || !inlet) return;

    int left = std::min(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) - 4;
    int top = std::min(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) - 4;
    int right = std::max(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) + 4;
    int bottom = std::max(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) + 4;

    origin = Rectangle<int>(left, top, right - left, bottom - top).getPosition();

    auto pstart = outlet->getCanvasBounds().getCentre() - origin;
    auto pend = inlet->getCanvasBounds().getCentre() - origin;

    bool segmentedConnection = connectionStyle == var(true);

    if (!segmentedConnection)
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
        snap(pend, currentPlan.size() - 1, currentPlan.size() - 2);

        Path connectionPath;
        connectionPath.startNewSubPath(pstart.toFloat());

        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++)
        {
            if (connectionPath.contains(currentPlan[n].toFloat())) continue;

            connectionPath.lineTo(currentPlan[n].toFloat() - origin.toFloat());
        }

        connectionPath.lineTo(pend.toFloat());
        toDraw = connectionPath.createPathWithRoundedCorners(8.0f);

        repaint();
    }

    auto bounds = toDraw.getBounds().toNearestInt().expanded(4);
    setBounds(bounds + origin);

    if (bounds.getX() < 0 || bounds.getY() < 0)
    {
        toDraw.applyTransform(AffineTransform::translation(-bounds.getX(), -bounds.getY()));
    }

    offset = {-bounds.getX(), -bounds.getY()};
}

void Connection::findPath()
{
    if (!outlet || !inlet) return;

    auto pstart = inlet->getCanvasBounds().getCentre();
    auto pend = outlet->getCanvasBounds().getCentre();

    auto pathStack = PathPlan();
    auto bestPath = PathPlan();

    pathStack.reserve(8);

    auto numFound = 0;
    auto resolution = 3;

    int incrementX, incrementY;

    auto distance = pend.getDistanceFrom(pstart);
    auto bounds = Rectangle<int>(pstart, pend);

    // Look for paths at an increasing resolution
    while (!numFound && resolution < 7 && distance > 40)
    {
        // Find paths on an resolution*resolution lattice grid
        incrementX = std::max(abs(pstart.x - pend.x) / resolution, resolution);
        incrementY = std::max(abs(pstart.y - pend.y) / resolution, resolution);

        numFound = findLatticePaths(bestPath, pathStack, pstart, pend, {incrementX, incrementY});
        resolution++;
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
        if (pstart.y < pend.y)
        {
            int xHalfDistance = (pend.x - pstart.x) / 2;

            simplifiedPath.push_back(pstart);  // double to make it draggable
            simplifiedPath.push_back(pstart);
            simplifiedPath.push_back({pstart.x + xHalfDistance, pstart.y});
            simplifiedPath.push_back({pstart.x + xHalfDistance, pend.y});
            simplifiedPath.push_back(pend);
            simplifiedPath.push_back(pend);
        }
        else
        {
            int yHalfDistance = (pend.y - pstart.y) / 2;
            simplifiedPath.push_back(pstart);
            simplifiedPath.push_back({pstart.x, pstart.y + yHalfDistance});
            simplifiedPath.push_back({pend.x, pstart.y + yHalfDistance});
            simplifiedPath.push_back(pend);
        }
    }
    std::reverse(simplifiedPath.begin(), simplifiedPath.end());

    currentPlan = simplifiedPath;

    auto state = getState();
    lastId = getId();
    cnv->patch.setExtraInfo(lastId, state);
}

int Connection::findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<int> pstart, Point<int> pend, Point<int> increment)
{
    // Stop after we've found a path
    if (!bestPath.empty()) return 0;

    // Add point to path
    pathStack.push_back(pstart);

    // Check if it intersects any box
    if (pathStack.size() > 1 && straightLineIntersectsObject(Line<int>(pathStack.back(), *(pathStack.end() - 2))))
    {
        return 0;
    }

    bool endVertically = pathStack[0].y > pend.y;

    // Check if we've reached the destination
    if (abs(pstart.x - pend.x) < (increment.x * 0.5) && abs(pstart.y - pend.y) < (increment.y * 0.5))
    {
        bestPath = pathStack;
        return 1;
    }

    // Count the number of found paths
    int count = 0;

    // Get current stack to revert to after each trial
    auto pathCopy = pathStack;

    auto followLine = [this, &count, &pathCopy, &bestPath, &pathStack, &increment](Point<int> outlet, Point<int> inlet, bool isX)
    {
        auto& coord1 = isX ? outlet.x : outlet.y;
        auto& coord2 = isX ? inlet.x : inlet.y;
        auto& incr = isX ? increment.x : increment.y;

        if (abs(coord1 - coord2) >= incr)
        {
            coord1 > coord2 ? coord1 -= incr : coord1 += incr;
            count += findLatticePaths(bestPath, pathStack, outlet, inlet, increment);
            pathStack = pathCopy;
        }
    };

    // If we're halfway on the axis, change preferred direction by inverting search order
    // This will make it do a staircase effect
    if (endVertically)
    {
        if (abs(pstart.y - pend.y) >= abs(pathStack[0].y - pend.y) * 0.5)
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
        if (abs(pstart.x - pend.x) >= abs(pathStack[0].x - pend.x) * 0.5)
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

bool Connection::straightLineIntersectsObject(Line<int> toCheck)
{
    for (const auto& box : cnv->boxes)
    {
        auto bounds = box->getBounds().expanded(3);

        if (auto* graphics = box->graphics.get()) bounds = graphics->getBounds().expanded(3) + box->getPosition();

        if (box == outlet->box || box == inlet->box || !bounds.intersects(getLocalBounds())) continue;

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
    }

    return false;
}
