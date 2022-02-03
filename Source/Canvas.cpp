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
Canvas::Canvas(PlugDataPluginEditor& parent, pd::Patch patch, bool graph, bool graphChild)
    : MultiComponentDragger<Box>(this, &boxes)
    , main(parent)
    , pd(&parent.pd)
    , patch(patch)
    , suggestor(parent.resources.get())
{
    isGraph = graph;
    isGraphChild = graphChild;
    
    parent.sendChangeMessage();

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
    
    addChildComponent(suggestor);
    
    synchronise();
    
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
    setTransform(main.transform);

    // TODO: do we need to do this here?
    main.inspector.deselect();
    main.inspector.setVisible(false);
    main.console->setVisible(true);

    pd->waitForStateUpdate();
    deselectAll();
    

    patch.setCurrent();
    patch.updateExtraInfo();

    //connections.clear();
    
    auto objects = patch.getObjects();
    auto isObjectDeprecated = [&](pd::Object* obj) {
        for (auto& pdobj : objects) {
            if (pdobj == *obj) {
                return false;
            }
        }
        return true;
    };
    
    if(!isGraph) {
        // Remove deprecated connections
        for (int n = connections.size() - 1; n >= 0; n--) {
            auto connection = connections[n];
            
            if(isObjectDeprecated(connection->start->box->pdObject.get()) || isObjectDeprecated(connection->end->box->pdObject.get())) {
                connections.remove(n);
            }
            else
            {
                auto* start = (t_text*)connection->start->box->pdObject->getPointer();
                auto* end = (t_text*)connection->end->box->pdObject->getPointer();
                
                if(!canvas_isconnected(patch.getPointer(), start, connection->outIdx, end, connection->inIdx)) {
                    connections.remove(n);
                }
            }
        }
    }

    // Clear deleted boxes
    for (int n = boxes.size() - 1; n >= 0; n--) {
        auto* box = boxes[n];
        if (box->pdObject && isObjectDeprecated(box->pdObject.get())) {
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

            auto type = pd::Gui::getType(object.getPointer());
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
            newBox->toFront(false);
            
            if(newBox->graphics && newBox->graphics->label) newBox->graphics->label->toFront(false);

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
                if(box->graphics && (!box->graphics->fakeGUI() || box->graphics->getGUI().getType() == pd::Type::Comment)) y -= 22;
                box->setTopLeftPosition(x, y);
            }

            box->toFront(false);
            if(box->graphics && box->graphics->label) box->graphics->label->toFront(false);

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

    auto* x = patch.getPointer();

    auto pd_connections = patch.getConnections();
    
    if(!isGraph) {
        
        for(auto& connection : pd_connections) {
            
            auto& [inno, inobj, outno, outobj] = connection;
            
            int srcno = patch.getIndex(&inobj->te_g);
            int sinkno = patch.getIndex(&outobj->te_g);
            
            auto& srcEdges = boxes[srcno]->edges;
            auto& sinkEdges = boxes[sinkno]->edges;
            
            // TEMP: remove when we're sure this works
            if(srcno >= boxes.size() || sinkno >= boxes.size() || outno >= srcEdges.size() || inno >= sinkEdges.size()) {
                jassertfalse;
            }
            
            auto it = std::find_if(connections.begin(), connections.end(), [this, &connection, &srcno, &sinkno](Connection* c) {
            
                auto& [inno, inobj, outno, outobj] = connection;
                
                if(!c->start || !c->end) return false;
                
                bool sameStart = c->start->box == boxes[srcno];
                bool sameEnd = c->end->box == boxes[sinkno];

                return c->inIdx == inno && c->outIdx == outno && sameStart && sameEnd;

            });
            
            if (it == connections.end()) {
                connections.add(new Connection(this, srcEdges[boxes[srcno]->numInputs + outno], sinkEdges[inno], true));
            }
            else {
                auto& connection = *(*it);
                
                auto currentID = connection.getID();
                if(connection.lastID.isNotEmpty() && connection.lastID != currentID) {
                    patch.setExtraInfoID(connection.lastID, currentID);
                    connection.lastID = currentID;
                }
                
                auto info = patch.getExtraInfo(currentID);
                if(info.getSize()) connection.setState(info);
                connection.repaint();
            }
            
        }
    }

    //patch.deselectAll();

    // Resize canvas to fit objects
    checkBounds();
}



void Canvas::mouseDown(const MouseEvent& e)
{
   
    if(suggestor.openedEditor) {
        suggestor.currentBox->textLabel.hideEditor(false);
        return;
    }
    
    // Ignore if locked
    if (pd->locked)
        return;

    auto* source = e.originalComponent;

    // Select parent box when clicking on graphs
    if (isGraph) {
        auto* box = findParentComponentOfClass<Box>();
        box->cnv->setSelected(box, true);
        return;
    }

    // Left-click
    if (!ModifierKeys::getCurrentModifiers().isRightButtonDown()) {

        dragStartPosition = e.getMouseDownPosition();

        // Connecting objects by dragging
        if (source == this || source == graphArea.get()) {
            if(Edge::connectingEdge) {
                Edge::connectingEdge = nullptr;
                repaint();
            }
            
            lasso.beginLasso(e.getEventRelativeTo(this), this);


                if(!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown()) {
                    deselectAll();
                }
        }

    // Right click
    } else {
        // Info about selection status
        auto& lassoSelection = getLassoSelection();
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
                auto* newCanvas = main.canvases.add(new Canvas(main, *subpatch, false, isGraphChild));
                newCanvas->title = lassoSelection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false);

                
                auto [x, y, w, h] = subpatch->getBounds();

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
                    main.console->logMessage("Couldn't find help file");
                    return;
                }

                auto* ownedProcessor = new PlugDataAudioProcessor(pd->console);

                auto* lock = pd->getCallbackLock();

                lock->enter();
                    
                ownedProcessor->loadPatch(File(helpName));
                    
                
                auto* new_cnv = main.canvases.add(new Canvas(main, ownedProcessor->getPatch()));
                new_cnv->aux_instance.reset(ownedProcessor); // Help files need their own instance
                new_cnv->pd = ownedProcessor;
                new_cnv->title = String(helpName).fromLastOccurrenceOf("/", false, false);
                
                ownedProcessor->prepareToPlay(pd->getSampleRate(), pd->AudioProcessor::getBlockSize());
                main.addTab(new_cnv);

                lock->exit();

                break;
            }
        };
        // Open popupmenu with different positions for these origins
        if (auto* box = dynamic_cast<ClickLabel*>(e.originalComponent)) {
            if (!box->getCurrentTextEditor()) {
                popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(box).withParentComponent(&main), ModalCallbackFunction::create(callback));
            }
        } else if (auto* box = dynamic_cast<Box*>(e.originalComponent)) {
            if (!box->textLabel.getCurrentTextEditor()) {
                popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel).withParentComponent(&main), ModalCallbackFunction::create(callback));
            }
        } else if (auto* gui = dynamic_cast<GUIComponent*>(e.originalComponent)) {
            auto* box = gui->box;
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel).withParentComponent(&main), ModalCallbackFunction::create(callback));
        } else if (auto* gui = dynamic_cast<Canvas*>(e.originalComponent)) {
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetScreenArea({ e.getScreenX(), e.getScreenY(), 10, 10 }).withParentComponent(&main), ModalCallbackFunction::create(callback));
        }
    }
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    
    
    // Ignore on graphs or when locked
    if (isGraph || pd->locked)
        return;

    auto* source = e.originalComponent;
    
    //if(source != this) repaint();

    // Drag lasso
    if (dynamic_cast<Connection*>(source)) {
        lasso.dragLasso(e.getEventRelativeTo(this));
    } else if (source == this) {
        Edge::connectingEdge = nullptr;
        lasso.dragLasso(e);
        
        if(e.getDistanceFromDragStart() < 5) return;

        for (auto& con : connections) {
            bool intersect = false;
            for (float i = 0; i < 1; i += 0.005) {
                auto point = con->toDraw.getPointAlongPath(i * con->toDraw.getLength());
                
                if(!point.isFinite()) continue;
                
                if (lasso.getBounds().contains(point.toInt() + con->getPosition())) {
                    intersect = true;
                }
            }

            if (!con->isSelected && intersect) {
                con->isSelected = true;
                con->repaint();
            } else if (con->isSelected && !intersect && !ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown()) {
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
    
    
    if(auto* box = dynamic_cast<Box*>(e.originalComponent->getParentComponent())) {
        
        if(!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown() && e.getDistanceFromDragStart() < 2) {
            
            deselectAll();
        }
        
        if(!isGraph && box->getParentComponent() == this) {
            setSelected(box, true);
        }
    }
    
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
        
        repaint();
    }

    auto& lassoSelection = getLassoSelection();

    // Pass parameters of selected box to inspector
    if (lassoSelection.getNumSelected() == 1) {
        auto* box = lassoSelection.getSelectedItem(0);
        if (box->graphics) {
            main.inspector.loadData(box->graphics->getParameters());
            main.inspector.setVisible(true);
            main.console->setVisible(false);
        }
        else {
            main.inspector.deselect();
            main.inspector.setVisible(false);
            main.console->setVisible(true);
        }
    }
    else {
        main.inspector.deselect();
        main.inspector.setVisible(false);
        main.console->setVisible(true);
    }

    lasso.endLasso();
}

void Canvas::dragCallback(int dx, int dy)
{
    // Ignore when locked
    if (pd->locked)
        return;

    auto objects = std::vector<pd::Object*>();

    for (auto* box : getLassoSelection()) {
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
    
    for(auto& box : boxes) {
        if(!box->pdObject) continue;
        
        auto* gobj = (t_gobj*)box->pdObject.get()->getPointer();
        
        // Recurse for graphs
        if (gobj->g_pd == canvas_class) {
            if (box->graphics && box->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
                auto* canvas = box->graphics->getCanvas();
                
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
    
    if(Edge::connectingEdge) {
        repaint();
    }
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

    
    return false;
}

void Canvas::deselectAll()
{
    // Deselects boxes
    MultiComponentDragger::deselectAll();
    
    // Deselect connections
    for(auto& connection : connections) {
        if(connection->isSelected) {
            connection->isSelected = false;
            connection->repaint();
        }
    }
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection()) {
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
    
    for(auto* box : boxes) {
        if(glist_isselected(patch.getPointer(), (t_gobj*)box->pdObject->getPointer())) {
            setSelected(box, true);
        }
    }
    
    patch.deselectAll();
}

void Canvas::duplicateSelection()
{

    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection()) {
        if (auto* box = dynamic_cast<Box*>(sel)) {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to duplicate
    patch.duplicate();
    
    // Load state from pd, don't update positions
    synchronise(false);
    
    // Select the newly duplicated objects
    for(auto* box : boxes) {
        if(glist_isselected(patch.getPointer(), (t_gobj*)box->pdObject->getPointer())) {
            setSelected(box, true);
        }
    }
    
    patch.deselectAll();
}

void Canvas::removeSelection()
{
    // Make sure object isn't selected and stop updating gui
    main.inspector.deselect();
    main.inspector.setVisible(false);
    main.console->setVisible(true);
    //main.stopTimer();
    

    // Find selected objects and make them selected in pd
    Array<pd::Object*> objects;
    for (auto* sel : getLassoSelection()) {
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
    
    patch.finishRemove();  // Makes sure that the extra removed connections will be grouped in the same undo action


    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);
    
    patch.deselectAll();

    // Update GUI
    main.updateValues();
    main.updateUndoState();
}

void Canvas::undo()
{
    // Tell pd to undo the last action
    patch.undo();
    
    // Load state from pd
    synchronise();
    
    patch.deselectAll();
    
    main.updateUndoState();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();
    
    // Load state from pd
    synchronise();
    
    patch.deselectAll();
    main.updateUndoState();
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
