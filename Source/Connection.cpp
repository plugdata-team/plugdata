/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include <nanovg.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/NVGComponent.h"

#include "Connection.h"

#include "Canvas.h"
#include "Iolet.h"
#include "Object.h"
#include "PluginProcessor.h"
#include "PluginEditor.h" // might not need this?
#include "LookAndFeel.h"
#include "Pd/Patch.h"
#include "Dialogs/ConnectionMessageDisplay.h"

Connection::Connection(Canvas* parent, Iolet* start)
    : NVGComponent(static_cast<Component&>(*this))
    , inlet(start->isInlet ? start : nullptr)
    , outlet(start->isInlet ? nullptr : start)
    , cnv(parent)
    , ptr(parent->pd)
    , generateHitPath(false)
{
    setInterceptsMouseClicks(false, true);
    //cnv->addMouseListener(this, true);
    if (outlet)
        outlet->addMouseListener(this, true);
    else
        inlet->addMouseListener(this, true);

    cnv->connectionLayer.addAndMakeVisible(this);

    // init colours first time
    lookAndFeelChanged();
}

Connection::Connection(Canvas* parent, Iolet* s, Iolet* e, t_outconnect* oc)
    : NVGComponent(static_cast<Component&>(*this))
    , inlet(s->isInlet ? s : e)
    , outlet(s->isInlet ? e : s)
    , inobj(inlet->object)
    , outobj(outlet->object)
    , cnv(parent)
    , ptr(parent->pd)
    , generateHitPath(true)
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
    //outlet->addComponentListener(this);
    //inlet->addComponentListener(this);

    setInterceptsMouseClicks(true, true);

    cnv->connectionLayer.addAndMakeVisible(this);

    //componentMovedOrResized(*outlet, true, true);
    //componentMovedOrResized(*inlet, true, true);

    updateOverlays(cnv->getOverlays());

    lookAndFeelChanged();

    updateBounds();
}

