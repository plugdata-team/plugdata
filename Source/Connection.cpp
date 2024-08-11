/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "NVGSurface.h"
#include "Connection.h"

#include "Canvas.h"
#include "Iolet.h"
#include "Object.h"
#include "PluginProcessor.h"
#include "PluginEditor.h" // might not need this?
#include "Pd/Patch.h"
#include "Dialogs/ConnectionMessageDisplay.h"

Connection::Connection(Canvas* parent, Iolet* s, Iolet* e, t_outconnect* oc)
    : NVGComponent(this)
    , inlet(s->isInlet ? s : e)
    , outlet(s->isInlet ? e : s)
    , inobj(inlet->object)
    , outobj(outlet->object)
    , cnv(parent)
    , ptr(parent->pd)
{
    cnv->selectedComponents.addChangeListener(this);

    locked.referTo(parent->locked);
    presentationMode.referTo(parent->presentationMode);

    // Make sure it's not 2x the same iolet
    if (!outlet || !inlet || outlet->isInlet == inlet->isInlet) {
        outlet = nullptr;
        inlet = nullptr;
        jassertfalse;
        return;
    }

    cableType = DataCable;

    if (outlet && outlet->isSignal) {
        cableType = SignalCable;
    }
    if (outlet && outlet->isGemState) {
        cableType = GemCable;
    }

    setStrokeThickness(12.0f); // This will make sure the DrawablePath's bounds get expanded, which we use for hit detection and drawing reconnect handles

    inIdx = inlet->ioletIdx;
    outIdx = outlet->ioletIdx;

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

    setInterceptsMouseClicks(true, true);

    addMouseListener(cnv, true);

    cnv->connectionLayer.addAndMakeVisible(this);

    setAccessible(false); // TODO: implement accessibility. We disable default, since it makes stuff slow on macOS
    lookAndFeelChanged();
}

Connection::~Connection()
{
    cnv->pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
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

    auto* nvg = cnv->editor->nvgSurface.getRawContext();
    if (nvg && cacheId >= 0)
        nvgDeletePath(nvg, cacheId);
    if (nvg && cacheId >= 0 && cableType == SignalCable) {
        nvgDeletePath(nvg, std::numeric_limits<int32_t>::max() - cacheId);
    }
}

void Connection::changeListenerCallback(ChangeBroadcaster* source)
{
    if (auto selectedItems = dynamic_cast<SelectedItemSet<WeakReference<Component>>*>(source))
        setSelected(selectedItems->isSelected(this));
}

void Connection::lookAndFeelChanged()
{
    baseColour = convertColour(findColour(PlugDataColour::connectionColourId));
    dataColour = convertColour(findColour(PlugDataColour::dataColourId));
    signalColour = convertColour(findColour(PlugDataColour::signalColourId));
    handleColour = outlet->isSignal ? dataColour : signalColour;
    shadowColour = convertColour(findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.06f).withAlpha(0.24f));
    outlineColour = convertColour(findColour(PlugDataColour::objectOutlineColourId));
    gemColour = convertColour(findColour(PlugDataColour::gemColourId));

    textColour = convertColour(findColour(PlugDataColour::objectSelectedOutlineColourId).contrasting());

    if (connectionStyle != PlugDataLook::getConnectionStyle()){
        connectionStyle = PlugDataLook::getConnectionStyle();
        cachedIsValid = false;
    }

    updatePath();
    repaint();
}

