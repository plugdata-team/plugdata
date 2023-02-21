/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Canvas.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
}

#include "Object.h"
#include "Connection.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "SuggestionComponent.h"
#include "Tabbar.h"

#include "Utility/GraphArea.h"
#include "Utility/RateReducer.h"

Canvas::Canvas(PluginEditor* parent, pd::Patch& p, Component* parentGraph)
    : editor(parent)
    , pd(parent->pd)
    , patch(p)
    , pathUpdater(new ConnectionPathUpdater(this))
{
    isGraphChild = glist_isgraph(p.getPointer());
    hideNameAndArgs = static_cast<bool>(p.getPointer()->gl_hidetext);
    xRange = Array<var> { var(p.getPointer()->gl_x1), var(p.getPointer()->gl_x2) };
    yRange = Array<var> { var(p.getPointer()->gl_y2), var(p.getPointer()->gl_y1) };

    isGraphChild.addListener(this);
    hideNameAndArgs.addListener(this);
    xRange.addListener(this);
    yRange.addListener(this);

    // Check if canvas belongs to a graph
    if (parentGraph) {
        setLookAndFeel(&editor->getLookAndFeel());
        parentGraph->addAndMakeVisible(this);
        setInterceptsMouseClicks(false, true);
        isGraph = true;
    } else {
        isGraph = false;
    }

    suggestor = new SuggestionComponent;

    commandLocked.referTo(pd->commandLocked);
    commandLocked.addListener(this);

    // TODO: use SettingsFileListener
    gridEnabled.referTo(SettingsFile::getInstance()->getPropertyAsValue("grid_enabled"));

    tabbar = &editor->tabbar;

    // Add draggable border for setting graph position
    if (static_cast<bool>(isGraphChild.getValue()) && !isGraph) {
        graphArea = new GraphArea(this);
        addAndMakeVisible(graphArea);
        graphArea->setAlwaysOnTop(true);
    }

    setSize(600, 400);

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);

    setWantsKeyboardFocus(true);

    if (!isGraph) {
        viewport = new InsetViewport; // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);

        viewport->setScrollBarsShown(true, true, true, true);

        presentationMode.referTo(editor->statusbar.presentationMode);
        presentationMode.addListener(this);
    } else {
        presentationMode = false;
    }

    synchronise();

    // Start in unlocked mode if the patch is empty
    if (objects.isEmpty()) {
        locked = false;
        patch.getPointer()->gl_edit = false;
    } else {
        locked = !patch.getPointer()->gl_edit;
    }

    locked.addListener(this);
}

Canvas::~Canvas()
{
    delete graphArea;
    delete suggestor;
}

void Canvas::timerCallback()
{
    rateLimit = true;
}

void Canvas::lookAndFeelChanged()
{
    lasso.setColour(LassoComponent<Object>::lassoFillColourId, findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f));
}

