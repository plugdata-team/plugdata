/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <queue>

#include <nanovg_async.h>
#include "Utility/Config.h"
#include "Utility/NVGUtils.h"
#include "Utility/SettingsFile.h"

#include "Connection.h"

#include "Canvas.h"
#include "Iolet.h"
#include "Object.h"
#include "PluginProcessor.h"
#include "PluginEditor.h" // might not need this?
#include "CanvasViewport.h"
#include "Pd/Patch.h"
#include "Components/ConnectionMessageDisplay.h"

Connection::Connection(Canvas* parent, Iolet* s, Iolet* e, t_outconnect* oc)
    : NVGComponent(this)
    , inlet(s->isInlet() ? s : e)
    , outlet(s->isInlet() ? e : s)
    , inobj(inlet->getObject())
    , outobj(outlet->getObject())
    , cnv(parent)
    , ptr(parent->pd)
{
    cnv->selectedComponents.addChangeListener(this);

    locked.referTo(parent->locked);
    presentationMode.referTo(parent->presentationMode);

    // Make sure it's not 2x the same iolet
    if (!outlet || !inlet || outlet->isInlet() == inlet->isInlet()) {
        outlet = nullptr;
        inlet = nullptr;
        jassertfalse;
        return;
    }

    cableType = DataCable;

    if (outlet && outlet->isSignal()) {
        cableType = SignalCable;
    }
    if (outlet && outlet->isGemState()) {
        cableType = GemCable;
    }

    inIdx = inlet->getIndex();
    outIdx = outlet->getIndex();

    outlet->repaint();
    inlet->repaint();

    // If it doesn't already exist in pd, create connection in pd
    if (!oc) {
        auto* checkedOut = pd::Interface::checkObject(outobj->getPointer());
        auto* checkedIn = pd::Interface::checkObject(inobj->getPointer());
        if (checkedOut && checkedIn) {
            oc = parent->patch.createAndReturnConnection(checkedOut, outIdx, checkedIn, inIdx);
            setPointer(oc);
        } else {
            jassertfalse;
            return;
        }
    } else {
        setPointer(oc);
        popPathState();
    }

    // Listen to changes at iolets
    outobj->addComponentListener(this);
    inobj->addComponentListener(this);

    setPaintingIsUnclipped(true);
    setInterceptsMouseClicks(true, true);

    addMouseListener(cnv, true);

    cnv->connectionLayer.addAndMakeVisible(this);

    updater.addAnimator(activityStateAnimator);

    setAccessible(false);
    lookAndFeelChanged();
}

Connection::~Connection()
{
    if (cnv->pd->connectionListener)
        cnv->pd->connectionListener.load()->setConnection(nullptr);

    cnv->pd->unregisterMessageListener(this);
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
    if (auto const selectedItems = dynamic_cast<SelectedItemSet<WeakReference<Component>>*>(source))
        setSelected(selectedItems->isSelected(this));
}

void Connection::lookAndFeelChanged()
{
    auto const& colours = getThemeColours(*this);

    handleColour = outlet->isSignal() ? nvgColour(colours.dataColour) : nvgColour(colours.signalColour);
    shadowColour = nvgColour(colours.canvasBackgroundColour.contrasting(0.06f).withAlpha(0.24f));
    outlineColour = nvgColour(colours.objectOutlineColour);

    if (connectionStyle != getPlugDataLook(*this).getConnectionStyle()) {
        connectionStyle = getPlugDataLook(*this).getConnectionStyle();
        cachedPath.clear();
    }

    updatePath();
    repaint();
}

NVGcolor Connection::getConnectionColour() const
{
    auto const& colours = getThemeColours(*this);

    Colour c = colours.connectionColour;
    if (isSelected() || isHovering) {
        if (outlet->isSignal()) {
            c = colours.signalColour;
        } else if (outlet->isGemState()) {
            c = colours.gemColour;
        } else {
            c = colours.dataColour;
        }
    }
    return nvgColour(isHovering ? c.brighter() : c);
}

void Connection::render(NVGcontext* nvg)
{
    auto connectionColour = getConnectionColour();
    nanovg::nvgTranslate(nvg, getX(), getY());

    bool isSignalCable = cableType == SignalCable && connectionStyle != PlugDataLook::ConnectionStyleVanilla;
    auto dashColor = shadowColour;
    if (isSignalCable) {
        dashColor.a = 255;
        dashColor.r *= 0.4f;
        dashColor.g *= 0.4f;
        dashColor.b *= 0.4f;
    }

    float cableThickness = getPathWidth();

    // Draw a fake path dot if the path is less than 1pt in length.
    // Paths don't draw currently if they have length of zero points
    if (pathLength < 1.0f) {
        auto pathFromOrigin = getPath();
        pathFromOrigin.applyTransform(AffineTransform::translation(-getX(), -getY()));
        auto startPoint = pathFromOrigin.getPointAlongPath(0.0);

        nanovg::nvgBeginPath(nvg);
        nanovg::nvgFillColor(nvg, shadowColour);
        nanovg::nvgCircle(nvg, startPoint.x, startPoint.y, cableThickness * 0.5f); // cableThickness is diameter, while circle is radius
        nanovg::nvgFill(nvg);

        nanovg::nvgBeginPath(nvg);
        nanovg::nvgFillColor(nvg, connectionColour);
        nanovg::nvgCircle(nvg, startPoint.x, startPoint.y, cableThickness * 0.25f);
        nanovg::nvgFill(nvg);
        return;
    }

    float dashSize = isSignalCable ? numSignalChannels <= 1 ? 2.5f : 1.5f : 0.0f;
    auto useGradientLook = getPlugDataLook(*this).getUseGradientConnectionLook() && !(isSelected() || isHovering);
    auto showActivity = cableType == DataCable && cnv->shouldShowConnectionActivity();
    nanovg::nvgStrokePaint(nvg, nanovg::nvgDoubleStroke(nvg, connectionColour, shadowColour, dashColor, dashSize, useGradientLook, showActivity, offset));
    nanovg::nvgStrokeWidth(nvg, cableThickness);

    bool cacheHit = cachedPath.stroke();
    if (!cacheHit) {
        auto pathFromOrigin = getPath();
        pathFromOrigin.applyTransform(AffineTransform::translation(-getX(), -getY()));

        setJUCEPath(nvg, pathFromOrigin);
        nanovg::nvgStroke(nvg);
        cachedPath.save(nvg);
    }

    if (isSelected() && isHovering) {
        auto expandedStartHandle = isInStartReconnectHandle ? startReconnectHandle.expanded(3.0f) : startReconnectHandle;
        auto expandedEndHandle = isInEndReconnectHandle ? endReconnectHandle.expanded(3.0f) : endReconnectHandle;

        nanovg::nvgFillColor(nvg, handleColour);

        nanovg::nvgBeginPath(nvg);
        nanovg::nvgCircle(nvg, expandedStartHandle.getCentreX() - getX(), expandedStartHandle.getCentreY() - getY(), expandedStartHandle.getWidth() / 2);
        nanovg::nvgCircle(nvg, expandedEndHandle.getCentreX() - getX(), expandedEndHandle.getCentreY() - getY(), expandedEndHandle.getWidth() / 2);
        nanovg::nvgFill(nvg);
    }

    // draw direction arrow if activated in overlay menu
    //              c
    //              |\
    //              | \
    //              |  \
    //  ___path___  |   \a  ___path___
    //              |   /
    //              |  /
    //              | /
    //              |/
    //              b
    // setup arrow parameters
    constexpr float arrowWidth = 8.0f;
    constexpr float arrowLength = 12.0f;

    auto renderArrow = [nvg, connectionColour](Path const& path, float const connectionLength) {
        // get the center point of the connection path
        auto const arrowCenter = connectionLength * 0.5f;
        auto const arrowBase = path.getPointAlongPath(arrowCenter - arrowLength * 0.5f);
        auto const arrowTip = path.getPointAlongPath(arrowCenter + arrowLength * 0.5f);

        Line<float> const arrowLine(arrowBase, arrowTip);
        auto const point_a = arrowTip;
        auto const point_b = arrowLine.getPointAlongLine(0.0f, -(arrowWidth * 0.5f));
        auto const point_c = arrowLine.getPointAlongLine(0.0f, arrowWidth * 0.5f);

        // draw the arrow
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgFillColor(nvg, connectionColour);
        nanovg::nvgMoveTo(nvg, point_a.x, point_a.y);
        nanovg::nvgLineTo(nvg, point_b.x, point_b.y);
        nanovg::nvgLineTo(nvg, point_c.x, point_c.y);
        nanovg::nvgClosePath(nvg);
        nanovg::nvgStrokeWidth(nvg, 1.0f);
        nanovg::nvgFill(nvg);
    };

    if (cnv->shouldShowConnectionDirection()) {
        if (isSegmented()) {
            for (int i = 1; i < currentPlan.size(); i++) {
                auto const pathLine = Line<float>(currentPlan[i - 1], currentPlan[i]);
                auto const length = pathLine.getLength();
                // don't show arrow if start or end segment is too small, to give room for the reconnect handle
                auto const isStartOrEnd = i == 1 || i == currentPlan.size() - 1;
                if (length > arrowLength * (isStartOrEnd ? 3 : 2)) {
                    Path segmentedPath;
                    segmentedPath.addLineSegment(pathLine, 0.0f);
                    segmentedPath.applyTransform(AffineTransform::translation(-getX(), -getY()));
                    renderArrow(segmentedPath, length);
                }
            }
        } else {
            auto connectionPath = getPath();
            connectionPath.applyTransform(AffineTransform::translation(-getX(), -getY()));
            if (pathLength > arrowLength * 2) {
                renderArrow(connectionPath, pathLength);
            }
        }
    }
}

