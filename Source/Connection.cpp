/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Connection.h"

#include "Canvas.h"
#include "Iolet.h"
#include "LookAndFeel.h"

Connection::Connection(Canvas* parent, Iolet* s, Iolet* e, bool exists)
    : cnv(parent)
    , outlet(s->isInlet ? e : s)
    , inlet(s->isInlet ? s : e)
    , outobj(outlet->object)
    , inobj(inlet->object)
{
    locked.referTo(parent->locked);
    presentationMode.referTo(parent->presentationMode);
    presentationMode.addListener(this);

    // Make sure it's not 2x the same iolet
    if (!outlet || !inlet || outlet->isInlet == inlet->isInlet) {
        outlet = nullptr;
        inlet = nullptr;
        jassertfalse;
        return;
    }

    inIdx = inlet->ioletIdx;
    outIdx = outlet->ioletIdx;

    outlet->repaint();
    inlet->repaint();

    // If it doesn't already exist in pd, create connection in pd
    if (!exists) {
        bool canConnect = parent->patch.createConnection(outobj->getPointer(), outIdx, inobj->getPointer(), inIdx);

        if (!canConnect) {
            outlet = nullptr;
            inlet = nullptr;

            MessageManager::callAsync([this]() { cnv->connections.removeObject(this); });

            return;
        }
    } else {
        auto info = cnv->storage.getInfo(getId(), "Path");
        if (!info.isEmpty())
            setState(info);
    }

    // Listen to changes at iolets
    outobj->addComponentListener(this);
    inobj->addComponentListener(this);
    outlet->addComponentListener(this);
    inlet->addComponentListener(this);

    setInterceptsMouseClicks(true, false);

    addMouseListener(cnv, true);

    cnv->addAndMakeVisible(this);
    setAlwaysOnTop(true);

    // Update position (TODO: don't invoke virtual functions from constructor!)
    componentMovedOrResized(*outlet, true, true);
    componentMovedOrResized(*inlet, true, true);

    useStraightConnections.referTo(cnv->pd->settingsTree.getPropertyAsValue("StraightConnections", nullptr));
    useStraightConnections.addListener(this);
    valueChanged(useStraightConnections);

    // Attach useDashedSignalConnection to the DashedSignalConnection property
    useDashedSignalConnection.referTo(cnv->pd->settingsTree.getPropertyAsValue("DashedSignalConnection", nullptr));

    // Listen for signal connection proptery changes
    useDashedSignalConnection.addListener(this);

    // Make sure it gets updated on init
    valueChanged(useDashedSignalConnection);

    updatePath();
    repaint();
}

void Connection::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(useDashedSignalConnection)) {
        useDashed = static_cast<bool>(useDashedSignalConnection.getValue());
    }
    else if (v.refersToSameSourceAs(presentationMode)) {
        setVisible(presentationMode == var(true) ? false : true);
    }
    else if (v.refersToSameSourceAs(useStraightConnections)) {
        useStraight = static_cast<bool>(useStraightConnections.getValue());
        updatePath();
    }
}

String Connection::getState()
{
    String pathAsString;

    MemoryOutputStream stream;

    for (auto& point : currentPlan) {
        stream.writeInt(point.x - outlet->getCanvasBounds().getCentre().x);
        stream.writeInt(point.y - outlet->getCanvasBounds().getCentre().y);
    }

    return stream.getMemoryBlock().toBase64Encoding();
}

