/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Connection.h"

#include "Canvas.h"
#include "Iolet.h"
#include "Object.h"
#include "PluginProcessor.h"
#include "PluginEditor.h" // might not need this?
#include "LookAndFeel.h"
#include "Pd/Patch.h"

Connection::Connection(Canvas* parent, Iolet* s, Iolet* e, void* oc)
    : cnv(parent)
    , outlet(s->isInlet ? e : s)
    , inlet(s->isInlet ? s : e)
    , outobj(outlet->object)
    , inobj(inlet->object)
    , ptr(static_cast<t_fake_outconnect*>(oc))
{
    cnv->selectedComponents.addChangeListener(this);

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
    if (!oc) {
        auto* oc = parent->patch.createAndReturnConnection(outobj->getPointer(), outIdx, inobj->getPointer(), inIdx);

        ptr = static_cast<t_fake_outconnect*>(oc);

        if (!ptr) {
            outlet = nullptr;
            inlet = nullptr;

            // MessageManager::callAsync([this]() { cnv->connections.removeObject(this); });

            return;
        }
    } else {

        popPathState();
    }

    // Listen to changes at iolets
    outobj->addComponentListener(this);
    inobj->addComponentListener(this);
    outlet->addComponentListener(this);
    inlet->addComponentListener(this);

    setInterceptsMouseClicks(true, true);

    addMouseListener(cnv, true);

    cnv->addAndMakeVisible(this);
    setAlwaysOnTop(true);

    // Update position (TODO: don't invoke virtual functions from constructor!)
    componentMovedOrResized(*outlet, true, true);
    componentMovedOrResized(*inlet, true, true);

    valueChanged(presentationMode);

    //updatePath();
    //repaint();

    updateOverlays(cnv->getOverlays());

    cnv->pd->registerMessageListener(ptr, this);
}

Connection::~Connection()
{
    cnv->pd->unregisterMessageListener(ptr, this);
    cnv->selectedComponents.removeChangeListener(this);

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

void Connection::changeListenerCallback(ChangeBroadcaster* source)
{
    if (auto selectedItems = dynamic_cast<SelectedItemSet<WeakReference<Component>>*>(source))
        setSelected(selectedItems->isSelected(this));
}

void Connection::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(presentationMode)) {
        setVisible(presentationMode != var(true) && !cnv->isGraph);
    }
}

void Connection::lookAndFeelChanged()
{
    updatePath();
}

void Connection::pushPathState()
{
    t_symbol* newPathState;
    if (segmented) {
        MemoryOutputStream stream;

        for (auto& point : currentPlan) {
            stream.writeInt(point.x);
            stream.writeInt(point.y);
        }
        auto base64 = stream.getMemoryBlock().toBase64Encoding();
        newPathState = cnv->pd->generateSymbol(base64);
    } else {
        newPathState = cnv->pd->generateSymbol("empty");
    }

    cnv->pathUpdater->pushPathState(this, newPathState);
}

void Connection::popPathState()
{
    if (!ptr || !ptr->outconnect_path_data || !ptr->outconnect_path_data->s_name)
        return;

    auto const state = String::fromUTF8(ptr->outconnect_path_data->s_name);

    if (state == "empty") {
        segmented = false;
        updatePath();
        return;
    }

    auto block = MemoryBlock();
    auto succeeded = block.fromBase64Encoding(state);

    auto plan = PathPlan();

    if (succeeded) {
        auto stream = MemoryInputStream(block, false);

        while (!stream.isExhausted()) {
            auto x = stream.readInt();
            auto y = stream.readInt();

            plan.emplace_back(x, y);
        }
        segmented = !plan.empty();
    } else {
        segmented = false;
    }

    currentPlan = plan;
    updatePath();
}

void Connection::setPointer(void* newPtr)
{
    ptr = static_cast<t_fake_outconnect*>(newPtr);
}

void* Connection::getPointer()
{
    return ptr;
}

t_symbol* Connection::getPathState()
{
    return ptr->outconnect_path_data;
}

