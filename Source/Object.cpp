/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Object.h"

#include "Canvas.h"
#include "SuggestionComponent.h"
#include "Connection.h"
#include "Iolet.h"
#include "Constants.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ObjectGrid.h"
#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"

#include "Utility/ObjectBoundsConstrainer.h"
#include "Pd/Patch.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
#include <x_libpd_extra_utils.h>
}

Object::Object(Canvas* parent, String const& name, Point<int> position)
    : cnv(parent), ds(parent->dragState), gui(nullptr)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    if (cnv->attachNextObjectToMouse) {
        cnv->attachNextObjectToMouse = false;
        attachedToMouse = true;
        startTimer(16);
    }

    initialise();

    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty()) {
        setSize(100, height);
    } else {
        setType(name);
    }

    // Open the text editor of a new object if it has one
    // Don't do this if the object is attached to the mouse
    // Param objects are an exception where we don't want to open on mouse-down
    if (attachedToMouse && !name.startsWith("param") && !static_cast<bool>(locked.getValue())) {
        createEditorOnMouseDown = true;
    } else if (!attachedToMouse && !static_cast<bool>(locked.getValue())) {
        showEditor();
    }
}

Object::Object(void* object, Canvas* parent) : ds(parent->dragState), gui(nullptr)
{
    cnv = parent;

    initialise();

    setType("", object);
}

Object::~Object()
{
    cnv->editor->removeModifierKeyListener(this);

    if (attachedToMouse) {
        stopTimer();
    }
}

Rectangle<int> Object::getObjectBounds()
{
    return getBounds().reduced(margin) - cnv->canvasOrigin;
}

void Object::setObjectBounds(Rectangle<int> bounds)
{
    setBounds(bounds.expanded(margin) + cnv->canvasOrigin);
}

void Object::initialise()
{
    constrainer = std::make_unique<ObjectBoundsConstrainer>();

    cnv->addAndMakeVisible(this);
    cnv->editor->addModifierKeyListener(this);

    // Updates lock/unlock mode
    locked.referTo(cnv->locked);
    commandLocked.referTo(cnv->pd->commandLocked);
    presentationMode.referTo(cnv->presentationMode);
    hvccMode.referTo(cnv->editor->hvccMode);

    presentationMode.addListener(this);
    locked.addListener(this);
    commandLocked.addListener(this);
    hvccMode.addListener(this);

    originalBounds.setBounds(0, 0, 0, 0);
    constrainer->setMinimumSize(minimumSize, minimumSize);
}

void Object::timerCallback()
{
    auto pos = cnv->getMouseXYRelative();
    if (pos != getBounds().getCentre()) {
        auto viewArea = cnv->viewport->getViewArea() / cnv->editor->getZoomScaleForCanvas(cnv);
        setCentrePosition(viewArea.getConstrainedPoint(pos));
    }
}

void Object::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(hvccMode)) {
        if (gui) {
            auto typeName = String::fromUTF8(libpd_get_object_class_name(gui->ptr));
            // Check hvcc compatibility
            bool isSubpatch = gui ? gui->getPatch() != nullptr : false;
            isHvccCompatible = !static_cast<bool>(hvccMode.getValue()) || isSubpatch || hvccObjects.contains(typeName);

            if (!isHvccCompatible) {
                cnv->pd->logWarning(String("Warning: object \"" + typeName + "\" is not supported in Compiled Mode").toRawUTF8());
            }

            repaint();
        }

        return;
    }

    // else it was a lock/unlock/presentation mode action
    // Hide certain objects in GOP
    setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));

    if (gui) {
        gui->lock(locked == var(true) || commandLocked == var(true));
    }

    repaint();
}

bool Object::hitTest(int x, int y)
{
    if (Canvas::panningModifierDown())
        return false;

    if (gui && !gui->canReceiveMouseEvent(x, y)) {
        return false;
    }

    // Mouse over object
    if (getLocalBounds().reduced(margin).contains(x, y)) {
        return true;
    }

    // Mouse over iolets
    for (auto* iolet : iolets) {
        if (iolet->getBounds().contains(x, y))
            return true;
    }

    if (cnv->isSelected(this)) {

        for (auto& corner : getCorners()) {
            if (corner.contains(x, y))
                return true;
        }

        return getLocalBounds().reduced(margin).contains(x, y);
    }

    return false;
}