void Canvas::paint(Graphics& g)
{
    if (!isGraph) {

        g.fillAll(findColour(PlugDataColour::toolbarBackgroundColourId));

        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRect(canvasOrigin.x, canvasOrigin.y, getWidth(), getHeight());

        // draw origin
        g.setColour(Colour(100, 100, 100));
        g.drawLine(canvasOrigin.x - 1, canvasOrigin.y - 1, canvasOrigin.x - 1, getHeight() + 2);
        g.drawLine(canvasOrigin.x - 1, canvasOrigin.y - 1, getWidth() + 2, canvasOrigin.y - 1);
    }

    if (locked == var(false) && !isGraph) {
        Rectangle<int> const clipBounds = g.getClipBounds();

        g.setColour(findColour(PlugDataColour::canvasDotsColourId));

        for (int x = canvasOrigin.getX() + objectGrid.gridSize; x < clipBounds.getRight(); x += objectGrid.gridSize) {
            for (int y = canvasOrigin.getY() + objectGrid.gridSize; y < clipBounds.getBottom(); y += objectGrid.gridSize) {
                g.fillRect(static_cast<float>(x), static_cast<float>(y), 1.0, 1.0);
            }
        }
    }
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::synchronise(bool updatePosition)
{
    pd->waitForStateUpdate();

    patch.setCurrent(true);

    auto pdObjects = patch.getObjects();
    auto isObjectDeprecated = [&](void* obj) {
        return std::all_of(pdObjects.begin(), pdObjects.end(), [obj](const auto* obj2) {
            return obj != obj2;
        });
    };

    // Remove deprecated connections
    for (int n = connections.size() - 1; n >= 0; n--) {
        auto* connection = connections[n];

        if (!connection->inlet || !connection->outlet || isObjectDeprecated(connection->inobj->getPointer()) || isObjectDeprecated(connection->outobj->getPointer())) {
            connections.remove(n);
        } else {
            auto* inlet = static_cast<t_text*>(connection->inobj->getPointer());
            auto* outlet = static_cast<t_text*>(connection->outobj->getPointer());

            if (!canvas_isconnected(patch.getPointer(), outlet, connection->outIdx, inlet, connection->inIdx)) {
                connections.remove(n);
            }
        }
    }

    // Clear deleted objects
    for (int n = objects.size() - 1; n >= 0; n--) {
        auto* object = objects[n];
        if (object->gui && isObjectDeprecated(object->getPointer())) {
            setSelected(object, false);
            objects.remove(n);
        }
    }

    for (auto* object : pdObjects) {
        auto* it = std::find_if(objects.begin(), objects.end(), [&object](Object* b) { return b->getPointer() && b->getPointer() == object; });

        if (it == objects.end()) {
            auto* newBox = objects.add(new Object(object, this));
            newBox->toFront(false);

            // TODO: don't do this on Canvas!!
            if (newBox->gui && newBox->gui->getLabel())
                newBox->gui->getLabel()->toFront(false);
        } else {
            auto* object = *it;

            // Check if number of inlets/outlets is correct
            object->updateIolets();

            // Only update positions if we need to and there is a significant difference
            // There may be rounding errors when scaling the gui, this makes the experience smoother
            if (updatePosition)
                object->updateBounds();

            object->toFront(false);
            if (object->gui && object->gui->getLabel())
                object->gui->getLabel()->toFront(false);
        }
    }

    // Make sure objects have the same order
    std::sort(objects.begin(), objects.end(),
        [&pdObjects](Object* first, Object* second) mutable {
            size_t idx1 = std::find(pdObjects.begin(), pdObjects.end(), first->getPointer()) - pdObjects.begin();
            size_t idx2 = std::find(pdObjects.begin(), pdObjects.end(), second->getPointer()) - pdObjects.begin();

            return idx1 < idx2;
        });

    auto pdConnections = patch.getConnections();

    for (auto& connection : pdConnections) {
        auto& [ptr, inno, inobj, outno, outobj] = connection;

        int srcno = patch.getIndex(&inobj->te_g);
        int sinkno = patch.getIndex(&outobj->te_g);

        auto& srcEdges = objects[srcno]->iolets;
        auto& sinkEdges = objects[sinkno]->iolets;

        // TODO: sometimes we hit this if-statement when we hide iolets because of send/receive symbols
        // not sure if that's a problem or not
        if (srcno >= objects.size() || sinkno >= objects.size() || outno >= srcEdges.size() || inno >= sinkEdges.size()) {
            continue;
        }

        // TODO: compare by pointer, not by start/end!!
        auto* it = std::find_if(connections.begin(), connections.end(),
            [this, &connection, &srcno, &sinkno](Connection* c) {
                auto& [ptr, inno, inobj, outno, outobj] = connection;
                return ptr == c->getPointer();
            });

        if (it == connections.end()) {
            connections.add(new Connection(this, srcEdges[objects[srcno]->numInputs + outno], sinkEdges[inno], ptr));
        } else {
            auto& c = *(*it);
            c.popPathState();
        }
    }

    if (!isGraph) {
        setTransform(editor->transform);
    }

    // Resize canvas to fit objects
    // By checking asynchronously, we make sure the objects bounds have been updated
    MessageManager::callAsync([_this = SafePointer(this)]() {
        if (!_this)
            return;
        _this->pd->waitForStateUpdate();
        _this->checkBounds();
    });

    editor->updateCommandStatus();
    repaint();
}

void Canvas::updateDrawables()
{
    for (auto* object : objects) {
        if (object->gui) {
            object->gui->updateDrawables();
        }
    }
}

void Canvas::mouseDown(MouseEvent const& e)
{
    auto* source = e.originalComponent;

    PopupMenu::dismissAllActiveMenus();

    // Middle mouse click
    if (viewport && e.mods.isMiddleButtonDown()) {
        setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
        viewportPositionBeforeMiddleDrag = viewport->getViewPosition();
    }
    // Left-click
    else if (!e.mods.isRightButtonDown()) {
        if (source == this /*|| source == graphArea */) {

            cancelConnectionCreation();

            if (e.mods.isCommandDown()) {
                // Lock if cmd + click on canvas
                deselectAll();
                if (locked.getValue()) {
                    locked.setValue(false);
                } else {
                    locked.setValue(true);
                }
            }
            if (!e.mods.isShiftDown()) {
                deselectAll();
            }

            lasso.beginLasso(e.getEventRelativeTo(this), this);
            isDraggingLasso = true;
        }

        // Update selected object in sidebar when we click a object
        if (source && source->findParentComponentOfClass<Object>()) {
            updateSidebarSelection();
        }

        editor->updateCommandStatus();
    }
    // Right click
    else {
        Dialogs::showCanvasRightClickMenu(this, source, e.getScreenPosition());
    }
}

void Canvas::mouseDrag(MouseEvent const& e)
{
    if (canvasRateReducer.tooFast())
        return;

    if (connectingWithDrag) {
        for (auto* obj : objects) {
            for (auto* iolet : obj->iolets) {
                iolet->mouseDrag(e.getEventRelativeTo(iolet));
            }
        }
    }

    bool objectIsBeingEdited = ObjectBase::isBeingEdited();

    // Ignore on graphs or when locked
    if ((isGraph || locked == var(true) || commandLocked == var(true)) && !objectIsBeingEdited) {
        bool hasToggled = false;

        // Behaviour for dragging over toggles, bang and radiogroup to toggle them
        for (auto* object : objects) {
            if (!object->getBounds().contains(e.getEventRelativeTo(this).getPosition()) || !object->gui)
                continue;

            if (auto* obj = object->gui.get()) {
                obj->toggleObject(e.getEventRelativeTo(obj).getPosition());
                hasToggled = true;
                break;
            }
        }

        if (!hasToggled) {
            for (auto* object : objects) {
                if (auto* obj = object->gui.get()) {
                    obj->untoggleObject();
                }
            }
        }

        return;
    }

    if (viewport) {
        auto viewportEvent = e.getEventRelativeTo(viewport);
        auto const scrollSpeed = 8.5f;

        // Middle mouse pan
        if (e.mods.isMiddleButtonDown() && !ObjectBase::isBeingEdited()) {

            auto delta = Point<int>(viewportEvent.getDistanceFromDragStartX(), viewportEvent.getDistanceFromDragStartY());

            viewport->setViewPosition(viewportPositionBeforeMiddleDrag.x - delta.x, viewportPositionBeforeMiddleDrag.y - delta.y);

            return; // Middle mouse button cancels any other drag actions
        }
        
        if (!ObjectBase::isBeingEdited() && autoscroll(viewportEvent)) {
            beginDragAutoRepeat(25);
        }
    }

    // Drag lasso
    lasso.dragLasso(e);
}

bool Canvas::autoscroll(MouseEvent const& e)
{
    auto ret = false;
    auto x = viewport->getViewPositionX();
    auto y = viewport->getViewPositionY();
    
    if(e.x > viewport->getWidth()) {
        ret = true;
        x += std::clamp((e.x - viewport->getWidth()) / 6, 1, 14);
    }
    else if(e.x < 0) {
        ret = true;
        x -= std::clamp(-e.x / 6, 1, 14);
    }
    if(e.y > viewport->getHeight()) {
        ret = false;
        y += std::clamp((e.y - viewport->getHeight()) / 6, 1, 14);
    }
    else if(e.y < 0) {
        ret = false;
        y -= std::clamp(-e.y / 6, 1, 14);
    }
    
    viewport->setViewPosition(x, y);
    
    return ret;
}

void Canvas::mouseUp(MouseEvent const& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
    editor->updateCommandStatus();

    // Double-click canvas to create new object
    if (e.mods.isLeftButtonDown() && (e.getNumberOfClicks() == 2) && (e.originalComponent == this) && !isGraph && !static_cast<bool>(locked.getValue())) {
        objects.add(new Object(this, "", lastMousePosition));
        deselectAll();
        setSelected(objects[objects.size() - 1], true); // Select newly created object
    }

    updateSidebarSelection();

    editor->updateCommandStatus();

    lasso.endLasso();
    isDraggingLasso = false;
    for (auto* object : objects)
        object->originalBounds = Rectangle<int>(0, 0, 0, 0);

    if (wasDragDuplicated) {
        patch.endUndoSequence("Duplicate");
        wasDragDuplicated = false;
    }

    mouseDownObjectPositions.clear();

    // TODO: this is a hack, find a better solution
    if (connectingWithDrag) {
        for (auto* obj : objects) {
            for (auto* iolet : obj->iolets) {
                auto relativeEvent = e.getEventRelativeTo(this);
                if (iolet->getCanvasBounds().expanded(50).contains(relativeEvent.getPosition())) {
                    iolet->mouseUp(relativeEvent);
                }
            }
        }
    }
}

void Canvas::updateSidebarSelection()
{
    auto lassoSelection = getSelectionOfType<Object>();

    if (lassoSelection.size() == 1) {
        auto* object = lassoSelection.getFirst();
        auto params = object->gui ? object->gui->getParameters() : ObjectParameters();

        if (!object->gui) {
            editor->sidebar.hideParameters();
            return;
        }

        if (commandLocked == var(true)) {
            editor->sidebar.hideParameters();
        } else if (!params.empty() || editor->sidebar.isPinned()) {
            editor->sidebar.showParameters(object->gui->getText(), params);
        } else {
            editor->sidebar.hideParameters();
        }
    } else {
        editor->sidebar.hideParameters();
    }
}

// TODO: can we get rid of this?
void Canvas::mouseMove(MouseEvent const& e)
{
    lastMousePosition = getMouseXYRelative();
}

bool Canvas::keyPressed(KeyPress const& key)
{
    if (editor->getCurrentCanvas() != this || isGraph)
        return false;

    int keycode = key.getKeyCode();

    auto moveSelection = [this](int x, int y) {
        auto objects = getSelectionOfType<Object>();
        std::vector<void*> pdObjects;

        for (auto* object : objects) {
            pdObjects.push_back(object->getPointer());
        }

        patch.moveObjects(pdObjects, x, y);

        for (auto* object : objects) {
            object->updateBounds();

            if (!getBounds().contains(object->getBounds())) {
                checkBounds();
            }
        }
    };

    // Move objects with arrow keys
    int moveDistance = 10;
    if (key.getModifiers().isShiftDown()) {
        moveDistance = 1;
    } else if (key.getModifiers().isCommandDown()) {
        moveDistance = 50;
    }

    if (keycode == KeyPress::leftKey) {
        moveSelection(-moveDistance, 0);
        return true;
    }
    if (keycode == KeyPress::rightKey) {
        moveSelection(moveDistance, 0);
        return true;
    }
    if (keycode == KeyPress::upKey) {
        moveSelection(0, -moveDistance);
        return true;
    }
    if (keycode == KeyPress::downKey) {
        moveSelection(0, moveDistance);
        return true;
    }

    // Ignore backspace, arrow keys, return key and more that might cause actions in pd
    if (keycode == KeyPress::backspaceKey || keycode == KeyPress::pageUpKey || keycode == KeyPress::pageDownKey || keycode == KeyPress::homeKey || keycode == KeyPress::escapeKey || keycode == KeyPress::deleteKey || keycode == KeyPress::returnKey || keycode == KeyPress::tabKey) {
        return false;
    }
    return false;
}

void Canvas::deselectAll()
{
    auto selection = selectedComponents;

    selectedComponents.deselectAll();

    // Deselect objects
    for (auto c : selection)
        if (!c.wasObjectDeleted())
            c->repaint();

    editor->sidebar.hideParameters();
}

void Canvas::hideAllActiveEditors()
{
    for (auto* object : objects) {
        object->hideEditor();
    }
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* object : getSelectionOfType<Object>()) {
        patch.selectObject(object->getPointer());
    }

    // Tell pd to copy
    patch.copy();
    patch.deselectAll();
}

