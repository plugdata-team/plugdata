/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Canvas.h"

extern "C"
{
#include <m_pd.h>
#include <m_imp.h>
}

#include <memory>

#include "Box.h"
#include "Connection.h"
#include "PluginProcessor.h"

//==============================================================================
Canvas::Canvas(PlugDataPluginEditor& parent, const pd::Patch& patch, bool graph, bool graphChild) : MultiComponentDragger<Box>(this, &boxes), main(parent), pd(&parent.pd), patch(patch)
{
    isGraph = graph;
    isGraphChild = graphChild;

    parent.sendChangeMessage();

    tabbar = &parent.tabbar;

    // Add draggable border for setting graph position
    if (isGraphChild)
    {
        graphArea = std::make_unique<GraphArea>(this);
        addAndMakeVisible(graphArea.get());
    }

    setSize(600, 400);

    if (!isGraph)
    {
        // Apply zooming
        setTransform(parent.transform);
    }

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha(0.3f));

    addKeyListener(this);
    
    

    setWantsKeyboardFocus(true);

    if (!isGraph)
    {
        viewport = new Viewport;  // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);
        viewport->setBufferedToImage(true);
    }

    addChildComponent(suggestor);

    synchronise();
}

Canvas::~Canvas()
{
   
    Component::removeAllChildren();
    removeKeyListener(this);
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::synchronise(bool updatePosition)
{
    if (!isGraph)
    {
        setTransform(main.transform);
    }

    main.inspector.deselect();
    main.inspector.setVisible(false);
    main.console->setVisible(true);

    pd->waitForStateUpdate();
    deselectAll();

    patch.setCurrent(true);
    patch.updateExtraInfo();

    auto objects = patch.getObjects();
    auto isObjectDeprecated = [&](pd::Object* obj)
    {
        // NOTE: replace with std::ranges::all_of when available
        for (auto& pdobj : objects)
        {
            if (pdobj == *obj)
            {
                return false;
            }
        }
        return true;
    };

    if (!isGraph)
    {
        // Remove deprecated connections
        for (int n = connections.size() - 1; n >= 0; n--)
        {
            auto connection = connections[n];

            if (isObjectDeprecated(connection->start->box->pdObject.get()) || isObjectDeprecated(connection->end->box->pdObject.get()))
            {
                connections.remove(n);
            }
            else
            {
                auto* start = static_cast<t_text*>(connection->start->box->pdObject->getPointer());
                auto* end = static_cast<t_text*>(connection->end->box->pdObject->getPointer());

                if (!canvas_isconnected(patch.getPointer(), start, connection->outIdx, end, connection->inIdx))
                {
                    connections.remove(n);
                }
            }
        }
    }

    // Clear deleted boxes
    for (int n = boxes.size() - 1; n >= 0; n--)
    {
        auto* box = boxes[n];
        if (box->pdObject && isObjectDeprecated(box->pdObject.get()))
        {
            boxes.remove(n);
        }
    }

    for (auto& object : objects)
    {
        auto it = std::find_if(boxes.begin(), boxes.end(), [&object](Box* b) { return b->pdObject && *b->pdObject == object; });

        if (it == boxes.end())
        {
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
            auto guiSimplify = [](String& target, const StringArray& selectors)
            {
                for (auto& str : selectors)
                {
                    if (target.startsWith(str))
                    {
                        target = str;
                        return;
                    }
                }
            };

            x += zeroPosition.x;
            y += zeroPosition.y;

            // These objects have extra info (like size and colours) in their names that we want to hide
            guiSimplify(name, {"bng", "tgl", "nbx", "hsl", "vsl", "hradio", "vradio", "pad", "cnv"});

            auto* newBox = boxes.add(new Box(pdObject, this, name, {static_cast<int>(x), static_cast<int>(y)}));
            newBox->toFront(false);

            if (newBox->graphics && newBox->graphics->label) newBox->graphics->label->toFront(false);

            // Don't show non-patchable (internal) objects
            if (!pd::Patch::checkObject(&object)) newBox->setVisible(false);
        }
        else
        {
            auto* box = *it;
            auto [x, y, w, h] = object.getBounds();

            x += zeroPosition.x;
            y += zeroPosition.y;

            // Only update positions if we need to and there is a significant difference
            // There may be rounding errors when scaling the gui, this makes the experience smoother
            if (updatePosition && box->getPosition().getDistanceFrom(Point<int>(x, y)) > 8)
            {
                box->setTopLeftPosition(x, y);
            }

            box->toFront(false);
            if (box->graphics && box->graphics->label) box->graphics->label->toFront(false);

            // Reload colour information for
            if (box->graphics)
            {
                box->graphics->initParameters();
            }

            // Don't show non-patchable (internal) objects
            if (!pd::Patch::checkObject(&object)) box->setVisible(false);
        }
    }

    // Make sure objects have the same order
    std::sort(boxes.begin(), boxes.end(),
              [&objects](Box* first, Box* second) mutable
              {
                  size_t idx1 = std::find(objects.begin(), objects.end(), *first->pdObject) - objects.begin();
                  size_t idx2 = std::find(objects.begin(), objects.end(), *second->pdObject) - objects.begin();

                  return idx1 < idx2;
              });

    auto pdConnections = patch.getConnections();

    if (!isGraph)
    {
        for (auto& connection : pdConnections)
        {
            auto& [inno, inobj, outno, outobj] = connection;

            int srcno = patch.getIndex(&inobj->te_g);
            int sinkno = patch.getIndex(&outobj->te_g);

            auto& srcEdges = boxes[srcno]->edges;
            auto& sinkEdges = boxes[sinkno]->edges;

            // TEMP: remove when we're sure this works
            if (srcno >= boxes.size() || sinkno >= boxes.size() || outno >= srcEdges.size() || inno >= sinkEdges.size())
            {
                pd->console.logError("Error: impossible connection");
                continue;
            }

            auto it = std::find_if(connections.begin(), connections.end(),
                                   [this, &connection, &srcno, &sinkno](Connection* c)
                                   {
                                       auto& [inno, inobj, outno, outobj] = connection;

                                       if (!c->start || !c->end) return false;

                                       bool sameStart = c->start->box == boxes[srcno];
                                       bool sameEnd = c->end->box == boxes[sinkno];

                                       return c->inIdx == inno && c->outIdx == outno && sameStart && sameEnd;
                                   });

            if (it == connections.end())
            {
                connections.add(new Connection(this, srcEdges[boxes[srcno]->numInputs + outno], sinkEdges[inno], true));
            }
            else
            {
                auto& c = *(*it);

                auto currentId = c.getId();
                if (c.lastId.isNotEmpty() && c.lastId != currentId)
                {
                    patch.setExtraInfoId(c.lastId, currentId);
                    c.lastId = currentId;
                }

                auto info = patch.getExtraInfo(currentId);
                if (info.getSize()) c.setState(info);
                c.repaint();
            }
        }
    }

    // patch.deselectAll();

    // Resize canvas to fit objects
    checkBounds();
}

void Canvas::mouseDown(const MouseEvent& e)
{
    if (suggestor.openedEditor && e.originalComponent != suggestor.openedEditor)
    {
        suggestor.currentBox->textLabel.hideEditor();
        return;
    }

    auto openSubpatch = [this](Box* parent)
    {
        if (!parent->graphics) return;

        auto* subpatch = parent->graphics->getPatch();

        for (int n = 0; n < tabbar->getNumTabs(); n++)
        {
            auto* tabCanvas = main.getCanvas(n);
            if (tabCanvas->patch == *subpatch)
            {
                tabbar->setCurrentTabIndex(n);
                return;
            }
        }
        bool isGraphChild = parent->graphics->getGui().getType() == pd::Type::GraphOnParent;
        auto* newCanvas = main.canvases.add(new Canvas(main, *subpatch, false, isGraphChild));

        auto [x, y, w, h] = subpatch->getBounds();

        if (isGraphChild)
        {
            newCanvas->graphArea->setBounds(x, y, std::max(w, 60), std::max(h, 60));
        }

        main.addTab(newCanvas);
        newCanvas->checkBounds();
    };

    auto* source = e.originalComponent;

    // Ignore if locked
    if (pd->locked)
    {
        if (!ModifierKeys::getCurrentModifiers().isRightButtonDown())
        {
            if (auto* label = dynamic_cast<ClickLabel*>(source))
            {
                auto* box = static_cast<Box*>(label->getParentComponent());

                if (box->graphics && box->graphics->getGui().getType() == pd::Type::Subpatch)
                {
                    openSubpatch(box);
                }
            }
        }
        return;
    }

    // Select parent box when clicking on graphs
    if (isGraph)
    {
        auto* box = findParentComponentOfClass<Box>();
        box->cnv->setSelected(box, true);
        return;
    }

    // Left-click
    if (!ModifierKeys::getCurrentModifiers().isRightButtonDown())
    {
        if (pd->locked)
        {
            if (auto* label = dynamic_cast<ClickLabel*>(source))
            {
                auto* box = static_cast<Box*>(label->getParentComponent());
                openSubpatch(box);
                return;
            }
        }
        // Connecting objects by dragging
        if (source == this || source == graphArea.get())
        {
            if (connectingEdge)
            {
                connectingEdge = nullptr;
                repaint();
            }

            lasso.beginLasso(e.getEventRelativeTo(this), this);
            if (!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown())
            {
                deselectAll();
            }
        }
    }
    // Right click
    else
    {
        // Info about selection status
        auto& lassoSelection = getLassoSelection();
        bool hasSelection = lassoSelection.getNumSelected();
        bool multiple = lassoSelection.getNumSelected() > 1;

        bool isSubpatch = hasSelection && (lassoSelection.getSelectedItem(0)->graphics && (lassoSelection.getSelectedItem(0)->graphics->getGui().getType() == pd::Type::GraphOnParent || lassoSelection.getSelectedItem(0)->graphics->getGui().getType() == pd::Type::Subpatch));

        bool isGui = hasSelection && !multiple && lassoSelection.getSelectedItem(0)->graphics && !lassoSelection.getSelectedItem(0)->graphics->fakeGui();

        // Create popup menu
        popupMenu.clear();
        popupMenu.addItem(1, "Open", hasSelection && !multiple && isSubpatch);  // for opening subpatches
        // popupMenu.addItem(10, "Edit", isGui);
        popupMenu.addSeparator();
        popupMenu.addItem(4, "Cut", hasSelection);
        popupMenu.addItem(5, "Copy", hasSelection);
        popupMenu.addItem(6, "Duplicate", hasSelection);
        popupMenu.addItem(7, "Delete", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(8, "To Front", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(9, "Help", hasSelection);  // Experimental: opening help files

        auto callback = [this, &lassoSelection, openSubpatch](int result)
        {
            if (result < 1) return;

            switch (result)
            {
                case 1:
                {
                    openSubpatch(lassoSelection.getSelectedItem(0));
                    break;
                }
                case 4:  // Cut
                    copySelection();
                    removeSelection();
                    break;
                case 5:  // Copy
                    copySelection();
                    break;
                case 6:
                {  // Duplicate
                    duplicateSelection();
                    break;
                }
                case 7:  // Remove
                    removeSelection();
                    break;

                case 8:  // To Front
                    lassoSelection.getSelectedItem(0)->toFront(false);
                    break;

                case 9:
                {  // Open help

                    // Find name of help file
                    auto helpPatch = lassoSelection.getSelectedItem(0)->pdObject->getHelp();

                    if (!helpPatch.getPointer())
                    {
                        main.console->logMessage("Couldn't find help file");
                        return;
                    }

                    auto* newCnv = main.canvases.add(new Canvas(main, helpPatch));
                    main.addTab(newCnv, true);

                    // lock->exit();

                    break;
                }

                default:
                    break;
            }
        };

        auto showMenu = [this, callback](Component* target, Rectangle<int> bounds = {0, 0, 0, 0})
        {
            auto options = PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(&main);

            if (target)
            {
                options = options.withTargetComponent(target);
            }
            else
            {
                options = options.withTargetScreenArea(bounds);
            }

            popupMenu.showMenuAsync(options, ModalCallbackFunction::create(callback));
        };
        // Open popupmenu with different positions for these origins
        if (auto* box = dynamic_cast<ClickLabel*>(e.originalComponent))
        {
            if (!box->getCurrentTextEditor())
            {
                showMenu(box);
            }
        }
        else if (auto* box = dynamic_cast<Box*>(e.originalComponent))
        {
            if (!box->textLabel.getCurrentTextEditor())
            {
                showMenu(&box->textLabel);
            }
        }
        else if (auto* gui = dynamic_cast<GUIComponent*>(e.originalComponent))
        {
            auto* box = gui->box;
            showMenu(&box->textLabel);
        }
        else if (dynamic_cast<Canvas*>(e.originalComponent))
        {
            showMenu(nullptr, {e.getScreenX(), e.getScreenY(), 10, 10});
        }
    }
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    // Ignore on graphs or when locked
    if (isGraph || pd->locked) return;

    auto* source = e.originalComponent;

    // if(source != this) repaint();

    // Drag lasso
    if (dynamic_cast<Connection*>(source))
    {
        lasso.dragLasso(e.getEventRelativeTo(this));
    }
    else if (source == this)
    {
        connectingEdge = nullptr;
        lasso.dragLasso(e);

        if (e.getDistanceFromDragStart() < 5) return;

        for (auto& con : connections)
        {
            bool intersect = false;

            // Check intersection with path
            for (int i = 0; i < 200; i++)
            {
                float position = static_cast<float>(i) / 200.0f;
                auto point = con->toDraw.getPointAlongPath(position * con->toDraw.getLength());

                if (!point.isFinite()) continue;

                if (lasso.getBounds().contains(point.toInt() + con->getPosition()))
                {
                    intersect = true;
                }
            }

            if (!con->isSelected && intersect)
            {
                con->isSelected = true;
                con->repaint();
            }
            else if (con->isSelected && !intersect && !ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown())
            {
                con->isSelected = false;
                con->repaint();
            }
        }
    }

    if (connectingWithDrag) repaint();
}

void Canvas::mouseUp(const MouseEvent& e)
{
    if (auto* box = dynamic_cast<Box*>(e.originalComponent->getParentComponent()))
    {
        if (!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown() && e.getDistanceFromDragStart() < 2)
        {
            deselectAll();
        }

        if (!pd->locked && !isGraph && box->getParentComponent() == this)
        {
            setSelected(box, true);
        }
    }

    // Releasing a connect by drag action
    if (connectingWithDrag)
    {
        auto pos = e.getEventRelativeTo(this).getPosition();

        // Find all edges
        Array<Edge*> allEdges;
        for (auto* box : boxes)
        {
            for (auto* edge : box->edges)
            {
                allEdges.add(edge);
            }
        }

        Edge* nearestEdge = nullptr;

        for (auto& edge : allEdges)
        {
            auto bounds = edge->getCanvasBounds().expanded(150, 150);
            if (bounds.contains(pos))
            {
                if (!nearestEdge) nearestEdge = edge;

                auto oldPos = nearestEdge->getCanvasBounds().getCentre();
                auto newPos = bounds.getCentre();
                nearestEdge = newPos.getDistanceFrom(pos) < oldPos.getDistanceFrom(pos) ? edge : nearestEdge;
            }
        }

        if (nearestEdge) nearestEdge->createConnection();

        connectingEdge = nullptr;
        connectingWithDrag = false;

        repaint();
    }

    auto& lassoSelection = getLassoSelection();

    // Pass parameters of selected box to inspector
    if (lassoSelection.getNumSelected() == 1)
    {
        auto* box = lassoSelection.getSelectedItem(0);
        if (box->graphics)
        {
            main.inspector.loadData(box->graphics->getParameters());
            main.inspector.setVisible(true);
            main.console->setVisible(false);
        }
        else
        {
            main.inspector.deselect();
            main.inspector.setVisible(false);
            main.console->setVisible(true);
        }
    }
    else
    {
        main.inspector.deselect();
        main.inspector.setVisible(false);
        main.console->setVisible(true);
    }

    lasso.endLasso();
}

void Canvas::dragCallback(int dx, int dy)
{
    // Ignore when locked
    if (pd->locked) return;

    auto objects = std::vector<pd::Object*>();

    for (auto* box : getLassoSelection())
    {
        if (box->pdObject)
        {
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

void Canvas::findDrawables(Graphics& g)
{
    // Find all drawables (from objects like drawpolygon, filledcurve, etc.)
    // Pd draws this over all siblings, even when drawn inside a graph!
    // To mimic this we find the drawables from the top-level canvas and paint it over everything

    for (auto& box : boxes)
    {
        if (!box->pdObject) continue;

        auto* gobj = static_cast<t_gobj*>(box->pdObject->getPointer());

        // Recurse for graphs
        if (gobj->g_pd == canvas_class)
        {
            if (box->graphics && box->graphics->getGui().getType() == pd::Type::GraphOnParent)
            {
                auto* canvas = box->graphics->getCanvas();

                g.saveState();
                auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
                auto bounds = canvas->getParentComponent()->getLocalBounds().withPosition(pos);
                g.reduceClipRegion(bounds);

                canvas->findDrawables(g);

                g.restoreState();
            }
        }
        // Scalar found!
        if (gobj->g_pd == scalar_class)
        {
            auto* x = reinterpret_cast<t_scalar*>(gobj);
            auto* templ = template_findbyname(x->sc_template);
            auto* templatecanvas = template_findcanvas(templ);
            t_gobj* y;
            t_float basex, basey;
            scalar_getbasexy(x, &basex, &basey);

            if (!templatecanvas) return;

            for (y = templatecanvas->gl_list; y; y = y->g_next)
            {
                const t_parentwidgetbehavior* wb = pd_getparentwidget(&y->g_pd);
                if (!wb) continue;

                // This function is a work-in-progress conversion from pd's drawing to JUCE drawing
                TemplateDraw::paintOnCanvas(g, this, x, y, static_cast<int>(basex), static_cast<int>(basey));
            }
        }
    }
}

//==============================================================================
void Canvas::paintOverChildren(Graphics& g)
{
    // Pd Template drawing: not the most efficient implementation but it seems to work!
    // Graphs are drawn from their parent, so pd drawings are always on top of other objects
    if (!isGraph)
    {
        findDrawables(g);
    }

    // Draw connections in the making over everything else
    if (connectingEdge)
    {
        Point<float> mousePos = getMouseXYRelative().toFloat();
        Point<int> edgePos = connectingEdge->getCanvasBounds().getCentre();

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

    if (connectingEdge)
    {
        repaint();
    }
}

void Canvas::resized()
{
}

bool Canvas::keyPressed(const KeyPress& key, Component* originatingComponent)
{
    if (main.getCurrentCanvas() != this) return false;
    if (isGraph) return false;

    patch.keyPress(key.getKeyCode(), key.getModifiers().isShiftDown());

    return false;
}

void Canvas::deselectAll()
{
    // Deselects boxes
    MultiComponentDragger::deselectAll();

    // Deselect connections
    for (auto& connection : connections)
    {
        if (connection->isSelected)
        {
            connection->isSelected = false;
            connection->repaint();
        }
    }
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
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

    for (auto* box : boxes)
    {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(box->pdObject->getPointer())))
        {
            setSelected(box, true);
        }
    }

    patch.deselectAll();
}

void Canvas::duplicateSelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to duplicate
    patch.duplicate();

    // Load state from pd, don't update positions
    synchronise(false);

    // Select the newly duplicated objects
    for (auto* box : boxes)
    {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(box->pdObject->getPointer())))
        {
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
    // main.stopTimer();

    // Find selected objects and make them selected in pd
    Array<pd::Object*> objects;
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
            if (box->pdObject)
            {
                patch.selectObject(box->pdObject.get());
                objects.add(box->pdObject.get());
            }
        }
    }

    // remove selection
    patch.removeSelection();

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto& con : connections)
    {
        if (con->isSelected)
        {
            if (!(objects.contains(con->outObj->get()) || objects.contains(con->inObj->get())))
            {
                patch.removeConnection(con->outObj->get(), con->outIdx, con->inObj->get(), con->inIdx);
            }
        }
    }

    patch.finishRemove();  // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    // patch.deselectAll();

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

    if (viewport)
    {
        viewWidth = viewport->getWidth();
        viewHeight = viewport->getHeight();
    }

    // Check new bounds
    int minX = zeroPosition.x;
    int minY = zeroPosition.y;
    int maxX = std::max(getWidth() - minX, viewWidth);
    int maxY = std::max(getHeight() - minY, viewHeight);

    for (auto obj : boxes)
    {
        maxX = std::max<int>(maxX, obj->getX() + obj->getWidth());
        maxY = std::max<int>(maxY, obj->getY() + obj->getHeight());
        minX = std::min<int>(minX, obj->getX());
        minY = std::min<int>(minY, obj->getY());
    }

    if (!isGraph)
    {
        for (auto& box : boxes)
        {
            box->setBounds(box->getBounds().translated(-minX, -minY));
        }

        zeroPosition -= {minX, minY};
        setSize(maxX - minX, maxY - minY);
    }

    if (graphArea)
    {
        auto [x, y, w, h] = patch.getBounds();
        graphArea->setBounds(x, y, w, h);
    }
}