// To make iolets show/hide
void Object::mouseEnter(MouseEvent const& e)
{
    for (auto* iolet : iolets)
        iolet->repaint();
}

void Object::mouseExit(MouseEvent const& e)
{
    // we need to reset the resizeZone & validResizeZone,
    // otherwise it can have an old zone already selected on re-entry
    resizeZone = ResizableBorderComponent::Zone(ResizableBorderComponent::Zone::centre);
    validResizeZone = false;

    for (auto* iolet : iolets)
        iolet->repaint();
}

void Object::mouseMove(MouseEvent const& e)
{
    if (!cnv->isSelected(this) || locked == var(true) || commandLocked == var(true)) {
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        return;
    }

    resizeZone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), Point<int>(e.x, e.y));

    validResizeZone = resizeZone.getZoneFlags() != ResizableBorderComponent::Zone::centre && e.originalComponent == this;

    setMouseCursor(validResizeZone ? resizeZone.getMouseCursor() : MouseCursor::NormalCursor);
    updateMouseCursor();
}

void Object::updateBounds()
{
    if (gui) {
        cnv->pd->setThis();

        // Get the bounds of the object in Pd
        auto newBounds = gui->getPdBounds();
        
        // Objects may return empty bounds if they are not a real object (like scalars)
        if(!newBounds.isEmpty())
            setObjectBounds(newBounds);
    }

    if (newObjectEditor) {
        textEditorTextChanged(*newObjectEditor);
    }

    resized();
}

void Object::setType(String const& newType, void* existingObject)
{
    // Change object type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    void* objectPtr;
    // "exists" indicates that this object already exists in pd
    // When setting exists to true, the gui needs to be assigned already
    if (!existingObject || cnv->patch.objectWasDeleted(existingObject)) {
        auto* patch = &cnv->patch;
        if (gui) {
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections())
                cnv->connections.removeObject(connection);

            objectPtr = patch->renameObject(getPointer(), newType);

            // Synchronise to make sure connections are preserved correctly
            cnv->synchronise();
        } else {
            auto rect = getObjectBounds();
            objectPtr = patch->createObject(newType, rect.getX(), rect.getY());
        }
    } else {
        objectPtr = existingObject;
    }

    // Create gui for the object
    gui.reset(ObjectBase::createGui(objectPtr, this));

    if (gui) {
        gui->lock(locked == var(true));
        gui->addMouseListener(this, true);
        addAndMakeVisible(gui.get());
    }

    auto typeName = String::fromUTF8(libpd_get_object_class_name(objectPtr));
    // Check hvcc compatibility
    bool isSubpatch = gui ? gui->getPatch() != nullptr : false;
    isHvccCompatible = !static_cast<bool>(hvccMode.getValue()) || isSubpatch || hvccObjects.contains(typeName);

    if (!isHvccCompatible) {
        cnv->pd->logWarning(String("Warning: object \"" + typeName + "\" is not supported in Compiled Mode").toRawUTF8());
    }

    // Update inlets/outlets
    updateIolets();
    updateBounds();

    // Auto patching
    if (static_cast<bool>(!attachedToMouse && cnv->editor->autoconnect.getValue()) && numInputs && cnv->lastSelectedObject && cnv->lastSelectedObject->numOutputs) {
        auto outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        auto inlet = iolets[0];
        if (outlet->isSignal == inlet->isSignal) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet]() {
                cnv->connections.add(new Connection(cnv, outlet, inlet, nullptr));
            });
        }
    }
    if (cnv->lastSelectedConnection && numInputs && numOutputs) {
        // if 1 connection is selected, connect the new object in middle of connection
        auto outobj = cnv->lastSelectedConnection->outobj;
        auto inobj = cnv->lastSelectedConnection->inobj;
        auto outlet = outobj->iolets[outobj->numInputs + cnv->lastSelectedConnection->outIdx];
        auto inlet = inobj->iolets[cnv->lastSelectedConnection->inIdx];
        if ((outlet->isSignal == iolets[0]->isSignal) && (inlet->isSignal == iolets[this->numInputs]->isSignal)) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet]() {
                cnv->connections.add(new Connection(cnv, outlet, iolets[0], nullptr));
                cnv->connections.add(new Connection(cnv, iolets[this->numInputs], inlet, nullptr));
            });
            // remove the previous connection
            cnv->patch.removeConnection(outobj->getPointer(), cnv->lastSelectedConnection->outIdx, inobj->getPointer(), cnv->lastSelectedConnection->inIdx, cnv->lastSelectedConnection->getPathState());
            cnv->connections.removeObject(cnv->lastSelectedConnection);
        }
    }
    cnv->lastSelectedObject = nullptr;
    cnv->lastSelectedConnection = nullptr;

    cnv->editor->updateCommandStatus();
}

