/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Connection.h"

#include <set>

#include "Box.h"
#include "Canvas.h"
#include "Edge.h"

Connection::Connection(Canvas* parent, Edge* s, Edge* e, bool exists) : start(s), end(e)
{
    // Should improve performance
    setBufferedToImage(true);

    cnv = parent;

    // Receive mouse events on canvas
    addMouseListener(cnv, true);

    // Make sure it's not 2x the same edge
    if (!start || !end || start->isInput == end->isInput)
    {
        start = nullptr;
        end = nullptr;
        jassertfalse;
        return;
    }

    // check which is the input
    if (start->isInput)
    {
        inIdx = start->edgeIdx;
        outIdx = end->edgeIdx;
        inObj = &start->box->pdObject;
        outObj = &end->box->pdObject;
    }
    else
    {
        inIdx = end->edgeIdx;
        outIdx = start->edgeIdx;
        inObj = &end->box->pdObject;
        outObj = &start->box->pdObject;
    }

    start->repaint();
    end->repaint();

    // If it doesn't already exist in pd, create connection in pd
    if (!exists)
    {
        bool canConnect = parent->patch.createConnection(outObj->get(), outIdx, inObj->get(), inIdx);

        if (!canConnect)
        {
            start = nullptr;
            end = nullptr;

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
    start->box->addComponentListener(this);
    end->box->addComponentListener(this);
    start->addComponentListener(this);
    end->addComponentListener(this);

    // Don't need mouse clicks
    setInterceptsMouseClicks(true, false);

    cnv->addAndMakeVisible(this);

    setAlwaysOnTop(true);

    // Update position
    componentMovedOrResized(*start, true, true);
    componentMovedOrResized(*end, true, true);
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
    int idx1 = cnv->patch.getIndex(inObj->get()->getPointer());
    int idx2 = cnv->patch.getIndex(outObj->get()->getPointer());
    return "c" + String(idx1) + String(idx2) + String(inIdx) + String(outIdx);
}

Connection::~Connection()
{
    if (start && start->box)
    {
        start->repaint();
        start->box->removeComponentListener(this);
        start->removeComponentListener(this);
    }
    if (end && end->box)
    {
        end->repaint();
        end->box->removeComponentListener(this);
        end->removeComponentListener(this);
    }
}

bool Connection::hitTest(int x, int y)
{
    if (start->box->locked || end->box->locked) return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));

    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);

    // Get start and end point
    Point<float> pstart = start->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    Point<float> pend = end->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();

    // If we click too close to the end, don't register the click on the connection
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
    g.setColour(Colours::grey);
    g.strokePath(toDraw, PathStrokeType(3.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    auto baseColour = Colours::white;

    if (isSelected)
    {
        baseColour = start->isSignal ? Colours::yellow : findColour(Slider::thumbColourId);
    }

    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(toDraw, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void Connection::mouseMove(const MouseEvent& e)
{
    auto scaledPlan = scalePath(currentPlan);

    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);
    if (curvedConnection && scaledPlan.size() > 2)
    {
        for (int n = 2; n < scaledPlan.size() - 1; n++)
        {
            auto line = Line<int>(scaledPlan[n - 1], scaledPlan[n]);
            Point<int> nearest;
            if (line.getDistanceFromPoint(e.getPosition(), nearest) < 3)
            {
                setMouseCursor(line.isVertical() ? MouseCursor::LeftRightResizeCursor : MouseCursor::UpDownResizeCursor);
                return;
            }
            else
            {
                setMouseCursor(MouseCursor::NormalCursor);
            }
        }
    }
}

void Connection::mouseDown(const MouseEvent& e)
{
    if (currentPlan.empty()) return;

    const auto scaledPlan = scalePath(currentPlan);

    if (scaledPlan.size() <= 2) return;

    for (int n = 1; n < scaledPlan.size(); n++)
    {
        auto line = Line<int>(scaledPlan[n - 1], scaledPlan[n]);
        Point<int> nearest;

        if (line.getDistanceFromPoint(e.getPosition(), nearest) < 5)
        {
            if (line.isVertical())
            {
                mouseDownPosition = currentPlan[n].x;
            }
            else
            {
                mouseDownPosition = currentPlan[n].y;
            }

            dragIdx = n;

            break;
        }
    }

    // Deselect all other connection if shift or command is not down
    if (!ModifierKeys::getCurrentModifiers().isCommandDown() && !ModifierKeys::getCurrentModifiers().isShiftDown())
    {
        cnv->deselectAll();
    }

    isSelected = true;

    repaint();
}

void Connection::mouseDrag(const MouseEvent& e)
{
    if (currentPlan.empty()) return;

    auto& first = start->isInput ? start : end;
    auto& last = start->isInput ? end : start;

    Point<int> pstart = first->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = last->getCanvasBounds().getCentre() - getPosition();

    auto planDistance = currentPlan.front() - currentPlan.back();
    auto currentDistance = pstart - pend;

    float lastWidth = std::max<float>(abs(currentPlan.front().x - currentPlan.back().x), 1.0f);
    float lastHeight = std::max<float>(abs(currentPlan.front().y - currentPlan.back().y), 1.0f);

    bool flippedX = planDistance.x * currentDistance.x < 0;
    bool flippedY = planDistance.y * currentDistance.y < 0;

    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);
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
            currentPlan[n - 1].x = std::clamp<int>(mouseDownPosition + delta.x / scaleX, 0, getWidth() / fabs(scaleX));
            currentPlan[n].x = std::clamp<int>(mouseDownPosition + delta.x / scaleX, 0, getWidth() / fabs(scaleX));
        }
        else
        {
            currentPlan[n - 1].y = std::clamp<int>(mouseDownPosition + delta.y / scaleY, 0, getHeight() / fabs(scaleY));
            currentPlan[n].y = std::clamp<int>(mouseDownPosition + delta.y / scaleY, 0, getHeight() / fabs(scaleY));
        }

        resized();
        repaint();
    }
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
    int left = std::min(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) - 4;
    int top = std::min(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) - 4;
    int right = std::max(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) + 4;
    int bottom = std::max(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) + 4;

    // Leave some extra room
    setBounds(left, top, right - left, bottom - top);
}

