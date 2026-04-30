/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/NVGUtils.h"

#include "NVGSurface.h"
#include "Iolet.h"

#include "Object.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "Connection.h"
#include "LookAndFeel.h"

Iolet::Iolet(Object* parent, bool const isInlet, uint16 index)
    : NVGComponent(this)
    , object(parent)
    , ioletIdx(index)
    , inlet(isInlet)
    , type(Data)
    , insideGraph(parent->cnv->isGraph)
{
    setSize(8, 8);

    setAlwaysOnTop(true);

    parent->addAndMakeVisible(this);

    auto* cnv = object->cnv;
    cnv->locked.addListener(this);
    cnv->commandLocked.addListener(this);
    cnv->presentationMode.addListener(this);

    locked = getValue<bool>(cnv->locked);
    commandLocked = getValue<bool>(cnv->commandLocked);
    presentationMode = getValue<bool>(cnv->presentationMode);

    patchDownwardsOnly = SettingsFile::getInstance()->getProperty<bool>("patch_downwards_only");

    setVisible(!presentationMode && !insideGraph);
}

Iolet::~Iolet()
{
    auto* cnv = object->cnv;
    cnv->locked.removeListener(this);
    cnv->commandLocked.removeListener(this);
    cnv->presentationMode.removeListener(this);
}

void Iolet::settingsChanged(String const& name, var const& value)
{
    if (name == "patch_downwards_only") {
        patchDownwardsOnly = static_cast<bool>(value);
    }
}

Rectangle<int> Iolet::getCanvasBounds() const
{
    // Get bounds relative to canvas, used for positioning connections
    return getBounds() + object->getBounds().getPosition();
}

void Iolet::render(NVGcontext* nvg)
{
    if (!isVisible())
        return;

    bool const isLocked = locked || commandLocked;
    bool const isHovering = targeted && !isLocked;

    auto const innerCol = isLocked ? nvgColour(PlugDataColours::canvasBackgroundColour.contrasting(0.5f)) : type == Signal ? nvgColour(PlugDataColours::signalColour)
        : type == GemState                                                                                                 ? nvgColour(PlugDataColours::gemColour)
                                                                                                                           : nvgColour(PlugDataColours::dataColour);
    auto iB = PlugDataLook::useSquareIolets ? getLocalBounds().toFloat().reduced(2.0f, 3.33f) : getLocalBounds().toFloat().reduced(2.0f);
    if (isHovering)
        iB.expand(1.0f, 1.0f);

    nvgDrawRoundedRect(nvg, iB.getX(), iB.getY(), iB.getWidth(), iB.getHeight(), innerCol, nvgColour(PlugDataColours::objectOutlineColour), PlugDataLook::useSquareIolets ? 0.0f : iB.getWidth() * 0.5f);
}

bool Iolet::hitTest(int const x, int const y)
{
    // If locked, don't intercept mouse clicks
    if (locked)
        return false;

    if (patchDownwardsOnly && inlet && !object->cnv->connectingWithDrag)
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
    if (locked || commandLocked || e.mods.isMiddleButtonDown() || (patchDownwardsOnly && inlet))
        return;

    auto* cnv = object->cnv;
    if (!cnv->connectionCancelled && cnv->connectionsBeingCreated.empty() && e.getLengthOfMousePress() > 100) {
        MessageManager::callAsync([_this = SafePointer(this)] {
            if (_this) {
                _this->createConnection();
                _this->object->cnv->connectingWithDrag = true;
            }
        });
    }
    if (cnv->connectingWithDrag && !cnv->connectionsBeingCreated.empty()) {

        if (auto const* connectingIolet = cnv->connectionsBeingCreated[0]->getIolet()) {
            auto* nearest = findNearestIolet(cnv, e.getEventRelativeTo(cnv).getPosition(), !connectingIolet->inlet, connectingIolet->getObject());

            if (nearest && cnv->nearestIolet != nearest) {
                nearest->setTargeted(true);
                auto const tooltip = nearest->getTooltip();
                if (tooltip.isNotEmpty()) {
                    cnv->editor->tooltipWindow.displayTip(nearest->getScreenPosition(), tooltip);
                }

                if (cnv->nearestIolet) {
                    cnv->nearestIolet->setTargeted(false);
                }

                cnv->nearestIolet = nearest;
                cnv->nearestIolet->repaint();
            } else if (!nearest && cnv->nearestIolet) {
                cnv->editor->tooltipWindow.hideTip();
                cnv->nearestIolet->setTargeted(false);
                cnv->nearestIolet = nullptr;
            }
        }
    }
}

void Iolet::mouseUp(MouseEvent const& e)
{
    if (locked || commandLocked || e.mods.isRightButtonDown())
        return;

    auto* cnv = object->cnv;

    bool const wasDragged = e.mouseWasDraggedSinceMouseDown();
    cnv->editor->tooltipWindow.hideTip();

    if (!wasDragged && cnv->connectionsBeingCreated.empty()) {
        createConnection();

    } else if (!cnv->connectionsBeingCreated.empty()) {
        // Releasing a connect-by-click action
        if (!wasDragged) {
            createConnection();
            if (!e.mods.isShiftDown())
                cnv->cancelConnectionCreation();

        } else if (cnv->connectingWithDrag && cnv->nearestIolet) {
            // Releasing a connect-by-drag action
            cnv->nearestIolet->setTargeted(false);

            // CreateConnection will automatically create connections for all connections that are being created!
            cnv->nearestIolet->createConnection();

            if (!e.mods.isShiftDown())
                cnv->cancelConnectionCreation();
            cnv->nearestIolet = nullptr;
            cnv->connectingWithDrag = false;

        } else if (cnv->connectingWithDrag) {
            cnv->cancelConnectionCreation();
        }
        if (cnv->connectionsBeingCreated.size() != 1) {
            cnv->connectionsBeingCreated.clear();
            cnv->repaint();
            cnv->connectingWithDrag = false;
        }
        if (cnv->nearestIolet) {
            cnv->nearestIolet->setTargeted(false);
            cnv->nearestIolet = nullptr;
        }
    }
    cnv->connectionCancelled = false;
}

