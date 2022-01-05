/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <m_pd.h>
#include <m_imp.h>


#include "Box.h"
#include "Canvas.h"
#include "Connection.h"
#include "PluginProcessor.h"

//==============================================================================
Canvas::Canvas(PlugDataPluginEditor& parent, bool graph, bool graphChild)
    : main(parent)
    , pd(&parent.pd)
{
    isGraph = graph;
    isGraphChild = graphChild;

    tabbar = &parent.getTabbar();

    // Add draggable border for setting graph position
    if (isGraphChild) {
        graphArea.reset(new GraphArea(this));
        addAndMakeVisible(graphArea.get());
    }

    setSize(600, 400);

    // Apply zooming
    setTransform(parent.transform);

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha((float)0.3));

    addKeyListener(this);

    setWantsKeyboardFocus(true);

    if (!isGraph) {
        viewport = new Viewport; // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);
    }

    main.startTimer(guiUpdateMs);
    
}

Canvas::~Canvas()
{
    popupMenu.setLookAndFeel(nullptr);
    Component::removeAllChildren();
    removeKeyListener(this);
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::synchronise(bool updatePosition)
{
    main.stopTimer();
    setTransform(main.transform);

    main.inspector.deselect();

    pd->waitForStateUpdate();
    dragger.deselectAll();

    patch.setCurrent();

    connections.clear();

    auto objects = patch.getObjects();

    auto isObjectDeprecated = [&](pd::Object* obj) {
        for (auto& pdobj : objects) {
            if (pdobj == *obj) {
                return false;
            }
        }
        return true;
    };

    // Clear deleted boxes
    for (int n = boxes.size() - 1; n >= 0; n--) {
        auto* box = boxes[n];
        if (isObjectDeprecated(box->pdObject.get())) {
            boxes.remove(n);
        }
    }

    for (auto& object : objects) {

        auto it = std::find_if(boxes.begin(), boxes.end(), [&object](Box* b) {
            return b->pdObject.get() != nullptr && *b->pdObject.get() == object;
        });

        if (it == boxes.end()) {
            auto [x, y, w, h] = object.getBounds();
            auto name = String(object.getText());

            auto type = pd::Gui::getType(object.getPointer(), name.toStdString());
            auto isGui = type != pd::Type::Undefined;
            auto* pdObject = isGui ? new pd::Gui(object.getPointer(), &patch, pd) : new pd::Object(object);

            if (type == pd::Type::Message)
                name = "msg";
            else if (type == pd::Type::AtomNumber)
                name = "floatatom";
            else if (type == pd::Type::AtomSymbol)
                name = "symbolatom";

            // Some of these GUI objects have a lot of extra symbols that we don't want to show
            auto guiSimplify = [](String& target, const StringArray selectors) {
                for (auto& str : selectors) {
                    if (target.startsWith(str)) {
                        target = str;
                        return;
                    }
                }
            };
            
            x += zeroPosition.x;
            y += zeroPosition.y;

            // These objects have extra info (like size and colours) in their names that we want to hide
            guiSimplify(name, {"bng", "tgl", "nbx", "hsl", "vsl", "hradio", "vradio", "pad", "cnv"});

            auto* newBox = boxes.add(new Box(pdObject, this, name, { (int)x, (int)y }));
            newBox->toBack();

            // Don't show non-patchable (internal) objects
            if (!patch.checkObject(&object))
                newBox->setVisible(false);
        } else {
            auto* box = *it;
            auto [x, y, h, w] = object.getBounds();
            
            x += zeroPosition.x;
            y += zeroPosition.y;

            // Only update positions if we need to and there is a significant difference
            // There may be rounding errors when scaling the gui, this makes the experience smoother
            if (updatePosition && box->getPosition().getDistanceFrom(Point<int>(x, y)) > 8) {
                box->setTopLeftPosition(x, y);
            }

            box->toBack();

            // Reload colour information for
            if (box->graphics) {
                box->graphics->initParameters();
            }

            // Don't show non-patchable (internal) objects
            if (!patch.checkObject(&object))
                box->setVisible(false);
        }
    }

    // Make sure objects have the same order
    std::sort(boxes.begin(), boxes.end(), [&objects](Box* first, Box* second) mutable {
        size_t idx1 = std::find(objects.begin(), objects.end(), *first->pdObject.get()) - objects.begin();
        size_t idx2 = std::find(objects.begin(), objects.end(), *second->pdObject.get()) - objects.begin();

        return idx1 < idx2;
    });

    t_linetraverser t;
    t_outconnect* oc;

    auto* x = patch.getPointer();

    // Get connections from pd
    linetraverser_start(&t, x);
    while ((oc = linetraverser_next(&t))) {
        int srcno = canvas_getindex(x, &t.tr_ob->ob_g);
        int sinkno = canvas_getindex(x, &t.tr_ob2->ob_g);

        auto& srcEdges = boxes[srcno]->edges;
        auto& sinkEdges = boxes[sinkno]->edges;

        if (srcno < boxes.size() && sinkno < boxes.size()) {
            connections.add(new Connection(this, srcEdges[boxes[srcno]->numInputs + t.tr_outno], sinkEdges[t.tr_inno], true));
        }
    }

    patch.deselectAll();

    // Resize canvas to fit objects
    checkBounds();

    main.startTimer(guiUpdateMs);
}

void Canvas::createPatch()
{
    // Create the pd save file
    auto tempPatch = File::createTempFile(".pd");
    tempPatch.replaceWithText(main.defaultPatch);

    auto* cs = pd->getCallbackLock();
    // Load the patch into libpd
    pd->openPatch(tempPatch.getParentDirectory().getFullPathName().toStdString(), tempPatch.getFileName().toStdString());

    patch = pd->getPatch();

    synchronise();
}

void Canvas::loadPatch(pd::Patch patch)
{
    this->patch = patch;
    synchronise();
}


void Canvas::mouseDown(const MouseEvent& e)
{
   
    
    // Ignore if locked
    if (pd->locked)
        return;

    auto* source = e.originalComponent;

    // Select parent box when clicking on graphs
    if (isGraph) {
        auto* box = findParentComponentOfClass<Box>();
        box->dragger.setSelected(box, true);
        return;
    }

    // Left-click
    if (!ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
        main.inspector.deselect();
        if(source == this) dragger.deselectAll();

        dragStartPosition = e.getMouseDownPosition();

        // Drag lasso
        if (dynamic_cast<Connection*>(source)) {
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);
        }
        // Connecting objects by dragging
        if (source == this || source == graphArea.get()) {
            Edge::connectingEdge = nullptr;
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);

            for (auto& con : connections) {
                con->isSelected = false;
                con->repaint();
            }
        }

    // Right click
    } else {
        // Info about selection status
        auto& lassoSelection = dragger.getLassoSelection();
        bool hasSelection = lassoSelection.getNumSelected();
        bool multiple = lassoSelection.getNumSelected() > 1;

        bool isSubpatch = hasSelection && (lassoSelection.getSelectedItem(0)->graphics && (lassoSelection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Type::GraphOnParent || lassoSelection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Type::Subpatch));

        // Create popup menu
        popupMenu.clear();
        popupMenu.addItem(1, "Open", !multiple && isSubpatch); // for opening subpatches
        popupMenu.addSeparator();
        popupMenu.addItem(4, "Cut", hasSelection);
        popupMenu.addItem(5, "Copy", hasSelection);
        popupMenu.addItem(6, "Duplicate", hasSelection);
        popupMenu.addItem(7, "Delete", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(8, "To Front", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(9, "Help", hasSelection); // Experimental: opening help files
        popupMenu.setLookAndFeel(&getLookAndFeel());

        auto callback = [this, &lassoSelection](int result) {
            if (result < 1)
                return;

            switch (result) {
            case 1: { // Open subpatch
                auto* subpatch = lassoSelection.getSelectedItem(0)->graphics.get()->getPatch();

                for (int n = 0; n < tabbar->getNumTabs(); n++) {
                    auto* tabCanvas = main.getCanvas(n);
                    if (tabCanvas->patch == *subpatch) {
                        tabbar->setCurrentTabIndex(n);
                        return;
                    }
                }
                bool isGraphChild = lassoSelection.getSelectedItem(0)->graphics.get()->getGUI().getType() == pd::Type::GraphOnParent;
                auto* newCanvas = new Canvas(main, false, isGraphChild);
                newCanvas->title = lassoSelection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false);
                auto patchCopy = *subpatch;
                newCanvas->loadPatch(patchCopy);

                auto [x, y, w, h] = patchCopy.getBounds();

                if (isGraphChild) {
                    newCanvas->graphArea->setBounds(x, y, std::max(w, 60), std::max(h, 60));
                }

                main.addTab(newCanvas);
                newCanvas->checkBounds();
                break;
            }
            case 4: // Cut
                copySelection();
                removeSelection();
                break;
            case 5: // Copy
                copySelection();
                break;
            case 6: { // Duplicate
                duplicateSelection();
                break;
            }
            case 7: // Remove
                removeSelection();
                break;

            case 8: // To Front
                lassoSelection.getSelectedItem(0)->toFront(false);
                break;

            case 9: // Open help

                // Find name of help file
                std::string helpName = lassoSelection.getSelectedItem(0)->pdObject->getHelp();

                if (!helpName.length()) {
                    main.console->logMessage(helpName);
                    return;
                }

                auto* ownedProcessor = new PlugDataAudioProcessor(pd->console);

                auto* lock = pd->getCallbackLock();

                lock->enter();

                auto* new_cnv = main.canvases.add(new Canvas(main));
                new_cnv->aux_instance.reset(ownedProcessor); // Help files need their own instance
                new_cnv->pd = ownedProcessor;

                ownedProcessor->loadPatch(helpName);
                new_cnv->loadPatch(ownedProcessor->getPatch());

                ownedProcessor->prepareToPlay(pd->getSampleRate(), pd->AudioProcessor::getBlockSize());
                main.addTab(new_cnv);

                lock->exit();

                break;
            }
        };
        // Open popupmenu with different positions for these origins
        if (auto* box = dynamic_cast<ClickLabel*>(e.originalComponent)) {
            if (!box->getCurrentTextEditor()) {
                popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(box), ModalCallbackFunction::create(callback));
            }
        } else if (auto* box = dynamic_cast<Box*>(e.originalComponent)) {
            if (!box->textLabel.getCurrentTextEditor()) {
                popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
            }
        } else if (auto* gui = dynamic_cast<GUIComponent*>(e.originalComponent)) {
            auto* box = gui->box;
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
        } else if (auto* gui = dynamic_cast<Canvas*>(e.originalComponent)) {
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetScreenArea({ e.getScreenX(), e.getScreenY(), 10, 10 }), ModalCallbackFunction::create(callback));
        }
    }
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    repaint();
    // Ignore on graphs or when locked
    if (isGraph || pd->locked)
        return;

    auto* source = e.originalComponent;

    // Drag lasso
    if (dynamic_cast<Connection*>(source)) {
        lasso.dragLasso(e.getEventRelativeTo(this));
    } else if (source == this) {
        Edge::connectingEdge = nullptr;
        lasso.dragLasso(e);

        for (auto& con : connections) {
            if (!con->start || !con->end) {
                connections.removeObject(con);
                continue;
            }

            Line<int> path(con->start->getCanvasBounds().getCentre(), con->end->getCanvasBounds().getCentre());

            bool intersect = false;
            for (float i = 0; i < 1; i += 0.005) {
                if (lasso.getBounds().contains(path.getPointAlongLineProportionally(i))) {
                    intersect = true;
                }
            }

            if (!con->isSelected && intersect) {
                con->isSelected = true;
                con->repaint();
            } else if (con->isSelected && !intersect) {
                con->isSelected = false;
                con->repaint();
            }
        }
    }

    if (connectingWithDrag)
        repaint();
}

