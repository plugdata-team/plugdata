/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Iolet.h"

#include "Canvas.h"
#include "Connection.h"
#include "LookAndFeel.h"

Iolet::Iolet(Object* parent, bool inlet) : object(parent)
{
    isInlet = inlet;
    setSize(8, 8);
    
    setAlwaysOnTop(true);

    parent->addAndMakeVisible(this);

    locked.referTo(parent->cnv->locked);
}

Rectangle<int> Iolet::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return getBounds() + object->getPosition();
}

void Iolet::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    bool isLocked = static_cast<bool>(locked.getValue());
    bool down = isMouseButtonDown();
    bool over = isMouseOver();
    
    if ((!isTargeted && !over) || isLocked)
    {
        bounds = bounds.reduced(2);
    }
    
    auto backgroundColour = isSignal ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId);

    if ((down || over) && !isLocked) backgroundColour = backgroundColour.contrasting(down ? 0.2f : 0.05f);

    if (isLocked)
    {
        backgroundColour = findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.5f);
    }

    // Instead of drawing pie segments, just clip the graphics region to the visible iolets of the object
    // This is much faster!
    bool stateSaved = false;
    if (!(object->isMouseOverOrDragging(true) || over || isTargeted) || isLocked)
    {
        g.saveState();
        g.reduceClipRegion(getLocalArea(object, object->getLocalBounds().reduced(Object::margin)));
        stateSaved = true;
    }

    g.setColour(backgroundColour);
    g.fillEllipse(bounds);

    g.setColour(findColour(PlugDataColour::objectOutlineColourId));
    g.drawEllipse(bounds, 1.0f);

    if (stateSaved)
    {
        g.restoreState();
    }
}

void Iolet::resized()
{
}

void Iolet::mouseDrag(const MouseEvent& e)
{
    // Ignore when locked
    if (static_cast<bool>(locked.getValue()))
        return;

    auto* cnv = findParentComponentOfClass<Canvas>();

    if (cnv->connectingIolets.isEmpty() && e.getLengthOfMousePress() > 100) {
        createConnection();
        cnv->connectingWithDrag = true;
    }
    if (cnv->connectingWithDrag && !cnv->connectingIolets.isEmpty()) {
        auto& connectingEdge = cnv->connectingIolets.getReference(0);

        if (connectingEdge) {
            auto* nearest = findNearestEdge(cnv, e.getEventRelativeTo(cnv).getPosition(), !connectingEdge->isInlet, connectingEdge->object);

            if (nearest && cnv->nearestEdge != nearest) {
                nearest->isTargeted = true;

                if (cnv->nearestEdge) {
                    cnv->nearestEdge->isTargeted = false;
                    cnv->nearestEdge->repaint();
                }

                cnv->nearestEdge = nearest;
                cnv->nearestEdge->repaint();
            }
            else if (!nearest && cnv->nearestEdge) {
                cnv->nearestEdge->isTargeted = false;
                cnv->nearestEdge->repaint();
                cnv->nearestEdge = nullptr;
            }
        }
        cnv->repaint();
    }
}