Connection::~Connection()
{
    cnv->pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
    cnv->selectedComponents.removeChangeListener(this);

    if (outlet) {
        outlet->repaint();
        outlet->removeComponentListener(this);
        outlet->removeMouseListener(this);
    }
    if (outobj) {
        outobj->removeComponentListener(this);
    }

    if (inlet) {
        inlet->repaint();
        inlet->removeComponentListener(this);
        inlet->removeMouseListener(this);
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

void Connection::lookAndFeelChanged()
{
    baseColour = cnv->findColour(PlugDataColour::connectionColourId);
    dataColour = cnv->findColour(PlugDataColour::dataColourId);
    signalColour = cnv->findColour(PlugDataColour::signalColourId);
    if (outlet)
        handleColour = outlet->isSignal ? dataColour : signalColour;
    shadowColour = findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.06f).withAlpha(0.24f);
}

Point<float> Connection::bezierPointAtDistance(const Point<float>& start, const Point<float>& cp1,
                                         const Point<float>& cp2, const Point<float>& end,
                                         float distance) {
    // De Casteljau's algorithm to find a point at parameter 't' along the Bezier curve
    auto deCasteljau = [](const Point<float>& p0, const Point<float>& p1,
                          const Point<float>& p2, const Point<float>& p3, float t) {
        Point<float> result =
            p0 * std::pow(1.0f - t, 3) +
            p1 * (3.0f * std::pow(1.0f - t, 2) * t) +
            p2 * (3.0f * (1.0f - t) * std::pow(t, 2)) +
            p3 * std::pow(t, 3);
        return result;
    };

    // Function to calculate the length of the Bezier curve between t1 and t2
    auto calculateSegmentLength = [&](float t1, float t2) {
        float step = 0.01f;
        float segmentLength = 0.0f;

        Point<float> prevPoint = deCasteljau(start, cp1, cp2, end, t1);
        for (float t = t1 + step; t <= t2; t += step) {
            Point<float> currentPoint = deCasteljau(start, cp1, cp2, end, t);

            segmentLength += prevPoint.getDistanceFrom(currentPoint);
            prevPoint = currentPoint;
        }

        return segmentLength;
    };
    
    // Use binary search to find the correct t value
    float epsilon = 0.0001f;
    float minT = 0.0f;
    float maxT = 1.0f;
    float midT = 0.5f;

    while (maxT - minT > epsilon) {
        float lengthAtMidT = calculateSegmentLength(0.0f, midT);
        if (lengthAtMidT < distance) {
            minT = midT;
        } else {
            maxT = midT;
        }
        midT = (minT + maxT) / 2.0f;
    }

    // Use the found t value to calculate the point
    Point<float> point = deCasteljau(start, cp1, cp2, end, midT);

    return point;
}


void Connection::render(NVGcontext* nvg)
{
    auto finalBaseColour = baseColour;
    if (inobj && outobj) {
        if (isSelected() || isMouseOver()) {

            if (outlet->isSignal) {
                finalBaseColour = signalColour;
            } else if (outlet->isGemState) {
                finalBaseColour = cnv->findColour(PlugDataColour::gemColourId);
            } else {
                finalBaseColour = dataColour;
            }
        }
        if (isMouseOver()) {
            finalBaseColour = finalBaseColour.brighter(0.6f);
        }
    }

    auto drawStraightConnection = [this, nvg, finalBaseColour](){
        nvgBeginPath(nvg);
        nvgLineStyle(nvg, NVG_LINE_SOLID);
        nvgMoveTo(nvg, start_.x, start_.y);
        nvgLineTo(nvg, end_.x, end_.y);
        nvgStrokeColor(nvg, convertColour(shadowColour));
        nvgLineCap(nvg, NVG_ROUND);
        nvgStrokeWidth(nvg, 4.0f);
        nvgStroke(nvg);

        nvgStrokeColor(nvg, convertColour(finalBaseColour));
        nvgStrokeWidth(nvg, 2.0f);
        nvgStroke(nvg);
    };
    
    auto drawCurvedConnection = [this, nvg, finalBaseColour](){
        // semi-transparent background line
        nvgBeginPath(nvg);
        nvgLineStyle(nvg, NVG_LINE_SOLID);
        nvgMoveTo(nvg, start_.x, start_.y);
        nvgBezierTo(nvg, cp1_.x, cp1_.y, cp2_.x, cp2_.y, end_.x, end_.y);
        nvgStrokeColor(nvg, convertColour(shadowColour));
        nvgLineCap(nvg, NVG_ROUND);
        nvgStrokeWidth(nvg, 4.0f);
        nvgStroke(nvg);

        nvgStrokeColor(nvg, convertColour(finalBaseColour));
        nvgStrokeWidth(nvg, 2.0f);
        nvgStroke(nvg);
    };
    
    auto drawSegmentedConnection = [this, nvg, finalBaseColour](){
        auto snap = [this](Point<float> point, int idx1, int idx2) {
            if (Line<float>(currentPlan[idx1], currentPlan[idx2]).isVertical()) {
                currentPlan[idx2].x = point.x;
            } else {
                currentPlan[idx2].y = point.y;
            }

            currentPlan[idx1] = point;
        };

        auto pstart = getStartPoint();
        auto pend = getEndPoint();
        
        snap(pstart, 0, 1);
        snap(pend, static_cast<int>(currentPlan.size() - 1), static_cast<int>(currentPlan.size() - 2));

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, pstart.x, pstart.y);
        
        // Add points in between if we've found a path
        for (int n = 1; n < currentPlan.size() - 1; n++) {
            auto p1 = currentPlan[n-1];
            auto p2 = currentPlan[n];
            auto p3 = currentPlan[n+1];
            
            if(p2.y == p3.y && p2.x == p3.x)
            {
                nvgLineTo(nvg, p2.x, p2.y);
            }
            else if(p2.y == p3.y)
            {
                auto offsetX = std::clamp<float>(p1.x - p3.x, -8, 8);
                auto offsetY = std::clamp<float>(p1.y - p3.y, -8, 8);
                nvgLineTo(nvg, p2.x, p2.y + offsetY);
                nvgQuadTo(nvg, p2.x, p2.y, p2.x - offsetX, p2.y);
            }
            else {
                auto offsetX = std::clamp<float>(p1.x - p3.x, -8, 8);
                auto offsetY = std::clamp<float>(p1.y - p3.y, -8, 8);
                nvgLineTo(nvg, p2.x + offsetX, p2.y);
                nvgQuadTo(nvg, p2.x, p2.y, p2.x, p2.y - offsetY);
            }
        }

        nvgLineTo(nvg, pend.x, pend.y);
        
        nvgStrokeColor(nvg, convertColour(shadowColour));
        nvgStrokeWidth(nvg, 3.0f);
        nvgStroke(nvg);
        
        nvgLineStyle(nvg, NVG_LINE_SOLID);
        nvgStrokeColor(nvg, convertColour(finalBaseColour));
        nvgStrokeWidth(nvg, 2.0f);
        nvgStroke(nvg);
    };
    
    if(segmented)
    {
        drawSegmentedConnection();
    }
    else if (!PlugDataLook::getUseStraightConnections()) {
        drawCurvedConnection();
    } else {
        drawStraightConnection();
    }
       
    // draw reconnect handles if connection is both selected & mouse is hovering over
    if (isSelected() && isHovering) {
        auto mousePos = cnv->getLastMousePosition();
        auto expandedStartHandle = startReconnectHandle.contains(mousePos.toFloat()) ? startReconnectHandle.expanded(3.0f) : startReconnectHandle;
        auto expandedEndHandle = startReconnectHandle.contains(mousePos.toFloat()) ? endReconnectHandle.expanded(3.0f) : endReconnectHandle;
   
        nvgFillColor(nvg, convertColour(handleColour));
        nvgStrokeColor(nvg, convertColour(cnv->findColour(PlugDataColour::objectOutlineColourId)));
        nvgStrokeWidth(nvg, 0.5f);
        
        nvgBeginPath(nvg);
        nvgCircle(nvg, expandedStartHandle.getCentreX(), expandedStartHandle.getCentreY(), expandedStartHandle.getWidth() / 2);
        nvgFill(nvg);
        nvgStroke(nvg);
        
        nvgBeginPath(nvg);
        nvgCircle(nvg, expandedEndHandle.getCentreX(), expandedEndHandle.getCentreY(), expandedEndHandle.getWidth() / 2);
        nvgFill(nvg);
        nvgStroke(nvg);
    }

#ifdef DEBUG_CONNECTION_BOUNDS
    auto b = getBoundsInParent();
    nvgBeginPath(nvg);
    nvgRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
    nvgStrokeColor(nvg, nvgRGBf(1, 0, 0));
    nvgStrokeWidth(nvg, 1.0f);
    nvgStroke(nvg);
#endif

#ifdef DEBUG_CONNECTION_PATH_CP
    nvgStrokeWidth(nvg, 1.0f);
    nvgStrokeColor(nvg, nvgRGB(0,255,0));
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, start_.x, start_.y);
    nvgLineTo(nvg, cp1_.x, cp1_.y);
    nvgStroke(nvg);

    nvgBeginPath(nvg);
    nvgMoveTo(nvg, end_.x, end_.y);
    nvgLineTo(nvg, cp2_.x, cp2_.y);
    nvgStroke(nvg);

    nvgBeginPath(nvg);
    nvgFillColor(nvg, nvgRGB(0,255,0));
    nvgCircle(nvg, cp1_.x, cp1_.y, 5);
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, cp2_.x, cp2_.y, 5);
    nvgFill(nvg);