void Canvas::pasteSelection()
{
    patch.startUndoSequence("Paste");
    // Tell pd to paste
    patch.paste();
    
    deselectAll();
    
    // Load state from pd, don't update positions
    synchronise(false);

    patch.setCurrent();

    std::vector<void*> pastedObjects;

    sys_lock();
    for (auto* object : objects) {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(object->getPointer()))) {
            setSelected(object, true);
            pastedObjects.emplace_back(object->getPointer());
        }
    }
    sys_unlock();

    // Paste at mousePos, adds padding if pasted the same place
    if (lastMousePosition == pastedPosition) {
        pastedPadding.addXY(10, 10);
    } else {
        pastedPadding.setXY(-10, -10);
    }
    pastedPosition = lastMousePosition;

    if (getSelectionOfType<Object>().size() > 0) {
        auto copyPosition = getSelectionOfType<Object>().getFirst()->getPosition();
        patch.moveObjects(pastedObjects,
            pastedPosition.x - copyPosition.x + pastedPadding.x,
            pastedPosition.y - copyPosition.y + pastedPadding.y);
    }

    patch.deselectAll();
    pastedObjects.clear();
    patch.endUndoSequence("Paste");
}

void Canvas::duplicateSelection()
{
    Array<Connection*> conInlets, conOutlets;
    auto selection = getSelectionOfType<Object>();
    patch.startUndoSequence("Duplicate");

    for (auto* object : selection) {
        // Check if object exists in pd and is not attached to mouse
        if (!object->gui || object->attachedToMouse)
            return;
        // Tell pd to select all objects that are currently selected
        patch.selectObject(object->getPointer());

        if (!wasDragDuplicated && editor->autoconnect.getValue()) {
            // Store connections for auto patching
            for (auto* connection : connections) {
                if (connection->inlet == object->iolets[0]) {
                    conInlets.add(connection);
                }
                if (connection->outlet == object->iolets[object->numInputs]) {
                    conOutlets.add(connection);
                }
            }
        }
    }

    // Tell pd to duplicate
    patch.duplicate();
    
    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    // Store the duplicated objects for later selection
    Array<Object*> duplicated;
    for (auto* object : objects) {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(object->getPointer()))) {
            duplicated.add(object);
        }
    }

    // Auto patching
    if (!wasDragDuplicated && editor->autoconnect.getValue()) {
        std::vector<void*> moveObjects;
        for (auto* object : objects) {
            int iolet = 1;
            for (auto* objIolet : object->iolets) {
                if (duplicated.size() == 1) {
                    for (auto* dup : duplicated) {
                        for (auto* conIn : conInlets) {
                            if ((conIn->outlet == objIolet) && object->iolets[iolet] && !dup->iolets.contains(conIn->outlet)) {
                                connections.add(new Connection(this, dup->iolets[0], object->iolets[iolet], nullptr));
                            }
                        }
                        for (auto* conOut : conOutlets) {
                            if ((conOut->inlet == objIolet) && (iolet < object->numInputs)) {
                                connections.add(new Connection(this, dup->iolets[dup->numInputs], object->iolets[iolet], nullptr));
                            }
                        }
                    }
                    iolet = iolet + 1;
                }
            }
        }

        // Move duplicated objects if they overlap exisisting objects
        for (auto* dup : duplicated) {
            moveObjects.emplace_back(dup->getPointer());
        }
        bool overlap = true;
        int moveDistance = 0;
        while (overlap) {
            overlap = false;
            for (auto* object : objects) {
                if (!duplicated.contains(object) && duplicated[0]->getBounds().translated(moveDistance, 0).intersects(object->getBounds())) {
                    overlap = true;
                    moveDistance += object->getWidth() - 10;
                    duplicated[0]->updateBounds();
                }
            }
        }

        patch.moveObjects(moveObjects, moveDistance - 10, -10);
        moveObjects.clear();
    }

    // Select the newly duplicated objects
    for (auto* obj : duplicated) {
        setSelected(obj, true);
    }

    patch.endUndoSequence("Duplicate");
    patch.deselectAll();
}