void Connection::renderConnectionOrder(NVGcontext* nvg) const
{
    if (cableType == DataCable && getNumberOfConnections() > 1) {
        auto connectionPath = getPath();
        connectionPath.applyTransform(AffineTransform::translation(-getX(), -getY()));
        auto const pos = cnv->getLocalPoint(this, connectionPath.getPointAlongPath(jmax(pathLength - 8.5f * 3, 9.5f)));
        // circle background
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgStrokeColor(nvg, outlineColour);
        nanovg::nvgFillColor(nvg, getConnectionColour());
        constexpr auto radius = 7.0f;
        constexpr auto diameter = radius * 2.0f;
        auto const circleTopLeft = pos - Point<float>(radius, radius);
        nanovg::nvgRoundedRect(nvg, circleTopLeft.getX(), circleTopLeft.getY(), diameter, diameter, radius);
        nanovg::nvgStrokeWidth(nvg, 1.0f);
        nanovg::nvgFill(nvg);
        nanovg::nvgStroke(nvg);

        // connection index number
        Fonts::drawText(cnv->editor->getNanoLLGC(), String(getMultiConnectNumber()), Rectangle<float>(radius, radius).withCentre(pos.toFloat()), Fonts::getDefaultFont().withHeight(9), getThemeColours(*this).objectSelectedOutlineColour.contrasting(), Justification::centred);
    }
}

void Connection::pushPathState(bool const force)
{
    if (!inlet || !outlet)
        return;

    t_symbol* newPathState;
    if (segmented) {
        MemoryOutputStream stream;

        for (auto const& point : currentPlan) {
            stream.writeInt(point.x - outlet->getCanvasBounds().getCentre().x);
            stream.writeInt(point.y - outlet->getCanvasBounds().getCentre().y);
        }
        auto const base64 = stream.getMemoryBlock().toBase64Encoding();
        newPathState = cnv->pd->generateSymbol(base64);
    } else {
        newPathState = cnv->pd->generateSymbol("empty");
    }

    cnv->pathUpdater->pushPathState(this, newPathState);
    if (force)
        cnv->pathUpdater->timerCallback();
}

void Connection::popPathState()
{
    if (!inlet || !outlet)
        return;

    String state;
    if (auto oc = ptr.get<t_outconnect>()) {
        auto const* pathData = outconnect_get_path_data(oc.get());
        if (!pathData || !pathData->s_name)
            return;
        state = String::fromUTF8(pathData->s_name);
    }

    auto block = MemoryBlock();
    auto const succeeded = block.fromBase64Encoding(state);

    auto plan = PathPlan();

    if (succeeded) {
        auto stream = MemoryInputStream(block, false);

        while (!stream.isExhausted()) {
            auto const x = stream.readInt();
            auto const y = stream.readInt();

            plan.emplace_back(x + outlet->getCanvasBounds().getCentreX(), y + outlet->getCanvasBounds().getCentreY());
        }
        segmented = !plan.empty();
    } else {
        segmented = false;
    }

    currentPlan = plan;
    numSignalChannels = getNumSignalChannels();
    updatePath();
}

void Connection::setPointer(t_outconnect* newPtr)
{
    auto const originalPointer = ptr.getRawUnchecked<t_outconnect>();
    if (originalPointer != newPtr) {
        ptr = pd::WeakReference(newPtr, cnv->pd);

        cnv->pd->unregisterMessageListener(this);
        cnv->pd->registerMessageListener(newPtr, this);
    }
}

t_outconnect* Connection::getPointer() const
{
    return ptr.getRaw<t_outconnect>();
}

t_symbol* Connection::getPathState() const
{
    if (auto oc = ptr.get<t_outconnect>()) {
        return outconnect_get_path_data(oc.get());
    }

    return nullptr;
}

