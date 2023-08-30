/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Sidebar/Sidebar.h"
#include "Statusbar.h"
#include "Canvas.h"
#include "Object.h"
#include "Connection.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "SuggestionComponent.h"
#include "CanvasViewport.h"
#include "SplitView.h"

#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"
#include "Utility/GraphArea.h"
#include "Utility/RateReducer.h"

extern "C" {
void canvas_setgraph(t_glist* x, int flag, int nogoprect);
}

Canvas::Canvas(PluginEditor* parent, pd::Patch::Ptr p, Component* parentGraph)
    : editor(parent)
    , pd(parent->pd)
    , refCountedPatch(p)
    , patch(*p)
    , pathUpdater(new ConnectionPathUpdater(this))
    , graphArea(nullptr)
    , canvasOrigin(Point<int>(infiniteCanvasSize / 2, infiniteCanvasSize / 2))
{
    if (auto patchPtr = patch.getPointer()) {
        isGraphChild = glist_isgraph(patchPtr.get());
    }

    hideNameAndArgs = static_cast<bool>(patch.getPointer()->gl_hidetext);
    xRange = Array<var> { var(patch.getPointer()->gl_x1), var(patch.getPointer()->gl_x2) };
    yRange = Array<var> { var(patch.getPointer()->gl_y2), var(patch.getPointer()->gl_y1) };

    pd->registerMessageListener(patch.getPointer().get(), this);

    isGraphChild.addListener(this);
    hideNameAndArgs.addListener(this);
    xRange.addListener(this);
    yRange.addListener(this);

    patchWidth = patch.getBounds().getWidth();
    patchHeight = patch.getBounds().getHeight();

    patchWidth.addListener(this);
    patchHeight.addListener(this);

    // Check if canvas belongs to a graph
    if (parentGraph) {
        setLookAndFeel(&editor->getLookAndFeel());
        parentGraph->addAndMakeVisible(this);
        setInterceptsMouseClicks(false, true);
        isGraph = true;
    } else {
        isGraph = false;
    }

    if(!isGraph) {
        auto* canvasViewport = new CanvasViewport(editor, this);
        
        canvasViewport->setViewedComponent(this, false);
        
        canvasViewport->onScroll = [this]() {
            if (suggestor) {
                suggestor->updateBounds();
            }
            if (graphArea) {
                graphArea->updateBounds();
            }
        };
        
        canvasViewport->setScrollBarsShown(true, true, true, true);
        
        viewport.reset(canvasViewport); // Owned by the tabbar, but doesn't exist for graph!
        jumpToOrigin();
    }
    

    suggestor = new SuggestionComponent;

    commandLocked.referTo(pd->commandLocked);
    commandLocked.addListener(this);

    // init border for testing
    propertyChanged("border", SettingsFile::getInstance()->getPropertyAsValue("border"));

    // Add draggable border for setting graph position
    if (getValue<bool>(isGraphChild) && !isGraph) {
        graphArea = std::make_unique<GraphArea>(this);
        addAndMakeVisible(*graphArea);
        graphArea->setAlwaysOnTop(true);
    }

    updateOverlays();

    setSize(infiniteCanvasSize, infiniteCanvasSize);

    // initialize per canvas zoom to 100% when first creating canvas
    zoomScale.setValue(1.0f);
    zoomScale.addListener(this);

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);

    setWantsKeyboardFocus(true);

    if (!isGraph) {
        presentationMode.addListener(this);
    } else {
        presentationMode = false;
    }
    performSynchronise();

    // Start in unlocked mode if the patch is empty
    if (objects.isEmpty()) {
        locked = false;
        patch.getPointer()->gl_edit = false;
    } else {
        locked = !patch.getPointer()->gl_edit;
    }

    locked.addListener(this);

    editor->addModifierKeyListener(this);
    Desktop::getInstance().addFocusChangeListener(this);

    parameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" }, 0);
    parameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" }, 0);
    parameters.addParamRange("X range", cGeneral, &xRange, { 0.0f, 1.0f });
    parameters.addParamRange("Y range", cGeneral, &yRange, { 1.0f, 0.0f });
    parameters.addParamInt("Width", cDimensions, &patchWidth, 527);
    parameters.addParamInt("Height", cDimensions, &patchHeight, 327);
}

Canvas::~Canvas()
{
    zoomScale.removeListener(this);
    editor->removeModifierKeyListener(this);
    pd->unregisterMessageListener(patch.getPointer().get(), this);

    Desktop::getInstance().removeFocusChangeListener(this);

    delete suggestor;
}

void Canvas::propertyChanged(String const& name, var const& value)
{
    switch (hash(name)) {
    case hash("grid_size"):
        repaint();
        break;
    case hash("border"):
        showBorder = static_cast<int>(value);
        repaint();
        break;
    case hash("edit"):
    case hash("lock"):
    case hash("run"):
    case hash("alt"):
    case hash("alt_mode"): {
        updateOverlays();
        break;
    }
    }
}