Array<Rectangle<float>> Object::getCorners() const
{
    auto rect = getLocalBounds().reduced(margin);
    float const offset = 2.0f;

    Array<Rectangle<float>> corners = { Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
        Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset) };

    return corners;
}

void Object::paintOverChildren(Graphics& g)
{
    if (isSearchTarget) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::violet);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (!isHvccCompatible) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::orange);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (attachedToMouse) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::lightgreen);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (indexShown) {
        int halfHeight = 5;

        auto text = String(cnv->objects.indexOf(this));
        int textWidth = Fonts::getMonospaceFont().withHeight(10).getStringWidth(text) + 5;
        int left = std::min<int>(getWidth() - (1.5 * margin), getWidth() - textWidth);

        auto indexBounds = Rectangle<int>(left, (getHeight() / 2) - halfHeight, getWidth() - left, halfHeight * 2);

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
        g.fillRoundedRectangle(indexBounds.toFloat(), 2.0f);

        Fonts::drawStyledText(g, text, indexBounds, findColour(PlugDataColour::objectSelectedOutlineColourId).contrasting(), Monospace, 10, Justification::centred);
    }
}

void Object::paint(Graphics& g)
{
    if ((cnv->isSelected(this) && !cnv->isGraph) || newObjectEditor) {

        if (newObjectEditor) {

            g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), Corners::objectCornerRadius);

            g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), Corners::objectCornerRadius, 1.0f);
        }

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));

        g.saveState();
        g.excludeClipRegion(getLocalBounds().reduced(margin + 1));

        for (auto& rect : getCorners()) {
            g.fillRoundedRectangle(rect, Corners::objectCornerRadius);
        }
        g.restoreState();
    }
}

void Object::resized()
{
    setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));

    if (gui) {
        gui->setBounds(getLocalBounds().reduced(margin));
    }

    if (newObjectEditor) {
        newObjectEditor->setBounds(getLocalBounds().reduced(margin));
    }

    int ioletSize = 13;
    int ioletHitBox = 4;

    int maxIoletWidth = std::min(((getWidth() - doubleMargin) / std::max(numInputs, 1)) - 4, ((getWidth() - doubleMargin) / std::max(numOutputs, 1)) - 4);
    int maxIoletHeight = (getHeight() / 2.0f) - 3;

    ioletSize = std::max(std::min({ ioletSize, maxIoletWidth, maxIoletHeight }), 10);
    int borderWidth = jmap<float>(ioletSize, 10, 13, 9, 14);

    auto inletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, inletBounds.getWidth() - (ioletHitBox * numInputs) - borderWidth)) {
        inletBounds.removeFromLeft(spaceToRemove);
        inletBounds.removeFromRight(spaceToRemove);
    }

    auto outletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, outletBounds.getWidth() - (ioletHitBox * numOutputs) - borderWidth)) {
        outletBounds.removeFromLeft(spaceToRemove);
        outletBounds.removeFromRight(spaceToRemove);
    }

    int index = 0;
    for (auto& iolet : iolets) {
        bool const isInlet = iolet->isInlet;
        int const position = index < numInputs ? index : index - numInputs;
        int const total = isInlet ? numInputs : numOutputs;
        float const yPosition = (isInlet ? (margin + 1) : getHeight() - margin) - ioletSize / 2.0f;

        auto const bounds = isInlet ? inletBounds : outletBounds;

        if (total == 1 && position == 0) {
            int xPosition = getWidth() < 40 ? getLocalBounds().getCentreX() - ioletSize / 2.0f : bounds.getX();
            iolet->setBounds(xPosition, yPosition, ioletSize, ioletSize);
        } else if (total > 1) {
            float const ratio = (bounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
            iolet->setBounds(bounds.getX() + ratio * position, yPosition, ioletSize, ioletSize);
        }

        index++;
    }
}

