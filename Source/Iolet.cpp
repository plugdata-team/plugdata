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
#include "Iolet.h"

#include "Object.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "Connection.h"
#include "LookAndFeel.h"

Iolet::Iolet(Object* parent, bool inlet)
    : NVGComponent(this)
    , object(parent)
    , isSignal(false)
    , isGemState(false)
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

    // replicate behaviour of PD-Vanilla downwards only patching - optional
    patchDownwardsOnly.referTo(SettingsFile::getInstance()->getValueTree(), "patch_downwards_only", nullptr);
}

Rectangle<int> Iolet::getCanvasBounds()
{
    // Get bounds relative to canvas, used for positioning connections
    return getBounds() + object->getBounds().getPosition();
}

void Iolet::render(NVGcontext* nvg)
{
    if (!isVisible())
        return;

    auto& fb = cnv->ioletBuffer;

    if (!fb.isValid())
        return;

    bool isLocked = getValue<bool>(locked) || getValue<bool>(commandLocked);
    bool overObject = object->drawIoletExpanded;
    bool isHovering = isTargeted && !isLocked;
    
    // If a connection is being created, don't hide iolets with a symbol defined
    if (cnv->connectionsBeingCreated.isEmpty() || cnv->connectionsBeingCreated[0]->getIolet()->isInlet == isInlet) {
        if ((isLocked && isSymbolIolet) || (isSymbolIolet && !isHovering && !overObject && !object->isSelected()))
            return;
    }

    int type = isSignal + (isGemState * 2);
    if (isLocked)
        type = 3;
    
    if (isLocked || !(overObject || isHovering) || (patchDownwardsOnly.get() && isInlet)) {
        auto clipBounds = object->getLocalBounds().reduced(Object::margin) - getPosition();
        nvgIntersectScissor(nvg, clipBounds.getX(), clipBounds.getY(), clipBounds.getWidth(), clipBounds.getHeight());
    }

    auto scale = getWidth() / 13.0f;
    auto offset = isInlet ? 0.5f : 0.0f;
    if(scale != 1.0f) nvgScale(nvg, scale, scale); // If the iolet is shrunk because there is little space, we scale it down
    nvgFillPaint(nvg, nvgImagePattern(nvg, isHovering * -16 - 1.5f, type * -16 - offset, 16 * 4, 16 * 4, 0, fb.getImage(), 1));

    nvgFillRect(nvg, 0, 0, 13, 13);
}

bool Iolet::hitTest(int x, int y)
{
    // If locked, don't intercept mouse clicks
    if ((getValue<bool>(locked) || getValue<bool>(commandLocked)))
        return false;

    if (patchDownwardsOnly.get() && isInlet && !cnv->connectingWithDrag)
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
    if (getValue<bool>(locked) || e.mods.isMiddleButtonDown() || (patchDownwardsOnly.get() && isInlet))
        return;

    if (!cnv->connectionCancelled && cnv->connectionsBeingCreated.isEmpty() && e.getLengthOfMousePress() > 100) {
        MessageManager::callAsync([_this = SafePointer(this)]() {
            if(_this) {
                _this->createConnection();
                _this->object->cnv->connectingWithDrag = true;
            }
        });
    }
    if (cnv->connectingWithDrag && !cnv->connectionsBeingCreated.isEmpty()) {
        auto* connectingIolet = cnv->connectionsBeingCreated[0]->getIolet();

        if (connectingIolet) {
            auto* nearest = findNearestIolet(cnv, e.getEventRelativeTo(cnv).getPosition(), !connectingIolet->isInlet, connectingIolet->object);

            if (nearest && cnv->nearestIolet != nearest) {
                nearest->isTargeted = true;
                auto tooltip = nearest->getTooltip();
                if (tooltip.isNotEmpty()) {
                    cnv->editor->tooltipWindow.displayTip(nearest->getScreenPosition(), tooltip);
                }

                if (cnv->nearestIolet) {
                    cnv->nearestIolet->isTargeted = false;
                    cnv->nearestIolet->repaint();
                }

                cnv->nearestIolet = nearest;
                cnv->nearestIolet->repaint();
            } else if (!nearest && cnv->nearestIolet) {
                cnv->editor->tooltipWindow.hideTip();
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

    bool wasDragged = e.mouseWasDraggedSinceMouseDown();
    auto* cnv = findParentComponentOfClass<Canvas>();

    if (!cnv)
        return;

    cnv->editor->tooltipWindow.hideTip();
    
    if (!wasDragged && cnv->connectionsBeingCreated.isEmpty()) {
        createConnection();

    } else if (!cnv->connectionsBeingCreated.isEmpty()) {
        // Releasing a connect-by-click action
        if (!wasDragged) {
            createConnection();
            if(!e.mods.isShiftDown()) cnv->cancelConnectionCreation();

        } else if (cnv->connectingWithDrag && cnv->nearestIolet) {
            // Releasing a connect-by-drag action
            cnv->nearestIolet->isTargeted = false;
            cnv->nearestIolet->repaint();

            // CreateConnection will automatically create connections for all connections that are being created!
            cnv->nearestIolet->createConnection();

            if(!e.mods.isShiftDown()) cnv->cancelConnectionCreation();
            cnv->nearestIolet = nullptr;
            cnv->connectingWithDrag = false;

        }
        else if(cnv->connectingWithDrag)
        {
            cnv->cancelConnectionCreation();
        }
        if (cnv->connectionsBeingCreated.size() != 1) {
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
}

void Iolet::mouseEnter(MouseEvent const& e)
{
    isTargeted = true;
    object->drawIoletExpanded = true;

    auto tooltip = getTooltip();
    if (cnv->connectionsBeingCreated.size() == 1 && tooltip.isNotEmpty()) {
        cnv->editor->tooltipWindow.displayTip(getScreenPosition(), tooltip);
    }

    for (auto& iolet : object->iolets)
        iolet->repaint();
}

void Iolet::mouseExit(MouseEvent const& e)
{
    isTargeted = false;
    object->drawIoletExpanded = false;

    if (cnv->connectionsBeingCreated.size() == 1) {
        cnv->editor->tooltipWindow.hideTip();
    }

    for (auto& iolet : object->iolets)
        iolet->repaint();
}

Iolet* Iolet::getNextIolet()
{
    int oldIdx = object->iolets.indexOf(this);
    int ioletCount = object->iolets.size();
    
    for(int offset = 1; offset < ioletCount; offset++)
    {
        int nextIdx = (oldIdx + offset) % ioletCount;
        if(object->iolets[nextIdx]->isInlet == isInlet)
        {
            return object->iolets[nextIdx];
        }
    }
    
    return this;
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
        if(Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown()) {
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

Iolet* Iolet::findNearestIolet(Canvas* cnv, Point<int> position, bool inlet, Object* objectToExclude)
{
    // Find all potential iolets
    Array<Iolet*> allIolets;
    for (auto* object : cnv->objects) {
        for (auto* iolet : object->iolets) {
            if (iolet->isInlet == inlet && iolet->object != objectToExclude) {
                allIolets.add(iolet);
            }
        }
    }

    Iolet* nearestIolet = nullptr;
    for (auto& iolet : allIolets) {
        auto bounds = iolet->getCanvasBounds().expanded(20);
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
        setVisible(!getValue<bool>(presentationMode) && !insideGraph);
        repaint();
    }
}

void Iolet::setHidden(bool hidden)
{
    isSymbolIolet = hidden;
    setVisible(!getValue<bool>(presentationMode) && !insideGraph);
    repaint();
}
