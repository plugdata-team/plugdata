/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Iolet.h"

#include "Object.h"
#include "Canvas.h"
#include "Connection.h"
#include "LookAndFeel.h"

Iolet::Iolet(Object* parent, bool inlet)
    : object(parent)
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

    // Drawing circles is more expensive than you might think, especially because there can be a lot of iolets!
    setBufferedToImage(true);

    cnv = findParentComponentOfClass<Canvas>();
}

Rectangle<int> Iolet::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return cnv->getLocalArea(this, getLocalBounds());
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

void Iolet::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    bool isLocked = getValue<bool>(locked) || getValue<bool>(commandLocked);
    bool down = isMouseButtonDown();
    bool over = isMouseOver();

    if ((!isTargeted && !over) || isLocked) {
        bounds = bounds.reduced(2);
    }

    auto backgroundColour = isSignal ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId);

    if ((down || over) && !isLocked)
        backgroundColour = backgroundColour.contrasting(down ? 0.2f : 0.05f);

    if (isLocked) {
        backgroundColour = findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.5f);
    }

    // Instead of drawing pie segments, just clip the graphics region to the visible iolets of the object
    // This is much faster!
    bool stateSaved = false;
    if (!(object->isMouseOverOrDragging(true) || over || isTargeted) || isLocked) {
        g.saveState();
        g.reduceClipRegion(getLocalArea(object, object->getLocalBounds().reduced(Object::margin)));
        stateSaved = true;
    }

    // TODO: this is kind of a hack to force inlets to align correctly. Find a better way to fix this!
    if ((getHeight() % 2) == 0) {
        bounds.translate(0.0f, isInlet ? -1.0f : 0.0f);
    }

    if (PlugDataLook::getUseSquareIolets()) {
        g.setColour(backgroundColour);
        g.fillRect(bounds);

        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRect(bounds, 1.0f);
    } else {
        g.setColour(backgroundColour);
        g.fillEllipse(bounds);

        g.setColour(findColour(PlugDataColour::ioletOutlineColourId));
        g.drawEllipse(bounds, 1.0f);
    }

    if (stateSaved) {
        g.restoreState();
    }
}