void Object::updateTooltips()
{
    if (!gui)
        return;

    std::vector<std::pair<int, String>> inletMessages;
    std::vector<std::pair<int, String>> outletMessages;

    // Set object tooltip
    gui->setTooltip(cnv->pd->objectLibrary.getObjectTooltip(gui->getType()));

    /*
    if (auto* subpatch = gui->getPatch()) {

        // Check child objects of subpatch for inlet/outlet messages
        for (auto* obj : subpatch->getObjects()) {

            const String name = libpd_get_object_class_name(obj);
            if (name == "inlet" || name == "inlet~") {

                int size;
                char* str_ptr;
                libpd_get_object_text(obj, &str_ptr, &size);

                int x, y, w, h;
                libpd_get_object_bounds(subpatch->getPointer(), obj, &x, &y, &w, &h);

                // Anything after the first space will be the comment
                auto const text = String::fromUTF8(str_ptr, size);
                inletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));
                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
            if (name == "outlet" || name == "outlet~") {
                int size;
                char* str_ptr;
                libpd_get_object_text(obj, &str_ptr, &size);

                int x, y, w, h;
                libpd_get_object_bounds(subpatch->getPointer(), obj, &x, &y, &w, &h);

                auto const text = String::fromUTF8(str_ptr, size);
                outletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));

                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
        }
    } */

    auto sortFunc = [](std::pair<int, String>& a, std::pair<int, String>& b) {
        return a.first < b.first;
    };

    std::sort(inletMessages.begin(), inletMessages.end(), sortFunc);
    std::sort(outletMessages.begin(), outletMessages.end(), sortFunc);

    int numIn = 0;
    int numOut = 0;

    // Check pd library for pddp tooltips, those have priority
    auto ioletTooltips = cnv->pd->objectLibrary.getIoletTooltips(gui->getType(), gui->getText(), numInputs, numOutputs);

    for (int i = 0; i < iolets.size(); i++) {
        auto* iolet = iolets[i];

        auto& tooltip = ioletTooltips[!iolet->isInlet][iolet->isInlet ? i : i - numInputs];

        // Don't overwrite custom documentation
        if (tooltip.isNotEmpty()) {
            iolet->setTooltip(tooltip);
            continue;
        }

        if ((iolet->isInlet && numIn >= inletMessages.size()) || (!iolet->isInlet && numOut >= outletMessages.size()))
            continue;

        auto& [x, message] = iolet->isInlet ? inletMessages[numIn++] : outletMessages[numOut++];
        iolet->setTooltip(message);
    }
}

void Object::updateIolets()
{
    if (!getPointer())
        return;

    // update inlets and outlets
    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto* iolet : iolets) {
        iolet->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (auto* ptr = pd::Patch::checkObject(getPointer())) {
        numInputs = libpd_ninlets(ptr);
        numOutputs = libpd_noutlets(ptr);
    }

    for (auto* iolet : iolets) {
        if (gui && !iolet->isInlet) {
            iolet->setHidden(gui->hideOutlets());
        } else if (gui && iolet->isInlet) {
            iolet->setHidden(gui->hideInlets());
        }
    }

    while (numInputs < oldNumInputs)
        iolets.remove(--oldNumInputs);
    while (numInputs > oldNumInputs)
        iolets.insert(oldNumInputs++, new Iolet(this, true));
    while (numOutputs < oldNumOutputs)
        iolets.remove(numInputs + (--oldNumOutputs));
    while (numOutputs > oldNumOutputs)
        iolets.insert(numInputs + (++oldNumOutputs), new Iolet(this, false));

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++) {
        auto* iolet = iolets[i];
        bool input = iolet->isInlet;

        bool isSignal;
        if (i < numInputs) {
            isSignal = libpd_issignalinlet(pd::Patch::checkObject(getPointer()), i);
        } else {
            isSignal = libpd_issignaloutlet(pd::Patch::checkObject(getPointer()), i - numInputs);
        }

        iolet->ioletIdx = input ? numIn : numOut;
        iolet->isSignal = isSignal;
        iolet->repaint();

        numIn += input;
        numOut += !input;
    }

    updateTooltips();
    resized();
}