void Connection::setState(String const& state)
{
    auto plan = PathPlan();

    auto block = MemoryBlock();
    auto succeeded = block.fromBase64Encoding(state);

    if (succeeded) {
        auto stream = MemoryInputStream(block, false);

        while (!stream.isExhausted()) {
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

    // TODO: check if connection is still valid before requesting idx from object

    stream.writeInt(cnv->patch.getIndex(inobj->getPointer()));
    stream.writeInt(cnv->patch.getIndex(outobj->getPointer()));
    stream.writeInt(inIdx);
    stream.writeInt(outobj->numInputs + outIdx);

    return stream.getMemoryBlock().toBase64Encoding();
}

Connection::~Connection()
{
    if (outlet) {
        outlet->repaint();
        outlet->removeComponentListener(this);
    }
    if (outobj) {
        outobj->removeComponentListener(this);
    }

    if (inlet) {
        inlet->repaint();
        inlet->removeComponentListener(this);
    }
    if (inobj) {
        inobj->removeComponentListener(this);
    }
}

bool Connection::hitTest(int x, int y)
{
    if (locked == var(true) || !cnv->connectingIolets.isEmpty())
        return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));

    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto pstart = getStartPoint().toFloat();
    auto pend = getEndPoint().toFloat();

    if (cnv->isSelected(this) && (startReconnectHandle.contains(position) || endReconnectHandle.contains(position))) {
        repaint();
        return true;
    }

    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position + getPosition().toFloat()) < 8.0f || pend.getDistanceFrom(position + getPosition().toFloat()) < 8.0f)
        return false;

    return nearestPoint.getDistanceFrom(position) < 3;
}