int Canvas::getOverlays() const
{
    int overlayState = 0;

    auto overlaysTree = SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays");

    auto altModeEnabled = overlaysTree.getProperty("alt_mode");

    if (!locked.getValue()) {
        overlayState = overlaysTree.getProperty("edit");
    }
    if (locked.getValue() || commandLocked.getValue()) {
        overlayState = overlaysTree.getProperty("lock");
    }
    if (presentationMode.getValue()) {
        overlayState = overlaysTree.getProperty("run");
    }
    if (altModeEnabled) {
        overlayState = overlaysTree.getProperty("alt");
    }

    return overlayState;
}

void Canvas::moved()
{
}

void Canvas::resized()
{
}

void Canvas::updateOverlays()
{
    int overlayState = getOverlays();

    showBorder = overlayState & Border;
    showOrigin = overlayState & Origin;

    for (auto* object : objects) {
        object->updateOverlays(overlayState);
    }

    for (auto* connection : connections) {
        connection->updateOverlays(overlayState);
    }

    repaint();
}


void Canvas::jumpToOrigin()
{
    viewport->setViewPosition(canvasOrigin.transformedBy(getTransform()) + Point<int>(1, 1));
}

void Canvas::zoomToFitAll()
{
    if (objects.isEmpty() || !viewport)
        return;

    auto scale = getValue<float>(zoomScale);

    auto regionOfInterest = Rectangle<int>();
    for (auto* object : objects) {
        regionOfInterest = regionOfInterest.getUnion(object->getBounds().reduced(Object::margin));
    }

    // Add a bit of margin to make it nice
    regionOfInterest = regionOfInterest.expanded(16);

    auto viewArea = viewport->getViewArea() / scale;

    auto roiHeight = static_cast<float>(regionOfInterest.getHeight());
    auto roiWidth = static_cast<float>(regionOfInterest.getWidth());
    auto viewHeight = viewArea.getHeight();
    auto viewWidth = viewArea.getWidth();

    if (roiWidth > viewWidth || roiHeight > viewHeight) {
        auto scaleWidth = viewWidth / roiWidth;
        auto scaleHeight = viewHeight / roiHeight;
        scale = jmin(scaleWidth, scaleHeight);

        auto transform = getTransform();
        transform = transform.scaled(scale);
        setTransform(transform);

        scale = std::sqrt(std::abs(transform.getDeterminant()));
        zoomScale.setValue(scale);
    }

    auto viewportCentre = viewport->getViewArea().withZeroOrigin().getCentre();
    auto newViewPos = regionOfInterest.transformedBy(getTransform()).getCentre() - viewportCentre;
    viewport->setViewPosition(newViewPos);
}

void Canvas::lookAndFeelChanged()
{
    lasso.setColour(LassoComponent<Object>::lassoFillColourId, findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f));
}

void Canvas::paint(Graphics& g)
{
    if (isGraph)
        return;

    g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));

    if (viewport)
        g.reduceClipRegion(viewport->getViewArea().transformedBy(getTransform().inverted()));
    auto clipBounds = g.getClipBounds();

    // Clip bounds so that we have the smallest lines that fit the viewport, but also
    // compensate for line start, so the dashes don't stay fixed in place if they are drawn from
    // the top of the viewport
    auto clippedOrigin = Point<float>(std::max(canvasOrigin.x, clipBounds.getX()), std::max(canvasOrigin.y, clipBounds.getY()));

    auto originDiff = canvasOrigin.toFloat() - clippedOrigin;

    // draw patch window dashed outline
    auto patchWidthCanvas = clippedOrigin.x + (getValue<int>(patchWidth) + originDiff.x);
    auto patchHeightCanvas = clippedOrigin.y + (getValue<int>(patchHeight) + originDiff.y);

    clippedOrigin.x += fmod(originDiff.x, 10.0f) - 0.5f;
    clippedOrigin.y += fmod(originDiff.y, 10.0f) - 0.5f;

    if (!getValue<bool>(locked)) {

        auto startX = (canvasOrigin.x % objectGrid.gridSize);
        startX += ((clipBounds.getX() / objectGrid.gridSize) * objectGrid.gridSize);

        auto startY = (canvasOrigin.y % objectGrid.gridSize);
        startY += ((clipBounds.getY() / objectGrid.gridSize) * objectGrid.gridSize);

        g.setColour(findColour(PlugDataColour::canvasDotsColourId));

        for (int x = startX; x < clipBounds.getRight(); x += objectGrid.gridSize) {
            for (int y = startY; y < clipBounds.getBottom(); y += objectGrid.gridSize) {

                // Don't draw over origin line
                if (showBorder || showOrigin) {
                    if ((x == canvasOrigin.x && y >= canvasOrigin.y && y <= patchHeightCanvas) || (y == canvasOrigin.y && x >= canvasOrigin.x && x <= patchWidthCanvas))
                        continue;
                }
                g.fillRect(static_cast<float>(x) - 0.5f, static_cast<float>(y) - 0.5f, 1.0, 1.0);
            }
        }
    }

    if (!showOrigin && !showBorder)
        return;

    /*
     ┌────────┐
     │a      b│
     │        │
     │        │
     │d      c│
     └────────┘
     */

    // points for border
    auto pointA = Point<float>(clippedOrigin.x, clippedOrigin.y);
    auto pointB = Point<float>(patchWidthCanvas, clippedOrigin.y);
    auto pointC = Point<float>(patchWidthCanvas, patchHeightCanvas);
    auto pointD = Point<float>(clippedOrigin.x, patchHeightCanvas);

    auto extentTop = Line<float>(pointA, pointB);
    auto extentLeft = Line<float>(pointA, pointD);

    // arrange line points so that dashes appear to grow from origin and bottom right
    if (showOrigin) {

        // points for origin extending to edge of view
        auto pointOriginB = Point<float>(clipBounds.getRight(), clippedOrigin.y);
        auto pointOriginD = Point<float>(clippedOrigin.x, clipBounds.getBottom());

        extentTop = Line<float>(pointA, pointOriginB);
        extentLeft = Line<float>(pointA, pointOriginD);
    }

    float dash[2] = { 5.0f, 5.0f };

    g.setColour(findColour(PlugDataColour::canvasDotsColourId));

    g.drawDashedLine(extentLeft, dash, 2, 1.0f);
    g.drawDashedLine(extentTop, dash, 2, 1.0f);

    if (showBorder) {
        auto extentRight = Line<float>(pointC, pointB);
        auto extentBottom = Line<float>(pointC, pointD);

        g.drawDashedLine(extentRight, dash, 2, 1.0f);
        g.drawDashedLine(extentBottom, dash, 2, 1.0f);
    }
}