void Object::mouseDown(MouseEvent const& e)
{
    // TODO: why would this ever happen??
    if (!getLocalBounds().contains(e.getPosition()))
        return;

    if (attachedToMouse) {
        attachedToMouse = false;
        stopTimer();
        repaint();

        // Tell pd about new position
        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), b = getObjectBounds()]() {
                if (!_this || !_this->gui || _this->cnv->patch.objectWasDeleted(_this->gui->ptr)) {
                    return;
                }
                _this->gui->setPdBounds(b);
            });

        if (createEditorOnMouseDown) {
            createEditorOnMouseDown = false;

            // Prevent losing focus because of click event
            MessageManager::callAsync([_this = SafePointer(this)]() { _this->showEditor(); });
        }

        return;
    }

    if (cnv->isGraph || cnv->presentationMode == var(true) || locked == var(true) || commandLocked == var(true)) {
        wasLockedOnMouseDown = true;
        return;
    }

    wasLockedOnMouseDown = false;

    bool wasSelected = cnv->isSelected(this);

    if (e.mods.isRightButtonDown()) {
        cnv->setSelected(this, true);

        PopupMenu::dismissAllActiveMenus();
        Dialogs::showCanvasRightClickMenu(cnv, this, e.getScreenPosition());

        return;
    }
    if (e.mods.isShiftDown()) {
        // select multiple objects
        ds.wasSelectedOnMouseDown = cnv->isSelected(this);
    } else if (!cnv->isSelected(this)) {
        cnv->deselectAll();
    }
    cnv->setSelected(this, true); // TODO: can we move this up, so we don't need it twice?

    ds.componentBeingDragged = this;

    for (auto* object : cnv->getSelectionOfType<Object>()) {
        object->originalBounds = object->getBounds();
        object->setBufferedToImage(true);
    }

    repaint();

    ds.canvasDragStartPosition = cnv->getPosition();

    cnv->updateSidebarSelection();

    if (cnv->isSelected(this) != wasSelected) {
        selectionStateChanged = true;
    }
}

void Object::mouseUp(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    if (!ds.didStartDragging && !static_cast<bool>(locked.getValue()) && e.mods.isAltDown()) {
        // Show help file on alt+mouseup if object were not draged
        openHelpPatch();
    }

    if (ds.wasResized) {
        std::map<SafePointer<Object>, Rectangle<int>> newObjectSizes;
        for (auto* obj : cnv->getSelectionOfType<Object>())
            newObjectSizes[obj] = obj->getObjectBounds();

        auto* patch = &cnv->patch;

        cnv->objectGrid.handleMouseUp(e.getOffsetFromDragStart());

        cnv->pd->enqueueFunction(
            [newObjectSizes, patch]() mutable {
                patch->startUndoSequence("resize");

                for (auto& [object, bounds] : newObjectSizes) {
                    if (!object || !object->gui)
                        return;

                    auto* obj = static_cast<t_gobj*>(object->getPointer());
                    auto* cnv = object->cnv;

                    if (cnv->patch.objectWasDeleted(obj))
                        return;

                    // Used for size changes, could also be used for properties
                    libpd_undo_apply(cnv->patch.getPointer(), obj);

                    object->gui->setPdBounds(bounds);

                    canvas_dirty(cnv->patch.getPointer(), 1);

                    // To make sure it happens after setting object bounds
                    // TODO: do we need this??
                    if (!cnv->viewport->getViewArea().contains(object->getBounds())) {
                        MessageManager::callAsync([o = object]() {
                            if (o) o->cnv->checkBounds();
                        });
                    }
                }

                patch->endUndoSequence("resize");
            });

        ds.wasResized = false;
        originalBounds.setBounds(0, 0, 0, 0);
    } else {
        if (cnv->isGraph)
            return;

        if (e.mods.isShiftDown() && ds.wasSelectedOnMouseDown && !ds.didStartDragging) {
            // Unselect object if selected
            cnv->setSelected(this, false);
        } else if (!e.mods.isShiftDown() && !e.mods.isAltDown() && cnv->isSelected(this) && !ds.didStartDragging && !e.mods.isRightButtonDown()) {

            // Don't run normal deselectAll, that would clear the sidebar inspector as well
            // We'll update sidebar selection later in this function
            cnv->selectedComponents.deselectAll();

            cnv->setSelected(this, true);
        }

        cnv->updateSidebarSelection();

        if (ds.didStartDragging) {
            auto objects = std::vector<void*>();

            for (auto* object : cnv->getSelectionOfType<Object>()) {
                if (object->getPointer())
                    objects.push_back(object->getPointer());
            }

            auto distance = Point<int>(e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY());

            // In case we dragged near the iolet and the canvas moved
            auto canvasMoveOffset = ds.canvasDragStartPosition - cnv->getPosition();

            distance = cnv->objectGrid.handleMouseUp(distance) + canvasMoveOffset;

            // When done dragging objects, update positions to pd
            cnv->patch.moveObjects(objects, distance.x, distance.y);

            cnv->pd->waitForStateUpdate();

            cnv->checkBounds();
            ds.didStartDragging = false;
        }

        if (ds.objectSnappingInbetween) {
            auto* c = ds.connectionToSnapInbetween.getComponent();

            cnv->patch.startUndoSequence("SnapInbetween");

            cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

            cnv->patch.createConnection(c->outobj->getPointer(), c->outIdx, ds.objectSnappingInbetween->getPointer(), 0);
            cnv->patch.createConnection(ds.objectSnappingInbetween->getPointer(), 0, c->inobj->getPointer(), c->inIdx);

            cnv->patch.endUndoSequence("SnapInbetween");

            ds.objectSnappingInbetween->iolets[0]->isTargeted = false;
            ds.objectSnappingInbetween->iolets[ds.objectSnappingInbetween->numInputs]->isTargeted = false;
            ds.objectSnappingInbetween = nullptr;

            cnv->synchronise();
        }

        if (ds.wasDragDuplicated) {
            cnv->patch.endUndoSequence("Duplicate");
            ds.wasDragDuplicated = false;
        }

        for (auto* object : cnv->getSelectionOfType<Object>()) {
            object->setBufferedToImage(false);
            object->repaint();
        }

        ds.componentBeingDragged = nullptr;

        repaint();
    }

    if (ds.wasDragDuplicated) {
        cnv->patch.endUndoSequence("Duplicate");
        ds.wasDragDuplicated = false;
    }

    if (gui && !selectionStateChanged && cnv->isSelected(this) && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isRightButtonDown()) {
        gui->showEditor();
    }

    selectionStateChanged = false;
}

