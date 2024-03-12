/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <juce_gui_basics/juce_gui_basics.h>
#include <nanovg.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Iolet.h"

#include "Object.h"
#include "Canvas.h"
#include "Connection.h"
#include "LookAndFeel.h"

Iolet::Iolet(Object* parent, bool inlet)
    : NVGComponent(static_cast<Component&>(*this))
    , object(parent)
    , insideGraph(parent->cnv->isGraph)
{
    isInlet = inlet;
    setSize(8, 8);

    setAlwaysOnTop(true);

    parent->addAndMakeVisible(this);

    locked.referTo(object->cnv->locked);
    locked.addListener(this);

    commandLocked.referTo(object->cnv->commandLocked);
    commandLocked.addListener(this);

    presentationMode.referTo(object->cnv->presentationMode);
    presentationMode.addListener(this);

    bool isPresenting = getValue<bool>(presentationMode);
    setVisible(!isPresenting && !insideGraph);

    cnv = findParentComponentOfClass<Canvas>();
}

Rectangle<int> Iolet::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return getSafeBounds() + object->getSafeBounds().getPosition();
}

void Iolet::render(NVGcontext* nvg)
{
    auto bounds = getSafeLocalBounds().toFloat().reduced(0.5f);

    bool isLocked = getValue<bool>(locked) || getValue<bool>(commandLocked);
    bool over = getCanvasBounds().contains(cnv->getLastMousePosition());

    auto backgroundColour = isSignal ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId);
    if(isGemState)
    {
        backgroundColour = findColour(PlugDataColour::gemColourId);
    }
    
    if ((mouseIsDown || over) && !isLocked)
        backgroundColour = backgroundColour.contrasting(mouseIsDown ? 0.2f : 0.05f);

    if (isLocked) {
        backgroundColour = findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.5f);
    }

    auto outlineColour = findColour(PlugDataColour::objectOutlineColourId);
    
    if (PlugDataLook::getUseSquareIolets()) {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(backgroundColour.getRed(), backgroundColour.getGreen(), backgroundColour.getBlue()));
        nvgRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        nvgFill(nvg);
        
        nvgStrokeColor(nvg, nvgRGB(outlineColour.getRed(), outlineColour.getGreen(), outlineColour.getBlue()));
        nvgStroke(nvg);
    } else {
    // ALEX only done round at this point
        
        nvgBeginPath(nvg);

        const auto ioletCentre = bounds.getCentre().translated(0.f, isInlet ? -1.0f : 1.0f);

        if (isTargeted) {
            nvgDrawRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), convertColour(backgroundColour), convertColour(outlineColour), bounds.getWidth() / 2.0f);
        } else {
            nvgFillColor(nvg, convertColour(backgroundColour));
            nvgStrokeColor(nvg, nvgRGB(outlineColour.getRed(), outlineColour.getGreen(), outlineColour.getBlue()));
            nvgStrokeWidth(nvg, 1.0f);
            
            nvgArc(nvg, ioletCentre.x, ioletCentre.y, bounds.getWidth() * 0.35f, 0, NVG_PI, isInlet ? NVG_CW : NVG_CCW);
            nvgFill(nvg);
            nvgStroke(nvg);
        }
    }
}

bool Iolet::hitTest(int x, int y)
{
    // If locked, don't intercept mouse clicks
    if ((getValue<bool>(locked) || getValue<bool>(commandLocked)))
        return false;

    Path smallBounds;
    smallBounds.addEllipse(getLocalBounds().toFloat().reduced(2));
    smallBounds.closeSubPath();

    // Check if the small iolet bounds contains mouse, if so, return true
    if (smallBounds.contains(x, y)) {
        return true;
    }

    // Check if we're hovering a resize zone
    if (object->validResizeZone) {
        return false;
    }

    // Check if we're hovering the total iolet hitbox
    return getLocalBounds().contains(x, y);
}

void Iolet::mouseDrag(MouseEvent const& e)
{
    // Ignore when locked or if middlemouseclick?
    if (getValue<bool>(locked) || e.mods.isMiddleButtonDown())
        return;

    if (!cnv->connectingWithDrag && e.getLengthOfMousePress() > 100) {
            startConnection();
            cnv->connectingWithDrag = true;
    }

    if (cnv->connectingWithDrag) {
        auto* nearest = findNearestIolet(cnv, e.getEventRelativeTo(cnv).getPosition(), !isInlet, object);

        if (nearest && cnv->nearestIolet != nearest) {
            nearest->isTargeted = true;

            if (cnv->nearestIolet) {
                cnv->nearestIolet->isTargeted = false;
                cnv->nearestIolet->repaint();
            }

            cnv->nearestIolet = nearest;
            cnv->nearestIolet->repaint();
        } else if (!nearest && cnv->nearestIolet) {
            cnv->nearestIolet->isTargeted = false;
            cnv->nearestIolet->repaint();
            cnv->nearestIolet = nullptr;
        }
    }
}

void Iolet::startConnection()
{
    std::cout << "starting connection" << std::endl;
    cnv->hideAllActiveEditors();
    cnv->connections.add(new Connection(cnv, this));
}