bool Connection::hitTest(int const x, int const y)
{
    if (inlet == nullptr || outlet == nullptr)
        return false;

    if (cnv->panningModifierDown())
        return false;

    if (cnv->commandLocked == var(true) || locked == var(true) || !cnv->connectionsBeingCreated.empty())
        return false;

    Point<float> const position = Point<float>(static_cast<float>(x), static_cast<float>(y)) + getPosition().toFloat();

    Point<float> nearestPoint;

    auto const path = getPath();
    path.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto const pstart = getStartPoint();
    auto const pend = getEndPoint();

    if (selectedFlag && (startReconnectHandle.contains(position) || endReconnectHandle.contains(position))) {
        repaint();
        return true;
    }

    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position) < 8.0f || pend.getDistanceFrom(position) < 8.0f)
        return false;

    return nearestPoint.getDistanceFrom(position) < 3;
}

bool Connection::intersects(Rectangle<float> const toCheck, int const accuracy) const
{
    PathFlatteningIterator i(getPath());

    while (i.next()) {
        auto const point1 = Point<float>(i.x1, i.y1);

        // Skip points to reduce accuracy a bit for better performance
        // We can only skip points if there are many points!
        if (!getPlugDataLook(*this).getUseStraightConnections()) {
            for (int n = 0; n < accuracy; n++) {
                auto const next = i.next();
                if (!next)
                    break;
            }
        }

        auto const point2 = Point<float>(i.x2, i.y2);

        auto currentLine = Line<float>(point1, point2);

        if (toCheck.intersects(currentLine)) {
            return true;
        }
    }

    return false;
}

void Connection::forceUpdate()
{
    updatePath();
    repaint();
}

bool Connection::isSegmented() const
{
    return segmented;
}

void Connection::setSegmented(bool const isSegmented)
{
    segmented = isSegmented;
    updatePath();
    repaint();
    pushPathState();
}

void Connection::setSelected(bool const shouldBeSelected)
{
    if (selectedFlag != shouldBeSelected) {
        selectedFlag = shouldBeSelected;
        // Make the connection rise to the top of the connection layer
        // This is so resize handles can easily be hit when the connection is selected
        setAlwaysOnTop(shouldBeSelected);
        repaint();
    }
}

bool Connection::isSelected() const
{
    return selectedFlag;
}

void Connection::mouseMove(MouseEvent const& e)
{
    auto setReconnectFlag = [this](bool const start, bool const end) {
        if (isInStartReconnectHandle != start || isInEndReconnectHandle != end) {
            isInStartReconnectHandle = start;
            isInEndReconnectHandle = end;
            repaint();
        }
    };

    if (startReconnectHandle.contains(e.getPosition().toFloat().translated(getX(), getY()))) {
        setReconnectFlag(selectedFlag, false);
    } else if (endReconnectHandle.contains(e.getPosition().toFloat().translated(getX(), getY()))) {
        setReconnectFlag(false, selectedFlag);
    } else {
        setReconnectFlag(false, false);
    }

    if (isInStartReconnectHandle || isInEndReconnectHandle) {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }

    int const n = getClosestLineIdx(e.getPosition().toFloat(), currentPlan);

    if (isSegmented() && currentPlan.size() > 2 && n > 0) {
        auto const line = Line<float>(currentPlan[n - 1], currentPlan[n]);

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
}

StringArray Connection::getMessageFormated() const
{
    auto const& args = lastValue;
    auto const numArgs = args.size();
    auto const name = lastSelector ? String::fromUTF8(lastSelector->s_name) : "";

    StringArray formatedMessage;

    if (name == "float" && numArgs > 0) {
        formatedMessage.add("float:");
        formatedMessage.add(args[0].toString());
    } else if (name == "symbol" && numArgs > 0) {
        formatedMessage.add("symbol:");
        formatedMessage.add(args[0].toString());
    } else if (name == "list") {
        if (numArgs >= 15) {
            formatedMessage.add("list (14+):");
        } else {
            formatedMessage.add("list (" + String(numArgs) + "):");
        }
        for (int arg = 0; arg < numArgs; arg++) {
            if (args[arg].isFloat()) {
                formatedMessage.add(String(args[arg].getFloat()));
            } else if (args[arg].isSymbol()) {
                formatedMessage.add(args[arg].toString());
            }
        }
        if (numArgs >= 15) {
            formatedMessage.add("...");
        }
    } else {
        formatedMessage.add(name);
        for (int arg = 0; arg < numArgs; arg++) {
            if (args[arg].isFloat()) {
                formatedMessage.add(String(args[arg].getFloat()));
            } else if (args[arg].isSymbol()) {
                formatedMessage.add(args[arg].toString());
            }
        }
    }
    return formatedMessage;
}

void Connection::mouseEnter(MouseEvent const& e)
{
    isHovering = true;
    if (plugdata_debugging_enabled()) {
        Point<float> nearest;
        getPath().getNearestPoint(cnv->getLocalPoint(this, e.position), nearest);
        cnv->editor->connectionMessageDisplay->setConnection(this, cnv->localPointToGlobal(nearest).roundToInt().translated(20, 15));
    }
    repaint();
}

void Connection::mouseExit(MouseEvent const& e)
{
    cnv->editor->connectionMessageDisplay->setConnection(nullptr);
    isHovering = false;
    repaint();
}

void Connection::mouseDown(MouseEvent const& e)
{
    if (e.mods.isShiftDown() && e.getNumberOfClicks() == 2 && cnv->getSelectionOfType<Connection>().size() == 2) {
        if (auto oc = ptr.get<t_outconnect>()) {
            auto* patch = cnv->patch.getRawPointer();
            auto* other = cnv->getSelectionOfType<Connection>()[0]->getPointer();
            if (patch && other) {
                pd::Interface::swapConnections(patch, oc.get(), other);
            }
        }
        cnv->synchronise();
        return;
    }
    cnv->editor->connectionMessageDisplay->setConnection(nullptr);

    // Deselect all other connection if shift or command is not down
    if (!e.mods.isCommandDown() && !e.mods.isShiftDown() && !e.mods.isPopupMenu()) {
        cnv->deselectAll();
    }

    wasSelected = selectedFlag;
    cnv->setSelected(this, true);
    repaint();

    static auto getClosestCornerIdx = [](PathPlan const& plan, Point<float> const position) {
        int closestIdx = -1;
        auto closestDistance = 8.0f;

        for (int n = 1; n < static_cast<int>(plan.size()) - 1; n++) {
            auto const distance = plan[n].getDistanceFrom(position);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestIdx = n;
            }
        }

        return closestIdx;
    };

    // Double-clicking adds a corner, or removes the one under the mouse
    auto const position = cnv->getLocalPoint(this, e.position);
    auto const isOnReconnectHandle = startReconnectHandle.contains(position) || endReconnectHandle.contains(position);

    if (e.getNumberOfClicks() == 2 && e.mods.isLeftButtonDown() && !e.mods.isAnyModifierKeyDown() && !isOnReconnectHandle) {
        if (auto const cornerIdx = getClosestCornerIdx(currentPlan, position); cornerIdx >= 0) {
            removeCorner(cornerIdx);
        } else {
            addCorner(position);
        }
        return;
    }

    if (currentPlan.size() <= 2)
        return;

    int const n = getClosestLineIdx(e.position, currentPlan);
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
    cnv->editor->connectionMessageDisplay->setConnection(nullptr);

    bool const isDragging = e.getDistanceFromDragStart() > 6;

    if (wasSelected && isInStartReconnectHandle) {
        if (isDragging) {
            cnv->connectingWithDrag = true;
            reconnect(inlet);
        }
        return;
    }
    if (wasSelected && isInEndReconnectHandle) {
        if (isDragging) {
            cnv->connectingWithDrag = true;
            reconnect(outlet);
        }
        return;
    }

    if (currentPlan.empty())
        return;

    if (isSegmented() && dragIdx != -1) {
        auto const n = dragIdx;
        auto const delta = e.getPosition() - e.getMouseDownPosition();
        auto const line = Line<float>(currentPlan[n - 1], currentPlan[n]);

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
                    canvas->connections.remove_one(c.getComponent());
                }
            }
        });

        reconnecting.clear();
    }
}