void Object::mouseDrag(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    cnv->cancelConnectionCreation();

    if (e.mods.isMiddleButtonDown())
        return;

    if (validResizeZone && !originalBounds.isEmpty()) {

        auto draggedBounds = resizeZone.resizeRectangleBy(originalBounds, e.getOffsetFromDragStart());
        auto dragDistance = cnv->objectGrid.performResize(this, e.getOffsetFromDragStart(), draggedBounds);

        auto toResize = cnv->getSelectionOfType<Object>();

        for (auto* obj : toResize) {
            auto const newBounds = resizeZone.resizeRectangleBy(obj->originalBounds, dragDistance);

            bool useConstrainer = obj->gui && !obj->gui->checkBounds(obj->originalBounds - cnv->canvasOrigin, newBounds - cnv->canvasOrigin, resizeZone.isDraggingLeftEdge());

            if (useConstrainer) {
                obj->constrainer->setBoundsForComponent(obj, newBounds, resizeZone.isDraggingTopEdge(),
                    resizeZone.isDraggingLeftEdge(),
                    resizeZone.isDraggingBottomEdge(),
                    resizeZone.isDraggingRightEdge());
            }
        }

        ds.wasResized = true;
    } else if(!cnv->isGraph) {
        /** Ensure tiny movements don't start a drag. */
        if (!ds.didStartDragging && e.getDistanceFromDragStart() < cnv->minimumMovementToStartDrag)
            return;

        if (!ds.didStartDragging) {
            ds.didStartDragging = true;
        }

        auto selection = cnv->getSelectionOfType<Object>();

        auto dragDistance = e.getOffsetFromDragStart();

        // In case we dragged near the edge and the canvas moved
        auto canvasMoveOffset = ds.canvasDragStartPosition - cnv->getPosition();

        if (static_cast<bool>(cnv->gridEnabled.getValue()) && ds.componentBeingDragged) {
            dragDistance = cnv->objectGrid.performMove(this, dragDistance);
        }

        // alt+drag will duplicate selection
        if (!ds.wasDragDuplicated && e.mods.isAltDown()) {

            Array<Point<int>> mouseDownObjectPositions; // Stores object positions for alt + drag

            // Single for undo for duplicate + move
            cnv->patch.startUndoSequence("Duplicate");

            // Sort selection indexes to match pd indexes
            std::sort(selection.begin(), selection.end(),
                [this](auto* a, auto* b) -> bool {
                    return cnv->objects.indexOf(a) < cnv->objects.indexOf(b);
                });

            int draggedIdx = selection.indexOf(ds.componentBeingDragged.getComponent());

            // Store origin object positions
            for (auto object : selection) {
                mouseDownObjectPositions.add(object->getPosition());
            }

            // Duplicate once
            ds.wasDragDuplicated = true;
            cnv->duplicateSelection();
            cnv->cancelConnectionCreation();

            selection = cnv->getSelectionOfType<Object>();

            // Sort selection indexes to match pd indexes
            std::sort(selection.begin(), selection.end(),
                [this](auto* a, auto* b) -> bool {
                    return cnv->objects.indexOf(a) < cnv->objects.indexOf(b);
                });

            int i = 0;
            for(auto* selected : selection)
            {
                selected->originalBounds = selected->getBounds().withPosition(mouseDownObjectPositions[i]);
                i++;
            }

            if(isPositiveAndBelow(draggedIdx, selection.size())) {
                ds.componentBeingDragged = selection[draggedIdx];
            }
        }

        // FIXME: stop the mousedrag event from blocking the objects from redrawing, we shouldn't need to do this? JUCE bug?
        if (!cnv->objectRateReducer.tooFast()) {
            for (auto* object : selection) {
                object->setTopLeftPosition(object->originalBounds.getPosition() + dragDistance + canvasMoveOffset);
            }

            auto draggedBounds = ds.componentBeingDragged->getBounds().expanded(6);
            auto xEdge = e.getDistanceFromDragStartX() < 0 ? draggedBounds.getX() : draggedBounds.getRight();
            auto yEdge = e.getDistanceFromDragStartY() < 0 ? draggedBounds.getY() : draggedBounds.getBottom();
            if (cnv->autoscroll(e.getEventRelativeTo(this).withNewPosition(Point<int>(xEdge, yEdge)).getEventRelativeTo(cnv->viewport))) {
                beginDragAutoRepeat(25);
            }
        }

        // This handles the "unsnap" action when you shift-drag a connected object
        if (e.mods.isShiftDown() && selection.size() == 1 && e.getDistanceFromDragStart() > 15) {
            auto* object = selection.getFirst();

            Array<Connection*> inputs, outputs;
            for (auto* connection : cnv->connections) {
                if (connection->inlet == object->iolets[0]) {
                    inputs.add(connection);
                }
                if (connection->outlet == object->iolets[object->numInputs]) {
                    outputs.add(connection);
                }
            }

            // Two cases that are allowed: either 1 input and multiple outputs,
            // or 1 output and multiple inputs
            if (inputs.size() == 1 && outputs.size()) {
                auto* outlet = inputs[0]->outlet.getComponent();

                for (auto* c : outputs) {
                    cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                    cnv->connections.add(new Connection(cnv, outlet, c->inlet, nullptr));
                    cnv->connections.removeObject(c);
                }

                auto* c = inputs[0];
                cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            } else if (inputs.size() && outputs.size() == 1) {
                auto* inlet = outputs[0]->inlet.getComponent();

                for (auto* c : inputs) {
                    cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                    cnv->connections.add(new Connection(cnv, c->outlet, inlet, nullptr));
                    cnv->connections.removeObject(c);
                }

                auto* c = outputs[0];
                cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            }
        }

        // Behaviour for shift-dragging objects over
        if (ds.objectSnappingInbetween) {
            if (ds.connectionToSnapInbetween->intersectsObject(ds.objectSnappingInbetween)) {
                return;
            }

            // If we're here, it's not snapping anymore
            ds.objectSnappingInbetween->iolets[0]->isTargeted = false;
            ds.objectSnappingInbetween->iolets[ds.objectSnappingInbetween->numInputs]->isTargeted = false;
            ds.objectSnappingInbetween = nullptr;
        }

        if (e.mods.isShiftDown() && selection.size() == 1) {
            auto* object = selection.getFirst();
            if (object->numInputs && object->numOutputs) {
                bool intersected = false;
                for (auto* connection : cnv->connections) {
                    if (connection->intersectsObject(object)) {
                        object->iolets[0]->isTargeted = true;
                        object->iolets[object->numInputs]->isTargeted = true;
                        object->iolets[0]->repaint();
                        object->iolets[object->numInputs]->repaint();
                        ds.connectionToSnapInbetween = connection;
                        ds.objectSnappingInbetween = object;
                        intersected = true;
                        break;
                    }
                }

                if (!intersected) {
                    object->iolets[0]->isTargeted = false;
                    object->iolets[object->numInputs]->isTargeted = false;
                    object->iolets[0]->repaint();
                    object->iolets[object->numInputs]->repaint();
                }
            }
        }
    }
}