TabComponent* Canvas::getTabbar()
{
    for (auto* split : editor->splitView.splits) {
        auto tabbar = split->getTabComponent();
        if (tabbar->getIndexOfCanvas(this) >= 0)
            return tabbar;
    }

    return nullptr;
}

void Canvas::globalFocusChanged(Component* focusedComponent)
{
    // ALEX do we need any of this?
    //if (!focusedComponent || !editor->splitView.isSplitEnabled() || editor->splitView.hasFocus(this))
    //    return;
    //
    //if (focusedComponent == this || focusedComponent->findParentComponentOfClass<Canvas>() == this) {
    //    editor->splitView.setFocus(this);
    //}
}

void Canvas::tabChanged()
{
    patch.setCurrent();

    synchronise();
    updateDrawables();

    // update GraphOnParent when changing tabs
    // TODO: shouldn't we do this always on sync?
    for (auto* obj : objects) {
        if (!obj->gui)
            continue;

        obj->gui->tabChanged();
    }
}

int Canvas::getTabIndex()
{
    if(auto* tabbar = getTabbar())
    {
        return tabbar->getIndexOfCanvas(this);
    }
    return -1;
}

void Canvas::handleAsyncUpdate()
{
    performSynchronise();
}

void Canvas::synchronise()
{
    triggerAsyncUpdate();
}