int Connection::getClosestLineIdx(Point<float> const& position, PathPlan const& plan) const
{
    if (plan.size() < 2)
        return -1;

    for (int n = 2; n < plan.size() - 1; n++) {
        auto line = Line<float>(plan[n - 1], plan[n]);
        Point<float> nearest;

        // Zero-length segments have no direction to drag in
        if (line.getLength() < 1.0f)
            continue;

        if (line.getDistanceFromPoint(cnv->getLocalPoint(this, position), nearest) < 3) {
            return n;
        }
    }

    return -1;
}

int Connection::getNumCorners() const
{
    int numCorners = 0;
    auto lastDirection = ZeroLength;

    for (int n = 0; n < static_cast<int>(currentPlan.size()) - 1; n++) {
        auto const delta = currentPlan[n + 1] - currentPlan[n];

        // Sub-pixel segments don't count as a corner
        if (delta.getDistanceFromOrigin() < 1.0f)
            continue;

        auto const direction = std::abs(delta.y) > std::abs(delta.x) ? Vertical : Horizontal;
        if (lastDirection != ZeroLength && direction != lastDirection)
            numCorners++;

        lastDirection = direction;
    }

    return numCorners;
}

// Direction of the segment between point idx and idx + 1
Connection::SegmentDirection Connection::getSegmentDirection(int const idx) const
{
    auto const delta = currentPlan[idx + 1] - currentPlan[idx];

    if (delta.getDistanceFromOrigin() < 0.01f)
        return ZeroLength;

    return std::abs(delta.y) > std::abs(delta.x) ? Vertical : Horizontal;
}

// Removes points that would make the path go diagonal when it gets dragged or an object moves,
// without changing the shape of the path
void Connection::sanitisePlan()
{
    for (int n = 1; n < static_cast<int>(currentPlan.size()) - 1;) {
        auto const lastIdx = static_cast<int>(currentPlan.size()) - 1;
        auto const before = currentPlan[n - 1];
        auto const point = currentPlan[n];
        auto const after = currentPlan[n + 1];

        auto const isOnTopOfBefore = getSegmentDirection(n - 1) == ZeroLength;
        auto const isOnTopOfAfter = getSegmentDirection(n) == ZeroLength;

        // A zero-length segment can only turn into a corner if the segments around it run the same way.
        // At the iolets, updatePath() snaps it vertically, so the segment next to it has to be horizontal
        bool isUnusable;
        if (isOnTopOfBefore && isOnTopOfAfter) {
            isUnusable = true;
        } else if (isOnTopOfBefore) {
            isUnusable = n == 1 ? getSegmentDirection(n) != Horizontal : getSegmentDirection(n - 2) != getSegmentDirection(n);
        } else if (isOnTopOfAfter) {
            isUnusable = n == lastIdx - 1 ? getSegmentDirection(n - 1) != Horizontal : getSegmentDirection(n + 1) != getSegmentDirection(n - 1);
        } else {
            isUnusable = (approximatelyEqual(before.x, point.x) && approximatelyEqual(point.x, after.x))
                || (approximatelyEqual(before.y, point.y) && approximatelyEqual(point.y, after.y));
        }

        if (isUnusable) {
            // Line up the remaining points exactly, so no segment ends up slightly off-axis
            if (isOnTopOfBefore) {
                currentPlan[n - 1] = point;
                if (isOnTopOfAfter)
                    currentPlan[n + 1] = point;
            } else if (isOnTopOfAfter) {
                currentPlan[n + 1] = point;
            } else if (approximatelyEqual(before.x, point.x)) {
                currentPlan[n + 1].x = before.x;
            } else {
                currentPlan[n + 1].y = before.y;
            }

            currentPlan.remove_at(n);
            n = std::max(1, n - 1);
        } else {
            n++;
        }
    }
}

void Connection::addCorner(Point<float> const& position)
{
    if (!inlet || !outlet)
        return;

    if (!segmented) {
        auto const pstart = getStartPoint();
        auto const pend = getEndPoint();

        auto plan = PathPlan();
        plan.add(pstart);

        // Run the middle segment through the clicked position, in the direction the connection mostly goes
        if (std::abs(pend.y - pstart.y) >= std::abs(pend.x - pstart.x)) {
            plan.emplace_back(position.x, pstart.y);
            plan.emplace_back(position.x, pend.y);
        } else {
            plan.emplace_back(pstart.x, position.y);
            plan.emplace_back(pend.x, position.y);
        }

        plan.add(pend);

        currentPlan = plan;
        segmented = true;
    } else {
        static auto getClosestSegmentIdx = [](PathPlan const& plan, Point<float> const position) {
            int closestIdx = -1;
            auto closestDistance = std::numeric_limits<float>::max();

            for (int n = 0; n < static_cast<int>(plan.size()) - 1; n++) {
                Point<float> nearest;
                auto const distance = Line<float>(plan[n], plan[n + 1]).getDistanceFromPoint(position, nearest);
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestIdx = n;
                }
            }

            return closestIdx;
        };

        auto const segmentIdx = getClosestSegmentIdx(currentPlan, position);
        if (segmentIdx < 0)
            return;

        auto const direction = getSegmentDirection(segmentIdx);

        // Insert the corner twice, the zero-length segment in between becomes the new step
        Point<float> corner;
        Line<float>(currentPlan[segmentIdx], currentPlan[segmentIdx + 1]).getDistanceFromPoint(position, corner);
        currentPlan.insert(segmentIdx + 1, corner);
        currentPlan.insert(segmentIdx + 1, corner);

        // Move one half of the segment halfway towards its neighbour to make the step visible.
        // The half that's attached to an iolet can't move
        auto const lastIdx = static_cast<int>(currentPlan.size()) - 1;
        int startIdx = -1, endIdx = 0, beforeIdx = 0, afterIdx = 0;

        if (segmentIdx + 3 < lastIdx) {
            // The half after the corner
            startIdx = segmentIdx + 2;
            endIdx = segmentIdx + 3;
            beforeIdx = segmentIdx + 1;
            afterIdx = segmentIdx + 4;
        } else if (segmentIdx > 0) {
            // The half before the corner
            startIdx = segmentIdx;
            endIdx = segmentIdx + 1;
            beforeIdx = segmentIdx - 1;
            afterIdx = segmentIdx + 2;
        }

        if (startIdx >= 0) {
            if (direction == Vertical) {
                auto const halfway = (currentPlan[beforeIdx].x + currentPlan[afterIdx].x) * 0.5f;
                currentPlan[startIdx].x = halfway;
                currentPlan[endIdx].x = halfway;
            } else {
                auto const halfway = (currentPlan[beforeIdx].y + currentPlan[afterIdx].y) * 0.5f;
                currentPlan[startIdx].y = halfway;
                currentPlan[endIdx].y = halfway;
            }
        }
    }

    sanitisePlan();
    updatePath();
    repaint();
    pushPathState();
}

