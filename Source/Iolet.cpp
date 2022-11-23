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

    parent->addAndMakeVisible(this);

    locked.referTo(parent->cnv->pd->locked);
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

    g.setColour(findColour(PlugDataColour::outlineColourId));
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
    if (bool(locked.getValue()))
        return;

    auto* cnv = findParentComponentOfClass<Canvas>();

    if (cnv->connectingEdges.isEmpty() && e.getLengthOfMousePress() > 100) {
        createConnection();
        cnv->connectingWithDrag = true;
    }
    if (cnv->connectingWithDrag && !cnv->connectingEdges.isEmpty()) {
        auto& connectingEdge = cnv->connectingEdges.getReference(0);

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
        }
        cnv->repaint();
    }
}

void Iolet::mouseUp(const MouseEvent& e)
{
    if (bool(locked.getValue()))
        return;

    auto* cnv = findParentComponentOfClass<Canvas>();

    if (!e.mouseWasDraggedSinceMouseDown() && cnv->connectingEdges.isEmpty()) {
        createConnection();

    } else if (!cnv->connectingEdges.isEmpty()) {

        if (!e.mouseWasDraggedSinceMouseDown() && !e.mods.isShiftDown()) {
            createConnection();
            cnv->connectingEdges.clear();

        } else if (cnv->connectingWithDrag && cnv->nearestEdge && !e.mods.isShiftDown()) {
            // Releasing a connect-by-drag action

            cnv->nearestEdge->isTargeted = false;
            cnv->nearestEdge->repaint();

            for (auto& iolet : cnv->connectingEdges) {
                cnv->nearestEdge->createConnection();
            }

            cnv->connectingEdges.clear();
            cnv->nearestEdge = nullptr;
            cnv->connectingWithDrag = false;
            cnv->repaint();

        } else if ((cnv->getSelectionOfType<Object>().contains(object) || (cnv->nearestEdge && cnv->getSelectionOfType<Object>().contains(cnv->nearestEdge->object)))
            && e.mods.isShiftDown() && cnv->getSelectionOfType<Object>().size() > 1 && (cnv->connectingEdges.size() == 1)) {

            // Auto patching - connect to all selected objects
            // if shift is pressed after mouse down

            auto selection = cnv->getSelectionOfType<Object>();

            Object* nearestObject = object;
            if (cnv->nearestEdge) {
                // If connected by drag
                nearestObject = cnv->nearestEdge->object;
            }

            if (selection.contains(nearestObject)) {

                // Sort selected objects by X position
                std::sort(selection.begin(), selection.end(), cnv->sortObjectsByPos);

                auto* conObj = cnv->connectingEdges.getFirst()->object;

                if ((conObj->numOutputs > 1) && selection.contains(conObj)) {

                    // If selected start object has multiple outlets, connect them in selected order
                    int outletIdx = conObj->numInputs + cnv->connectingEdges.getFirst()->ioletIdx;
                    for (auto* sel : selection) {
                        if ((sel != conObj) && (conObj->iolets[outletIdx]) && (sel->numInputs)) {
                            cnv->connections.add(new Connection(cnv, conObj->iolets[outletIdx], sel->iolets.getFirst(), false));
                            outletIdx = outletIdx + 1;
                        }
                    }
                } else {
                    // Else connect all selected objects to the only outlet
                    for (auto* sel : selection) {
                        sel->iolets.getFirst()->createConnection();
                    }
                }
            }
            cnv->connectingEdges.clear();

        } else if (!e.mouseWasDraggedSinceMouseDown() && e.mods.isShiftDown()) {
            createConnection();

        } else if (cnv->connectingWithDrag && cnv->nearestEdge && e.mods.isShiftDown()) {
            // Releasing a connect-by-drag action

            cnv->nearestEdge->isTargeted = false;
            cnv->nearestEdge->repaint();

            for (auto& iolet : cnv->connectingEdges) {
                cnv->nearestEdge->createConnection();
            }

            cnv->nearestEdge = nullptr;
            cnv->connectingWithDrag = false;
            cnv->repaint();
        }
        if (!e.mods.isShiftDown() || cnv->connectingEdges.size() != 1) {
            cnv->connectingEdges.clear();
        }

        // TODO: is this needed?
        //
        // Unless the call originates from a connection, clear any connections that are being created
        /* if (cnv->connectingWithDrag && !dynamic_cast<Connection*>(e.originalComponent)) {
             cnv->connectingEdges.clear();
             cnv->connectingWithDrag = false;
             cnv->repaint();
         }  */

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
    if (!object->cnv->connectingEdges.isEmpty())
    {
        for(auto& iolet : object->cnv->connectingEdges) {
            // Check type for input and output
            bool sameDirection = isInlet == iolet->isInlet;

            bool connectionAllowed = iolet->object != object && !sameDirection;

            // Don't create if this is the same iolet
            if (iolet == this)
            {
                object->cnv->connectingEdges.remove(&iolet);
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
                    object->cnv->connectingEdges.add(selectedBox->iolets[position]);
                }
                else if(!isInlet && position < selectedBox->numOutputs) {
                    object->cnv->connectingEdges.add(selectedBox->iolets[selectedBox->numInputs + position]);
                }
            }
        }
        else {
            object->cnv->connectingEdges.add(this);
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
        auto bounds = iolet->getCanvasBounds().expanded(150, 150);
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