bool Connection::intersects(Rectangle<float> toCheck, int accuracy) const
{
    PathFlatteningIterator i(toDraw);

    while (i.next()) {
        auto point1 = Point<float>(i.x1, i.y1);

        // Skip points to reduce accuracy a bit for better performance
        for (int n = 0; n < accuracy; n++) {
            auto next = i.next();
            if (!next)
                break;
        }

        auto point2 = Point<float>(i.x2, i.y2);
        auto currentLine = Line<float>(point1, point2);

        if (toCheck.intersects(currentLine)) {
            return true;
        }
    }

    return false;
}
void Connection::paint(Graphics& g)
{
    auto baseColour = findColour(PlugDataColour::connectionColourId);
    auto dataColour = findColour(PlugDataColour::dataColourId);
    auto signalColour = findColour(PlugDataColour::signalColourId);
    auto handleColour = outlet->isSignal ? dataColour : signalColour;

    if (cnv->isSelected(this)) {
        baseColour = outlet->isSignal ? signalColour : dataColour;
    } else if (isMouseOver()) {
        baseColour = outlet->isSignal ? signalColour : dataColour;
        baseColour = baseColour.brighter(0.6f);
    }

    // outer stroke
    g.setColour(baseColour.darker(1.0f));
    g.strokePath(toDraw, PathStrokeType(2.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    // inner stroke
    g.setColour(baseColour);
    Path innerPath = toDraw;
    PathStrokeType innerStroke(1.5f);

    if (useDashed && outlet->isSignal) {
        PathStrokeType dashedStroke(0.8f);
        float dash[1] = { 5.0f };
        Path dashedPath;
        dashedStroke.createDashedStroke(dashedPath, toDraw, dash, 1);
        innerPath = dashedPath;
        innerStroke = dashedStroke;
    }
    innerStroke.setEndStyle(PathStrokeType::EndCapStyle::rounded);
    g.strokePath(innerPath, innerStroke);

    if (cnv->isSelected(this)) {
        auto mousePos = getMouseXYRelative();

        bool overStart = startReconnectHandle.contains(mousePos.toFloat());
        bool overEnd = endReconnectHandle.contains(mousePos.toFloat());

        g.setColour(handleColour);

        g.fillEllipse(startReconnectHandle.expanded(overStart ? 3.0f : 0.0f));
        g.fillEllipse(endReconnectHandle.expanded(overEnd ? 3.0f : 0.0f));

        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
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

void Connection::mouseMove(MouseEvent const& e)
{
    int n = getClosestLineIdx(e.getPosition().toFloat() + origin, currentPlan);

    if (isSegmented() && currentPlan.size() > 2 && n > 0) {
        auto line = Line<float>(currentPlan[n - 1], currentPlan[n]);

        if (line.isVertical()) {
            setMouseCursor(MouseCursor::LeftRightResizeCursor);
        } else if (line.isHorizontal()) {
            setMouseCursor(MouseCursor::UpDownResizeCursor);
        } else {
            setMouseCursor(MouseCursor::NormalCursor);
        }
    } else {
        setMouseCursor(MouseCursor::NormalCursor);
    }

    repaint();
}

void Connection::mouseExit(MouseEvent const& e)
{
    repaint();
}

void Connection::mouseDown(MouseEvent const& e)
{
    wasSelected = cnv->isSelected(this);

    // Deselect all other connection if shift or command is not down
    if (!e.mods.isCommandDown() && !e.mods.isShiftDown()) {
        cnv->deselectAll();
    }

    cnv->setSelected(this, true);
    repaint();

    if (currentPlan.size() <= 2)
        return;

    int n = getClosestLineIdx(e.position + origin, currentPlan);
    if (n < 0)
        return;

    if (Line<float>(currentPlan[n - 1], currentPlan[n]).isVertical()) {
        mouseDownPosition = currentPlan[n].x;
    } else {
        mouseDownPosition = currentPlan[n].y;
    }

    dragIdx = n;
}

void Connection::mouseDrag(MouseEvent const& e)
{
    if (wasSelected && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6) {
        reconnect(inlet, true);
    }
    if (wasSelected && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6) {
        reconnect(outlet, true);
    }

    if (currentPlan.empty())
        return;

    if (isSegmented() && dragIdx != -1) {
        auto n = dragIdx;
        auto delta = e.getPosition() - e.getMouseDownPosition();
        auto line = Line<float>(currentPlan[n - 1], currentPlan[n]);

        if (line.isVertical()) {
            currentPlan[n - 1].x = mouseDownPosition + delta.x;
            currentPlan[n].x = mouseDownPosition + delta.x;
        } else {
            currentPlan[n - 1].y = mouseDownPosition + delta.y;
            currentPlan[n].y = mouseDownPosition + delta.y;
        }

        updatePath();
        repaint();
    }
}

void Connection::mouseUp(MouseEvent const& e)
{
    if (dragIdx != -1) {
        auto state = getState();
        lastId = getId();

        cnv->storage.setInfo(lastId, "Path", state);
        dragIdx = -1;
    }

    if (wasSelected && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && startReconnectHandle.contains(e.position)) {
        reconnect(inlet, false);
    }
    if (wasSelected && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && endReconnectHandle.contains(e.position)) {
        reconnect(outlet, false);
    }
    if (reconnecting.size()) {
        // Async to safely self-destruct
        MessageManager::callAsync([canvas = SafePointer(cnv), r = reconnecting]() mutable {
            for (auto& c : r) {
                if (c && canvas) {
                    canvas->connections.removeObject(c.getComponent());
                }
            }
        });

        reconnecting.clear();
    }
}

int Connection::getClosestLineIdx(Point<float> const& position, PathPlan const& plan)
{
    if (plan.size() < 2)
        return -1;

    for (int n = 2; n < plan.size() - 1; n++) {
        auto line = Line<float>(plan[n - 1], plan[n]);
        Point<float> nearest;
        if (line.getDistanceFromPoint(position - offset, nearest) < 3) {
            return n;
        }
    }

    return -1;
}

void Connection::reconnect(Iolet* target, bool dragged)
{
    if (!reconnecting.isEmpty())
        return;

    auto& otherEdge = target == inlet ? outlet : inlet;

    Array<Connection*> connections = { this };

    if (Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
        for (auto* c : otherEdge->object->getConnections()) {
            if (c == this || !cnv->isSelected(c))
                continue;

            connections.add(c);
        }
    }

    for (auto* c : connections) {

        if (cnv->patch.hasConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx)) {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx);
        }

        // Create new connection
        cnv->connectingIolets.add(target->isInlet ? c->inlet : c->outlet);

        cnv->connectingWithDrag = true;
        c->setVisible(false);

        reconnecting.add(SafePointer(c));

        // Make sure we're deselected and remove object
        cnv->setSelected(c, false);
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    if (!inlet || !outlet)
        return;

    auto pstart = getStartPoint();
    auto pend = getEndPoint();

    if (currentPlan.size() <= 2 || cnv->storage.getInfo(getId(), "Style") == "0") {
        updatePath();
        return;
    }

    // If both inlet and outlet are selected we can just move the connection cord
    if ((cnv->isSelected(outobj) && cnv->isSelected(inobj)) || cnv->updatingBounds) {
        auto offset = pstart - currentPlan[0];
        for (auto& point : currentPlan)
            point += offset;
        updatePath();
        return;
    }

    int idx1 = 0;
    int idx2 = 1;

    auto& position = pstart;
    if (&component == inlet || &component == inobj) {
        idx1 = static_cast<int>(currentPlan.size() - 1);
        idx2 = static_cast<int>(currentPlan.size() - 2);
        position = pend;
    }

    if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
        currentPlan[idx2].x = position.x;
    } else {
        currentPlan[idx2].y = position.y;
    }

    currentPlan[idx1] = position;
    updatePath();
}

Point<float> Connection::getStartPoint()
{
    return Point<float>(outlet->getCanvasBounds().toFloat().getCentreX(), outlet->getCanvasBounds().toFloat().getCentreY() + 0.5f);
}

Point<float> Connection::getEndPoint()
{
    return Point<float>(inlet->getCanvasBounds().toFloat().getCentreX(), inlet->getCanvasBounds().toFloat().getCentreY() - 1.0f);
}

void Connection::updatePath()
{
    if (!outlet || !inlet)
        return;

    float left = std::min(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) - 4;
    float top = std::min(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) - 4;
    float right = std::max(outlet->getCanvasBounds().getCentreX(), inlet->getCanvasBounds().getCentreX()) + 4;
    float bottom = std::max(outlet->getCanvasBounds().getCentreY(), inlet->getCanvasBounds().getCentreY()) + 4;

    origin = Rectangle<float>(left, top, right - left, bottom - top).getPosition();

    auto pstart = getStartPoint() - origin;
    auto pend = getEndPoint() - origin;

    if (lastId.isEmpty())
        lastId = getId();

    segmented = cnv->storage.getInfo(lastId, "Segmented") == "1";

    if (!segmented) {
        toDraw.clear();
        toDraw.startNewSubPath(pstart);
        if (!useStraight) {
            float const width = std::max(pstart.x, pend.x) - std::min(pstart.x, pend.x);
            float const height = std::max(pstart.y, pend.y) - std::min(pstart.y, pend.y);

            float const min = std::min<float>(width, height);
            float const max = std::max<float>(width, height);

            float const maxShiftY = 20.f;
            float const maxShiftX = 20.f;

            float const shiftY = std::min<float>(maxShiftY, max * 0.5);

            float const shiftX = ((pstart.y >= pend.y) ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * ((pstart.x < pend.x) ? -1. : 1.);

            Point<float> const ctrlPoint1 { pstart.x - shiftX, pstart.y + shiftY };
            Point<float> const ctrlPoint2 { pend.x + shiftX, pend.y - shiftY };

            toDraw.cubicTo(ctrlPoint1, ctrlPoint2, pend);
        } else {
            toDraw.lineTo(pend);
        }
        currentPlan.clear();
    } else {
        if (currentPlan.empty()) {
            findPath();
        }

        auto snap = [this](Point<float> point, int idx1, int idx2) {
            if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
                currentPlan[idx2].x = point.x + origin.x;
            } else {
                currentPlan[idx2].y = point.y + origin.y;
            }

            currentPlan[idx1] = point + origin;
        };

        snap(pstart, 0, 1);
        snap(pend, static_cast<int>(currentPlan.size() - 1), static_cast<int>(currentPlan.size() - 2));

        Path connectionPath;
        connectionPath.startNewSubPath(pstart.toFloat());

        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++) {
            if (connectionPath.contains(currentPlan[n].toFloat()))
                continue; // ??

            connectionPath.lineTo(currentPlan[n].toFloat() - origin.toFloat());
        }

        connectionPath.lineTo(pend.toFloat());
        toDraw = connectionPath.createPathWithRoundedCorners(8.0f);
    }

    repaint();

    auto bounds = toDraw.getBounds().expanded(8);
    setBounds((bounds + origin).getSmallestIntegerContainer());

    if (bounds.getX() < 0 || bounds.getY() < 0) {
        toDraw.applyTransform(AffineTransform::translation(-bounds.getX() + 0.5f, -bounds.getY()));
    }

    offset = { -bounds.getX(), -bounds.getY() };

    startReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(8.5f));
    endReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(std::max(toDraw.getLength() - 8.5f, 9.5f)));
}