bool Connection::hitTest(int x, int y)
{
    if (inlet == nullptr || outlet == nullptr)
        return false;

    if (Canvas::panningModifierDown())
        return false;

    if (locked == var(true) || !cnv->connectionsBeingCreated.isEmpty())
        return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y));

    Point<float> nearestPoint;
    toDraw.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto pstart = getStartPoint().toFloat();
    auto pend = getEndPoint().toFloat();

    if (selectedFlag && (startReconnectHandle.contains(position) || endReconnectHandle.contains(position))) {
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

void Connection::renderConnectionPath(Graphics& g,
    Canvas* cnv,
    Path connectionPath,
    bool isSignal,
    bool isMouseOver,
    bool showDirection,
    bool showConnectionOrder,
    bool isSelected,
    Point<int> mousePos,
    bool isHovering,
    int connectionCount,
    int multiConnectNumber)
{
    auto baseColour = cnv->findColour(PlugDataColour::connectionColourId);
    auto dataColour = cnv->findColour(PlugDataColour::dataColourId);
    auto signalColour = cnv->findColour(PlugDataColour::signalColourId);
    auto handleColour = isSignal ? dataColour : signalColour;

    auto connectionLength = connectionPath.getLength();

    if (isSelected) {
        baseColour = isSignal ? signalColour : dataColour;
    } else if (isMouseOver) {
        baseColour = isSignal ? signalColour : dataColour;
        baseColour = baseColour.brighter(0.6f);
    }

    bool useThinConnection = PlugDataLook::getUseThinConnections();

    // outer stroke
    g.setColour(baseColour.darker(1.0f));
    g.strokePath(connectionPath, PathStrokeType(useThinConnection ? 1.0f : 2.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    // inner stroke
    g.setColour(baseColour);
    Path innerPath = connectionPath;
    PathStrokeType innerStroke(useThinConnection ? 1.0f : 1.5f);

    if (PlugDataLook::getUseDashedConnections() && isSignal) {
        PathStrokeType dashedStroke(useThinConnection ? 0.5f : 0.8f);
        float dash[1] = { 5.0f };
        Path dashedPath;
        dashedStroke.createDashedStroke(dashedPath, connectionPath, dash, 1);
        innerPath = dashedPath;
        innerStroke = dashedStroke;
    }
    innerStroke.setEndStyle(PathStrokeType::EndCapStyle::rounded);
    g.strokePath(innerPath, innerStroke);

    // draw direction arrow if button is toggled (per canvas, default state is false)
    //            c
    //            |\
    //            | \
    //            |  \
    //  ___path___|   \a___path___
    //            |   /
    //            |  /
    //            | /
    //            |/
    //            b

    // setup arrow parameters
    float arrowWidth = 8.0f;
    float arrowLength = 12.0f;

    if (showDirection && connectionLength > arrowLength * 2) {
        // get the center point of the connection path
        auto arrowCenter = connectionLength * 0.5f;
        auto arrowBase = connectionPath.getPointAlongPath(arrowCenter - (arrowLength * 0.5f));
        auto arrowTip = connectionPath.getPointAlongPath(arrowCenter + (arrowLength * 0.5f));

        Line<float> arrowLine(arrowBase, arrowTip);
        auto point_a = arrowTip;
        auto point_b = arrowLine.getPointAlongLine(0.0f, -(arrowWidth * 0.5f));
        auto point_c = arrowLine.getPointAlongLine(0.0f, (arrowWidth * 0.5f));

        // create the arrow path
        Path arrow;
        arrow.addTriangle(point_a, point_b, point_c);

        // draw the arrow
        g.setColour(baseColour);
        g.fillPath(arrow);

        // draw arrow outline to aid in visibility for dark / light themes
        g.setColour(baseColour.darker(1.0f));
        PathStrokeType arrowOutline(0.5f);
        g.strokePath(arrow, arrowOutline);
    }

    // draw connection index number
    if (showConnectionOrder && !isSignal && connectionCount > 1) {
        auto endCableOrderDisplay = Rectangle<float>(13, 13).withCentre(connectionPath.getPointAlongPath(jmax(connectionPath.getLength() - 8.5f * 3, 9.5f)));
        g.setColour(baseColour);
        g.fillEllipse(endCableOrderDisplay);
        g.setColour(baseColour.darker(1.0f));
        g.drawEllipse(endCableOrderDisplay, 0.5f);
        Fonts::drawStyledText(g, String(multiConnectNumber), endCableOrderDisplay.toNearestIntEdges(), cnv->findColour(PlugDataColour::objectSelectedOutlineColourId).contrasting(), Monospace, 10, Justification::centred);
    }

    // draw reconnect handles if connection is both selected & mouse is hovering over
    if (isSelected && isHovering) {
        auto startReconnectHandle = Rectangle<float>(5, 5).withCentre(connectionPath.getPointAlongPath(8.5f));
        auto endReconnectHandle = Rectangle<float>(5, 5).withCentre(connectionPath.getPointAlongPath(jmax(connectionLength - 8.5f, 9.5f)));

        bool overStart = startReconnectHandle.contains(mousePos.toFloat());
        bool overEnd = endReconnectHandle.contains(mousePos.toFloat());

        g.setColour(handleColour);

        g.fillEllipse(startReconnectHandle.expanded(overStart ? 3.0f : 0.0f));
        g.fillEllipse(endReconnectHandle.expanded(overEnd ? 3.0f : 0.0f));

        g.setColour(cnv->findColour(PlugDataColour::objectOutlineColourId));
        g.drawEllipse(startReconnectHandle.expanded(overStart ? 3.0f : 0.0f), 0.5f);
        g.drawEllipse(endReconnectHandle.expanded(overEnd ? 3.0f : 0.0f), 0.5f);
    }
}

void Connection::updateOverlays(int overlay)
{
    showDirection = overlay & Overlay::Direction;
    showConnectionOrder = overlay & Overlay::Order;
    repaint();
}

void Connection::paint(Graphics& g)
{
    renderConnectionPath(g,
        cnv,
        toDraw,
        outlet->isSignal,
        isMouseOver(),
        showDirection,
        showConnectionOrder,
        selectedFlag,
        getMouseXYRelative(),
        isHovering,
        getNumberOfConnections(),
        getMultiConnectNumber());

    //debug
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds(), 1.0f);

}

bool Connection::isSegmented()
{
    return segmented;
}

void Connection::setSegmented(bool isSegmented)
{
    segmented = isSegmented;
    pushPathState();
    updatePath();
    repaint();
}

void Connection::setSelected(bool shouldBeSelected)
{
    if (selectedFlag != shouldBeSelected) {
        selectedFlag = shouldBeSelected;
        repaint();
    }
}

bool Connection::isSelected()
{
    return selectedFlag;
}

void Connection::mouseMove(MouseEvent const& e)
{
    int n = getClosestLineIdx(e.getPosition().toFloat(), currentPlan);

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

void Connection::mouseEnter(MouseEvent const& e)
{
    lastValueMutex.lock();
    auto args = lastValue;
    auto name = lastSelector;
    lastValueMutex.unlock();

    String tooltip;

    if (name == "float" && args.size() >= 1) {
        tooltip = "(float) " + String(args[0].getFloat());
    } else if (name == "symbol" && args.size() >= 1) {
        tooltip = "(symbol): " + args[0].getSymbol();
    } else if (name == "list") {
        StringArray result = { "(list)" };
        for (auto& arg : args) {
            if (arg.isFloat()) {
                result.add(String(arg.getFloat()));
            } else if (arg.isSymbol()) {
                result.add(arg.getSymbol());
            }
        }
        tooltip = result.joinIntoString(" ");
    } else {
        StringArray result = { name };
        for (auto& arg : args) {
            if (arg.isFloat()) {
                result.add(String(arg.getFloat()));
            } else if (arg.isSymbol()) {
                result.add(arg.getSymbol());
            }
        }

        tooltip = result.joinIntoString(" ");
    }

    setTooltip(tooltip);

    isHovering = true;
    repaint();
}

void Connection::mouseExit(MouseEvent const& e)
{
    isHovering = false;
    repaint();
}

void Connection::mouseDown(MouseEvent const& e)
{
    // Deselect all other connection if shift or command is not down
    if (!e.mods.isCommandDown() && !e.mods.isShiftDown()) {
        cnv->deselectAll();
    }

    cnv->setSelected(this, true);
    repaint();

    if (currentPlan.size() <= 2)
        return;

    int n = getClosestLineIdx(e.position, currentPlan);
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
    if (selectedFlag && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6) {
        cnv->connectingWithDrag = true;
        reconnect(inlet);
    }
    if (selectedFlag && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && e.getDistanceFromDragStart() > 6) {
        cnv->connectingWithDrag = true;
        reconnect(outlet);
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
        resizeToFit();
        repaint();
    }
}

void Connection::mouseUp(MouseEvent const& e)
{
    if (dragIdx != -1) {

        pushPathState();
        dragIdx = -1;
    }

    if (selectedFlag && startReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && startReconnectHandle.contains(e.position)) {
        reconnect(inlet);
    }
    if (selectedFlag && endReconnectHandle.contains(e.getMouseDownPosition().toFloat()) && endReconnectHandle.contains(e.position)) {
        reconnect(outlet);
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
        if (line.getDistanceFromPoint(position, nearest) < 3) {
            return n;
        }
    }

    return -1;
}

void Connection::reconnect(Iolet* target)
{
    if (!reconnecting.isEmpty())
        return;

    auto& otherEdge = target == inlet ? outlet : inlet;

    Array<Connection*> connections = { this };

    if (Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
        for (auto* c : otherEdge->object->getConnections()) {
            if (c == this || !c->isSelected())
                continue;

            connections.add(c);
        }
    }

    for (auto* c : connections) {

        if (cnv->patch.hasConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx)) {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
        }

        // Create new connection
        cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(target->isInlet ? c->inlet : c->outlet, cnv));

        c->setVisible(false);

        reconnecting.add(SafePointer(c));

        // Make sure we're deselected and remove object
        cnv->setSelected(c, false, false);
    }
}

void Connection::resizeToFit()
{
    pStart = getStartPoint();
    pEnd = getEndPoint();
    
    auto newBounds = Rectangle<float>(pStart, pEnd).expanded(8).getSmallestIntegerContainer();
    if (segmented) {
        newBounds = newBounds.getUnion(cnv->getLocalArea(this, toDraw.getBounds().expanded(8).getSmallestIntegerContainer()));
    }
    if (newBounds != getBounds()) {
        setBounds(newBounds);
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    if (!inlet || !outlet)
        return;

    pStart = getStartPoint();
    pEnd = getEndPoint();

    // If both inlet and outlet are selected we can move the connection
    if ((outobj->isSelected() && inobj->isSelected())) {
        auto offset = pStart - previousPosition;
        previousPosition = pStart;
        setTopLeftPosition(getBounds().getTopLeft() + offset.toInt());
        repaint();
        return;
    }
    previousPosition = pStart;

    if (currentPlan.size() <= 2) {
        resizeToFit();
        updatePath();
        repaint();
        return;
    }

    int idx1 = 0;
    int idx2 = 1;

    pStartLocal = getLocalPoint(cnv, pStart);
    pEndLocal = getLocalPoint(cnv, pEnd);

    auto& position = pStartLocal;
    if (&component == inlet || &component == inobj) {
        idx1 = static_cast<int>(currentPlan.size() - 1);
        idx2 = static_cast<int>(currentPlan.size() - 2);
        position = pEndLocal;
    }

    if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
        currentPlan[idx2].x = position.x;
    } else {
        currentPlan[idx2].y = position.y;
    }

    currentPlan[idx1] = position;
    //pStart = getStartPoint();
    //pEnd = getEndPoint();

    resizeToFit();
    updatePath();
    //pStart = getStartPoint();
    //pEnd = getEndPoint();
    //bounds = Rectangle<float>(pStart, pEnd).expanded(8).getSmallestIntegerContainer();
    //setBounds(bounds.getUnion(toDraw.getBounds().expanded(8).getSmallestIntegerContainer()));

    repaint();
}

Point<float> Connection::getStartPoint()
{
    return outlet->getCanvasBounds().toFloat().getCentre();
}

Point<float> Connection::getEndPoint()
{
    return inlet->getCanvasBounds().toFloat().getCentre();
}

Path Connection::getNonSegmentedPath(Point<float> start, Point<float> end)
{
    Path connectionPath;
    connectionPath.startNewSubPath(start);
    if (!PlugDataLook::getUseStraightConnections()) {
        float const width = std::max(start.x, end.x) - std::min(start.x, end.x);
        float const height = std::max(start.y, end.y) - std::min(start.y, end.y);

        float const min = std::min<float>(width, height);
        float const max = std::max<float>(width, height);

        float const maxShiftY = 20.f;
        float const maxShiftX = 20.f;

        float const shiftY = std::min<float>(maxShiftY, max * 0.5);

        float const shiftX = ((start.y >= end.y) ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * ((start.x < end.x) ? -1. : 1.);

        Point<float> const ctrlPoint1 { start.x - shiftX, start.y + shiftY };
        Point<float> const ctrlPoint2 { end.x + shiftX, end.y - shiftY };

        connectionPath.cubicTo(ctrlPoint1, ctrlPoint2, end);
    } else {
        connectionPath.lineTo(end);
    }

    return connectionPath;
}

int Connection::getNumberOfConnections()
{
    int count = 0;
    for (auto connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count++;
        }
    }
    return count;
}

int Connection::getMultiConnectNumber()
{
    int count = 0;
    for (auto connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count += 1;
            if (this == connection)
                return count;
        }
    }
    return -1;
};