void Connection::render(NVGcontext* nvg)
{
    connectionColour = baseColour;
    if (isSelected() || isHovering) {
        if (outlet->isSignal) {
            connectionColour = signalColour;
        } else if (outlet->isGemState) {
            connectionColour = gemColour;
        } else {
            connectionColour = dataColour;
        }
    }

    if (isHovering) {
        connectionColour.r *= 1.2f;
        connectionColour.g *= 1.2f;
        connectionColour.b *= 1.2f;
    }

    nvgSave(nvg);
    nvgTranslate(nvg, getX(), getY());

    bool isSignalCable = cableType == SignalCable && connectionStyle != PlugDataLook::ConnectionStyleVanilla;
    auto dashColor = shadowColour;
    if (isSignalCable){
        dashColor.a = 1.0f;
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

        nvgBeginPath(nvg);
        nvgFillColor(nvg, shadowColour);
        nvgCircle(nvg, startPoint.x, startPoint.y, cableThickness * 0.5f); // cableThickness is diameter, while circle is radius
        nvgFill(nvg);

        nvgBeginPath(nvg);
        nvgFillColor(nvg, connectionColour);
        nvgCircle(nvg, startPoint.x, startPoint.y, cableThickness * 0.25f);
        nvgFill(nvg);
        return;
    }

    float dashSize = isSignalCable ? (numSignalChannels <= 1) ? 2.5f : 1.5f : 0.0f;
    auto useGradientLook = PlugDataLook::getUseGradientConnectionLook() && !(isSelected() || isHovering);
    auto showActivity = cableType == DataCable && cnv->shouldShowConnectionActivity();
    nvgStrokePaint(nvg, nvgDoubleStroke(nvg, connectionColour, shadowColour, dashColor, dashSize, useGradientLook, showActivity, offset));
    nvgStrokeWidth(nvg, cableThickness);

    if (!cachedIsValid)
        nvgDeletePath(nvg, cacheId);
    
    bool cacheHit = nvgStrokeCachedPath(nvg, cacheId);
    if (!cacheHit) {
        auto pathFromOrigin = getPath();
        pathFromOrigin.applyTransform(AffineTransform::translation(-getX(), -getY()));

        setJUCEPath(nvg, pathFromOrigin);
        nvgStroke(nvg);
        cacheId = nvgSavePath(nvg, cacheId);
    }
    
    nvgRestore(nvg);
    cachedIsValid = true;

    if (isSelected() && isHovering) {
        auto expandedStartHandle = isInStartReconnectHandle ? startReconnectHandle.expanded(3.0f) : startReconnectHandle;
        auto expandedEndHandle = isInEndReconnectHandle ? endReconnectHandle.expanded(3.0f) : endReconnectHandle;

        nvgFillColor(nvg, handleColour);

        nvgBeginPath(nvg);
        nvgCircle(nvg, expandedStartHandle.getCentreX(), expandedStartHandle.getCentreY(), expandedStartHandle.getWidth() / 2);
        nvgFill(nvg);

        nvgBeginPath(nvg);
        nvgCircle(nvg, expandedEndHandle.getCentreX(), expandedEndHandle.getCentreY(), expandedEndHandle.getWidth() / 2);
        nvgFill(nvg);
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
    float const arrowWidth = 8.0f;
    float const arrowLength = 12.0f;

    auto renderArrow = [this, nvg, arrowLength, arrowWidth](Path& path, float connectionLength) {
        // get the center point of the connection path

        const auto arrowCenter = connectionLength * 0.5f;
        const auto arrowBase = path.getPointAlongPath(arrowCenter - (arrowLength * 0.5f));
        const auto arrowTip = path.getPointAlongPath(arrowCenter + (arrowLength * 0.5f));

        const Line<float> arrowLine(arrowBase, arrowTip);
        const auto point_a = cnv->getLocalPoint(this, arrowTip);
        const auto point_b = cnv->getLocalPoint(this, arrowLine.getPointAlongLine(0.0f, -(arrowWidth * 0.5f)));
        const auto point_c = cnv->getLocalPoint(this, arrowLine.getPointAlongLine(0.0f, (arrowWidth * 0.5f)));

        // draw the arrow
        nvgBeginPath(nvg);
        nvgStrokeColor(nvg, outlineColour);
        nvgFillColor(nvg, connectionColour);
        nvgMoveTo(nvg, point_a.x, point_a.y);
        nvgLineTo(nvg, point_b.x, point_b.y);
        nvgLineTo(nvg, point_c.x, point_c.y);
        nvgClosePath(nvg);
        nvgStrokeWidth(nvg, 1.0f);
        nvgFill(nvg);
        nvgStroke(nvg);
    };

    // TODO: refactor this outside of the render function
    if (cnv->shouldShowConnectionDirection()) {
        if (isSegmented()) {
            for (int i = 1; i < currentPlan.size(); i++) {
                auto const pathLine = Line<float>(currentPlan[i - 1], currentPlan[i]);
                auto const length = pathLine.getLength();
                // don't show arrow if start or end segment is too small, to give room for the reconnect handle
                auto const isStartOrEnd = (i == 1) || (i == currentPlan.size() - 1);
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

//#define BEZIER_DEBUG
#ifdef BEZIER_DEBUG
    auto getCubicBezierControlPoints = [this]() -> Array<Point<float>> {
        juce::Path::Iterator it(getPath());

        Array<Point<float>> points;

        while (it.next())
        {
            if (it.elementType == juce::Path::Iterator::cubicTo)
            {
                points.add( { it.x1, it.y1 } );
                points.add( { it.x2, it.y2 } );
            }
        }
        return points;
    };

    for (auto point : getCubicBezierControlPoints()) {
        nvgBeginPath(nvg);
        nvgCircle(nvg, point.x, point.y, 0.5f);
        nvgFillColor(nvg, nvgRGBAf(1,0,0,1));
        nvgFill(nvg);
    }
#endif
}

void Connection::renderConnectionOrder(NVGcontext* nvg)
{
    if ((cableType == DataCable) && (getNumberOfConnections() > 1)) {
        auto connectionPath = getPath();
        connectionPath.applyTransform(AffineTransform::translation(-getX(), -getY()));
        auto pos = cnv->getLocalPoint(this, connectionPath.getPointAlongPath(jmax(pathLength - 8.5f * 3, 9.5f)));
        // circle background
        nvgBeginPath(nvg);
        nvgStrokeColor(nvg, outlineColour);
        nvgFillColor(nvg, connectionColour);
        auto const radius = 7.0f;
        auto const diameter = radius * 2.0f;
        auto const circleTopLeft = pos - Point<float>(radius, radius);
        nvgRoundedRect(nvg, circleTopLeft.getX(), circleTopLeft.getY(), diameter, diameter, radius);
        nvgStrokeWidth(nvg, 1.0f);
        nvgFill(nvg);
        nvgStroke(nvg);
        // connection index number
        nvgFillColor(nvg, textColour);
        nvgFontSize(nvg, 9.0f);
        nvgTextAlign(nvg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
        nvgText(nvg, pos.getX(), pos.getY(), String(getMultiConnectNumber()).toUTF8(), nullptr);
    }
}

void Connection::pushPathState()
{
    if (!inlet || !outlet)
        return;

    t_symbol* newPathState;
    if (segmented) {
        MemoryOutputStream stream;

        for (auto& point : currentPlan) {
            stream.writeInt(point.x - outlet->getCanvasBounds().getCentre().x);
            stream.writeInt(point.y - outlet->getCanvasBounds().getCentre().y);
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
    if (!inlet || !outlet)
        return;

    String state;
    if (auto oc = ptr.get<t_outconnect>()) {
        auto* pathData = outconnect_get_path_data(oc.get());
        if (!pathData || !pathData->s_name)
            return;
        state = String::fromUTF8(pathData->s_name);
    }

    auto block = MemoryBlock();
    auto succeeded = block.fromBase64Encoding(state);

    auto plan = PathPlan();

    if (succeeded) {
        auto stream = MemoryInputStream(block, false);

        while (!stream.isExhausted()) {
            auto x = stream.readInt();
            auto y = stream.readInt();

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
    auto originalPointer = ptr.getRawUnchecked<t_outconnect>();
    if (originalPointer != newPtr) {
        ptr = pd::WeakReference(newPtr, cnv->pd);

        cnv->pd->unregisterMessageListener(originalPointer, this);
        cnv->pd->registerMessageListener(newPtr, this);
    }
}

t_outconnect* Connection::getPointer()
{
    return ptr.getRaw<t_outconnect>();
}

t_symbol* Connection::getPathState()
{
    if (auto oc = ptr.get<t_outconnect>()) {
        return outconnect_get_path_data(oc.get());
    }

    return nullptr;
}

bool Connection::hitTest(int x, int y)
{
    if (inlet == nullptr || outlet == nullptr)
        return false;

    if (cnv->panningModifierDown())
        return false;

    if (cnv->commandLocked == var(true) || locked == var(true) || !cnv->connectionsBeingCreated.isEmpty())
        return false;

    Point<float> position = Point<float>(static_cast<float>(x), static_cast<float>(y)) + getPosition().toFloat();

    Point<float> nearestPoint;

    auto path = getPath();
    path.getNearestPoint(position, nearestPoint);

    // Get outlet and inlet point
    auto pstart = getStartPoint();
    auto pend = getEndPoint();

    if (selectedFlag && (startReconnectHandle.contains(position) || endReconnectHandle.contains(position))) {
        repaint();
        return true;
    }

    // If we click too close to the inlet, don't register the click on the connection
    if (pstart.getDistanceFrom(position) < 8.0f || pend.getDistanceFrom(position) < 8.0f)
        return false;

    return nearestPoint.getDistanceFrom(position) < 3;
}

bool Connection::intersects(Rectangle<float> toCheck, int accuracy) const
{
    PathFlatteningIterator i(getPath());

    while (i.next()) {
        auto point1 = Point<float>(i.x1, i.y1);

        // Skip points to reduce accuracy a bit for better performance
        // We can only skip points if there are many points!
        if (!PlugDataLook::getUseStraightConnections()) {
            for (int n = 0; n < accuracy; n++) {
                auto next = i.next();
                if (!next)
                    break;
            }
         }

        auto point2 = Point<float>(i.x2, i.y2);

        auto currentLine = Line<float>(point1, point2);

        if (toCheck.intersects(currentLine)) {
            return true;
        }
    }

    return false;
}

void Connection::forceUpdate(bool updateCacheOnly)
{
    if (updateCacheOnly) {
        cachedIsValid = false;
    } else {
        updatePath();
    }

    repaint();
}

bool Connection::isSegmented() const
{
    return segmented;
}

void Connection::setSegmented(bool isSegmented)
{
    segmented = isSegmented;
    updatePath();
    repaint();
    pushPathState();
}

void Connection::setSelected(bool shouldBeSelected)
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
    auto setReconnectFlag = [this](bool start, bool end){
        if (isInStartReconnectHandle != start || isInEndReconnectHandle != end) {
            isInStartReconnectHandle = start;
            isInEndReconnectHandle = end;
            repaint();
        }
    };

    if (startReconnectHandle.contains(e.getPosition().toFloat().translated(getX(), getY()))) {
        setReconnectFlag(true, false);
    }
    else if (endReconnectHandle.contains(e.getPosition().toFloat().translated(getX(), getY()))) {
        setReconnectFlag(false, true);
    } else {
        setReconnectFlag(false, false);
    }

    if (isInStartReconnectHandle || isInEndReconnectHandle) {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }

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
}

void Connection::timerCallback(int ID)
{
    switch (ID) {
    case StopAnimation:
        stopTimer(Animation);
        stopTimer(StopAnimation);
        break;
    case Animation:
        animate();
        break;
    default:
        break;
    }
}

void Connection::animate()
{
    offset += 0.1f;
    if (offset >= 1.0f)
        offset = 0.0f;
    repaint();
}

StringArray Connection::getMessageFormated()
{
    auto args = lastValue;
    auto name = lastSelector ? String::fromUTF8(lastSelector->s_name) : "";

    StringArray formatedMessage;

    if (name == "float" && lastNumArgs > 0) {
        formatedMessage.add("float:");
        formatedMessage.add(args[0].toString());
    } else if (name == "symbol" && lastNumArgs > 0) {
        formatedMessage.add("symbol:");
        formatedMessage.add(args[0].toString());
    } else if (name == "list") {
        if (lastNumArgs >= 8) {
            formatedMessage.add("list (7+):");
        } else {
            formatedMessage.add("list (" + String(lastNumArgs) + "):");
        }
        for (int arg = 0; arg < lastNumArgs; arg++) {
            if (args[arg].isFloat()) {
                formatedMessage.add(String(args[arg].getFloat()));
            } else if (args[arg].isSymbol()) {
                formatedMessage.add(args[arg].toString());
            }
        }
        if (lastNumArgs >= 8) {
            formatedMessage.add("...");
        }
    } else {
        formatedMessage.add(name);
        for (int arg = 0; arg < lastNumArgs; arg++) {
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
        cnv->editor->connectionMessageDisplay->setConnection(this, cnv->localPointToGlobal(nearest).roundToInt().translated(60, 15));
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
    if(e.mods.isShiftDown() && e.getNumberOfClicks() == 2 && cnv->getSelectionOfType<Connection>().size() == 2)
    {
        if (auto oc = ptr.get<t_outconnect>()) {
            auto* patch = cnv->patch.getPointer().get();
            auto* other = cnv->getSelectionOfType<Connection>()[0]->getPointer();
            if(patch && other) {
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
    cnv->editor->connectionMessageDisplay->setConnection(nullptr);

    bool isDragging = e.getDistanceFromDragStart() > 6;

    if (selectedFlag && isInStartReconnectHandle) {
        if (isDragging) {
            cnv->connectingWithDrag = true;
            reconnect(inlet);
        }
        return;
    }
    if (selectedFlag && isInEndReconnectHandle) {
        if (isDragging) {
            cnv->connectingWithDrag = true;
            reconnect(outlet);
        }
        return;
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

        if (line.getDistanceFromPoint(cnv->getLocalPoint(this, position), nearest) < 3) {
            return n;
        }
    }

    return -1;
}

void Connection::pathChanged()
{
    strokePath.clear();
    strokeType.createStrokedPath (strokePath, path, AffineTransform(), 1.0f);
    setBoundsToEnclose (getDrawableBounds());
    repaint();
}

float const Connection::getPathWidth() {
    switch (connectionStyle) {
        case PlugDataLook::ConnectionStyleVanilla:
            return (cableType == SignalCable) ? 4.5f : 2.5f;
            break;
        case PlugDataLook::ConnectionStyleThin:
            return 3.0f;
            break;
        default:
            return 4.5f;
            break;
    }
}

void Connection::reconnect(Iolet* target)
{
    if (!reconnecting.isEmpty() || !target)
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

        auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
        auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

        if (checkedOut && checkedIn && cnv->patch.hasConnection(checkedOut, c->outIdx, checkedIn, c->inIdx)) {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
        }

        // Create new connection
        cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(target->isInlet ? c->inlet : c->outlet, cnv));

        c->setVisible(false);

        reconnecting.add(SafePointer(c));

        // Make sure we're deselected and remove object
        cnv->setSelected(c, false, false);
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    if (!inlet || !outlet)
        return;

    auto pstart = getStartPoint();
    auto pend = getEndPoint();
    // If both inlet and outlet are selected we can move the connection
    if (outobj->isSelected() && inobj->isSelected() && !wasResized) {
        // calculate the offset for moving the whole connection
        auto pointOffset = pstart - previousPStart;

        // Prevent a repaint if we're not moving
        // This will happen often since there's a move callback from both inlet and outlet
        if (pointOffset.isOrigin())
            return;

        previousPStart = pstart;
        setTopLeftPosition(getPosition() + pointOffset.toInt());

        for (auto& point : currentPlan) {
            point += pointOffset;
        }

        auto translation = AffineTransform::translation(pointOffset.x, pointOffset.y);

        auto offsetPath = getPath();
        offsetPath.applyTransform(translation);
        setPath(offsetPath);

        updateReconnectHandle();

        clipRegion.transformAll(translation);

        return;
    }

    previousPStart = pstart;
    cachedIsValid = false;

    if (currentPlan.size() <= 2) {
        updatePath();
        repaint();
        return;
    }

    bool isInlet = &component == inlet || &component == inobj;
    int idx1 = isInlet ? static_cast<int>(currentPlan.size() - 1) : 0;
    int idx2 = isInlet ? static_cast<int>(currentPlan.size() - 2) : 1;
    auto& position = isInlet ? pend : pstart;

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
    auto outletBounds = outlet->getCanvasBounds().toFloat();

    if (PlugDataLook::isFixedIoletPosition()) {
        return Point<float>(outletBounds.getX() + PlugDataLook::ioletSize * 0.5f, outletBounds.getCentreY());
    }

    return outletBounds.getCentre();
}

Point<float> Connection::getEndPoint() const
{
    auto inletBounds = inlet->getCanvasBounds().toFloat();
    if (PlugDataLook::isFixedIoletPosition()) {
        return Point<float>(inletBounds.getX() + PlugDataLook::ioletSize * 0.5f, inletBounds.getCentreY());
    }
    return inletBounds.getCentre();
}

Path Connection::getNonSegmentedPath(Point<float> start, Point<float> end)
{
    Path connectionPath;
    connectionPath.startNewSubPath(start);
    if (!PlugDataLook::getUseStraightConnections()) {
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

        float const maxShiftY = 20.f;
        float const maxShiftX = 20.f;

        float shiftY = std::min<float>(maxShiftY, max * 0.5);
        float shiftX = ((start.y >= end.y) ? std::min<float>(maxShiftX, min * 0.5) : 0.f) * ((start.x < end.x) ? -1. : 1.);

        // Adjust control points if they are pointing away from the path
        auto xPointOffset = std::abs(start.x - end.x);
        auto yPointOffset = start.y - end.y;
        auto pathInverted = start.y > end.y;

        if (xPointOffset <= 40.0f && pathInverted) {
            float xFactor = pow(1.0f - (xPointOffset / 40.0f), 0.9f);
            float yFactor = pow((jmin(1.0f, yPointOffset / 20.0f)), 0.9f);
            shiftY = shiftY - xFactor * yFactor * jmax(maxShiftY, yPointOffset * 0.5f);

            if ((xPointOffset <= 1.0f && yPointOffset <= 1.0f) || xPointOffset <= 1.0f || shiftY <= (end.y - start.y) * 0.5f) {
                connectionPath.lineTo(end);
                goto returnPath;
            }
            Point<float> const ctrlPoint1{start.x - shiftX, start.y + shiftY};
            Point<float> const ctrlPoint2{end.x + shiftX, end.y - shiftY};

            connectionPath.cubicTo(ctrlPoint1, ctrlPoint2, end);
        } else {
            Point<float> const ctrlPoint1{start.x - shiftX, start.y + shiftY};
            Point<float> const ctrlPoint2{end.x + shiftX, end.y - shiftY};

            connectionPath.cubicTo(ctrlPoint1, ctrlPoint2, end);
        }
    } else {
        connectionPath.lineTo(end);
    }

    returnPath:
    return connectionPath;
}

int Connection::getNumberOfConnections()
{
    int count = 0;
    for (auto* connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count++;
        }
    }
    return count;
}

int Connection::getMultiConnectNumber()
{
    int count = 0;
    for (auto* connection : cnv->connections) {
        if (outlet == connection->outlet) {
            count++;
            if (this == connection)
                return count;
        }
    }
    return -1;
}

int Connection::getSignalData(t_float* output, int maxChannels)
{
    if (auto oc = ptr.get<t_outconnect>()) {
        if (auto* signal = outconnect_get_signal(oc.get())) {
            auto numChannels = std::min(signal->s_nchans, maxChannels - 1);
            auto* samples = signal->s_vec;
            if (!samples)
                return 0;
            std::copy(samples, samples + (DEFDACBLKSIZE * numChannels), output);
            return numChannels;
        }
    }

    return 0;
}

int Connection::getNumSignalChannels()
{
    if (auto oc = ptr.get<t_outconnect>()) {
        if (auto* signal = outconnect_get_signal(oc.get())) {
            return signal->s_nchans;
        }
    }

    if (outlet) {
        return outlet->isSignal ? 1 : 0;
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

    auto pstart = getStartPoint();
    auto pend = getEndPoint();
    Path toDraw;

    if (!segmented) {
        toDraw = getNonSegmentedPath(pstart, pend);
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
        toDraw = connectionPath.createPathWithRoundedCorners(PlugDataLook::getUseStraightConnections() ? getPathWidth() : 8.0f);
    }
    
    if(getPath() == toDraw) {
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

    cachedIsValid = false;
}

bool Connection::intersectsRectangle(Rectangle<int> rectToIntersect)
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

        direction = approximatelyEqual(bestPath[0].x, bestPath[1].x);

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
            simplifiedPath.emplace_back(pend.x + xHalfDistance, pend.y);
            simplifiedPath.emplace_back(pend.x + xHalfDistance, pstart.y);
            simplifiedPath.push_back(pstart);
            simplifiedPath.push_back(pstart);
        } else {
            int yHalfDistance = (pstart.y - pend.y) / 2;
            simplifiedPath.push_back(pend);
            simplifiedPath.emplace_back(pend.x, pend.y + yHalfDistance);
            simplifiedPath.emplace_back(pstart.x, pend.y + yHalfDistance);
            simplifiedPath.push_back(pstart);
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
    stopTimer();

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

        if(auto patch = connection->cnv->patch.getPointer()) {
            
            // Get connections from pd
            linetraverser_start(&t, patch.get());
            
            while (auto* oc = linetraverser_next_nosize(&t)) {
                
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

void Connection::receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms)
{
    if (cnv->shouldShowConnectionActivity()) {
        startTimer(StopAnimation, 1000 / 8.0f);
        if (!isTimerRunning(Animation)) {
            startTimer(Animation, 1000 / 60.0f);
            animate();
        }
    }

    outobj->triggerOverlayActiveState();
    std::copy(atoms, atoms + numAtoms, lastValue);
    lastNumArgs = numAtoms;
    lastSelector = symbol;
}