void Canvas::removeSelection()
{
    // Make sure object isn't selected and stop updating gui
    editor->sidebar.hideParameters();

    // Make sure nothing is selected
    patch.deselectAll();

    // Find selected objects and make them selected in pd
    Array<void*> objects;
    for (auto* object : getSelectionOfType<Object>()) {
        if (object->getPointer()) {
            patch.selectObject(object->getPointer());
            objects.add(object->getPointer());
        }
    }

    // remove selection
    patch.removeSelection();

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto* con : connections) {
        if (isSelected(con)) {
            if (!(objects.contains(con->outobj->getPointer()) || objects.contains(con->inobj->getPointer()))) {
                patch.removeConnection(con->outobj->getPointer(), con->outIdx, con->inobj->getPointer(), con->inIdx, con->getPathState());
            }
        }
    }

    patch.finishRemove(); // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    patch.deselectAll();
}

void Canvas::encapsulateSelection()
{
    auto selectedBoxes = getSelectionOfType<Object>();

    // Sort by index in pd patch
    std::sort(selectedBoxes.begin(), selectedBoxes.end(),
        [this](auto* a, auto* b) -> bool {
            return objects.indexOf(a) < objects.indexOf(b);
        });

    // If two connections have the same target inlet/outlet, we only need 1 [inlet/outlet] object
    auto usedEdges = Array<Iolet*>();
    auto targetEdges = std::map<Iolet*, Array<Iolet*>>();

    auto newInternalConnections = String();
    auto newExternalConnections = std::map<int, Array<Iolet*>>();

    // First, find all the incoming and outgoing connections
    for (auto* connection : connections) {
        if (selectedBoxes.contains(connection->inobj.getComponent()) && !selectedBoxes.contains(connection->outobj.getComponent())) {
            auto* inlet = connection->inlet.getComponent();
            targetEdges[inlet].add(connection->outlet.getComponent());
            usedEdges.addIfNotAlreadyThere(inlet);
        }
    }
    for (auto* connection : connections) {
        if (selectedBoxes.contains(connection->outobj.getComponent()) && !selectedBoxes.contains(connection->inobj.getComponent())) {
            auto* outlet = connection->outlet.getComponent();
            targetEdges[outlet].add(connection->inlet.getComponent());
            usedEdges.addIfNotAlreadyThere(outlet);
        }
    }

    auto newEdgeObjects = String();

    // Sort by position
    std::sort(usedEdges.begin(), usedEdges.end(),
        [](auto* a, auto* b) -> bool {
            // Inlets before outlets
            if (a->isInlet != b->isInlet)
                return a->isInlet;

            auto apos = a->getCanvasBounds().getPosition();
            auto bpos = b->getCanvasBounds().getPosition();

            if (apos.x == bpos.x) {
                return apos.y < bpos.y;
            }

            return apos.x < bpos.x;
        });

    int i = 0;
    int numIn = 0;
    for (auto* iolet : usedEdges) {
        auto type = String(iolet->isInlet ? "inlet" : "outlet") + String(iolet->isSignal ? "~" : "");
        auto* targetEdge = targetEdges[iolet][0];
        auto pos = targetEdge->object->getPosition();
        newEdgeObjects += "#X obj " + String(pos.x) + " " + String(pos.y) + " " + type + ";\n";

        int objIdx = selectedBoxes.indexOf(iolet->object);
        int ioletObjectIdx = selectedBoxes.size() + i;
        if (iolet->isInlet) {
            newInternalConnections += "#X connect " + String(ioletObjectIdx) + " 0 " + String(objIdx) + " " + String(iolet->ioletIdx) + ";\n";
            numIn++;
        } else {
            newInternalConnections += "#X connect " + String(objIdx) + " " + String(iolet->ioletIdx) + " " + String(ioletObjectIdx) + " 0;\n";
        }

        for (auto* target : targetEdges[iolet]) {
            newExternalConnections[i].add(target);
        }

        i++;
    }

    patch.deselectAll();

    auto bounds = Rectangle<int>();
    for (auto* object : selectedBoxes) {
        if (object->getPointer()) {
            bounds = bounds.getUnion(object->getBounds());
            patch.selectObject(object->getPointer()); // TODO: do this inside enqueue
        }
    }
    auto centre = bounds.getCentre();

    auto copypasta = String("#N canvas 733 172 450 300 0 1;\n") + "$$_COPY_HERE_$$" + newEdgeObjects + newInternalConnections + "#X restore " + String(centre.x) + " " + String(centre.y) + " pd;\n";

    // Apply the changed on Pd's thread
    pd->enqueueFunction([this, copypasta, newExternalConnections, numIn]() mutable {
        int size;
        const char* text = libpd_copy(patch.getPointer(), &size);
        auto copied = String::fromUTF8(text, size);

        // Wrap it in an undo sequence, to allow undoing everything in 1 step
        patch.startUndoSequence("encapsulate");

        libpd_removeselection(patch.getPointer());

        auto replacement = copypasta.replace("$$_COPY_HERE_$$", copied);
        SystemClipboard::copyTextToClipboard(replacement);

        libpd_paste(patch.getPointer(), replacement.toRawUTF8());
        auto* newObject = static_cast<t_object*>(patch.getObjects().back());

        for (auto& [idx, iolets] : newExternalConnections) {
            for (auto* iolet : iolets) {
                auto* externalObject = static_cast<t_object*>(iolet->object->getPointer());
                if (iolet->isInlet) {
                    libpd_createconnection(patch.getPointer(), newObject, idx - numIn, externalObject, iolet->ioletIdx);
                } else {
                    libpd_createconnection(patch.getPointer(), externalObject, iolet->ioletIdx, newObject, idx);
                }
            }
        }

        patch.endUndoSequence("encapsulate");
    });

    synchronise(true);
    patch.deselectAll();
}