void Connection::removeCorner(int const cornerIdx)
{
    if (cornerIdx < 1 || cornerIdx >= static_cast<int>(currentPlan.size()) - 1)
        return;

    // Removing the last corner would leave a diagonal, so go back to a curve
    if (getNumCorners() <= 1) {
        setSegmented(false);
        return;
    }

    // Points on top of each other form a single corner
    int first = cornerIdx, last = cornerIdx;
    while (first > 1 && currentPlan[first - 1].getDistanceFrom(currentPlan[cornerIdx]) < 1.0f)
        first--;
    while (last < static_cast<int>(currentPlan.size()) - 2 && currentPlan[last + 1].getDistanceFrom(currentPlan[cornerIdx]) < 1.0f)
        last++;

    currentPlan.remove_range(first, last + 1);

    // Line up the two points that are now connected. A point can only move along its other segment,
    // otherwise the rest of the path breaks
    auto const prevIdx = first - 1;
    auto const nextIdx = first;
    auto const lastIdx = static_cast<int>(currentPlan.size()) - 1;

    if (!approximatelyEqual(currentPlan[prevIdx].x, currentPlan[nextIdx].x) && !approximatelyEqual(currentPlan[prevIdx].y, currentPlan[nextIdx].y)) {
        if (nextIdx < lastIdx) {
            if (approximatelyEqual(currentPlan[nextIdx].x, currentPlan[nextIdx + 1].x)) {
                currentPlan[nextIdx].y = currentPlan[prevIdx].y;
            } else {
                currentPlan[nextIdx].x = currentPlan[prevIdx].x;
            }
        } else if (prevIdx > 0) {
            if (approximatelyEqual(currentPlan[prevIdx].x, currentPlan[prevIdx - 1].x)) {
                currentPlan[prevIdx].y = currentPlan[nextIdx].y;
            } else {
                currentPlan[prevIdx].x = currentPlan[nextIdx].x;
            }
        }
    }

    sanitisePlan();

    if (getNumCorners() == 0) {
        setSegmented(false);
        return;
    }

    updatePath();
    repaint();
    pushPathState();
}

void Connection::setPath(Path const& newPath)
{
    path = newPath;
    updateBounds();
}

void Connection::updateBounds()
{
    // Resize the component to enclose the path, expanded by the stroke thickness.
    // The extra margin is used for hit detection and drawing the reconnect handles.
    if (path.isEmpty()) {
        setBounds({});
        return;
    }
    setBounds(path.getBounds().expanded(6.0f).getSmallestIntegerContainer());
}

float Connection::getPathWidth() const
{
    switch (connectionStyle) {
    case PlugDataLook::ConnectionStyleVanilla:
        return cableType == SignalCable ? 4.5f : 2.5f;
    case PlugDataLook::ConnectionStyleThin:
        return 3.0f;
    default:
        return 4.5f;
    }
}

void Connection::reconnect(Iolet const* target)
{
    if (!reconnecting.empty() || !target)
        return;

    auto const& otherIolet = target == inlet ? outlet : inlet;

    SmallArray<Connection*> connections = { this };

    if (Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
        for (auto* c : otherIolet->getObject()->getConnections()) {
            if (c == this || !c->isSelected())
                continue;

            connections.add(c);
        }
    }

    for (auto* c : connections) {

        auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
        auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

        if (checkedOut && checkedIn && cnv->patch.hasConnection(checkedOut, c->outIdx, checkedIn, c->inIdx)) {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
        }

        // Create new connection
        cnv->connectionsBeingCreated.add(target->isInlet() ? c->inlet : c->outlet, cnv);

        c->setVisible(false);

        reconnecting.add(SafePointer(c));

        // Make sure we're deselected and remove object
        cnv->setSelected(c, false, false);
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool const wasResized)
{
    if (!inlet || !outlet)
        return;

    auto const pstart = getStartPoint();
    auto const pend = getEndPoint();
    // If both inlet and outlet are selected we can move the connection
    if (outobj->isSelected() && inobj->isSelected() && !wasResized) {
        // calculate the offset for moving the whole connection
        auto const pointOffset = pstart - previousPStart;

        // Prevent a repaint if we're not moving
        // This will happen often since there's a move callback from both inlet and outlet
        if (pointOffset.isOrigin())
            return;

        previousPStart = pstart;
        setTopLeftPosition(getPosition() + pointOffset.toInt());

        for (auto& point : currentPlan) {
            point += pointOffset;
        }

        auto const translation = AffineTransform::translation(pointOffset.x, pointOffset.y);

        auto offsetPath = getPath();
        offsetPath.applyTransform(translation);
        setPath(offsetPath);

        updateReconnectHandle();

        clipRegion.transformAll(translation);

        return;
    }

    previousPStart = pstart;
    cachedPath.clear();

    if (currentPlan.size() <= 2) {
        updatePath();
        repaint();
        return;
    }

    bool const isInlet = &component == inlet || &component == inobj;
    int const idx1 = isInlet ? static_cast<int>(currentPlan.size() - 1) : 0;
    int const idx2 = isInlet ? static_cast<int>(currentPlan.size() - 2) : 1;
    auto const& position = isInlet ? pend : pstart;

    if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
        currentPlan[idx2].x = position.x;
    } else {
        currentPlan[idx2].y = position.y;
    }

    currentPlan[idx1] = position;

    if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
        currentPlan[idx2].x = position.x;
    } else {
        currentPlan[idx2].y = position.y;
    }

    currentPlan[idx1] = position;

    updatePath();
    repaint();
}

Point<float> Connection::getStartPoint() const
{
    auto const outletBounds = outlet->getCanvasBounds().toFloat();

    if (getPlugDataLook(*this).isFixedIoletPosition()) {
        return { outletBounds.getX() + getPlugDataLook(*this).getIoletSize() * 0.5f, outletBounds.getCentreY() };
    }
    return outletBounds.getCentre();
}