void Connection::findPath()
{
    if (!outlet || !inlet)
        return;

    auto pstart = getStartPoint();
    auto pend = getEndPoint();

    auto pathStack = PathPlan();
    auto bestPath = PathPlan();

    pathStack.reserve(8);

    auto numFound = 0;

    float incrementX, incrementY;

    auto distance = pstart.getDistanceFrom(pend);
    auto distanceX = std::abs(pstart.x - pend.x);
    auto distanceY = std::abs(pstart.y - pend.y);

    int maxXResolution = std::clamp<int>(distanceX / 10, 6, 14);
    int maxYResolution = std::clamp<int>(distanceY / 10, 6, 14);

    int resolutionX = 6;
    int resolutionY = 6;

    auto obstacles = Array<Rectangle<float>>();
    auto searchBounds = Rectangle<float>(pstart, pend);

    for (auto* object : cnv->objects) {
        if (object->getBounds().toFloat().intersects(searchBounds)) {
            obstacles.add(object->getBounds().toFloat());
        }
    }

    // Look for paths at an increasing resolution
    while (!numFound && resolutionX < maxXResolution && distance > 40) {

        // Find paths on a resolution*resolution lattice ObjectGrid
        incrementX = std::max<float>(1, distanceX / resolutionX);
        incrementY = std::max<float>(1, distanceY / resolutionY);

        numFound = findLatticePaths(bestPath, pathStack, pend, pstart, { incrementX, incrementY });

        if (resolutionX < maxXResolution)
            resolutionX++;
        if (resolutionY < maxXResolution)
            resolutionY++;

        if (resolutionX > maxXResolution || resolutionY > maxYResolution)
            break;

        pathStack.clear();
    }

    PathPlan simplifiedPath;

    bool direction;
    if (!bestPath.empty()) {
        simplifiedPath.push_back(bestPath.front());

        direction = bestPath[0].x == bestPath[1].x;

        if (!direction)
            simplifiedPath.push_back(bestPath.front());

        for (int n = 1; n < bestPath.size(); n++) {
            if ((bestPath[n].x != bestPath[n - 1].x && direction) || (bestPath[n].y != bestPath[n - 1].y && !direction)) {
                simplifiedPath.push_back(bestPath[n - 1]);
                direction = !direction;
            }
        }

        simplifiedPath.push_back(bestPath.back());

        if (!direction)
            simplifiedPath.push_back(bestPath.back());
    } else {
        if (pend.y < pstart.y) {
            int xHalfDistance = (pstart.x - pend.x) / 2;

            simplifiedPath.push_back(pend); // double to make it draggable
            simplifiedPath.push_back(pend);
            simplifiedPath.push_back({ pend.x + xHalfDistance, pend.y });
            simplifiedPath.push_back({ pend.x + xHalfDistance, pstart.y });
            simplifiedPath.push_back(pstart);
            simplifiedPath.push_back(pstart);
        } else {
            int yHalfDistance = (pstart.y - pend.y) / 2;
            simplifiedPath.push_back(pend);
            simplifiedPath.push_back({ pend.x, pend.y + yHalfDistance });
            simplifiedPath.push_back({ pstart.x, pend.y + yHalfDistance });
            simplifiedPath.push_back(pstart);
        }
    }
    std::reverse(simplifiedPath.begin(), simplifiedPath.end());

    currentPlan = simplifiedPath;

    auto state = getState();
    lastId = getId();
    cnv->storage.setInfo(lastId, "Path", state);
}