bool Canvas::canConnectSelectedObjects()
{
    auto selection = getSelectionOfType<Object>();
    bool rightSize = selection.size() == 2;

    if (!rightSize)
        return false;

    Object* topObject = selection[0]->getY() > selection[1]->getY() ? selection[1] : selection[0];
    Object* bottomObject = selection[0] == topObject ? selection[1] : selection[0];

    bool hasInlet = bottomObject->numInputs > 0;
    bool hasOutlet = topObject->numOutputs > 0;

    return hasInlet && hasOutlet;
}

bool Canvas::connectSelectedObjects()
{
    auto selection = getSelectionOfType<Object>();
    bool rightSize = selection.size() == 2;

    if (!rightSize)
        return false;

    Object* topObject = selection[0]->getY() > selection[1]->getY() ? selection[1] : selection[0];
    Object* bottomObject = selection[0] == topObject ? selection[1] : selection[0];

    patch.createConnection(topObject->getPointer(), 0, bottomObject->getPointer(), 0);

    synchronise();

    return true;
}

void Canvas::cancelConnectionCreation()
{
    connectionsBeingCreated.clear();
}

void Canvas::undo()
{
    // Tell pd to undo the last action
    patch.undo();

    // Load state from pd
    synchronise();

    patch.deselectAll();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();

    // Load state from pd
    synchronise();

    patch.deselectAll();
}