#endif
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

    if (Canvas::panningModifierDown())
        return false;

    if (cnv->commandLocked == var(true) || locked == var(true))
        return false;

    auto const hitPoint = Point<float>(x, y);

    auto const startExculsionDistance = getLocalPoint(cnv, start_).getDistanceFrom(hitPoint);
    auto const endExculsionDistance = getLocalPoint(cnv, end_).getDistanceFrom(hitPoint);

    if ((startExculsionDistance < 10) || (endExculsionDistance < 10))
        return false;

    Point<float> nearestPoint;
    hitTestPath.getNearestPoint(hitPoint, nearestPoint);
    auto distFromPath = nearestPoint.getDistanceFrom(hitPoint);
    return distFromPath < 3.0f;
}

bool Connection::intersects(Rectangle<float> toCheck, int accuracy) const
{
    PathFlatteningIterator i(hitTestPath);

    auto toCheckLocal = getLocalArea(cnv, toCheck);

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

        if (toCheckLocal.intersects(currentLine)) {
            return true;
        }
    }

    return false;
}

void Connection::updateOverlays(int overlay)
{
    if (!inlet || !outlet)
        return;

    showDirection = overlay & Overlay::Direction;
    showConnectionOrder = overlay & Overlay::Order;
    showActiveState = overlay & Overlay::ActivationState;
    updatePath();
    resizeToFit();
    repaint();
}

void Connection::forceUpdate()
{
    updatePath();
    resizeToFit();
    repaint();
}

void Connection::paintOverChildren(Graphics& g)
{
std::cout << "trying to paint" << std::endl;
#ifdef DEBUG_CONNECTION_OUTLINE
    g.setColour(Colours::green);
    g.drawRect(getLocalBounds(), 1.0f);
#endif
#ifdef DEBUG_CONNECTION_HITPATH
    g.setColour(Colours::green);
    g.strokePath(hitTestPath, PathStrokeType(3.0f));
#endif
}

bool Connection::isSegmented() const
{
    return segmented;
}

void Connection::setSegmented(bool isSegmented)
{
    segmented = isSegmented;
    updatePath();
    resizeToFit();
    repaint();
    pushPathState();
}