void Object::showEditor()
{
    if (!gui) {
        openNewObjectEditor();
    } else {
        gui->showEditor();
    }
}

void Object::hideEditor()
{
    if (gui) {
        gui->hideEditor();
    } else if (newObjectEditor) {
        WeakReference<Component> deletionChecker(this);
        std::unique_ptr<TextEditor> outgoingEditor;
        std::swap(outgoingEditor, newObjectEditor);

        cnv->hideSuggestions();

        outgoingEditor->removeListener(cnv->suggestor);

        // Get entered text, remove extra spaces at the end
        auto newText = outgoingEditor->getText().trimEnd();
        outgoingEditor.reset();

        repaint();
        setType(newText);
    }
}

Array<Connection*> Object::getConnections() const
{
    Array<Connection*> result;
    for (auto* iolet : iolets) {
        result.addArray(iolet->getConnections());
    }

    return result;
}

void* Object::getPointer() const
{
    return gui ? gui->ptr : nullptr;
}

void Object::openNewObjectEditor()
{
    if (!newObjectEditor) {
        newObjectEditor = std::make_unique<TextEditor>();

        auto* editor = newObjectEditor.get();
        editor->applyFontToAllText(Font(15));

        copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, findColour(PlugDataColour::canvasTextColourId));
        editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        editor->setAlwaysOnTop(true);
        editor->setMultiLine(false);
        editor->setReturnKeyStartsNewLine(false);
        editor->setBorder(BorderSize<int>(1, 7, 1, 2));
        editor->setIndents(0, 0);
        editor->setJustification(Justification::centredLeft);

        editor->setBounds(getLocalBounds().reduced(Object::margin));
        editor->addListener(this);

        // Allow cancelling object creation with escape
        editor->onEscapeKey = [this]() {
            MessageManager::callAsync([_this = SafePointer(this)]() {
                if (!_this)
                    return;
                auto* cnv = _this->cnv; // Copy pointer because _this will get deleted
                cnv->hideSuggestions();
                cnv->objects.removeObject(_this.getComponent());
                cnv->lastSelectedObject = nullptr;
                cnv->lastSelectedConnection = nullptr;
            });
        };

        addAndMakeVisible(editor);
        editor->grabKeyboardFocus();

        editor->onFocusLost = [this, editor]() {
            if (reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor) {
                editor->grabKeyboardFocus();
                return;
            }

            hideEditor();
        };

        cnv->showSuggestions(this, editor);

        resized();
        repaint();
    }
}