void Iolet::mouseDrag(MouseEvent const& e)
{
    // Ignore when locked or if middlemouseclick?
    if (getValue<bool>(locked) || e.mods.isMiddleButtonDown())
        return;

    if (!cnv->connectionCancelled && cnv->connectionsBeingCreated.isEmpty() && e.getLengthOfMousePress() > 100) {
        MessageManager::callAsync([_this = SafePointer(this)]() {
            _this->createConnection();
            _this->object->cnv->connectingWithDrag = true;
        });
    }
    if (cnv->connectingWithDrag && !cnv->connectionsBeingCreated.isEmpty()) {
        auto* connectingIolet = cnv->connectionsBeingCreated[0]->getIolet();

        if (connectingIolet) {
            auto* nearest = findNearestIolet(cnv, e.getEventRelativeTo(cnv).getPosition(), !connectingIolet->isInlet, connectingIolet->object);

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
}

void Iolet::mouseUp(MouseEvent const& e)
{
    if (getValue<bool>(locked) || e.mods.isRightButtonDown())
        return;

    // This might end up calling Canvas::synchronise, at which point we are not sure this class will survive, so we do an async call

    bool shiftIsDown = e.mods.isShiftDown();
    bool wasDragged = e.mouseWasDraggedSinceMouseDown();

    MessageManager::callAsync([this, _this = SafePointer(this), shiftIsDown, wasDragged]() mutable {
        if (!_this)
            return;

        auto* cnv = findParentComponentOfClass<Canvas>();

        if (!cnv)
            return;

        if (!wasDragged && cnv->connectionsBeingCreated.isEmpty()) {
            createConnection();

        } else if (!cnv->connectionsBeingCreated.isEmpty()) {
            if (!wasDragged && !shiftIsDown) {
                createConnection();
                cnv->cancelConnectionCreation();

            } else if (cnv->connectingWithDrag && cnv->nearestIolet && !shiftIsDown) {
                // Releasing a connect-by-drag action

                cnv->nearestIolet->isTargeted = false;
                cnv->nearestIolet->repaint();

                // CreateConnection will automatically create connections for all connections that are being created!
                cnv->nearestIolet->createConnection();

                cnv->cancelConnectionCreation();
                cnv->nearestIolet = nullptr;
                cnv->connectingWithDrag = false;

            } else if (shiftIsDown && cnv->getSelectionOfType<Object>().size() > 1 && (cnv->connectionsBeingCreated.size() == 1)) {

                //
                // Auto patching
                //

                auto selection = cnv->getSelectionOfType<Object>();

                Object* nearestObject = object;
                int inletIdx = ioletIdx;
                if (cnv->nearestIolet) {
                    // If connected by drag
                    nearestObject = cnv->nearestIolet->object;
                    inletIdx = cnv->nearestIolet->ioletIdx;
                }

                // Sort selected objects by X position
                std::sort(selection.begin(), selection.end(), [](Object const* lhs, Object const* rhs) {
                    return lhs->getX() < rhs->getX();
                });

                auto* conObj = cnv->connectionsBeingCreated.getFirst()->getIolet()->object;

                if ((conObj->numOutputs > 1) && selection.contains(conObj) && selection.contains(nearestObject)) {

                    // If selected 'start object' has multiple outlets
                    // Connect all selected objects beneath to 'start object' outlets, ordered by position
                    int outletIdx = conObj->numInputs + cnv->connectionsBeingCreated.getFirst()->getIolet()->ioletIdx;
                    for (auto* sel : selection) {
                        if ((sel != conObj) && (conObj->iolets[outletIdx]) && (sel->numInputs)) {
                            if ((sel->getX() >= nearestObject->getX()) && (sel->getY() > (conObj->getY() + conObj->getHeight() - 15))) {
                                cnv->connections.add(new Connection(cnv, conObj->iolets[outletIdx], sel->iolets.getFirst(), nullptr));
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

                                cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], nearestObject->iolets[inletIdx], nullptr));
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
                            cnv->connections.add(new Connection(cnv, cnv->connectionsBeingCreated.getFirst()->getIolet(), sel->iolets.getFirst(), nullptr));
                        } else {
                            cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], nearestObject->iolets.getFirst(), nullptr));
                        }
                    }
                }

                else {
                    // If 'start object' is selected
                    // Connect 'end inlet' with all selected objects
                    for (auto* sel : selection) {
                        if (cnv->nearestIolet) {
                            cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], cnv->nearestIolet, nullptr));
                        } else {
                            cnv->connections.add(new Connection(cnv, sel->iolets[sel->numInputs], this, nullptr));
                        }
                    }
                }

                cnv->connectionsBeingCreated.clear();

            } else if (!wasDragged && shiftIsDown) {
                createConnection();
            } else if (cnv->connectingWithDrag && cnv->nearestIolet && shiftIsDown) {
                // Releasing a connect-by-drag action
                cnv->nearestIolet->isTargeted = false;
                cnv->nearestIolet->repaint();

                cnv->nearestIolet->createConnection();

                cnv->nearestIolet = nullptr;
                cnv->connectingWithDrag = false;
                cnv->repaint();
            }
            if (!shiftIsDown || cnv->connectionsBeingCreated.size() != 1) {
                cnv->connectionsBeingCreated.clear();
                cnv->repaint();
                cnv->connectingWithDrag = false;
            }
            if (cnv->nearestIolet) {
                cnv->nearestIolet->isTargeted = false;
                cnv->nearestIolet->repaint();
                cnv->nearestIolet = nullptr;
            }
        }
        cnv->connectionCancelled = false;
    });
}

void Iolet::mouseEnter(MouseEvent const& e)
{
    for (auto& iolet : object->iolets)
        iolet->repaint();
}

void Iolet::mouseExit(MouseEvent const& e)
{
    for (auto& iolet : object->iolets)
        iolet->repaint();
}

void Iolet::createConnection()
{
    auto* cnv = object->cnv;

    cnv->hideAllActiveEditors();

    // Check if this is the start or end action of connecting
    if (!cnv->connectionsBeingCreated.isEmpty()) {

        cnv->patch.startUndoSequence("Connecting");

        for (auto& c : object->cnv->connectionsBeingCreated) {

            if (!c->getIolet())
                continue;

            // Check type for input and output
            bool sameDirection = isInlet == c->getIolet()->isInlet;

            bool connectionAllowed = c->getIolet() != this && c->getIolet()->object != object && !sameDirection;

            // Create new connection if allowed
            if (connectionAllowed) {

                auto outlet = isInlet ? c->getIolet() : this;
                auto inlet = isInlet ? this : c->getIolet();

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

    }
    // otherwise set this iolet as start of a connection
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
}

void Iolet::clearConnections()
{
    auto* cnv = object->cnv;
    for (auto* c : getConnections()) {
        auto* checkedOutObj = pd::Interface::checkObject(c->outobj->getPointer());
        auto* checkedInObj = pd::Interface::checkObject(c->inobj->getPointer());
        if (checkedInObj && checkedOutObj && cnv->patch.hasConnection(checkedOutObj, c->outIdx, checkedInObj, c->inIdx)) {
            // Delete connection from pd if we haven't done that yet
            cnv->patch.removeConnection(checkedOutObj, c->outIdx, checkedInObj, c->inIdx, c->getPathState());
        }

        cnv->connections.removeObject(c);
    }
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
    // Find all iolets
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
        auto bounds = iolet->getCanvasBounds().expanded(50);
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