void Connection::setSelected(bool shouldBeSelected)
{
    if (selectedFlag != shouldBeSelected) {
        selectedFlag = shouldBeSelected;
        updatePath();
        resizeToFit();
        repaint();
    }
}

bool Connection::isSelected() const
{
    return selectedFlag;
}

void Connection::mouseMove(MouseEvent const& e)
{
    isHovering = true;

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

    //repaint();
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
        cnv->editor->connectionMessageDisplay->setConnection(this, e.getScreenPosition());
    }
    repaint();
}

void Connection::mouseExit(MouseEvent const& e)
{
    cnv->editor->connectionMessageDisplay->setConnection(nullptr);
    isHovering = false;
    repaint();
}


void Connection::updateBounds()
{
    auto rectStart = Rectangle<int>(0,0,1,1);
    auto rectEnd = Rectangle<int>(0,0,1,1);
    if (!inlet || !outlet) {
        auto mousePos = cnv->getLastMousePosition();
        if (outlet) {
            start_ = cnv->getLocalPoint(outlet, outlet->getLocalBounds().toFloat().getCentre());
            end_ = mousePos.toFloat();

            rectStart = cnv->getLocalArea(outlet, outlet->getLocalBounds());
            rectEnd = rectEnd.withPosition(mousePos);
        } else {
            start_ = mousePos.toFloat();
            end_ = cnv->getLocalPoint(inlet, inlet->getLocalBounds().toFloat().getCentre());

            rectStart = rectStart.withPosition(mousePos);
            rectEnd = cnv->getLocalArea(inlet, inlet->getLocalBounds());
        }
    } else {
        start_ = cnv->getLocalPoint(outlet, outlet->getLocalBounds().toFloat().getCentre());
        end_ = cnv->getLocalPoint(inlet, inlet->getLocalBounds().toFloat().getCentre());

        rectStart = cnv->getLocalArea(outlet, outlet->getLocalBounds());
        rectEnd = cnv->getLocalArea(inlet, inlet->getLocalBounds());
    }

    width_ = std::max(start_.x, end_.x) - std::min(start_.x, end_.x);
    height_ = std::max(start_.y, end_.y) - std::min(start_.y, end_.y);

    auto b = Rectangle<int>(rectStart.getUnion(rectEnd));

    setBounds(b);
}

void Connection::resized()
{
    float const min = std::min<float>(width_, height_);
    float const max = std::max<float>(width_, height_);
    float const maxShift = 20.f;

    float const shiftY = std::min<float>(maxShift, max * 0.5);
    float const shiftX = ((start_.y >= end_.y) ? std::min<float>(maxShift, min * 0.5) : 0.f) * ((start_.x < end_.x) ? -1. : 1.);

    cp1_ = Point<float>(start_.x - shiftX, start_.y + shiftY);
    cp2_ = Point<float>(end_.x + shiftX, end_.y - shiftY);

    if (generateHitPath) {
        hitTestPath.clear();
        hitTestPath.startNewSubPath(getLocalPoint(cnv, start_));
        hitTestPath.cubicTo(getLocalPoint(cnv, cp1_), getLocalPoint(cnv, cp2_), getLocalPoint(cnv, end_));
    }
}

void Connection::mouseDown(MouseEvent const& e)
{
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

    updateBounds();
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

        /* ALEXCONREFACTOR
        // Create new connection
        cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(target->isInlet ? c->inlet : c->outlet, cnv));
        */
        c->setVisible(false);

        reconnecting.add(SafePointer(c));

        // Make sure we're deselected and remove object
        cnv->setSelected(c, false, false);
    }
}