int Connection::findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<float> pstart, Point<float> pend, Point<float> increment)
{

    auto obstacles = Array<Object*>();
    auto searchBounds = Rectangle<float>(pstart, pend);

    for (auto* object : cnv->objects) {
        if (object->getBounds().toFloat().intersects(searchBounds)) {
            obstacles.add(object);
        }
    }

    // Stop after we've found a path
    if (!bestPath.empty())
        return 0;

    // Add point to path
    pathStack.push_back(pstart);

    // Check if it intersects any object
    if (pathStack.size() > 1 && straightLineIntersectsObject(Line<float>(pathStack.back(), *(pathStack.end() - 2)), obstacles)) {
        return 0;
    }

    bool endVertically = pathStack[0].y > pend.y;

    // Check if we've reached the destination
    if (std::abs(pstart.x - pend.x) < (increment.x * 0.5) && std::abs(pstart.y - pend.y) < (increment.y * 0.5)) {
        bestPath = pathStack;
        return 1;
    }

    // Count the number of found paths
    int count = 0;

    // Get current stack to revert to after each trial
    auto pathCopy = pathStack;

    auto followLine = [this, &count, &pathCopy, &bestPath, &pathStack, &increment](Point<float> currentOutlet, Point<float> currentInlet, bool isX) {
        auto& coord1 = isX ? currentOutlet.x : currentOutlet.y;
        auto& coord2 = isX ? currentInlet.x : currentInlet.y;
        auto& incr = isX ? increment.x : increment.y;

        if (std::abs(coord1 - coord2) >= incr) {
            coord1 > coord2 ? coord1 -= incr : coord1 += incr;
            count += findLatticePaths(bestPath, pathStack, currentOutlet, currentInlet, increment);
            pathStack = pathCopy;
        }
    };

    // If we're halfway on the axis, change preferred direction by inverting search order
    // This will make it do a staircase effect
    if (endVertically) {
        if (std::abs(pstart.y - pend.y) >= std::abs(pathStack[0].y - pend.y) * 0.5) {
            followLine(pstart, pend, false);
            followLine(pstart, pend, true);
        } else {
            followLine(pstart, pend, true);
            followLine(pstart, pend, false);
        }
    } else {
        if (std::abs(pstart.x - pend.x) >= std::abs(pathStack[0].x - pend.x) * 0.5) {
            followLine(pstart, pend, true);
            followLine(pstart, pend, false);
        } else {
            followLine(pstart, pend, false);
            followLine(pstart, pend, true);
        }
    }

    return count;
}