Point<float> Connection::getEndPoint() const
{
    auto const inletBounds = inlet->getCanvasBounds().toFloat();
    if (getPlugDataLook(*this).isFixedIoletPosition()) {
        return Point<float>(inletBounds.getX() + getPlugDataLook(*this).getIoletSize() * 0.5f, inletBounds.getCentreY());
    }
    return inletBounds.getCentre();
}

Path Connection::getNonSegmentedPath(Point<float> const start, Point<float> const end, bool const useStraightConnections)
{
    Path connectionPath;
    connectionPath.startNewSubPath(start);
    if (!useStraightConnections) {
        float const width = std::max(start.x, end.x) - std::min(start.x, end.x);
        float const height = std::max(start.y, end.y) - std::min(start.y, end.y);

        // Hack for now to hide really poor control point maths
        // So we draw a straight line
        if (end.getDistanceFrom(start) < 4.0f) {
            connectionPath.lineTo(end);
            goto returnPath;
        }

        float const min = std::min<float>(width, height);
        float const max = std::max<float>(width, height);

        constexpr float maxShiftY = 20.f;
        constexpr float maxShiftX = 20.f;

        float shiftY = std::min<float>(maxShiftY, max * 0.5);
        float const shiftX = (start.y >= end.y ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * (start.x < end.x ? -1. : 1.);

        // Adjust control points if they are pointing away from the path
        auto const xPointOffset = std::abs(start.x - end.x);
        auto const yPointOffset = start.y - end.y;
        auto const pathInverted = start.y > end.y;

        if (xPointOffset <= 40.0f && pathInverted) {
            float const xFactor = pow(1.0f - xPointOffset / 40.0f, 0.9f);
            float const yFactor = pow(jmin(1.0f, yPointOffset / 20.0f), 0.9f);
            shiftY = shiftY - xFactor * yFactor * jmax(maxShiftY, yPointOffset * 0.5f);

            if ((xPointOffset <= 1.0f && yPointOffset <= 1.0f) || xPointOffset <= 1.0f || shiftY <= (end.y - start.y) * 0.5f) {
                connectionPath.lineTo(end);
                goto returnPath;
            }
            Point<float> const ctrlPoint1 { start.x - shiftX, start.y + shiftY };
            Point<float> const ctrlPoint2 { end.x + shiftX, end.y - shiftY };

            connectionPath.cubicTo(ctrlPoint1, ctrlPoint2, end);
        } else {
            Point<float> const ctrlPoint1 { start.x - shiftX, start.y + shiftY };
            Point<float> const ctrlPoint2 { end.x + shiftX, end.y - shiftY };

            connectionPath.cubicTo(ctrlPoint1, ctrlPoint2, end);
        }
    } else {
        connectionPath.lineTo(end);
    }

returnPath:
    return connectionPath;
}

int Connection::getNumberOfConnections() const
{
    int count = 0;
    for (auto const* connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count++;
        }
    }
    return count;
}

int Connection::getMultiConnectNumber() const
{
    int count = 0;
    for (auto const* connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count++;
            if (this == connection)
                return count;
        }
    }
    return -1;
}

int Connection::getNumSignalChannels() const
{
    if (auto oc = ptr.get<t_outconnect>()) {
        if (auto const* signal = outconnect_get_signal(oc.get())) {
            return signal->s_nchans;
        }
    }

    if (outlet) {
        return outlet->isSignal() ? 1 : 0;
    }

    return 0;
}

void Connection::updateReconnectHandle()
{
    startReconnectHandle = Rectangle<float>(5, 5).withCentre(path.getPointAlongPath(8.5f));
    endReconnectHandle = Rectangle<float>(5, 5).withCentre(path.getPointAlongPath(jmax(pathLength - 8.5f, 9.5f)));
}

void Connection::updatePath()
{
    if (!outlet || !inlet)
        return;

    auto const pstart = getStartPoint();
    auto const pend = getEndPoint();
    Path toDraw;

    if (!segmented) {
        toDraw = getNonSegmentedPath(pstart, pend, getPlugDataLook(*this).getUseStraightConnections());
        currentPlan.clear();
    } else {
        if (currentPlan.empty()) {
            findPath();
        }

        auto snap = [this](Point<float> const point, int const idx1, int const idx2) {
            if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
                currentPlan[idx2].x = point.x;
            } else {
                currentPlan[idx2].y = point.y;
            }

            currentPlan[idx1] = point;
        };

        snap(pstart, 0, 1);
        snap(pend, static_cast<int>(currentPlan.size() - 1), static_cast<int>(currentPlan.size() - 2));

        Path connectionPath;
        connectionPath.startNewSubPath(pstart);
        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++) {
            connectionPath.lineTo(currentPlan[n].toFloat());
        }
        connectionPath.lineTo(pend);
        // If theme is straight connections, make the rounded as small as the path width
        // Otherwise the path generation will draw the path on-top of the curve (as path flattening happens from centre out)
        toDraw = connectionPath.createPathWithRoundedCorners(getPlugDataLook(*this).getUseStraightConnections() ? getPathWidth() : 8.0f);
    }

    if (getPath() == toDraw) {
        return;
    }

    setPath(toDraw);
    previousPStart = pstart;
    pathLength = toDraw.getLength();

    clipRegion = RectangleList<int>();
    auto pathIter = PathFlatteningIterator(toDraw, AffineTransform(), 12.0f);
    while (pathIter.next()) // skip first item, since only the x2/y2 coords of that one are valid (and they will be the x1/y1 of the next item)
    {
        auto bounds = Rectangle<int>(Point<int>(pathIter.x1, pathIter.y1), Point<int>(pathIter.x2, pathIter.y2));
        clipRegion.add(bounds.expanded(3));
    }

    updateReconnectHandle();

    clipRegion.add(startReconnectHandle.toNearestIntEdges().expanded(4));
    clipRegion.add(endReconnectHandle.toNearestIntEdges().expanded(4));

    cachedPath.clear();
}

bool Connection::intersectsRectangle(Rectangle<int> const rectToIntersect) const
{
    if (rectToIntersect.contains(getBounds()))
        return true;

    return clipRegion.intersectsRectangle(rectToIntersect);
}

void Connection::applyBestPath()
{
    segmented = true;
    findPath();
    updatePath();
    repaint();
}