void Connection::resizeToFit()
{
    return;
    auto pStart = getStartPoint();

    Point<float> pEnd;
    if (inlet == nullptr)
        pEnd = Desktop::getMousePosition().toFloat();
    else
        pEnd = getEndPoint();


    // heuristics to allow the overlay & reconnection handle to paint inside bounds
    // consider moving them to their own layers in future
    auto safteyMargin = showConnectionOrder ? 13 : isSelected() ? 10
                                                                : 6;

    auto newBounds = Rectangle<float>(pStart, pEnd).expanded(safteyMargin).getSmallestIntegerContainer();

    if (segmented) {
        newBounds = newBounds.getUnion(toDraw.getBounds().expanded(safteyMargin).getSmallestIntegerContainer());
    }
    if (newBounds != getBounds()) {
        //setBounds(newBounds);
    }

    toDrawLocalSpace = toDraw;
    auto offset = getLocalPoint(cnv, Point<int>());
    toDrawLocalSpace.applyTransform(AffineTransform::translation(offset));
    
    if(segmented)
    {
        Point<float> startUnitDirection = (currentPlan[0] - currentPlan[1]) / (currentPlan[0].getDistanceFrom(currentPlan[1]));
        Point<float> endUnitDirection = (currentPlan[currentPlan.size() - 2] - currentPlan[currentPlan.size() - 1]) / (currentPlan[currentPlan.size() - 2].getDistanceFrom(currentPlan[currentPlan.size() - 1]));
        
        startReconnectHandle = Rectangle<float>(5, 5).withCentre(pStart - startUnitDirection * 8.5f);
        endReconnectHandle = Rectangle<float>(5, 5).withCentre(pEnd + endUnitDirection * 8.5f);
    }
    else if (!PlugDataLook::getUseStraightConnections()) {
        float const width = std::max(pStart.x, pEnd.x) - std::min(pStart.x, pEnd.x);
        float const height = std::max(pStart.y, pEnd.y) - std::min(pStart.y, pEnd.y);

        float const min = std::min<float>(width, height);
        float const max = std::max<float>(width, height);
        float const maxShift = 20.f;

        float const shiftY = std::min<float>(maxShift, max * 0.5);
        float const shiftX = ((pStart.y >= pEnd.y) ? std::min<float>(maxShift, min * 0.5) : 0.f) * ((pStart.x < pEnd.x) ? -1. : 1.);

        Point<float> const cp1 { pStart.x - shiftX, pStart.y + shiftY };
        Point<float> const cp2 { pEnd.x + shiftX, pEnd.y - shiftY };

        startReconnectHandle = Rectangle<float>(5, 5).withCentre(bezierPointAtDistance(pStart, cp1, cp2, pEnd, 8.5f));
        endReconnectHandle = Rectangle<float>(5, 5).withCentre(bezierPointAtDistance(pEnd, cp2, cp1, pStart, 8.5f));
    } else {
        float lineLength = pStart.getDistanceFrom(pEnd);
        Point<float> unitDirection = (pEnd - pStart) / lineLength;
        startReconnectHandle = Rectangle<float>(5, 5).withCentre(pStart + unitDirection * 8.5f);
        endReconnectHandle = Rectangle<float>(5, 5).withCentre(pEnd - unitDirection * 8.5f);
    }
}

void Connection::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    if (!inlet || !outlet)
        return;

    if (outobj->isSelected() && inobj->isSelected() && !wasResized)
        generateHitPath = false;
    else
        generateHitPath = true;

    updateBounds();

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

        return;
    }
    previousPStart = pstart;

    if (currentPlan.size() <= 2) {
        updatePath();
        resizeToFit();
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
    resizeToFit();
    repaint();
}

Point<float> Connection::getStartPoint() const
{
    return outlet->getCanvasBounds().toFloat().getCentre();
}

Point<float> Connection::getEndPoint() const
{
    return inlet->getCanvasBounds().toFloat().getCentre();
}

Path Connection::getNonSegmentedPath(Point<float> start, Point<float> end)
{
    std::cout << "making non segmented path" << std::endl;
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
            auto numChannels = std::min(signal->s_nchans, maxChannels);
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

void Connection::updatePath()
{
    return;
    auto pstart = getStartPoint();

    Point<float> pend;
    if (inlet == nullptr)
        pend = Desktop::getMousePosition().toFloat();
    else
        pend = getEndPoint();

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
            if (connectionPath.contains(currentPlan[n].toFloat()))
                continue; // ??

            connectionPath.lineTo(currentPlan[n].toFloat());
        }

        connectionPath.lineTo(pend);
        toDraw = connectionPath.createPathWithRoundedCorners(PlugDataLook::getUseStraightConnections() ? 0.0f : 8.0f);
    }
}

void Connection::applyBestPath()
{
    segmented = true;
    findPath();
    updatePath();
    resizeToFit();
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

bool Connection::intersectsObject(Object* object) const
{
    auto b = object->getBounds().toFloat();
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

        auto* patch = connection->cnv->patch.getPointer().get();
        if (!patch)
            continue;

        // Get connections from pd
        linetraverser_start(&t, patch);

        while (auto* oc = linetraverser_next(&t)) {

            if (oc && oc == connection->ptr.getRaw<t_outconnect>()) {

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

    canvas->patch.endUndoSequence("SetConnectionPaths");
}

void Connection::receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms)
{
    // TODO: indicator
    // messageActivity = messageActivity >= 12 ? 0 : messageActivity + 1;

    outobj->triggerOverlayActiveState();
    std::copy(atoms, atoms + numAtoms, lastValue);
    lastNumArgs = numAtoms;
    lastSelector = symbol;
}