void Canvas::mouseUp(const MouseEvent& e)
{
    // Releasing a connect by drag action
    if (connectingWithDrag) {
        auto pos = e.getEventRelativeTo(this).getPosition();

        // Find all edges
        Array<Edge*> allEdges;
        for (auto* box : boxes) {
            for (auto* edge : box->edges)
                allEdges.add(edge);
        };

        Edge* nearestEdge = nullptr;

        for (auto& edge : allEdges) {

            auto bounds = edge->getCanvasBounds().expanded(150, 150);
            if (bounds.contains(pos)) {
                if (!nearestEdge)
                    nearestEdge = edge;

                auto oldPos = nearestEdge->getCanvasBounds().getCentre();
                auto newPos = bounds.getCentre();
                nearestEdge = newPos.getDistanceFrom(pos) < oldPos.getDistanceFrom(pos) ? edge : nearestEdge;
            }
        }

        if (nearestEdge)
            nearestEdge->createConnection();

        Edge::connectingEdge = nullptr;
        connectingWithDrag = false;
    }

    auto& lassoSelection = dragger.getLassoSelection();

    // Pass parameters of selected box to inspector
    if (lassoSelection.getNumSelected() == 1) {
        auto* box = lassoSelection.getSelectedItem(0);
        if (box->graphics) {
            main.inspector.loadData(box->graphics->getParameters());
        }
    }

    lasso.endLasso();
}