void Canvas::checkBounds()
{
    if (isGraph) {

        auto viewBounds = Rectangle<int>(canvasOrigin.x, canvasOrigin.y, getWidth(), getHeight());

        for (auto* obj : objects) {
            viewBounds = viewBounds.getUnion(obj->getBoundsInParent());
        }

        canvasOrigin -= { viewBounds.getX(), viewBounds.getY() };

        setSize(viewBounds.getWidth(), viewBounds.getHeight());

        for (auto& object : objects) {
            object->updateBounds();
        }

        return;
    }

    if (!viewport)
        return;

    updatingBounds = true;

    float scale = std::max(1.0f, 1.0f / static_cast<float>(editor->zoomScale.getValue()));

    auto viewBounds = Rectangle<int>(canvasOrigin.x, canvasOrigin.y, (viewport->getWidth() - 8) * scale, (viewport->getHeight() - 8) * scale);

    for (auto* obj : objects) {
        viewBounds = viewBounds.getUnion(obj->getBoundsInParent().withTrimmedLeft(Object::margin).withTrimmedTop(Object::margin));
    }

    canvasOrigin -= { viewBounds.getX(), viewBounds.getY() };

    setSize(viewBounds.getWidth(), viewBounds.getHeight());

    for (auto& object : objects) {
        object->updateBounds();
    }

    if (graphArea) {
        graphArea->updateBounds();
    }

    updatingBounds = false;
}

void Canvas::valueChanged(Value& v)
{
    // When lock changes
    if (v.refersToSameSourceAs(locked)) {
        bool editMode = !static_cast<bool>(v.getValue());

        pd->enqueueFunction([this, editMode]() {
            patch.getPointer()->gl_edit = editMode;
        });

        cancelConnectionCreation();
        deselectAll();

        // move all connections to back when canvas is locked
        if (locked == var(true)) {
            // use reverse order to preserve correct connection layering
            for (int i = connections.size() - 1; i >= 0; i--) {
                connections[i]->setAlwaysOnTop(false);
                connections[i]->toBack();
            }
        } else {
            // otherwise move all connections to front
            for (auto connection : connections) {
                connection->setAlwaysOnTop(true);
                connection->toFront(false);
            }
        }

        repaint();

        // Makes sure no objects keep keyboard focus after locking/unlocking
        if (isShowing() && isVisible())
            grabKeyboardFocus();

        editor->updateCommandStatus();
    } else if (v.refersToSameSourceAs(commandLocked)) {
        repaint();
    }
    // Should only get called when the canvas isn't a real graph
    else if (v.refersToSameSourceAs(presentationMode)) {
        deselectAll();
        commandLocked.setValue(presentationMode.getValue());
    } else if (v.refersToSameSourceAs(isGraphChild)) {
        patch.getPointer()->gl_isgraph = static_cast<bool>(isGraphChild.getValue());

        if (static_cast<bool>(isGraphChild.getValue()) && !isGraph) {
            graphArea = new GraphArea(this);
            addAndMakeVisible(graphArea);
            graphArea->setAlwaysOnTop(true);
            graphArea->updateBounds();
        } else {
            delete graphArea;
            graphArea = nullptr;
        }
        repaint();
    } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
        patch.getPointer()->gl_hidetext = static_cast<bool>(hideNameAndArgs.getValue());
        repaint();
    } else if (v.refersToSameSourceAs(xRange)) {
        auto* glist = patch.getPointer();
        glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
        glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
        updateDrawables();
    } else if (v.refersToSameSourceAs(yRange)) {
        auto* glist = patch.getPointer();
        glist->gl_y2 = static_cast<float>(yRange.getValue().getArray()->getReference(0));
        glist->gl_y1 = static_cast<float>(yRange.getValue().getArray()->getReference(1));
        updateDrawables();
    }
}