void Canvas::synchroniseSplitCanvas()
{   
    for (auto split : editor->splitView.splits) {
        auto tabbar = split->getTabComponent();
        if (auto* activeTabCanvas = tabbar->getCurrentCanvas())
            activeTabCanvas->synchronise();
    }
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::performSynchronise()
{
    pd->lockAudioThread();

    patch.setCurrent();
    pd->sendMessagesFromQueue();

    pd->unlockAudioThread();

    // Remove deleted connections
    for (int n = connections.size() - 1; n >= 0; n--) {
        if (patch.connectionWasDeleted(connections[n]->getPointer())) {
            connections.remove(n);
        }
    }

    // Remove deleted objects
    for (int n = objects.size() - 1; n >= 0; n--) {
        auto* object = objects[n];
        if (!object->getPointer() || patch.objectWasDeleted(object->getPointer())) {
            setSelected(object, false, false);
            objects.remove(n);
        }
    }

    // Check for connections that need to be remade because of invalid iolets
    for (int n = connections.size() - 1; n >= 0; n--) {
        if (!connections[n]->inlet || !connections[n]->outlet) {
            connections.remove(n);
        }
    }

    auto pdObjects = patch.getObjects();

    for (auto* object : pdObjects) {
        auto* it = std::find_if(objects.begin(), objects.end(), [&object](Object* b) { return b->getPointer() && b->getPointer() == object; });

        if (patch.objectWasDeleted(object))
            continue;

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
            object->updateBounds();

            object->toFront(false);
            if (object->gui && object->gui->getLabel())
                object->gui->getLabel()->toFront(false);
            if (object->gui)
                object->gui->update();
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

        Iolet *inlet = nullptr, *outlet = nullptr;

        // Find the objects that this connection is connected to
        for (auto* obj : objects) {
            if (outobj && outobj == obj->getPointer()) {

                // Check if we have enough outlets, should never return false
                if (isPositiveAndBelow(obj->numInputs + outno, obj->iolets.size())) {
                    outlet = obj->iolets[obj->numInputs + outno];
                } else {
                    break;
                }
            }
            if (inobj && inobj == obj->getPointer()) {

                // Check if we have enough inlets, should never return false
                if (isPositiveAndBelow(inno, obj->iolets.size())) {
                    inlet = obj->iolets[inno];
                } else {
                    break;
                }
            }
        }

        // This shouldn't be necessary, but just to be sure...
        if (!inlet || !outlet) {
            jassertfalse;
            continue;
        }

        auto* it = std::find_if(connections.begin(), connections.end(),
            [c_ptr = ptr](auto* c) {
                return c_ptr == c->getPointer();
            });

        if (it == connections.end()) {
            connections.add(new Connection(this, inlet, outlet, ptr));
        } else {
            auto& c = *(*it);
            
            // This is necessary to make resorting a subpatchers iolets work
            // And it can't hurt to check if the connection is valid anyway
            if(c.inlet != inlet || c.outlet != outlet)
            {
                int idx = connections.indexOf(*it);
                connections.removeObject(*it);
                connections.insert(idx, new Connection(this, inlet, outlet, ptr));
            }
            else {
                c.popPathState();
            }
        }
    }

    if (!isGraph) {
        setTransform(AffineTransform().scaled(getValue<float>(zoomScale)));
    }
    
    if(graphArea) graphArea->updateBounds();

    editor->updateCommandStatus();
    repaint();

    pd->updateObjectImplementations();
}

void Canvas::updateDrawables()
{
    for (auto* object : objects) {
        if (object->gui) {
            object->gui->updateDrawables();
        }
    }
}

void Canvas::commandKeyChanged(bool isHeld)
{
    commandLocked = isHeld;
}

void Canvas::spaceKeyChanged(bool isHeld)
{
    checkPanDragMode();
}

void Canvas::middleMouseChanged(bool isHeld)
{
    checkPanDragMode();
}

void Canvas::altKeyChanged(bool isHeld)
{
    SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").setProperty("alt_mode", isHeld, nullptr);
}

void Canvas::focusGained(FocusChangeType type)
{
    //std::cout << "focus on canvas: " << this << std::endl;
}

void Canvas::focusLost(FocusChangeType type)
{
    //std::cout << "focus lost on canvas: " << this << std::endl;
}

void Canvas::mouseDown(MouseEvent const& e)
{
    PopupMenu::dismissAllActiveMenus();

    if (checkPanDragMode())
        return;

    auto* source = e.originalComponent;

    // Left-click
    if (!e.mods.isRightButtonDown()) {
        
        if (source == this /*|| source == graphArea */) {

            cancelConnectionCreation();

            if (e.mods.isCommandDown()) {
                // Lock if cmd + click on canvas
                deselectAll();

                presentationMode.setValue(false);
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
    else if (!editor->pluginMode) {
        Dialogs::showCanvasRightClickMenu(this, source, e.getScreenPosition());
    }
}

void Canvas::mouseDrag(MouseEvent const& e)
{
    if (canvasRateReducer.tooFast() || panningModifierDown())
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

    auto viewportEvent = e.getEventRelativeTo(viewport.get());
    if (viewport && !ObjectBase::isBeingEdited() && autoscroll(viewportEvent)) {
        beginDragAutoRepeat(25);
    }

    // Drag lasso
    lasso.dragLasso(e);
}

bool Canvas::autoscroll(MouseEvent const& e)
{
    if (!viewport)
        return false;

    auto x = viewport->getViewPositionX();
    auto y = viewport->getViewPositionY();
    auto oldY = y;
    auto oldX = x;

    auto pos = e.getPosition();

    if (pos.x > viewport->getWidth()) {
        x += std::clamp((pos.x - viewport->getWidth()) / 6, 1, 14);
    } else if (pos.x < 0) {
        x -= std::clamp(-pos.x / 6, 1, 14);
    }
    if (pos.y > viewport->getHeight()) {
        y += std::clamp((pos.y - viewport->getHeight()) / 6, 1, 14);
    } else if (pos.y < 0) {
        y -= std::clamp(-pos.y / 6, 1, 14);
    }

    if (x != oldX || y != oldY) {
        viewport->setViewPosition(x, y);
        return true;
    }

    return false;
}

void Canvas::mouseUp(MouseEvent const& e)
{
    setPanDragMode(false);
    setMouseCursor(MouseCursor::NormalCursor);

    connectionCancelled = false;

    // Double-click canvas to create new object
    if (e.mods.isLeftButtonDown() && (e.getNumberOfClicks() == 2) && (e.originalComponent == this) && !isGraph && !getValue<bool>(locked)) {
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
        auto showOnSelect = object->gui ? object->gui->showParametersWhenSelected() : false;
        
        if (!object->gui) {
            editor->sidebar->hideParameters();
            return;
        }

        if ((showOnSelect && !params.getParameters().isEmpty()) || editor->sidebar->isPinned()) {
            editor->sidebar->showParameters(object->gui->getText(), params);
        } else {
            editor->sidebar->hideParameters();
        }
    } else {
        editor->sidebar->hideParameters();
    }
}

void Canvas::mouseMove(MouseEvent const& e)
{
    // TODO: can we get rid of this?
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
            if(auto* ptr = object->getPointer()) {
                pdObjects.push_back(ptr);
            }
        }

        patch.moveObjects(pdObjects, x, y);
        
        // Update object bounds and store the total bounds of the selection
        auto totalBounds = Rectangle<int>();
        for (auto* object : objects) {
            object->updateBounds();
            totalBounds = totalBounds.getUnion(object->getBounds());
        }
        
        int viewX = viewport->getViewPositionX();
        int viewY = viewport->getViewPositionY();
        if(x < 0 && totalBounds.getX() < viewX)
        {
            viewX = totalBounds.getX();
        }
        else if(totalBounds.getRight() > viewX + viewport->getWidth()){
            viewX = totalBounds.getRight() - viewport->getWidth();
        }
        if(y < 0 && totalBounds.getY() < viewY)
        {
            viewY = totalBounds.getY();
        }
        else if(totalBounds.getBottom() > viewY + viewport->getHeight())
        {
            viewY = totalBounds.getBottom() - viewport->getHeight();
        }
        
        viewport->setViewPosition(viewX, viewY);
    };

    // Cancel connections being created by ESC key
    if (keycode == KeyPress::escapeKey && !connectionsBeingCreated.isEmpty()) {
        cancelConnectionCreation();
        return true;
    }

    // Move objects with arrow keys
    int moveDistance = objectGrid.gridSize;
    if (key.getModifiers().isShiftDown()) {
        moveDistance = 1;
    } else if (key.getModifiers().isCommandDown()) {
        moveDistance *= 4;
    }

    if (keycode == KeyPress::leftKey) {
        moveSelection(-moveDistance, 0);
        return false;
    }
    if (keycode == KeyPress::rightKey) {
        moveSelection(moveDistance, 0);
        return false;
    }
    if (keycode == KeyPress::upKey) {
        moveSelection(0, -moveDistance);
        return false;
    }
    if (keycode == KeyPress::downKey) {
        moveSelection(0, moveDistance);
        return false;
    }

    return false;
}

void Canvas::deselectAll()
{
    selectedComponents.deselectAll();

    editor->sidebar->hideParameters();
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
        if(auto* ptr = object->getPointer()) {
            patch.selectObject(ptr);
        }
    }

    // Tell pd to copy
    patch.copy();
    patch.deselectAll();
}

void Canvas::dragAndDropPaste(String const& patchString, Point<int> mousePos, int patchWidth, int patchHeight)
{
    locked = false;
    presentationMode = false;

    // force the valueChanged to run, and wait for them to return
    locked.getValueSource().sendChangeMessage(true);
    presentationMode.getValueSource().sendChangeMessage(true);

    MessageManager::callAsync([_this = SafePointer(this)]() {
        if (_this)
            _this->grabKeyboardFocus();
    });

    patch.startUndoSequence("DragAndDropPaste"); // TODO: we can add the name of the event that it's dragging from?

    auto patchSize = Point<int>(patchWidth, patchHeight);
    String translatedObjects = pd::Patch::translatePatchAsString(patchString, mousePos - (patchSize / 2.0f));

    if (auto patchPtr = patch.getPointer()) {
        libpd_paste(patchPtr.get(), translatedObjects.toRawUTF8());
    }

    deselectAll();

    // Load state from pd
    performSynchronise();

    patch.setCurrent();

    std::vector<void*> pastedObjects;

    auto* patchPtr = patch.getPointer().get();
    if (!patchPtr)
        return;

    pd->lockAudioThread();
    for (auto* object : objects) {
        auto* objectPtr = static_cast<t_gobj*>(object->getPointer());
        if (objectPtr && glist_isselected(patchPtr, objectPtr)) {
            setSelected(object, true);
            pastedObjects.emplace_back(objectPtr);
        }
    }
    pd->unlockAudioThread();

    patch.deselectAll();
    pastedObjects.clear();
    patch.endUndoSequence("DragAndDropPaste");
    
    updateSidebarSelection();
}

void Canvas::pasteSelection()
{
    patch.startUndoSequence("Paste");

    // Paste at mousePos, adds padding if pasted the same place
    if (lastMousePosition == pastedPosition) {
        pastedPadding.addXY(10, 10);
    } else {
        pastedPadding.setXY(-10, -10);
    }
    pastedPosition = lastMousePosition;

    // Tell pd to paste with offset applied to the clipboard string
    patch.paste(Point<int>(pastedPosition.x + pastedPadding.x, pastedPosition.y + pastedPadding.y));

    deselectAll();

    // Load state from pd
    performSynchronise();

    patch.setCurrent();

    std::vector<void*> pastedObjects;

    auto* patchPtr = patch.getPointer().get();
    if (!patchPtr)
        return;

    pd->lockAudioThread();
    for (auto* object : objects) {
        auto* objectPtr = static_cast<t_gobj*>(object->getPointer());
        if (objectPtr && glist_isselected(patchPtr, objectPtr)) {
            setSelected(object, true);
            pastedObjects.emplace_back(objectPtr);
        }
    }
    pd->unlockAudioThread();

    patch.deselectAll();
    pastedObjects.clear();
    patch.endUndoSequence("Paste");
    
    updateSidebarSelection();
}

void Canvas::duplicateSelection()
{
    Array<Connection*> conInlets, conOutlets;
    auto selection = getSelectionOfType<Object>();

    patch.startUndoSequence("Duplicate");

    // clear all previous selections from pd
    patch.deselectAll();

    for (auto* object : selection) {
        
        auto* ptr = object->getPointer();
        // Check if object exists in pd and is not attached to mouse
        if (!ptr || object->attachedToMouse)
            return;
        
        // Tell pd to select all objects that are currently selected
        patch.selectObject(ptr);

        if (!dragState.wasDragDuplicated && editor->autoconnect.getValue()) {
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

    // Load state from pd immediately
    performSynchronise();

    auto* patchPtr = patch.getPointer().get();
    if (!patchPtr)
        return;

    // Store the duplicated objects for later selection
    Array<Object*> duplicated;
    for (auto* object : objects) {
        auto* objectPtr = static_cast<t_gobj*>(object->getPointer());
        if (objectPtr && glist_isselected(patchPtr, objectPtr)) {
            duplicated.add(object);
        }
    }

    // Auto patching
    if (!dragState.wasDragDuplicated && editor->autoconnect.getValue()) {
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
                if (!duplicated.isEmpty() && !duplicated.contains(object) && duplicated[0]->getBounds().translated(moveDistance, 0).intersects(object->getBounds())) {
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
    editor->sidebar->hideParameters();

    // Make sure nothing is selected
    patch.deselectAll();

    // Find selected objects and make them selected in pd
    Array<void*> objects;
    for (auto* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            patch.selectObject(ptr);
            objects.add(ptr);
        }
    }

    // remove selection
    patch.removeSelection();

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto* con : connections) {
        if (con->isSelected()) {
            auto* outPtr = con->outobj->getPointer();
            auto* inPtr = con->inobj->getPointer();
            if (outPtr && inPtr && (!(objects.contains(outPtr) || objects.contains(inPtr)))) {
                patch.removeConnection(outPtr, con->outIdx, inPtr, con->inIdx, con->getPathState());
            }
        }
    }

    patch.finishRemove(); // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();

    synchroniseSplitCanvas();
}

void Canvas::removeSelectedConnections()
{
    patch.startUndoSequence("Remove Connections");

    for (auto* con : connections) {
        if (con->isSelected()) {
            patch.removeConnection(con->outobj->getPointer(), con->outIdx, con->inobj->getPointer(), con->inIdx, con->getPathState());
        }
    }

    patch.endUndoSequence("Remove Connections");

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    synchroniseSplitCanvas();
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
        if (selectedBoxes.contains(connection->inobj.get()) && !selectedBoxes.contains(connection->outobj.get())) {
            auto* inlet = connection->inlet.get();
            targetEdges[inlet].add(connection->outlet.get());
            usedEdges.addIfNotAlreadyThere(inlet);
        }
    }
    for (auto* connection : connections) {
        if (selectedBoxes.contains(connection->outobj.get()) && !selectedBoxes.contains(connection->inobj.get())) {
            auto* outlet = connection->outlet.get();
            targetEdges[outlet].add(connection->inlet.get());
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
        auto pos = targetEdge->object->getObjectBounds().getPosition();
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
        if (auto* ptr = object->getPointer()) {
            bounds = bounds.getUnion(object->getBounds());
            patch.selectObject(ptr);
        }
    }
    auto centre = bounds.getCentre() - canvasOrigin;

    auto copypasta = String("#N canvas 733 172 450 300 0 1;\n") + "$$_COPY_HERE_$$" + newEdgeObjects + newInternalConnections + "#X restore " + String(centre.x) + " " + String(centre.y) + " pd;\n";

    // Apply the changed on Pd's thread
    pd->lockAudioThread();

    auto* patchPtr = patch.getPointer().get();
    if (!patchPtr)
        return;

    int size;
    char const* text = libpd_copy(patchPtr, &size);
    auto copied = String::fromUTF8(text, size);

    // Wrap it in an undo sequence, to allow undoing everything in 1 step
    patch.startUndoSequence("encapsulate");

    libpd_removeselection(patchPtr);

    auto replacement = copypasta.replace("$$_COPY_HERE_$$", copied);

    libpd_paste(patchPtr, replacement.toRawUTF8());
    auto* newObject = static_cast<t_object*>(patch.getObjects().back());

    for (auto& [idx, iolets] : newExternalConnections) {
        for (auto* iolet : iolets) {
            if(auto* externalObject = static_cast<t_object*>(iolet->object->getPointer())) {
                if (iolet->isInlet) {
                    libpd_createconnection(patchPtr, newObject, idx - numIn, externalObject, iolet->ioletIdx);
                } else {
                    libpd_createconnection(patchPtr, externalObject, iolet->ioletIdx, newObject, idx);
                }
            }
        }
    }

    patch.endUndoSequence("encapsulate");

    pd->unlockAudioThread();

    synchronise();
    handleUpdateNowIfNeeded();

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

    void* topObject = selection[0]->getY() > selection[1]->getY() ? selection[1]->getPointer() : selection[0]->getPointer();
    void* bottomObject = selection[0] == topObject ? selection[1]->getPointer() : selection[0]->getPointer();

    if(topObject && bottomObject) {
        patch.createConnection(topObject, 0, bottomObject, 0);
    }

    synchronise();

    return true;
}

void Canvas::cancelConnectionCreation()
{
    connectionsBeingCreated.clear();
    if (connectingWithDrag) {
        connectingWithDrag = false;
        connectionCancelled = true;
        if (nearestIolet) {
            nearestIolet->isTargeted = false;
            nearestIolet->repaint();
            nearestIolet = nullptr;
        }
    }
}

void Canvas::alignObjects(Align alignment)
{
    auto getBoundingBox = [this](Array<Object *> objects, Array<std::tuple<void*, Rectangle<int>>> &pdObjectAndDimensions) -> Rectangle<int> {
        auto patchPtr = patch.getPointer().get();
        auto totalBounds = Rectangle<int>();
        for (auto* object : objects){
            if(auto* ptr = object->getPointer()) {
                pdObjectAndDimensions.add(std::make_tuple(static_cast<void*>(ptr), object->getBounds()));
                totalBounds = totalBounds.getUnion(object->getBounds());
                libpd_undo_apply(patchPtr, &patch.checkObject(ptr)->te_g);
            }
        }
        return totalBounds;
    };

    auto objects = getSelectionOfType<Object>();

    if (objects.isEmpty())
        return;

    patch.startUndoSequence("align objects");

    // mark canvas as dirty
    canvas_dirty(patch.getPointer().get(), 1);

    // get the bounding box of all selected objects
    Array<std::tuple<void*, Rectangle<int>>> pdObjectAndDimensions;
    auto selectedBounds = getBoundingBox(objects, pdObjectAndDimensions);

    switch(alignment){
    case Align::Left:{
        auto leftPos = selectedBounds.getTopLeft().x;
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, leftPos, objectBounds.getY());
        }
        break;
    }
    case Align::Right:{
        auto rightPos = selectedBounds.getRight();
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, rightPos - objectBounds.getWidth(), objectBounds.getY());
        }
        break;
    }
    case Align::VCenter:{
        auto centrePos = selectedBounds.getCentreX();
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, centrePos - objectBounds.withZeroOrigin().getCentreX(), objectBounds.getY());
        }
        break;
    }
    case Align::Top:{
        auto topPos = selectedBounds.getTopLeft().y;
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, objectBounds.getX(), topPos);
        }
        break;
    }
    case Align::Bottom:{
        auto bottomPos = selectedBounds.getBottom();
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, objectBounds.getX(), bottomPos - objectBounds.getHeight());
        }
        break;
    }
    case Align::HCenter:{
        auto centerPos = selectedBounds.getCentreY();
        for (auto& [ pdObject, objectBounds ] : pdObjectAndDimensions) {
            patch.moveObjectTo(pdObject, objectBounds.getX(), centerPos - objectBounds.withZeroOrigin().getCentreY());
        }
        break;
    }
    default:
        break;
    }

    performSynchronise();
    
    for (auto* connection : connections){
        connection->forceUpdate();
    }
    
    patch.endUndoSequence("align objects");
}