void Connection::updatePath()
{
    if (!outlet || !inlet)
        return;


    pStart = getStartPoint();
    pEnd = getEndPoint();

    pStartLocal = getLocalPoint(cnv, pStart);
    pEndLocal = getLocalPoint(cnv, pEnd);

    if (!segmented) {
        toDraw = getNonSegmentedPath(pStartLocal, pEndLocal);
        currentPlan.clear();
    } else {
        if (currentPlan.empty()) {
            findPath();
        }

        auto snap = [this](Point<float> point, int idx1, int idx2) {
            if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
                currentPlan[idx2].x = point.x;
            } else {
                currentPlan[idx2].y = point.y;
            }

            currentPlan[idx1] = point;
        };

        snap(pStartLocal, 0, 1);
        snap(pEndLocal, static_cast<int>(currentPlan.size() - 1), static_cast<int>(currentPlan.size() - 2));

        Path connectionPath;
        connectionPath.startNewSubPath(pStartLocal);

        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++) {
            if (connectionPath.contains(currentPlan[n].toFloat()))
                continue; // ??

            connectionPath.lineTo(currentPlan[n].toFloat());
        }

        connectionPath.lineTo(pEndLocal);
        toDraw = connectionPath.createPathWithRoundedCorners(PlugDataLook::getUseStraightConnections() ? 0.0f : 8.0f);
    }

    startReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(8.5f));
    endReconnectHandle = Rectangle<float>(5, 5).withCentre(toDraw.getPointAlongPath(std::max(toDraw.getLength() - 8.5f, 9.5f)));
}

