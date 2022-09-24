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
    
    auto backgroundColour = isSignal ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::highlightColourId);

    if ((down || over) && !isLocked) backgroundColour = backgroundColour.contrasting(down ? 0.2f : 0.05f);

    if (isLocked)
    {
        backgroundColour = findColour(PlugDataColour::canvasColourId).contrasting(0.5f);
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

    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
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
    if (bool(locked.getValue())) return;

    if (object->cnv->connectingEdges.isEmpty() && e.getLengthOfMousePress() > 300)
    {
        createConnection();
        auto* cnv = findParentComponentOfClass<Canvas>();
        cnv->connectingWithDrag = true;
    }
}

void Iolet::mouseUp(const MouseEvent& e)
{
    if (bool(locked.getValue())) return;

    if (!e.mouseWasDraggedSinceMouseDown())
    {
        bool needsClearing = !object->cnv->connectingEdges.isEmpty() && !(object->cnv->connectingEdges.size() == 1 && e.mods.isShiftDown());
        createConnection();
        if(needsClearing) object->cnv->connectingEdges.clear();
        object->cnv->repaint();
    }

    if (object->cnv->nearestEdge && !object->cnv->connectingEdges.isEmpty() && object->cnv->connectingEdges.getReference(0).getComponent() == this && getLocalBounds().contains(e.getPosition()))
    {
        object->cnv->nearestEdge->isTargeted = false;
        object->cnv->nearestEdge->repaint();
        object->cnv->nearestEdge = nullptr;
        object->cnv->connectingEdges.clear();
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