void Object::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (newObjectEditor) {
        newObjectEditor->giveAwayKeyboardFocus();
        cnv->grabKeyboardFocus();
    }
}

void Object::altKeyChanged(bool isHeld)
{
    if (isHeld != indexShown) {
        indexShown = isHeld;
        repaint();
    }
}

// For resize-while-typing behaviour
void Object::textEditorTextChanged(TextEditor& ed)
{
    String currentText;
    if (cnv->suggestor && !cnv->suggestor->getText().isEmpty()) {
        currentText = cnv->suggestor->getText();
    } else {
        currentText = ed.getText();
    }

    // For resize-while-typing behaviour
    auto width = Font(15).getStringWidth(currentText) + 14.0f;

    width += Object::doubleMargin;

    setSize(width, getHeight());
}

void Object::openHelpPatch() const
{
    cnv->pd->setThis();

    if (auto* ptr = static_cast<t_object*>(getPointer())) {

        auto file = cnv->pd->objectLibrary.findHelpfile(ptr, cnv->patch.getCurrentFile());

        if (!file.existsAsFile()) {
            cnv->pd->logMessage("Couldn't find help file");
            return;
        }

        cnv->pd->enqueueFunction([this, file]() mutable {
            cnv->pd->loadPatch(file);
        });

        return;
    }

    cnv->pd->logMessage("Couldn't find help file");
}