void Canvas::undo()
{
    // Tell pd to undo the last action
    patch.undo();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();

    synchroniseSplitCanvas();
    updateSidebarSelection();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();

    synchroniseSplitCanvas();
    updateSidebarSelection();
}

void Canvas::valueChanged(Value& v)
{
    // Update zoom
    if (v.refersToSameSourceAs(zoomScale)) {

        auto newScaleFactor = getValue<float>(v);

        if (newScaleFactor == 0) {
            newScaleFactor = 1.0f;
            zoomScale = 1.0f;
        }

        hideSuggestions();

        if (!viewport)
            return;

        // Get floating point mouse position relative to screen
        auto mousePosition = Desktop::getInstance().getMainMouseSource().getScreenPosition();
        // Get mouse position relative to canvas
        auto oldPosition = getLocalPoint(nullptr, mousePosition);
        // Apply transform and make sure viewport bounds get updated
        setTransform(AffineTransform().scaled(newScaleFactor));
        // After zooming, get mouse position relative to canvas again
        auto newPosition = getLocalPoint(nullptr, mousePosition);
        // Calculate offset to keep our mouse position the same as before this zoom action
        auto offset = newPosition - oldPosition;
        setTopLeftPosition(getPosition() + offset.roundToInt());
        // This is needed to make sure the viewport the current canvas bounds to the lastVisibleArea variable
        // Without this, future calls to getViewPosition() will give wrong results
        viewport->resized();

        // set and trigger the zoom label popup in the bottom left corner
        // TODO: move this to viewport, and have one per viewport?
        editor->setZoomLabelLevel(newScaleFactor);
    } else if (v.refersToSameSourceAs(patchWidth)) {
        // limit canvas width to smallest object (11px)
        patchWidth = jmax(11, getValue<int>(patchWidth));
        patch.getPointer()->gl_screenx2 = getValue<int>(patchWidth) + patch.getPointer()->gl_screenx1;
        repaint();
    } else if (v.refersToSameSourceAs(patchHeight)) {
        patchHeight = jmax(11, getValue<int>(patchHeight));
        patch.getPointer()->gl_screeny2 = getValue<int>(patchHeight) + patch.getPointer()->gl_screeny1;
        repaint();
    }
    // When lock changes
    else if (v.refersToSameSourceAs(locked)) {
        bool editMode = !getValue<bool>(v);

        pd->lockAudioThread();
        patch.getPointer()->gl_edit = editMode;
        pd->unlockAudioThread();

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
        updateOverlays();
    } else if (v.refersToSameSourceAs(commandLocked)) {
        updateOverlays();
        repaint();
    }
    // Should only get called when the canvas isn't a real graph
    else if (v.refersToSameSourceAs(presentationMode)) {
        deselectAll();
        commandLocked.setValue(presentationMode.getValue());
    }
    else if (v.refersToSameSourceAs(hideNameAndArgs)) {
        if (!patch.getPointer())
            return;
        
        int hideText = getValue<bool>(hideNameAndArgs);
        if (auto glist = patch.getPointer()) {
            hideText = glist->gl_isgraph && hideText;
            canvas_setgraph(glist.get(), glist->gl_isgraph + 2 * hideText, 0);
        }
        
        hideNameAndArgs = hideText;
    }
    else if (v.refersToSameSourceAs(isGraphChild)) {

        if (!patch.getPointer())
            return;

        int graphChild = getValue<bool>(isGraphChild);
        
        if (auto glist = patch.getPointer()) {
            canvas_setgraph(glist.get(), graphChild + 2 * (graphChild && glist->gl_hidetext), 0);
        }
        
        if(!graphChild)
        {
            hideNameAndArgs = false;
        }

        if (graphChild && !isGraph) {
            graphArea = std::make_unique<GraphArea>(this);
            addAndMakeVisible(*graphArea);
            graphArea->setAlwaysOnTop(true);
            graphArea->updateBounds();
        } else {
            graphArea.reset(nullptr);
        }

        updateOverlays();
        repaint();
    } else if (v.refersToSameSourceAs(xRange)) {
        if (auto glist = patch.getPointer()) {
            glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
            glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
        }
        updateDrawables();
    } else if (v.refersToSameSourceAs(yRange)) {
        if (auto glist = patch.getPointer()) {
            glist->gl_y2 = static_cast<float>(yRange.getValue().getArray()->getReference(0));
            glist->gl_y1 = static_cast<float>(yRange.getValue().getArray()->getReference(1));
        }
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
    if (!shouldNowBeSelected) {
        selectedComponents.deselect(component);
    } else {
        selectedComponents.addToSelection(component);
    }

    if (updateCommandStatus) {
        editor->updateCommandStatus();
    }
}

SelectedItemSet<WeakReference<Component>>& Canvas::getLassoSelection()
{
    return selectedComponents;
}

bool Canvas::checkPanDragMode()
{
    auto panDragEnabled = panningModifierDown();
    setPanDragMode(panDragEnabled);

    return panDragEnabled;
}

bool Canvas::setPanDragMode(bool shouldPan)
{
    if (auto* v = dynamic_cast<CanvasViewport*>(viewport.get())) {
        v->enableMousePanning(shouldPan);
        return true;
    }
    return false;
}

void Canvas::findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area)
{
    for (auto* object : objects) {
        if (area.intersects(object->getSelectableBounds())) {
            itemsFound.add(object);
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
        if (con->intersects(lasso.getBounds().toFloat())) {
            itemsFound.add(con);
        } else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
            setSelected(con, false, false);
        }
    }
}