void Iolet::mouseDown(MouseEvent const& e)
{
    mouseIsDown = true;
}

void Iolet::mouseUp(MouseEvent const& e)
{
    if (getValue<bool>(locked) || e.mods.isRightButtonDown())
        return;

    std::cout << "mouse up on iolet" << std::endl;

    // remove temp connections regardless if they are successful or not
    for (int i = cnv->connections.size() - 1; i >= 0; --i) {
        auto con = cnv->connections[i];
        if (con->inobj == nullptr)
            cnv->connections.removeObject(con, true);
        else
            break;
    }

    cnv->connectingWithDrag = false;
    mouseIsDown = false;

    if (cnv->nearestIolet) {
        std::cout << "making connection" << std::endl;
        auto newCon = new Connection(cnv, this, cnv->nearestIolet, nullptr);
        cnv->connections.add(newCon);
        cnv->connectionLayer.addAndMakeVisible(newCon);
        createConnection();
    }
}

void Iolet::mouseEnter(MouseEvent const& e)
{
    isTargeted = true;
    repaint();
}

void Iolet::mouseExit(MouseEvent const& e)
{
    isTargeted = false;
    repaint();
}

void Iolet::createConnection()
{
    auto* cnv = object->cnv;

    cnv->patch.startUndoSequence("Connecting");

    for (auto& c : cnv->connections) {

        if (!cnv->nearestIolet)
            continue;

        // Check type for input and output
        bool sameDirection = isInlet == cnv->nearestIolet->isInlet;

        bool connectionAllowed = cnv->nearestIolet != this && cnv->nearestIolet->object != object && !sameDirection;

        // Create new connection if allowed
        if (connectionAllowed) {
            auto outlet = isInlet ? cnv->nearestIolet.getComponent() : this;
            auto inlet = isInlet ? this : cnv->nearestIolet.getComponent();

            auto outobj = outlet->object;
            auto inobj = inlet->object;

            auto outIdx = outlet->ioletIdx;
            auto inIdx = inlet->ioletIdx;

            auto* outptr = pd::Interface::checkObject(outobj->getPointer());
            auto* inptr = pd::Interface::checkObject(inobj->getPointer());

            if (!outptr || !inptr)
                return;

            cnv->patch.createConnection(outptr, outIdx, inptr, inIdx);
        }
    }

    cnv->patch.endUndoSequence("Connecting");
    cnv->synchronise(); // Load all newly created connection from pd patch!

    // otherwise set this iolet as start of a connection
    /*
    else {
        if (Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
            // Auto patching - if shift is down at mouseDown
            // create connections from selected objects
            cnv->setSelected(object, true);

            int position = object->iolets.indexOf(this);
            position = isInlet ? position : position - object->numInputs;
            for (auto* selectedBox : object->cnv->getSelectionOfType<Object>()) {
                if (isInlet && position < selectedBox->numInputs) {
                    object->cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(selectedBox->iolets[position], selectedBox->cnv));
                } else if (!isInlet && position < selectedBox->numOutputs) {
                    object->cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(selectedBox->iolets[selectedBox->numInputs + position], selectedBox->cnv));
                }
            }
        } else {
            object->cnv->connectionsBeingCreated.add(new ConnectionBeingCreated(this, object->cnv));
        }
    }
     */
}

Array<Connection*> Iolet::getConnections()
{
    Array<Connection*> result;
    for (auto* c : object->cnv->connections) {
        if (c->inlet == this || c->outlet == this) {
            result.add(c);
        }
    }

    return result;
}

Iolet* Iolet::findNearestIolet(Canvas* cnv, Point<int> position, bool inlet, Object* boxToExclude)
{
    // Find all potential iolets
    Array<Iolet*> allEdges;

    for (auto* object : cnv->objects) {
        for (auto* iolet : object->iolets) {
            if (iolet->isInlet == inlet && iolet->object != boxToExclude) {
                allEdges.add(iolet);
            }
        }
    }

    Iolet* nearestIolet = nullptr;

    for (auto& iolet : allEdges) {
        auto bounds = iolet->getCanvasBounds().expanded(30);
        if (bounds.contains(position)) {
            if (!nearestIolet)
                nearestIolet = iolet;

            auto oldPos = nearestIolet->getCanvasBounds().getCentre();
            auto newPos = bounds.getCentre();
            nearestIolet = newPos.getDistanceFrom(position) < oldPos.getDistanceFrom(position) ? iolet : nearestIolet;
        }
    }

    return nearestIolet;
}

void Iolet::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(locked)) {
        repaint();
    }
    if (v.refersToSameSourceAs(commandLocked)) {
        repaint();
    }
    if (v.refersToSameSourceAs(presentationMode)) {
        setVisible(!getValue<bool>(presentationMode) && !insideGraph && !hideIolet);
        repaint();
    }
}

void Iolet::setHidden(bool hidden)
{
    hideIolet = hidden;
    setVisible(!getValue<bool>(presentationMode) && !insideGraph && !hideIolet);
    repaint();
}