void Canvas::dragCallback(int dx, int dy)
{
    // Ignore when locked
    if (pd->locked)
        return;

    auto objects = std::vector<pd::Object*>();

    for (auto* box : dragger.getLassoSelection()) {
        if (box->pdObject) {
            objects.push_back(box->pdObject.get());
        }
    }

    // When done dragging objects, update positions to pd
    patch.moveObjects(objects, dx, dy);

    // Check if canvas is large enough
    checkBounds();
    
    // Update undo state
    main.updateUndoState();
}

void Canvas::findDrawables(Graphics& g, t_canvas* cnv)
{
    
    // Find all drawables (from objects like drawpolygon, filledcurve, etc.)
    // Pd draws this over all siblings, even when drawn inside a graph!
    // To mimic this we find the drawables from the top-level canvas and paint it over everything
    
    int n = 0;
    for (auto* gobj = cnv->gl_list; gobj; gobj = gobj->g_next) {
        // Recurse for graphs
        if (gobj->g_pd == canvas_class) {
            if (boxes[n]->graphics && boxes[n]->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
                auto* canvas = boxes[n]->graphics->getCanvas();

                g.saveState();
                auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
                auto bounds = canvas->getParentComponent()->getLocalBounds().withPosition(pos);
                g.reduceClipRegion(bounds);

                canvas->findDrawables(g, static_cast<t_canvas*>((void*)gobj));

                g.restoreState();
            }
        }
        // Scalar found!
        if (gobj->g_pd == scalar_class) {
            t_scalar* x = (t_scalar*)gobj;
            t_template* templ = template_findbyname(x->sc_template);
            t_canvas* templatecanvas = template_findcanvas(templ);
            t_gobj* y;
            t_float basex, basey;
            scalar_getbasexy(x, &basex, &basey);

            if (!templatecanvas)
                return;

            for (y = templatecanvas->gl_list; y; y = y->g_next) {
                const t_parentwidgetbehavior* wb = pd_getparentwidget(&y->g_pd);
                if (!wb)
                    continue;
                
                // This function is a work-in-progress conversion from pd's drawing to JUCE drawing
                TemplateDraw::paintOnCanvas(g, this, x, y, basex, basey);
            }
        }
        n++;
    }
}