ObjectParameters& Canvas::getInspectorParameters()
{
    return parameters;
}

bool Canvas::panningModifierDown()
{
    return KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey) || ModifierKeys::getCurrentModifiersRealtime().isMiddleButtonDown();
}

void Canvas::receiveMessage(String const& symbol, int argc, t_atom* argv)
{
    auto atoms = pd::Atom::fromAtoms(argc, argv);
    switch (hash(symbol)) {
    case hash("obj"):
    case hash("msg"):
    case hash("floatatom"):
    case hash("listbox"):
    case hash("symbolatom"):
    case hash("text"):
    case hash("graph"):
    case hash("scalar"):
    case hash("bng"):
    case hash("toggle"):
    case hash("vslider"):
    case hash("hslider"):
    case hash("hdial"):
    case hash("vdial"):
    case hash("hradio"):
    case hash("vradio"):
    case hash("vumeter"):
    case hash("mycnv"):
    case hash("numbox"):
    case hash("connect"):
    case hash("clear"):
    case hash("cut"):
    case hash("disconnect"): {
        // This will trigger an asyncupdater, so it's thread-safe to do this here
        synchronise();
        break;
    }
    case hash("editmode"): {
        if (atoms.size() >= 1) {
            int flag = atoms[0].getFloat();
            if (flag % 2 == 0) {
                locked = true;
                presentationMode = false;
            } else {
                locked = false;
                presentationMode = false;
            }
        }
        break;
    }
    case hash("coords"):
    case hash("donecanvasdialog"): {
        if (auto* cnv = editor->getCurrentCanvas()) {
            cnv->synchronise();
            cnv->synchroniseSplitCanvas();
        }
        break;
    }
    }
}