void Canvas::showSuggestions(Object* object, TextEditor* textEditor)
{
    suggestor->createCalloutBox(object, textEditor);
}
void Canvas::hideSuggestions()
{
    suggestor->removeCalloutBox();
}

// Makes component selected
void Canvas::setSelected(Component* component, bool shouldNowBeSelected, bool updateCommandStatus)
{
    bool isAlreadySelected = isSelected(component);

    if (!isAlreadySelected && shouldNowBeSelected) {
        selectedComponents.addToSelection(component);
        component->repaint();
    }

    if (isAlreadySelected && !shouldNowBeSelected) {
        removeSelectedComponent(component);
        component->repaint();
    }

    if (updateCommandStatus) {
        editor->updateCommandStatus();
    }
}

bool Canvas::isSelected(Component* component) const
{
    return selectedComponents.isSelected(component);
}

void Canvas::objectMouseDown(Object* component, MouseEvent const& e)
{
    if (isGraph)
        return;

    if (e.mods.isRightButtonDown()) {
        setSelected(component, true);

        PopupMenu::dismissAllActiveMenus();
        Dialogs::showCanvasRightClickMenu(this, component, e.getScreenPosition());

        return;
    }
    if (e.mods.isShiftDown()) {
        // select multiple objects
        wasSelectedOnMouseDown = isSelected(component);
    } else if (!isSelected(component)) {
        // not interfeering with alt + drag
        // unselect all & select clicked object
        for (auto* object : objects) {
            setSelected(object, false);
        }
        for (auto* connection : connections) {
            setSelected(connection, false);
        }
    }
    setSelected(component, true);

    if (auto* object = dynamic_cast<Object*>(component)) {
        componentBeingDragged = object;
    }

    for (auto* object : getSelectionOfType<Object>()) {
        object->originalBounds = object->getBounds();
        object->setBufferedToImage(true);
    }

    if (component) {
        component->repaint();
    }

    canvasDragStartPosition = getPosition();

    updateSidebarSelection();
}

// Call from component's mouseUp
void Canvas::objectMouseUp(Object* component, MouseEvent const& e)
{
    if (isGraph)
        return;

    if (e.mods.isShiftDown() && wasSelectedOnMouseDown && !didStartDragging) {
        // Unselect object if selected
        setSelected(component, false);
    } else if (!e.mods.isShiftDown() && !e.mods.isAltDown() && isSelected(component) && !didStartDragging && !e.mods.isRightButtonDown()) {

        // Don't run normal deselectAll, that would clear the sidebar inspector as well
        // We'll update sidebar selection later in this function
        selectedComponents.deselectAll();

        setSelected(component, true);
    }

    updateSidebarSelection();

    if (didStartDragging) {
        auto objects = std::vector<void*>();

        for (auto* object : getSelectionOfType<Object>()) {
            if (object->getPointer())
                objects.push_back(object->getPointer());
        }

        auto distance = Point<int>(e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY());

        // In case we dragged near the iolet and the canvas moved
        auto canvasMoveOffset = canvasDragStartPosition - getPosition();

        distance = objectGrid.handleMouseUp(distance) + canvasMoveOffset;

        // When done dragging objects, update positions to pd
        patch.moveObjects(objects, distance.x, distance.y);

        pd->waitForStateUpdate();

        checkBounds();
        didStartDragging = false;
    }

    if (objectSnappingInbetween) {
        auto* c = connectionToSnapInbetween.getComponent();

        // TODO: this is potentially problematic
        // since some of these actions might be executed on different threads,
        // depending on if we can get a lock

        patch.startUndoSequence("SnapInbetween");

        patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

        patch.createConnection(c->outobj->getPointer(), c->outIdx, objectSnappingInbetween->getPointer(), 0);
        patch.createConnection(objectSnappingInbetween->getPointer(), 0, c->inobj->getPointer(), c->inIdx);

        patch.endUndoSequence("SnapInbetween");

        objectSnappingInbetween->iolets[0]->isTargeted = false;
        objectSnappingInbetween->iolets[objectSnappingInbetween->numInputs]->isTargeted = false;
        objectSnappingInbetween = nullptr;

        synchronise();
    }

    if (wasDragDuplicated) {
        patch.endUndoSequence("Duplicate");
        wasDragDuplicated = false;
    }

    for (auto* object : getSelectionOfType<Object>()) {
        object->setBufferedToImage(false);
        object->repaint();
    }

    mouseDownObjectPositions.clear();
    componentBeingDragged = nullptr;

    component->repaint();
}