//==============================================================================
void Canvas::paintOverChildren(Graphics& g)
{

    // Pd Template drawing: not the most efficient implementation but it seems to work!
    // Graphs are drawn from their parent, so pd drawings are always on top of other objects
    if (!isGraph) {
        findDrawables(g, patch.getPointer());
    }

    // Draw connections in the making over everything else
    if (Edge::connectingEdge && Edge::connectingEdge->box->cnv == this) {
        Point<float> mousePos = getMouseXYRelative().toFloat();
        Point<int> edgePos = Edge::connectingEdge->getCanvasBounds().getPosition();

        edgePos += Point<int>(4, 4);

        Path path;
        path.startNewSubPath(edgePos.toFloat());
        path.lineTo(mousePos);

        g.setColour(Colours::grey);
        g.strokePath(path, PathStrokeType(3.0f));
    }
}

void Canvas::mouseMove(const MouseEvent& e)
{
    // For deciding where to place a new object
    lastMousePos = e.getPosition();
    repaint();
}

void Canvas::resized()
{
}

bool Canvas::keyPressed(const KeyPress& key, Component* originatingComponent)
{
    if (main.getCurrentCanvas() != this)
        return false;
    if (isGraph)
        return false;

    patch.keyPress(key.getKeyCode(), key.getModifiers().isShiftDown());

    // cmd-e
    if (key.getModifiers().isCommandDown() && key.isKeyCode(69)) {
        main.lockButton.triggerClick();
        return true;
    }

    // Zoom in
    if (key.isKeyCode(61) && key.getModifiers().isCommandDown()) {
        main.transform = main.transform.scaled(1.25f);
        setTransform(main.transform);
        return true;
    }
    // Zoom out
    if (key.isKeyCode(45) && key.getModifiers().isCommandDown()) {
        main.transform = main.transform.scaled(0.8f);
        setTransform(main.transform);
        return true;
    }

    if (pd->locked)
        return false;

    // Key shortcuts for creating objects
    if (key.getTextCharacter() == 'n') {
        boxes.add(new Box(this, "", lastMousePos));
        return true;
    }
    if (key.isKeyCode(65) && key.getModifiers().isCommandDown()) {
        for (auto* child : boxes) {
            dragger.setSelected(child, true);
        }
        for (auto* child : connections) {
            child->isSelected = true;
            child->repaint();
        }
        return true;
    }
    if (key.getTextCharacter() == 'b') {
        boxes.add(new Box(this, "bng", lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'm') {
        boxes.add(new Box(this, "msg", lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'i') {
        boxes.add(new Box(this, "nbx", lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'f') {
        boxes.add(new Box(this, "floatatom", lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 't') {
        boxes.add(new Box(this, "tgl", lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 's') {
        boxes.add(new Box(this, "vsl", lastMousePos));
        return true;
    }

    if (key.getKeyCode() == KeyPress::backspaceKey) {
        removeSelection();
        return true;
    }
    // cmd-c
    if (key.getModifiers().isCommandDown() && key.isKeyCode(67)) {
        copySelection();
        return true;
    }
    // cmd-v
    if (key.getModifiers().isCommandDown() && key.isKeyCode(86)) {
        pasteSelection();
        return true;
    }
    // cmd-x
    if (key.getModifiers().isCommandDown() && key.isKeyCode(88)) {
        copySelection();
        removeSelection();
        return true;
    }
    // cmd-d
    if (key.getModifiers().isCommandDown() && key.isKeyCode(68)) {
        duplicateSelection();
        return true;
    }

    // cmd-shift-z
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.isKeyCode(90)) {
        redo();
        return true;
    }
    // cmd-z
    if (key.getModifiers().isCommandDown() && key.isKeyCode(90)) {
        undo();
        return true;
    }

    return false;
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : dragger.getLassoSelection()) {
        if (auto* box = dynamic_cast<Box*>(sel)) {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to copy
    patch.copy();
    patch.deselectAll();
}

void Canvas::pasteSelection()
{
    // Tell pd to paste
    patch.paste();
    
    // Load state from pd, don't update positions
    synchronise(false);
}

void Canvas::duplicateSelection()
{

    // Tell pd to select all objects that are currently selected
    for (auto* sel : dragger.getLassoSelection()) {
        if (auto* box = dynamic_cast<Box*>(sel)) {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to duplicate
    patch.duplicate();
    patch.deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);
}

void Canvas::removeSelection()
{
    // Make sure object isn't selected and stop updating gui
    main.inspector.deselect();
    main.stopTimer();

    // Find selected objects and make them selected in pd
    Array<pd::Object*> objects;
    for (auto* sel : dragger.getLassoSelection()) {
        if (auto* box = dynamic_cast<Box*>(sel)) {
            if (box->pdObject) {
                patch.selectObject(box->pdObject.get());
                objects.add(box->pdObject.get());
            }
        }
    }

    // remove selection
    patch.removeSelection();

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto& con : connections) {
        if (con->isSelected) {
            if (!(objects.contains(con->outObj->get()) || objects.contains(con->inObj->get()))) {
                patch.removeConnection(con->outObj->get(), con->outIdx, con->inObj->get(), con->inIdx);
            }
        }
    }


    dragger.deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    // Restart gui updating
    main.startTimer(guiUpdateMs);
    main.updateUndoState();
}

void Canvas::undo()
{
    // Tell pd to undo the last action
    patch.undo();
    
    // Load state from pd
    synchronise();
    main.updateUndoState();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();
    
    // Load state from pd
    synchronise();
    main.updateUndoState();
}

// Called from subpatcher objects to close all tabs that refer to that subpatcher
void Canvas::closeAllInstances()
{
    if (!tabbar)
        return;

    for (int n = 0; n < tabbar->getNumTabs(); n++) {
        auto* cnv = main.getCanvas(n);
        if (cnv && cnv->patch == patch && &cnv->main == &main) {
            tabbar->removeTab(n);
            main.canvases.removeObject(cnv);
        }
    }

    if (tabbar->getNumTabs() > 1) {
        tabbar->getTabbedButtonBar().setVisible(true);
        tabbar->setTabBarDepth(30);
    } else {
        tabbar->getTabbedButtonBar().setVisible(false);
        tabbar->setTabBarDepth(1);
    }
}

void Canvas::checkBounds()
{
    int viewHeight = 0;
    int viewWidth = 0;
    
    if (viewport) {
        viewWidth = viewport->getWidth();
        viewHeight = viewport->getHeight();
    }

    // Check new bounds
    int minX = zeroPosition.x;
    int minY = zeroPosition.y;
    int maxX = std::max(getWidth() - minX, viewWidth);
    int maxY = std::max(getHeight() - minY, viewHeight);

    for (auto obj : boxes) {
        maxX = std::max<int>(maxX, (int)obj->getX() + obj->getWidth());
        maxY = std::max<int>(maxY, (int)obj->getY() + obj->getHeight());
        minX = std::min<int>(minX, (int)obj->getX());
        minY = std::min<int>(minY, (int)obj->getY());
    }

    if (!isGraph) {
        
        for(auto& box : boxes) {
            box->setBounds(box->getBounds().translated(-minX, -minY));
        }
        
        zeroPosition -= {minX, minY};
        
        setSize(maxX - minX, maxY - minY);
    }

    if (graphArea) {
        auto [x, y, w, h] = patch.getBounds();
        graphArea->setBounds(x, y, w, h);
    }
}