// Finds the cheapest rectangular route around the obstacles. A route only has to bend next to an
// obstacle, so we only need to search a grid of lanes that run along their edges
PathPlan Connection::findRoute(Point<float> const start, Point<float> const end, SmallArray<Rectangle<float>> const& obstacles) const
{
    // Sorts and deduplicates the lanes, and adds one through the middle of every corridor: routes
    // there look tidier than routes that squeeze past an object
    static auto prepareRouteLanes = [](SmallArray<float>& lanes, float const firstEnd, float const secondEnd, float const gridOrigin, float const gridSize) {
        // The start and end lanes have to stay exact, or the cable won't line up with its iolets
        for (auto& lane : lanes) {
            if (std::abs(lane - firstEnd) < 0.5f)
                lane = firstEnd;
            else if (std::abs(lane - secondEnd) < 0.5f)
                lane = secondEnd;
        }

        std::ranges::sort(lanes);

        SmallArray<float> result;
        for (auto const lane : lanes) {
            if (result.empty() || lane - result.back() > 0.01f)
                result.add(lane);
        }

        auto const numLanes = static_cast<int>(result.size());
        for (int n = 1; n < numLanes; n++) {
            if (result[n] - result[n - 1] < routeClearance * 4.0f)
                continue;

            auto middle = (result[n - 1] + result[n]) * 0.5f;

            // Snap to the canvas grid, if that still keeps clear of both sides
            if (gridSize > 0.0f) {
                auto const snapped = gridOrigin + std::round((middle - gridOrigin) / gridSize) * gridSize;
                if (snapped - result[n - 1] > routeClearance && result[n] - snapped > routeClearance)
                    middle = snapped;
            }

            result.add(middle);
        }

        std::ranges::sort(result);
        lanes = result;
    };

    static auto findLaneIndex = [](SmallArray<float> const& lanes, float const coordinate) {
        for (int n = 0; n < static_cast<int>(lanes.size()); n++) {
            if (approximatelyEqual(lanes[n], coordinate))
                return n;
        }

        return -1;
    };

    auto columns = SmallArray<float> { start.x, end.x };
    auto rows = SmallArray<float> { start.y, end.y };

    for (auto const& obstacle : obstacles) {
        columns.add(obstacle.getX() - routeClearance);
        columns.add(obstacle.getRight() + routeClearance);
        rows.add(obstacle.getY() - routeClearance);
        rows.add(obstacle.getBottom() + routeClearance);
    }

    auto const* settings = SettingsFile::getInstance();
    auto const snapToGrid = settings->getProperty<int>("grid_enabled") && settings->getProperty<int>("grid_type") & 1;
    auto const gridSize = snapToGrid ? static_cast<float>(cnv->objectGrid.gridSize) : 0.0f;

    prepareRouteLanes(columns, start.x, end.x, cnv->canvasOrigin.x, gridSize);
    prepareRouteLanes(rows, start.y, end.y, cnv->canvasOrigin.y, gridSize);

    auto const numColumns = static_cast<int>(columns.size());
    auto const numRows = static_cast<int>(rows.size());
    auto const startColumn = findLaneIndex(columns, start.x);
    auto const startRow = findLaneIndex(rows, start.y);
    auto const endColumn = findLaneIndex(columns, end.x);
    auto const endRow = findLaneIndex(rows, end.y);

    if (numColumns * numRows > routeMaxGridPoints || startColumn < 0 || startRow < 0 || endColumn < 0 || endRow < 0)
        return {};

    // Mark which grid segments run into an obstacle. Obstacles grow by a bit less than the lanes are
    // offset from them, so the lanes alongside an obstacle stay free
    auto blockedHorizontally = HeapArray<uint8_t>(numRows * numColumns, 0);
    auto blockedVertically = HeapArray<uint8_t>(numRows * numColumns, 0);

    for (auto const& obstacle : obstacles) {
        auto const blocked = obstacle.expanded(routeClearance - 1.0f);

        for (int row = 0; row < numRows; row++) {
            if (rows[row] <= blocked.getY() || rows[row] >= blocked.getBottom())
                continue;

            for (int column = 0; column + 1 < numColumns; column++) {
                if (columns[column] < blocked.getRight() && columns[column + 1] > blocked.getX())
                    blockedHorizontally[row * numColumns + column] = true;
            }
        }

        for (int column = 0; column < numColumns; column++) {
            if (columns[column] <= blocked.getX() || columns[column] >= blocked.getRight())
                continue;

            for (int row = 0; row + 1 < numRows; row++) {
                if (rows[row] < blocked.getBottom() && rows[row + 1] > blocked.getY())
                    blockedVertically[row * numColumns + column] = true;
            }
        }
    }

    // Lanes close to an object cost extra, and so do lanes far from the middle of the route, so that
    // cables cross over halfway
    auto const middle = (start + end) * 0.5f;

    auto laneCost = [&obstacles](float const lane, float const middleOfRoute, bool const isColumn) {
        auto nearestObject = routePreferredClearance;

        for (auto const& obstacle : obstacles) {
            auto const distance = isColumn ? std::min(std::abs(lane - obstacle.getX()), std::abs(lane - obstacle.getRight()))
                                           : std::min(std::abs(lane - obstacle.getY()), std::abs(lane - obstacle.getBottom()));
            nearestObject = std::min(nearestObject, distance);
        }

        return routePreferredClearance - nearestObject + std::min(std::abs(lane - middleOfRoute) * 0.05f, 5.0f);
    };

    SmallArray<float> columnCost, rowCost;
    for (auto const column : columns)
        columnCost.add(laneCost(column, middle.x, true));
    for (auto const row : rows)
        rowCost.add(laneCost(row, middle.y, false));

    // Dijkstra, with the direction we arrived from as part of the state, so corners can cost extra
    constexpr int arrivedHorizontally = 0, arrivedVertically = 1;

    auto stateFor = [numColumns](int const column, int const row, int const direction) {
        return (row * numColumns + column) * 2 + direction;
    };

    auto costs = HeapArray<float>(numColumns * numRows * 2, std::numeric_limits<float>::max());
    auto cameFrom = HeapArray<int>(numColumns * numRows * 2, -1);

    // The cable leaves the outlet going down
    auto const firstState = stateFor(startColumn, startRow, arrivedVertically);
    costs[firstState] = 0.0f;

    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> queue;
    queue.emplace(0.0f, firstState);

    auto bestCost = std::numeric_limits<float>::max();
    auto bestState = -1;

    while (!queue.empty()) {
        auto const [cost, state] = queue.top();
        queue.pop();

        if (cost > bestCost)
            break;

        if (cost > costs[state])
            continue;

        auto const direction = state % 2;
        auto const column = state / 2 % numColumns;
        auto const row = state / 2 / numColumns;

        if (column == endColumn && row == endRow) {
            // Arriving sideways costs another corner, the cable enters the inlet from above
            auto const total = cost + (direction == arrivedVertically ? 0.0f : routeBendPenalty + columnCost[endColumn]);
            if (total < bestCost) {
                bestCost = total;
                bestState = state;
            }
            continue;
        }

        auto step = [&](int const nextColumn, int const nextRow, int const nextDirection, bool const isBlocked) {
            if (isBlocked)
                return;

            auto const nextState = stateFor(nextColumn, nextRow, nextDirection);
            auto const length = nextDirection == arrivedVertically ? std::abs(rows[nextRow] - rows[row]) : std::abs(columns[nextColumn] - columns[column]);
            auto const corner = nextDirection == direction ? 0.0f : routeBendPenalty + (nextDirection == arrivedVertically ? columnCost[column] : rowCost[row]);
            auto const nextCost = cost + length + corner;

            if (nextCost < costs[nextState]) {
                costs[nextState] = nextCost;
                cameFrom[nextState] = state;
                queue.emplace(nextCost, nextState);
            }
        };

        if (column > 0)
            step(column - 1, row, arrivedHorizontally, blockedHorizontally[row * numColumns + column - 1]);
        if (column + 1 < numColumns)
            step(column + 1, row, arrivedHorizontally, blockedHorizontally[row * numColumns + column]);
        if (row > 0)
            step(column, row - 1, arrivedVertically, blockedVertically[(row - 1) * numColumns + column]);
        if (row + 1 < numRows)
            step(column, row + 1, arrivedVertically, blockedVertically[row * numColumns + column]);
    }

    if (bestState < 0)
        return {};

    PathPlan route;
    for (auto state = bestState; state >= 0; state = cameFrom[state]) {
        auto const column = state / 2 % numColumns;
        auto const row = state / 2 / numColumns;
        route.emplace_back(columns[column], rows[row]);
    }

    std::ranges::reverse(route);
    return route;
}

