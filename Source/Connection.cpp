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

            jassertfalse;

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
        pathAsString += String(point.x) + "*" + String(point.y) + ",";
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

        auto x = point.upToFirstOccurrenceOf("*", false, false).getIntValue();
        auto y = point.fromFirstOccurrenceOf("*", false, false).getIntValue();

        plan.emplace_back(x, y);
    }

    applyPath(plan, false);
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
    
    auto& animator = Desktop::getInstance().getAnimator();
    animator.fadeOut(this, 150);
}

bool Connection::hitTest(int x, int y)
{
    if (locked == true) return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));

    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto pstart = outlet->getCanvasBounds().getCentre().toFloat() - origin.toFloat();
    auto pend = inlet->getCanvasBounds().getCentre().toFloat() - origin.toFloat();

    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position) < 10.0f) return false;
    if (pend.getDistanceFrom(position) < 10.0f) return false;

    if (nearestPoint.getDistanceFrom(position) < 5)
    {
        return true;
    }

    return false;
}

void Connection::paint(Graphics& g)
{
    // g.getInternalContext().clipToPath(toDraw, {});
    // g.reduceClipRegion(toDraw);

    g.setColour(Colours::grey);
    g.strokePath(toDraw, PathStrokeType(3.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    auto baseColour = Colours::white;

    if (cnv->isSelected(this))
    {
        baseColour = outlet->isSignal ? Colours::yellow : findColour(Slider::thumbColourId);
    }

    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(toDraw, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void Connection::mouseMove(const MouseEvent& e)
{
    auto scaledPlan = scalePath(currentPlan);

    bool segmentedConnection = connectionStyle == true;
    int n = getClosestLineIdx(e.getPosition(), scaledPlan);

    if (segmentedConnection && scaledPlan.size() > 2 && n >= 0)
    {
        auto line = Line<int>(scaledPlan[n - 1], scaledPlan[n]);
        setMouseCursor(line.isVertical() ? MouseCursor::LeftRightResizeCursor : MouseCursor::UpDownResizeCursor);
    }
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

    if (currentPlan.empty()) return;

    const auto scaledPlan = scalePath(currentPlan);

    if (scaledPlan.size() <= 2) return;

    int n = getClosestLineIdx(e.getPosition(), scaledPlan);
    if (n < 0) return;

    if (Line<int>(scaledPlan[n - 1], scaledPlan[n]).isVertical())
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

    auto planDistance = currentPlan.front() - currentPlan.back();
    auto currentDistance = pstart - pend;

    bool flippedX = planDistance.x * currentDistance.x < 0;
    bool flippedY = planDistance.y * currentDistance.y < 0;

    bool curvedConnection = cnv->pd->settingsTree.getProperty("ConnectionStyle");
    if (curvedConnection && dragIdx != -1)
    {
        auto n = dragIdx;
        auto delta = e.getPosition() - e.getMouseDownPosition();
        auto line = Line<int>(currentPlan[n - 1], currentPlan[n]);

        auto scaleX = static_cast<float>(abs(pstart.x - pend.x)) / static_cast<float>(abs(currentPlan.front().x - currentPlan.back().x));
        auto scaleY = static_cast<float>(abs(pstart.y - pend.y)) / static_cast<float>(abs(currentPlan.front().y - currentPlan.back().y));

        if (flippedX) scaleX *= -1.0f;
        if (flippedY) scaleY *= -1.0f;

        if (line.isVertical())
        {
            currentPlan[n - 1].x = mouseDownPosition + delta.x / scaleX;
            currentPlan[n].x = mouseDownPosition + delta.x / scaleX;
        }
        else
        {
            currentPlan[n - 1].y = mouseDownPosition + delta.y / scaleY;
            currentPlan[n].y = mouseDownPosition + delta.y / scaleY;
        }

        updatePath();
        repaint();
    }
}

int Connection::getClosestLineIdx(const Point<int>& position, const PathPlan& plan)
{
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
    int left = std::min(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX());
    int top = std::min(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY());
    int right = std::max(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX());
    int bottom = std::max(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY());

    // Leave some extra room
    // setBounds(left, top, right - left, bottom - top);

    setBounds(toDraw.getBounds().toNearestInt().getUnion(Rectangle<int>(left, top, right - left, bottom - top)));
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

    bool segmentedConnection = connectionStyle == true;

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

        const float shiftX = ((pend.y >= pstart.y) ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * ((pend.x < pstart.x) ? -1. : 1.);

        const Point<float> ctrlPoint1{pend.x - shiftX, pend.y + shiftY};
        const Point<float> ctrlPoint2{pstart.x + shiftX, pstart.y - shiftY};

        toDraw.startNewSubPath(pend.toFloat());
        toDraw.cubicTo(ctrlPoint1, ctrlPoint2, pstart.toFloat());
    }
    else
    {
        if (currentPlan.empty())
        {
            currentPlan = findPath();
        }

        auto plan = scalePath(currentPlan);

        Path connectionPath;
        connectionPath.startNewSubPath(pstart.toFloat());

        // Add points in between if we've found a path
        for (int n = 1; n < plan.size() - 1; n++)
        {
            if (connectionPath.contains(plan[n].toFloat())) continue;
            connectionPath.lineTo(plan[n].toFloat());
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

PathPlan Connection::scalePath(const PathPlan& plan)
{
    if (!outlet || !inlet || plan.empty()) return plan;

    auto pstart = outlet->getCanvasBounds().getCentre() - origin;
    auto pend = inlet->getCanvasBounds().getCentre() - origin;

    auto newPlan = plan;

    auto rect = Rectangle<int>(pstart, pend).expanded(4);

    auto lastWidth = std::max(abs(plan.front().x - plan.back().x), 1);
    auto lastHeight = std::max(abs(plan.front().y - plan.back().y), 1);

    float scaleX = static_cast<float>(abs(pstart.x - pend.x)) / lastWidth;
    float scaleY = static_cast<float>(abs(pstart.y - pend.y)) / lastHeight;

    auto planDistance = plan.front() - plan.back();
    auto currentDistance = pstart - pend;

    bool flippedX = planDistance.x * currentDistance.x < 0;
    bool flippedY = planDistance.y * currentDistance.y < 0;

    for (auto& point : newPlan)
    {
        // Don't scale if it's outside of the rectangle between the inlet and outlet
        // Doing so will cause the scaling direction to invert and look bad

        auto insideRect = rect.contains(point);

        if (flippedX)
        {
            point.x = lastWidth - point.x;
        }
        if (flippedY)
        {
            point.y = lastHeight - point.y;
        }

        point.x *= scaleX;
        point.y *= scaleY;
    }

    // Align with inlets and outlets
    if (Line<int>(newPlan[0], newPlan[1]).isVertical())
    {
        newPlan[0].x = pstart.x;
        newPlan[1].x = pstart.x;
    }
    else
    {
        newPlan[0].y = pstart.y;
        newPlan[1].y = pstart.y;
        newPlan[2].y = pstart.y;
    }
    if (Line<int>(newPlan[newPlan.size() - 2], newPlan[newPlan.size() - 1]).isVertical())
    {
        newPlan[newPlan.size() - 2].x = pend.x;
        newPlan[newPlan.size() - 1].x = pend.x;
    }
    else
    {
        newPlan[newPlan.size() - 3].y = pend.y;
        newPlan[newPlan.size() - 2].y = pend.y;
        newPlan[newPlan.size() - 1].y = pend.y;
    }

    return newPlan;
}

void Connection::applyPath(const PathPlan& plan, bool updateState)
{
    currentPlan = plan;

    if (updateState)
    {
        auto state = getState();
        lastId = getId();
        cnv->patch.setExtraInfo(lastId, state);
    }

    updatePath();
}

PathPlan Connection::findPath()
{
    if (!outlet || !inlet) return {};

    auto pstart = inlet->getCanvasBounds().getCentre() - origin;
    auto pend = outlet->getCanvasBounds().getCentre() - origin;

    auto pathStack = PathPlan();
    auto bestPath = PathPlan();

    pathStack.reserve(8);

    auto numFound = 0;
    auto resolution = 3;

    int incrementX, incrementY;

    auto distance = pend.getDistanceFrom(pstart);

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

    return simplifiedPath;
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
        auto bounds = box->getBounds().expanded(3) - origin;

        if (auto* graphics = box->graphics.get()) bounds = graphics->getBounds().expanded(3) + box->getPosition() - origin;

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