void Connection::findPath()
{
    if (!outlet || !inlet)
        return;

    auto pStartFindPath = getStartPoint();
    auto pEndFindPath = getEndPoint();


    auto pathStack = PathPlan();
    auto bestPath = PathPlan();

    pathStack.reserve(8);

    auto numFound = 0;

    float incrementX, incrementY;

    auto distance = pStartFindPath.getDistanceFrom(pEndFindPath);
    auto distanceX = std::abs(pStartFindPath.x - pEndFindPath.x);
    auto distanceY = std::abs(pStartFindPath.y - pEndFindPath.y);

    int maxXResolution = std::clamp<int>(distanceX / 10, 6, 14);
    int maxYResolution = std::clamp<int>(distanceY / 10, 6, 14);

    int resolutionX = 6;
    int resolutionY = 6;

    auto obstacles = Array<Rectangle<float>>();
    auto searchBounds = Rectangle<float>(pStartFindPath, pEndFindPath);

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

        numFound = findLatticePaths(bestPath, pathStack, pEndFindPath, pStartFindPath, { incrementX, incrementY });

        if (resolutionX < maxXResolution)
            resolutionX++;
        if (resolutionY < maxXResolution)
            resolutionY++;

        if (resolutionX > maxXResolution || resolutionY > maxYResolution)
            break;

        pathStack.clear();
    }

    // =============== in local space now ==============

    //pStartLocal = getLocalPoint(cnv, pStart);
    //pEndLocal = getLocalPoint(cnv, pEnd);

    pStartLocalFP = getLocalPoint(cnv, pStartFindPath);
    pEndLocalFP = getLocalPoint(cnv, pEndFindPath);

    PathPlan simplifiedPath;

    bool direction;
    if (!bestPath.empty()) {
        simplifiedPath.push_back(getLocalPoint(cnv, bestPath.front()));

        direction = bestPath[0].x == bestPath[1].x;

        if (!direction)
            simplifiedPath.push_back(getLocalPoint(cnv, bestPath.front()));

        for (int n = 1; n < bestPath.size(); n++) {
            if ((bestPath[n].x != bestPath[n - 1].x && direction) || (bestPath[n].y != bestPath[n - 1].y && !direction)) {
                simplifiedPath.push_back(getLocalPoint(cnv, bestPath[n - 1]));
                direction = !direction;
            }
        }

        simplifiedPath.push_back(getLocalPoint(cnv, bestPath.back()));

        if (!direction)
            simplifiedPath.push_back(getLocalPoint(cnv, bestPath.back()));
    } else {
        if (pEndLocalFP.y < pStartLocalFP.y) {
            auto xHalfDistance = (pStartLocalFP.x - pEndLocalFP.x) * 0.5f;

            simplifiedPath.push_back(pEndLocalFP); // double to make it draggable
            simplifiedPath.push_back(pEndLocalFP);
            simplifiedPath.push_back({ pEndLocalFP.x + xHalfDistance, pEndLocalFP.y });
            simplifiedPath.push_back({ pEndLocalFP.x + xHalfDistance, pStartLocalFP.y });
            simplifiedPath.push_back(pStartLocalFP);
            simplifiedPath.push_back(pStartLocalFP);
        } else {
            auto yHalfDistance = (pStartLocalFP.y - pEndLocalFP.y) * 0.5f;
            simplifiedPath.push_back(pEndLocalFP);
            simplifiedPath.push_back({ pEndLocalFP.x, pEndLocalFP.y + yHalfDistance });
            simplifiedPath.push_back({ pStartLocalFP.x, pEndLocal.y + yHalfDistance });
            simplifiedPath.push_back(pStartLocalFP);
        }
    }
    std::reverse(simplifiedPath.begin(), simplifiedPath.end());

    currentPlan = simplifiedPath;

    pushPathState();
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