void Iolet::mouseUp(const MouseEvent& e)
{
    if (static_cast<bool>(locked.getValue()) || e.mods.isRightButtonDown())
        return;

    auto* cnv = findParentComponentOfClass<Canvas>();
    
    if (!e.mouseWasDraggedSinceMouseDown() && cnv->connectingIolets.isEmpty()) {
        createConnection();

    } else if (!cnv->connectingIolets.isEmpty()) {

        if (!e.mouseWasDraggedSinceMouseDown() && !e.mods.isShiftDown()) {
            createConnection();
            cnv->cancelConnectionCreation();

        } else if (cnv->connectingWithDrag && cnv->nearestEdge && !e.mods.isShiftDown()) {
            // Releasing a connect-by-drag action

            cnv->nearestEdge->isTargeted = false;
            cnv->nearestEdge->repaint();

            for (auto& iolet : cnv->connectingIolets) {
                cnv->nearestEdge->createConnection();
            }

            cnv->cancelConnectionCreation();
            cnv->nearestEdge = nullptr;
            cnv->connectingWithDrag = false;

        } else if (e.mods.isShiftDown() && cnv->getSelectionOfType<Object>().size() > 1 && (cnv->connectingIolets.size() == 1)) {

            //
            // Auto patching
            //

            auto selection = cnv->getSelectionOfType<Object>();

            Object* nearestObject = object;
            int inletIdx = ioletIdx;
            if (cnv->nearestEdge) {
                // If connected by drag
                nearestObject = cnv->nearestEdge->object;
                inletIdx = cnv->nearestEdge->ioletIdx;
            }

            // Sort selected objects by X position
            std::sort(selection.begin(), selection.end(), [](const Object* lhs, const Object* rhs) {
                return lhs->getX() < rhs->getX();
            });

            auto* conObj = cnv->connectingIolets.getFirst()->object;

            if ((conObj->numOutputs > 1) && selection.contains(conObj) && selection.contains(nearestObject)) {

                // If selected 'start object' has multiple outlets
                // Connect all selected objects beneath to 'start object' outlets, ordered by position
                int outletIdx = conObj->numInputs + cnv->connectingIolets.getFirst()->ioletIdx;
                for (auto* sel : selection) {
                    if ((sel != conObj) && (conObj->iolets[outletIdx]) && (sel->numInputs)) {
                        if ((sel->getX() >= nearestObject->getX()) && (sel->getY() > (conObj->getY() + conObj->getHeight() - 15))) {
                            cnv->connections.add(new Connection(cnv, conObj->iolets[outletIdx], sel->iolets.getFirst(), false));
                            outletIdx = outletIdx + 1;
                        }
                    }
                }
            } else if ((nearestObject->numInputs > 1) && selection.contains(nearestObject)) {

                // If selected 'end object' has multiple inputs
                // Connect all selected objects above to 'end object' inlets, ordered by index
                for (auto* sel : selection) {
                    if ((nearestObject->numInputs > 1) && (nearestObject->getY() > (conObj->getY() + conObj->getHeight() - 15)) && (nearestObject->getY() > (sel->getY() + sel->getHeight() - 15))) {
                        if ((sel != nearestObject) && (sel->getX() >= conObj->getX()) && nearestObject->iolets[inletIdx]->isInlet && (sel->numOutputs)) {

                            cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], nearestObject->iolets[inletIdx], false));
                            inletIdx = inletIdx + 1;
                        }
                    }
                }

            } else if (selection.contains(nearestObject)) {

                // If 'end object' is selected
                // Connect 'start outlet' with all selected objects beneath
                // Connect all selected objects at or above to 'end object'
                for (auto* sel : selection) {
                    if ((sel->getY() > (conObj->getY() + conObj->getHeight() - 15))) {
                       cnv->connections.add(new Connection(cnv, cnv->connectingIolets.getFirst(), sel->iolets.getFirst(), false));
                    } else {
                        cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], nearestObject->iolets.getFirst(), false));
                    }
                }
            }

            else {

                // If 'start object' is selected
                // Connect 'end inlet' with all selected objects
                for (auto* sel : selection) {
                    if (cnv->nearestEdge) {
                        cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], cnv->nearestEdge, false));
                    } else {
                        cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], this, false));
                    }
                }
            }

            cnv->connectingIolets.clear();

        } else if (!e.mouseWasDraggedSinceMouseDown() && e.mods.isShiftDown()) {
            createConnection();

        } else if (cnv->connectingWithDrag && cnv->nearestEdge && e.mods.isShiftDown()) {
            // Releasing a connect-by-drag action
            cnv->nearestEdge->isTargeted = false;
            cnv->nearestEdge->repaint();

            for (auto& iolet : cnv->connectingIolets) {
                cnv->nearestEdge->createConnection();
            }

            cnv->nearestEdge = nullptr;
            cnv->connectingWithDrag = false;
            cnv->repaint();
        }
        if (!e.mods.isShiftDown() || cnv->connectingIolets.size() != 1) {
            cnv->connectingIolets.clear();
            cnv->repaint();
            cnv->connectingWithDrag = false;
        }

        // TODO: is this needed? Else delete.. :
        
        // Unless the call originates from a connection, clear any connections that are being created
        /*
         if (cnv->connectingWithDrag && !dynamic_cast<Connection*>(e.originalComponent)) {
             cnv->connectingIolets.clear();
             cnv->connectingWithDrag = false;
             cnv->repaint();
         } */

        if (cnv->nearestEdge) {
            cnv->nearestEdge->isTargeted = false;
            cnv->nearestEdge->repaint();
            cnv->nearestEdge = nullptr;
        }
    }
}

void Iolet::mouseEnter(const MouseEvent& e)
{
    for (auto& iolet : object->iolets) iolet->repaint();
}

void Iolet::mouseExit(const MouseEvent& e)
{
    for (auto& iolet : object->iolets) iolet->repaint();
}


void Iolet::createConnection()
{
    object->cnv->hideAllActiveEditors();
    
    // Check if this is the start or end action of connecting
    if (!object->cnv->connectingIolets.isEmpty())
    {
        for(auto& iolet : object->cnv->connectingIolets) {
            // Check type for input and output
            bool sameDirection = isInlet == iolet->isInlet;

            bool connectionAllowed = iolet->object != object && !sameDirection;

            // Don't create if this is the same iolet
            if (iolet == this)
            {
                object->cnv->connectingIolets.remove(&iolet);
            }
            // Create new connection if allowed
            else if (connectionAllowed)
            {
                auto* cnv = findParentComponentOfClass<Canvas>();
                cnv->connections.add(new Connection(cnv, iolet, this));
            }
        }
    }
    // Else set this iolet as start of a connection
    else
    {   
        if(Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown() && object->cnv->isSelected(object)) {
            // Auto patching - if shift is down at mouseDown 
            // create connections from selected objects
            int position = object->iolets.indexOf(this);
            position = isInlet ? position : position - object->numInputs;
            for(auto* selectedBox : object->cnv->getSelectionOfType<Object>()) {
                if(isInlet && position < selectedBox->numInputs) {
                    object->cnv->connectingIolets.add(selectedBox->iolets[position]);
                }
                else if(!isInlet && position < selectedBox->numOutputs) {
                    object->cnv->connectingIolets.add(selectedBox->iolets[selectedBox->numInputs + position]);
                }
            }
        }
        else {
            object->cnv->connectingIolets.add(this);
        }
    }
}

Iolet* Iolet::findNearestEdge(Canvas* cnv, Point<int> position, bool inlet, Object* boxToExclude)
{
    // Find all iolets
    Array<Iolet*> allEdges;
    for (auto* object : cnv->objects)
    {
        for (auto* iolet : object->iolets)
        {
            if (iolet->isInlet == inlet && iolet->object != boxToExclude)
            {
                allEdges.add(iolet);
            }
        }
    }

    Iolet* nearestEdge = nullptr;

    for (auto& iolet : allEdges)
    {
        auto bounds = iolet->getCanvasBounds().expanded(50, 50);
        if (bounds.contains(position))
        {
            if (!nearestEdge) nearestEdge = iolet;

            auto oldPos = nearestEdge->getCanvasBounds().getCentre();
            auto newPos = bounds.getCentre();
            nearestEdge = newPos.getDistanceFrom(position) < oldPos.getDistanceFrom(position) ? iolet : nearestEdge;
        }
    }

    return nearestEdge;
}