// Fallback for when there's no way around: whichever L or Z shape runs into the fewest objects
PathPlan Connection::findSimpleRoute(Point<float> const start, Point<float> const end, SmallArray<Rectangle<float>> const& obstacles)
{
    static auto routeIntersectsObstacle = [](Line<float> const segment, SmallArray<Rectangle<float>> const& obstacles) {
        for (auto const& obstacle : obstacles) {
            if (obstacle.expanded(routeClearance - 1.0f).intersects(segment))
                return true;
        }

        return false;
    };

    auto const middle = (start + end) * 0.5f;

    auto const candidates = SmallArray<PathPlan> {
        PathPlan { start, { start.x, middle.y }, { end.x, middle.y }, end },
        PathPlan { start, { middle.x, start.y }, { middle.x, end.y }, end },
        PathPlan { start, { start.x, end.y }, end },
        PathPlan { start, { end.x, start.y }, end }
    };

    PathPlan bestRoute;
    auto bestCost = std::numeric_limits<float>::max();

    for (auto const& candidate : candidates) {
        auto cost = 0.0f;

        for (int n = 1; n < static_cast<int>(candidate.size()); n++) {
            auto const segment = Line<float>(candidate[n - 1], candidate[n]);
            cost += segment.getLength() + (n > 1 ? routeBendPenalty : 0.0f);

            if (routeIntersectsObstacle(segment, obstacles))
                cost += routeBlockedPenalty;
        }

        if (cost < bestCost) {
            bestCost = cost;
            bestRoute = candidate;
        }
    }

    return bestRoute;
}

void Connection::findPath()
{
    // The connected objects count as obstacles too, so a cable that runs backwards goes around them.
    // Objects that contain the start or end of the route are skipped, there'd be no way out of those
    static auto getRouteObstacles = [](PooledPtrArray<Object>& objects, Rectangle<float> const searchBounds, Point<float> const start, Point<float> const end) {
        SmallArray<Rectangle<float>> obstacles;

        for (auto const* object : objects) {
            auto const bounds = object->getBounds().toFloat().reduced(Object::margin);

            if (!bounds.intersects(searchBounds))
                continue;

            auto const withClearance = bounds.expanded(routeClearance);
            if (withClearance.contains(start) || withClearance.contains(end))
                continue;

            obstacles.add(bounds);
        }

        // Only keep the closest objects, so the search stays fast in busy patches
        if (obstacles.size() > routeMaxObstacles) {
            auto const centre = searchBounds.getCentre();
            std::ranges::sort(obstacles, [centre](auto const& lhs, auto const& rhs) {
                return lhs.getCentre().getDistanceSquaredFrom(centre) < rhs.getCentre().getDistanceSquaredFrom(centre);
            });
            obstacles.resize(routeMaxObstacles);
        }

        return obstacles;
    };

    if (!outlet || !inlet)
        return;

    auto const pstart = getStartPoint();
    auto const pend = getEndPoint();

    // Leave the outlet and enter the inlet with a short straight segment, if there's room for it
    auto const verticalDistance = pend.y - pstart.y;
    auto const stubLength = verticalDistance > 0.0f ? std::min(routeStubLength, verticalDistance * 0.4f) : routeStubLength;

    auto const start = pstart.translated(0.0f, stubLength);
    auto const end = pend.translated(0.0f, -stubLength);

    auto const obstacles = getRouteObstacles(cnv->objects, Rectangle<float>(start, end).expanded(routeDetourMargin), start, end);

    auto route = findRoute(start, end, obstacles);
    if (route.size() < 2)
        route = findSimpleRoute(start, end, obstacles);

    currentPlan.clear();
    currentPlan.add(pstart);
    for (auto const& point : route)
        currentPlan.add(point);
    currentPlan.add(pend);

    sanitisePlan();

    // A straight cable needs a corner to bend on when the objects move apart
    if (getNumCorners() == 0) {
        auto const middleY = (pstart.y + pend.y) * 0.5f;
        currentPlan = PathPlan { pstart, { pstart.x, middleY }, { pend.x, middleY }, pend };
    }

    pushPathState();
}

void ConnectionPathUpdater::timerCallback()
{
    stopTimer();

    std::pair<Component::SafePointer<Connection>, t_symbol*> currentConnection;

    canvas->patch.startUndoSequence("SetConnectionPaths");

    while (connectionUpdateQueue.try_dequeue(currentConnection)) {

        auto& [connection, newPathState] = currentConnection;

        if (!connection)
            continue;

        t_linetraverser t;

        if (auto patch = connection->cnv->patch.getPointer()) {
            int inIdx;
            t_object* inObj;
            int outIdx;
            t_object* outObj;
            bool found = false;

            // Get connections from pd
            linetraverser_start(&t, patch.get());

            while (auto const* oc = linetraverser_next_nosize(&t)) {

                if (oc == connection->ptr.getRaw<t_outconnect>()) {

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

            if (auto oc = connection->ptr.get<t_outconnect>()) {
                t_symbol* oldPathState = outconnect_get_path_data(oc.get());
                auto* newConnection = connection->cnv->patch.setConnctionPath(outObj, outIdx, inObj, inIdx, oldPathState, newPathState);
                connection->setPointer(newConnection);
            }
        }
    }

    canvas->patch.endUndoSequence("SetConnectionPaths");
}

void Connection::receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms)
{
    if (cnv->shouldShowConnectionActivity()) {
        activityStateAnimator.start();
    }

    outobj->triggerOverlayActiveState();
    lastValue = atoms;
    lastSelector = symbol;
}

void ConnectionBeingCreated::scrollViewport(Component* cnvComp, MouseEvent const& e)
{
#if JUCE_MAC || JUCE_WINDOWS
    beginDragAutoRepeat(25); // Doing this leads to terrible performance on Linux, unfortunately
#endif
    auto* cnv = static_cast<Canvas*>(cnvComp);
    cnv->autoscroll(e.getEventRelativeTo(cnv->viewport.get()));
}