// Call from component's mouseDrag
void Canvas::objectMouseDrag(MouseEvent const& e)
{
    if (isGraph)
        return;

    /** Ensure tiny movements don't start a drag. */
    if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
        return;

    if (!didStartDragging) {
        didStartDragging = true;
    }

    auto selection = getSelectionOfType<Object>();

    auto dragDistance = e.getOffsetFromDragStart();

    // In case we dragged near the edge and the canvas moved
    auto canvasMoveOffset = canvasDragStartPosition - getPosition();

    if (static_cast<bool>(gridEnabled.getValue()) && componentBeingDragged) {
        dragDistance = objectGrid.performMove(componentBeingDragged, dragDistance);
    }


    // alt+drag will duplicate selection
    if (!wasDragDuplicated && e.mods.isAltDown()) {

        // Single for undo for duplicate + move
       patch.startUndoSequence("Duplicate");

        // Sort selection indexes to match pd indexes
        std::sort(selection.begin(), selection.end(),
            [this](auto* a, auto* b) -> bool {
                return objects.indexOf(a) < objects.indexOf(b);
            });

        // Store origin object positions
        for (auto object : selection) {
            mouseDownObjectPositions.add(object->getPosition());
        }

        // Duplicate once
        wasDragDuplicated = true;
        duplicateSelection();
        cancelConnectionCreation();
        componentBeingDragged.getComponent()->originalBounds.translate(-10, -10);
    }

    // FIXME: stop the mousedrag event from blocking the objects from redrawing, we shouldn't need to do this? JUCE bug?
    if (!objectRateReducer.tooFast()) {
        if (wasDragDuplicated) {
            // Correct distancing
            dragDistance = dragDistance.translated(10, 10);
            // Move duplicated objects according to the origin position
            for (auto object : selection) {

                if (!isPositiveAndBelow(selection.indexOf(object), mouseDownObjectPositions.size()))
                    continue;

                object->setTopLeftPosition(mouseDownObjectPositions[selection.indexOf(object)] + dragDistance + canvasMoveOffset);
            }
        } else {
            for (auto* object : selection) {
                object->setTopLeftPosition(object->originalBounds.getPosition() + dragDistance + canvasMoveOffset);
            }
        }
        
        auto draggedBounds = componentBeingDragged->getBounds().expanded(6);
        auto xEdge = e.getDistanceFromDragStartX() < 0 ? draggedBounds.getX() : draggedBounds.getRight();
        auto yEdge = e.getDistanceFromDragStartY() < 0 ? draggedBounds.getY() : draggedBounds.getBottom();
        if(autoscroll(e.getEventRelativeTo(this).withNewPosition(Point<int>(xEdge, yEdge)).getEventRelativeTo(viewport))) {
            beginDragAutoRepeat(25);
        }
    }

    // This handles the "unsnap" action when you shift-drag a connected object
    if (e.mods.isShiftDown() && selection.size() == 1 && e.getDistanceFromDragStart() > 15) {
        auto* object = selection.getFirst();

        Array<Connection*> inputs, outputs;
        for (auto* connection : connections) {
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
                patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                connections.add(new Connection(this, outlet, c->inlet, nullptr));
                connections.removeObject(c);
            }

            auto* c = inputs[0];
            patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
            connections.removeObject(c);

            object->iolets[0]->isTargeted = false;
            object->iolets[object->numInputs]->isTargeted = false;
            object->iolets[0]->repaint();
            object->iolets[object->numInputs]->repaint();

            objectSnappingInbetween = nullptr;
        } else if (inputs.size() && outputs.size() == 1) {
            auto* inlet = outputs[0]->inlet.getComponent();

            for (auto* c : inputs) {
                patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                connections.add(new Connection(this, c->outlet, inlet, nullptr));
                connections.removeObject(c);
            }

            auto* c = outputs[0];
            patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
            connections.removeObject(c);

            object->iolets[0]->isTargeted = false;
            object->iolets[object->numInputs]->isTargeted = false;
            object->iolets[0]->repaint();
            object->iolets[object->numInputs]->repaint();

            objectSnappingInbetween = nullptr;
        }
    }

    // Behaviour for shift-dragging objects over
    if (objectSnappingInbetween) {
        if (connectionToSnapInbetween->intersectsObject(objectSnappingInbetween)) {
            return;
        }

        // If we're here, it's not snapping anymore
        objectSnappingInbetween->iolets[0]->isTargeted = false;
        objectSnappingInbetween->iolets[objectSnappingInbetween->numInputs]->isTargeted = false;
        objectSnappingInbetween = nullptr;
    }

    if (e.mods.isShiftDown() && selection.size() == 1) {
        auto* object = selection.getFirst();
        if (object->numInputs && object->numOutputs) {
            bool intersected = false;
            for (auto* connection : connections) {
                if (connection->intersectsObject(object)) {
                    object->iolets[0]->isTargeted = true;
                    object->iolets[object->numInputs]->isTargeted = true;
                    object->iolets[0]->repaint();
                    object->iolets[object->numInputs]->repaint();
                    connectionToSnapInbetween = connection;
                    objectSnappingInbetween = object;
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

SelectedItemSet<WeakReference<Component>>& Canvas::getLassoSelection()
{
    return selectedComponents;
}

void Canvas::removeSelectedComponent(Component* component)
{
    selectedComponents.deselect(component);
}

void Canvas::findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area)
{
    for (auto* object : objects) {
        if (area.intersects(object->getBounds().reduced(Object::margin))) {
            itemsFound.add(object);
            setSelected(object, true, false);
        } else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
            setSelected(object, false, false);
        }
    }

    for (auto& con : connections) {
        // If total bounds don't intersect, there can't be an intersection with the line
        // This is cheaper than checking the path intersection, so do this first
        if (!con->getBounds().intersects(lasso.getBounds())) {
            setSelected(con, false, false);
            continue;
        }

        // Check if path intersects with lasso
        if (con->intersects(lasso.getBounds().translated(-con->getX(), -con->getY()).toFloat())) {
            itemsFound.add(con);
            setSelected(con, true, false);
        } else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
            setSelected(con, false, false);
        }
    }
}

ObjectParameters& Canvas::getInspectorParameters()
{
    return parameters;
}