void Iolet::mouseEnter(MouseEvent const& e)
{
    targeted = true;
    object->drawIoletExpanded = true;

    auto const tooltip = getTooltip();
    if (object->cnv->connectionsBeingCreated.size() == 1 && tooltip.isNotEmpty()) {
        object->cnv->editor->tooltipWindow.displayTip(getScreenPosition(), tooltip);
    }

    for (auto const& iolet : object->iolets)
        iolet->repaint();
}

void Iolet::mouseExit(MouseEvent const& e)
{
    targeted = false;
    object->drawIoletExpanded = false;

    if (object->cnv->connectionsBeingCreated.size() == 1) {
        object->cnv->editor->tooltipWindow.hideTip();
    }

    for (auto const& iolet : object->iolets)
        iolet->repaint();
}

Iolet* Iolet::getNextIolet()
{
    int const oldIdx = object->iolets.index_of(this);
    int const ioletCount = object->iolets.size();

    for (int offset = 1; offset < ioletCount; offset++) {
        int const nextIdx = (oldIdx + offset) % ioletCount;
        if (object->iolets[nextIdx]->inlet == inlet) {
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
    if (!cnv->connectionsBeingCreated.empty()) {

        cnv->patch.startUndoSequence("Connecting");

        for (auto const& c : object->cnv->connectionsBeingCreated) {

            if (!c->getIolet())
                continue;

            // Check type for input and output
            bool const sameDirection = inlet == c->getIolet()->inlet;

            // Create new connection if allowed
            if (c->getIolet() != this && c->getIolet()->object != object && !sameDirection) {

                auto const connectionOutlet = inlet ? c->getIolet() : this;
                auto const connectionInlet = inlet ? this : c->getIolet();

                auto const outobj = connectionOutlet->object;
                auto const inobj = connectionInlet->object;

                auto const outIdx = connectionOutlet->ioletIdx;
                auto const inIdx = connectionInlet->ioletIdx;

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

            int position = object->iolets.index_of(this);
            position = inlet ? position : position - object->numInputs;
            for (auto* selectedBox : object->cnv->getSelectionOfType<Object>()) {
                if (inlet && position < selectedBox->numInputs) {
                    object->cnv->connectionsBeingCreated.add(selectedBox->iolets[position], selectedBox->cnv);
                } else if (!inlet && position < selectedBox->numOutputs) {
                    object->cnv->connectionsBeingCreated.add(selectedBox->iolets[selectedBox->numInputs + position], selectedBox->cnv);
                }
            }
        } else {
            object->cnv->connectionsBeingCreated.add(this, object->cnv);
        }
    }
}

SmallArray<Connection*> Iolet::getConnections() const
{
    SmallArray<Connection*> result;
    for (auto* c : object->cnv->connections) {
        if (c->inlet == this || c->outlet == this) {
            result.add(c);
        }
    }

    return result;
}

Iolet* Iolet::findNearestIolet(Canvas* cnv, Point<int> const position, bool const inlet, Object const* objectToExclude)
{
    // Find all potential iolets
    SmallArray<Iolet*> allIolets;
    for (auto* object : cnv->objects) {
        for (auto& iolet : object->iolets) {
            if (iolet->inlet == inlet && iolet->getObject() != objectToExclude) {
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
    if (v.refersToSameSourceAs(object->cnv->locked)) {
        locked = getValue<bool>(v);
        repaint();
    } else if (v.refersToSameSourceAs(object->cnv->commandLocked)) {
        commandLocked = getValue<bool>(v);
        repaint();
    } else if (v.refersToSameSourceAs(object->cnv->presentationMode)) {
        presentationMode = getValue<bool>(v);
        setVisible(!isSymbolIolet && !presentationMode && !insideGraph);
        repaint();
    } else { // patch_downards_only changed
        patchDownwardsOnly = getValue<bool>(v);
    }
}

void Iolet::setHidden(bool const hidden)
{
    isSymbolIolet = hidden;
    setVisible(!isSymbolIolet && !presentationMode && !insideGraph);
    repaint();
}

void Iolet::setType(IoletType newType)
{
    type = newType;
    repaint();
}

uint16 Iolet::getIndex() const
{
    return ioletIdx;
}

bool Iolet::isInlet() const
{
    return inlet;
}

bool Iolet::isSignal() const
{
    return type == Signal;
}

bool Iolet::isGemState() const
{
    return type == GemState;
}

void Iolet::setTargeted(bool shouldBeTargeted)
{
    targeted = shouldBeTargeted;
    repaint();
}

bool Iolet::isTargeted() const
{
    return targeted;
}

Object* Iolet::getObject() const
{
    return object;
}