void Connection::resized()
{
    if (!start || !end) return;

    auto& s = start->isInput ? start : end;
    auto& e = start->isInput ? end : start;

    Point<int> pstart = s->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = e->getCanvasBounds().getCentre() - getPosition();

    bool curvedConnection = cnv->pd->settingsTree.getProperty(Identifiers::connectionStyle);

    if (!curvedConnection)
    {
        toDraw.clear();
        toDraw.startNewSubPath(pstart.toFloat());
        toDraw.lineTo(pend.toFloat());
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

        // Add points inbetween if we've found a path
        for (int n = 1; n < plan.size() - 1; n++)
        {
            if (connectionPath.contains(plan[n].toFloat())) continue;
            plan[n].x = std::clamp(plan[n].x, 2, getWidth() - 2);
            plan[n].y = std::clamp(plan[n].y, 2, getHeight() - 2);
            connectionPath.lineTo(plan[n].toFloat());
        }
        connectionPath.lineTo(pend.toFloat());

        toDraw = connectionPath.createPathWithRoundedCorners(8.0f);
        repaint();
    }
}

PathPlan Connection::scalePath(const PathPlan& plan)
{
    if (!start || !end || plan.empty()) return plan;

    auto& s = start->isInput ? start : end;
    auto& e = start->isInput ? end : start;

    Point<int> pstart = s->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = e->getCanvasBounds().getCentre() - getPosition();

    auto newPlan = plan;

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

    resized();
}

PathPlan Connection::findPath()
{
    if (!start || !end) return {};

    auto& s = start->isInput ? start : end;
    auto& e = start->isInput ? end : start;

    Point<int> pstart = s->getCanvasBounds().getCentre() - getPosition();
    Point<int> pend = e->getCanvasBounds().getCentre() - getPosition();

    PathPlan pathStack;
    PathPlan bestPath;

    pathStack.reserve(8);

    int numFound = 0;
    int resolution = 3;

    int incrementX, incrementY;

    int distance = pend.getDistanceFrom(pstart);

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

    auto followLine = [this, &count, &pathCopy, &bestPath, &pathStack, &increment](Point<int> start, Point<int> end, bool isX)
    {
        auto& coord1 = isX ? start.x : start.y;
        auto& coord2 = isX ? end.x : end.y;
        auto& incr = isX ? increment.x : increment.y;

        if (abs(coord1 - coord2) >= incr)
        {
            coord1 > coord2 ? coord1 -= incr : coord1 += incr;
            count += findLatticePaths(bestPath, pathStack, start, end, increment);
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
        auto bounds = box->getBounds().expanded(3) - getPosition();

        if (auto* graphics = box->graphics.get()) bounds = graphics->getBounds().expanded(3) + box->getPosition() - getPosition();

        if (box == start->box || box == end->box || !bounds.intersects(getLocalBounds())) continue;

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
