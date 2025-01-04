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

Iolet::Iolet(Object* parent, bool const inlet)
    : NVGComponent(this)
    , object(parent)
    , cnv(object->cnv)
    , isSignal(false)
    , isGemState(false)
    , insideGraph(parent->cnv->isGraph)
{
    isInlet = inlet;
    setSize(8, 8);

    setAlwaysOnTop(true);

    parent->addAndMakeVisible(this);
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
    bool const isHovering = isTargeted && !isLocked;

    auto const innerCol = isLocked ? cnv->ioletLockedCol : isSignal ? cnv->sigCol
        : isGemState                                                ? cnv->gemCol
                                                                    : cnv->dataCol;
    auto iB = PlugDataLook::useSquareIolets ? getLocalBounds().toFloat().reduced(2.0f, 3.33f) : getLocalBounds().toFloat().reduced(2.0f);
    if (isHovering)
        iB.expand(1.0f, 1.0f);

    nvgDrawRoundedRect(nvg, iB.getX(), iB.getY(), iB.getWidth(), iB.getHeight(), innerCol, cnv->objectOutlineCol, PlugDataLook::useSquareIolets ? 0.0f : iB.getWidth() * 0.5f);
}

bool Iolet::hitTest(int const x, int const y)
{
    // If locked, don't intercept mouse clicks
    if (locked)
        return false;

    if (patchDownwardsOnly && isInlet && !cnv->connectingWithDrag)
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
    if (locked || commandLocked || e.mods.isMiddleButtonDown() || (patchDownwardsOnly && isInlet))
        return;

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
            auto* nearest = findNearestIolet(cnv, e.getEventRelativeTo(cnv).getPosition(), !connectingIolet->isInlet, connectingIolet->object);

            if (nearest && cnv->nearestIolet != nearest) {
                nearest->isTargeted = true;
                auto const tooltip = nearest->getTooltip();
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
    if (locked || commandLocked || e.mods.isRightButtonDown())
        return;

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
            cnv->nearestIolet->isTargeted = false;
            cnv->nearestIolet->repaint();

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

    auto const tooltip = getTooltip();
    if (cnv->connectionsBeingCreated.size() == 1 && tooltip.isNotEmpty()) {
        cnv->editor->tooltipWindow.displayTip(getScreenPosition(), tooltip);
    }

    for (auto const& iolet : object->iolets)
        iolet->repaint();
}

void Iolet::mouseExit(MouseEvent const& e)
{
    isTargeted = false;
    object->drawIoletExpanded = false;

    if (cnv->connectionsBeingCreated.size() == 1) {
        cnv->editor->tooltipWindow.hideTip();
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
        if (object->iolets[nextIdx]->isInlet == isInlet) {
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
            bool const sameDirection = isInlet == c->getIolet()->isInlet;

            // Create new connection if allowed
            if (bool const connectionAllowed = c->getIolet() != this && c->getIolet()->object != object && !sameDirection) {

                auto const outlet = isInlet ? c->getIolet() : this;
                auto const inlet = isInlet ? this : c->getIolet();

                auto const outobj = outlet->object;
                auto const inobj = inlet->object;

                auto const outIdx = outlet->ioletIdx;
                auto const inIdx = inlet->ioletIdx;

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
            position = isInlet ? position : position - object->numInputs;
            for (auto* selectedBox : object->cnv->getSelectionOfType<Object>()) {
                if (isInlet && position < selectedBox->numInputs) {
                    object->cnv->connectionsBeingCreated.add(selectedBox->iolets[position], selectedBox->cnv);
                } else if (!isInlet && position < selectedBox->numOutputs) {
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

Iolet* Iolet::findNearestIolet(Canvas* cnv, Point<int> position, bool const inlet, Object* objectToExclude)
{
    // Find all potential iolets
    SmallArray<Iolet*> allIolets;
    for (auto* object : cnv->objects) {
        for (auto& iolet : object->iolets) {
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
    if (v.refersToSameSourceAs(cnv->locked)) {
        locked = getValue<bool>(v);
        repaint();
    } else if (v.refersToSameSourceAs(cnv->commandLocked)) {
        commandLocked = getValue<bool>(v);
        repaint();
    } else if (v.refersToSameSourceAs(cnv->presentationMode)) {
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