bool Connection::intersectsObject(Object* object)
{
    auto b = (object->getBounds() - getPosition()).toFloat();
    return toDraw.intersectsLine({ b.getTopLeft(), b.getTopRight() })
        || toDraw.intersectsLine({ b.getTopLeft(), b.getBottomLeft() })
        || toDraw.intersectsLine({ b.getBottomRight(), b.getBottomLeft() })
        || toDraw.intersectsLine({ b.getBottomRight(), b.getTopRight() });
}

bool Connection::straightLineIntersectsObject(Line<float> toCheck, Array<Object*>& objects)
{

    for (auto const& object : objects) {
        auto bounds = object->getBounds().expanded(1);

        if (object == outobj || object == inobj || !bounds.intersects(getBounds()))
            continue;

        auto intersectV = [](Line<float> first, Line<float> second) {
            if (first.getStartY() > first.getEndY()) {
                first = { first.getEnd(), first.getStart() };
            }

            return first.getStartX() > second.getStartX() && first.getStartX() < second.getEndX() && second.getStartY() > first.getStartY() && second.getStartY() < first.getEndY();
        };

        auto intersectH = [](Line<float> first, Line<float> second) {
            if (first.getStartX() > first.getEndX()) {
                first = { first.getEnd(), first.getStart() };
            }

            return first.getStartY() > second.getStartY() && first.getStartY() < second.getEndY() && second.getStartX() > first.getStartX() && second.getStartX() < first.getEndX();
        };

        bool intersectsV = toCheck.isVertical() && (intersectV(toCheck, Line<float>(bounds.getTopLeft().toFloat(), bounds.getTopRight().toFloat())) || intersectV(toCheck, Line<float>(bounds.getBottomRight().toFloat(), bounds.getBottomLeft().toFloat())));

        bool intersectsH = toCheck.isHorizontal() && (intersectH(toCheck, Line<float>(bounds.getTopRight().toFloat(), bounds.getBottomRight().toFloat())) || intersectH(toCheck, Line<float>(bounds.getTopLeft().toFloat(), bounds.getBottomLeft().toFloat())));
        if (intersectsV || intersectsH) {
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