void ConnectionPathUpdater::timerCallback()
{

    std::pair<Component::SafePointer<Connection>, t_symbol*> currentConnection;

    canvas->patch.startUndoSequence("SetConnectionPaths");

    while (connectionUpdateQueue.try_dequeue(currentConnection)) {

        auto& [connection, newPathState] = currentConnection;

        if (!connection)
            continue;

        bool found = false;
        t_linetraverser t;

        t_object* outObj;
        int outIdx;
        t_object* inObj;
        int inIdx;

        // Get connections from pd
        linetraverser_start(&t, connection->cnv->patch.getPointer());

        while (auto* oc = linetraverser_next(&t)) {
            if (reinterpret_cast<Connection::t_fake_outconnect*>(oc) == connection->ptr) {

                outObj = t.tr_ob;
                outIdx = t.tr_outno;
                inObj = t.tr_ob2;
                inIdx = t.tr_inno;

                found = true;
                break;
            }
        }

        if (!found)
            continue;

        t_symbol* oldPathState = connection->ptr->outconnect_path_data;

        // This will recreate the connection with the new connection path, and return the new pointer
        // Since we mostly used indices and object pointers to differentiate connections, this is fine

        auto* newConnection = connection->cnv->patch.setConnctionPath(outObj, outIdx, inObj, inIdx, oldPathState, newPathState);
        connection->ptr = static_cast<Connection::t_fake_outconnect*>(newConnection);
    }

    canvas->patch.endUndoSequence("SetConnectionPaths");

    stopTimer();
}

void Connection::receiveMessage(String const& name, int argc, t_atom* argv)
{
    if (lastValueMutex.try_lock()) {
        lastValue = pd::Atom::fromAtoms(argc, argv);
        lastSelector = name;
        lastValueMutex.unlock();
    }
}
