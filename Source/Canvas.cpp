/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include <m_pd.h>
#include <m_imp.h>


#include "Canvas.h"
#include "Box.h"
#include "Connection.h"
#include "PluginProcessor.h"


// Sorter for matching pd object order
struct TreeSorter
{
    
    Array<Box*>* indexTree;
    TreeSorter(Array<Box*>* index) : indexTree(index) {
        
    }
    
    int compareElements (Box* first, Box* second)
    {
        auto firstIdx = indexTree->indexOf(first);
        auto secondIdx = indexTree->indexOf(second);
        return (firstIdx < secondIdx) ? -1 : ((secondIdx < firstIdx) ? 1 : 0);
    }
};

//==============================================================================
Canvas::Canvas(PlugDataPluginEditor& parent, bool graph, bool graphChild) : main(parent), pd(parent.pd)
{
    isGraph = graph;
    isGraphChild = graphChild;
    
    if(isGraphChild) {
        graphArea.reset(new GraphArea(this));
        addAndMakeVisible(graphArea.get());
    }
    
    setSize (600, 400);
    
    setTransform(parent.transform);
    
    
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha((float)0.3));
    
    addKeyListener(this);
    
    setWantsKeyboardFocus(true);
    
    if(!isGraph) {
     
        transformLayer.addMouseListener(this, false);
        transformLayer.addAndMakeVisible(this);
        
        viewport = new Viewport; // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(&transformLayer, false);
        setPaintingIsUnclipped(true);
    }
    
    main.startTimer(guiUpdateMs);
}


Canvas::~Canvas()
{
    
   
    popupMenu.setLookAndFeel(nullptr);
    Component::removeAllChildren();
    removeKeyListener(this);
}

void Canvas::synchronise() {
    main.stopTimer();
    setTransform(main.transform);
    
    pd.waitForStateUpdate();
    dragger.deselectAll();
    
    patch.setCurrent();
    
    Array<Box*> removedBoxes;
    
    auto* cs = pd.getCallbackLock();
    cs->tryEnter();
    auto currentObjects = patch.getObjects(isGraph);
    cs->exit();

    // Clear connections
    // Currently there's no way we can check if these are alive,
    // We can only recreate them later from pd's save file
    connections.clear();
    
    Array<Box*> boxPointers;
    boxPointers.addArray(boxes);
    
    for(auto& object : currentObjects) {
        
        // Don't show non-patchable objects
        if(!patch.checkObject(&object)) continue;
        
        
        // Check if pd objects are already represented in the GUI
        Box* exists = nullptr;
        for(auto* box : boxPointers) {
            if(box->pdObject && object == *box->pdObject) {
                exists = box;
                break;
            }
        }
        // If so, update position
        if(exists) {
            auto [x, y, h, w] = object.getBounds();
            
            exists->setTopLeftPosition(x, y);
            exists->toBack();
        }
        else {
            auto [x, y, w, h] = object.getBounds();
            auto name = String(object.getText());
            
            auto type = pd::Gui::getType(object.getPointer(), name.toStdString());
            auto isGui = type != pd::Type::Undefined;
            auto* pdObject = isGui ? new pd::Gui(object.getPointer(), &patch, &pd) : new pd::Object(object);

            if(object.getName() == "message") {
                name = "msg " + name;
            }
            if(object.getName() == "gatom" && name.containsOnly("0123456789.")) { // TODO: is this correct?
                name = "floatatom";
            }
            else if(object.getName() == "gatom") {
                name = "symbolatom";
            }
            // Some of these GUI objects have a lot of extra symbols that we don't want to show
            auto guiSimplify = [](String& target, const String& selector) {
                if(target.startsWith(selector)) {
                    target = selector;
                }
            };
            
            guiSimplify(name, "bng");
            guiSimplify(name, "tgl");
            guiSimplify(name, "nbx");
            guiSimplify(name, "hsl");
            guiSimplify(name, "vsl");
            guiSimplify(name, "hradio");
            guiSimplify(name, "vradio");
            
            auto* newBox = boxes.add(new Box(pdObject, this, name, {(int)x, (int)y}));
            newBox->toBack();
        }
    }
    
    for(auto* box : boxPointers) {
        bool exists = false;
        for(const auto& object : currentObjects) {
            if(box->pdObject && object == *box->pdObject) {
                exists = true;
                break;
            }
        }
        if(!exists) {
            boxes.removeObject(box);
        }
    }
    

    Array<Box*> newOrder;
    newOrder.resize(boxes.size());
    
    // Fix order
    int n = 0;
    for(auto* box : boxes) {
        auto iter = std::find_if(currentObjects.begin(), currentObjects.end(), [box](pd::Object obj){
            return obj == *box->pdObject;
        });
        
        newOrder.set(iter - currentObjects.begin(), box);
        n++;
    }

    TreeSorter sorter(&newOrder);
    boxes.sort(sorter);

    checkBounds();
    
    // Skip connections for graphs
    if(isGraph)  {
        patch.deselectAll();
        main.startTimer(guiUpdateMs);
        return;
    }
    
    String content = patch.getCanvasContent();
    
    auto lines = StringArray::fromLines(content);
    lines.remove(0); // remove first canvas part
    
    // Connections are easier to read from the save file than to extract from the loaded patch
    // These connections already exists in libpd, we just need to add GUI components for them
    int canvasIdx = 0;
    for(auto& line : lines) {
        
        if(line.startsWith("#N canvas")) canvasIdx++;
        if(line.startsWith("#X restore")) canvasIdx--;
        
        if(line.startsWith("#X connect") && canvasIdx == 0) {
            auto segments = StringArray::fromTokens(line.fromFirstOccurrenceOf("#X connect ", false, false), " ", "\"");
            
            int endBox   = segments[0].getIntValue();
            int outlet    = segments[1].getIntValue();
            int startBox = segments[2].getIntValue();
            int inlet     = segments[3].getIntValue();
            
            auto& startEdges = boxes[startBox]->edges;
            auto& endEdges = boxes[endBox]->edges;
            
            if(startBox < boxes.size() && endBox < boxes.size()) {
                connections.add(new Connection(this, startEdges[inlet], endEdges[boxes[endBox]->numInputs + outlet], true));
            }
        }
    }

    patch.deselectAll();
    
    
    main.startTimer(guiUpdateMs);
}

void Canvas::createPatch() {
    // Create the pd save file
    auto tempPatch = File::createTempFile(".pd");
    tempPatch.replaceWithText(main.defaultPatch);
    
    auto* cs = pd.getCallbackLock();
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to create the gui
    pd.openPatch(tempPatch.getParentDirectory().getFullPathName().toStdString(), tempPatch.getFileName().toStdString());
    
    patch = pd.getPatch();
    
    
    synchronise();
}

void Canvas::loadPatch(pd::Patch& patch) {
    
    
    this->patch = patch;
    
    
    synchronise();
}

bool Canvas::hitTest(int x, int y) {
    if(x < 0 || y < 0) {
        transformLayer.repaint();
        return transformLayer.hitTest(x + getX(), y + getY());
    }
    
    return x < getWidth() && y < getHeight();
}

void Canvas::mouseDown(const MouseEvent& e)
{

    auto* source = e.originalComponent;
    
    if(isGraph)  {
        auto* box = findParentComponentOfClass<Box>();
        box->dragger.setSelected(box, true);
        return;
    }
    
    if(source == &transformLayer) {
        source = this;
    }
    
    
    if(!ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
       
        dragStartPosition = e.getMouseDownPosition();
        
        if(dynamic_cast<Connection*>(source)) {
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);
        }

        if(source == this || source == graphArea.get()){
            Edge::connectingEdge = nullptr;
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);
            
            for(auto& con : connections) {
                con->isSelected = false;
                con->repaint();
            }
        }

    }
    else
    {
        auto& lassoSelection = dragger.getLassoSelection();
        bool hasSelection = lassoSelection.getNumSelected();
        bool multiple = lassoSelection.getNumSelected() > 1;
        
        bool isSubpatch = hasSelection && (lassoSelection.getSelectedItem(0)->graphics &&  (lassoSelection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Type::GraphOnParent || lassoSelection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Type::Subpatch));
        
        popupMenu.clear();
        popupMenu.addItem(1, "Open", !multiple && isSubpatch);
        popupMenu.addSeparator();
        popupMenu.addItem(4,"Cut", hasSelection);
        popupMenu.addItem(5,"Copy", hasSelection);
        popupMenu.addItem(6,"Duplicate", hasSelection);
        popupMenu.addItem(7,"Delete", hasSelection);
        popupMenu.setLookAndFeel(&getLookAndFeel());
        
        auto callback = [this, &lassoSelection](int result) {
            if(result < 1) return;
            
            switch (result) {
                case 1: {
                    auto* subpatch = lassoSelection.getSelectedItem(0)->graphics.get()->getPatch();
                    auto& tabbar = main.getTabbar();
                    
                    for(int n = 0; n < tabbar.getNumTabs(); n++) {
                        auto* tabCanvas = main.getCanvas(n);
                        if(tabCanvas->patch == *subpatch) {
                            tabbar.setCurrentTabIndex(n);
                            return;
                        }
                    }
                    bool isGraphChild = lassoSelection.getSelectedItem(0)->graphics.get()->getGUI().getType() == pd::Type::GraphOnParent;
                    auto* newCanvas = new Canvas(main, false, isGraphChild);
                    newCanvas->title = lassoSelection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false);
                    auto patchCopy = *subpatch;
                    newCanvas->loadPatch(patchCopy);
                    
                    auto [x, y, w, h] = patchCopy.getBounds();

                    if(isGraphChild) {
                        newCanvas->graphArea->setBounds(x, y, std::max(w, 60), std::max(h, 60));
                    }
                    
                    main.addTab(newCanvas);
                    newCanvas->checkBounds();
                    break;
                }
                case 4:
                    copySelection();
                    removeSelection();
                    break;
                case 5:
                    copySelection();
                    break;
                case 6: {
                    duplicateSelection();
                    break;
                }
                case 7:
                    removeSelection();
                    break;
            }
        };
        
        if(auto* box = dynamic_cast<ClickLabel*>(e.originalComponent)) {
            if(!box->getCurrentTextEditor()) {
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(box), ModalCallbackFunction::create(callback));
            }
        }
        else if(auto* box = dynamic_cast<Box*>(e.originalComponent)) {
            if(!box->textLabel.getCurrentTextEditor()) {
                popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
            }
        }
        else if(auto* gui = dynamic_cast<GUIComponent*>(e.originalComponent)) {
            auto* box = gui->box;
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
        }
        else if(auto* gui = dynamic_cast<Canvas*>(e.originalComponent)){
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetScreenArea({e.getScreenX(), e.getScreenY(), 10, 10}), ModalCallbackFunction::create(callback));
        }

        
        
    }
}


void Canvas::mouseDrag(const MouseEvent& e)
{
    if(isGraph) return;
    
    auto* source = e.originalComponent;
    
    if(dynamic_cast<Connection*>(source)) {
        lasso.dragLasso(e.getEventRelativeTo(this));
    }
    else if(source == this){
        Edge::connectingEdge = nullptr;
        lasso.dragLasso(e);
        
        for(auto& con : connections) {
            if(!con->start || !con->end) {
                connections.removeObject(con);
                continue;
            }
            
            Line<int> path(con->start->getCanvasBounds().getCentre(), con->end->getCanvasBounds().getCentre());
            
            bool intersect = false;
            for(float i = 0; i < 1; i += 0.005) {
                if(lasso.getBounds().contains(path.getPointAlongLineProportionally(i))) {
                    intersect = true;
                }
            }
            
            if(!con->isSelected && intersect) {
                con->isSelected = true;
                con->repaint();
            }
            else if(con->isSelected && !intersect) {
                con->isSelected = false;
                con->repaint();
            }
        }
    }
    
    if(connectingWithDrag) repaint();
    transformLayer.repaint();
    
}

void Canvas::paint(Graphics&)
{
    transformLayer.repaint();
}

void Canvas::mouseUp(const MouseEvent& e)
{
    if(connectingWithDrag) {
        auto pos = e.getEventRelativeTo(this).getPosition();
        auto allEdges = getAllEdges();
        
        Edge* nearestEdge = nullptr;
        
        for(auto& edge : allEdges) {
            
            auto bounds = edge->getCanvasBounds().expanded(150, 150);
            if(bounds.contains(pos)) {
                if(!nearestEdge) nearestEdge = edge;
                
                auto oldPos = nearestEdge->getCanvasBounds().getCentre();
                auto newPos = bounds.getCentre();
                nearestEdge = newPos.getDistanceFrom(pos) < oldPos.getDistanceFrom(pos) ? edge : nearestEdge;
            }
        }
        
        if(nearestEdge) nearestEdge->createConnection();
        
        Edge::connectingEdge = nullptr;
        connectingWithDrag = false;
    }
    lasso.endLasso();
    
}

void Canvas::dragCallback(int dx, int dy)
{
    auto objects = std::vector<pd::Object*>();
    
    for(auto* box : dragger.getLassoSelection()) {
        if(box->pdObject) {
            objects.push_back(box->pdObject.get());
        }
    }
    
    patch.moveObjects(objects, dx, dy);
    
    checkBounds();
    main.updateUndoState();
}


//==============================================================================
void Canvas::paintOverChildren (Graphics& g)
{
    
    // Pd Template drawing: not the most efficient implementation but it seems to work!
    for (auto* gobj = patch.getPointer()->gl_list; gobj; gobj = gobj->g_next)
    {
        if (gobj->g_pd == scalar_class)
        {
            t_scalar *x = (t_scalar *)gobj;
            t_template *templ = template_findbyname(x->sc_template);
            t_canvas *templatecanvas = template_findcanvas(templ);
            t_gobj *y;
            t_float basex, basey;
            scalar_getbasexy(x, &basex, &basey);

            if (!templatecanvas)
            {
                return;
            }

            for (y = templatecanvas->gl_list; y; y = y->g_next)
            {
                const t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                
                if(!strcmp(y->g_pd->c_name->s_name, "filledcurve")) {
                    std::cout << "boom" << std::endl;
                }
                
                //std::cout << y->g_pd->c_name->s_name << std::endl;
                if (!wb) continue;
                
                TemplateDraw::paintOnCanvas(g, this, x, y, basex, basey);
            }
        }
    } 
    
    
    if(Edge::connectingEdge && Edge::connectingEdge->box->cnv == this) {
        Point<float> mousePos = getMouseXYRelative().toFloat();
        Point<int> edgePos =  Edge::connectingEdge->getCanvasBounds().getPosition();
        
        edgePos += Point<int>(4, 4);
        
        Path path;
        path.startNewSubPath(edgePos.toFloat());
        path.lineTo(mousePos);
        
        g.setColour(Colours::grey);
        g.strokePath(path, PathStrokeType(3.0f));
    }
}

void Canvas::mouseMove(const MouseEvent& e) {
    
    // For deciding where to place a new object
    lastMousePos = e.getPosition();
    repaint();
}

void Canvas::resized()
{
    transformLayer.setBounds(getLocalBounds());
}

bool Canvas::keyPressed(const KeyPress &key, Component *originatingComponent) {
    if(main.getCurrentCanvas() != this) return false;
    if(isGraph) return false;
    
    // Zoom in
    if(key.isKeyCode(61) && key.getModifiers().isCommandDown()) {        
        main.transform = main.transform.scaled(1.25f);
        setTransform(main.transform);
        return true;
    }
    // Zoom out
    if(key.isKeyCode(45) && key.getModifiers().isCommandDown()) {
        main.transform = main.transform.scaled(0.8f);
        setTransform(main.transform);
        return true;
    }
    // Key shortcuts for creating objects
    if(key.getTextCharacter() == 'n') {
        boxes.add(new Box(this, "", lastMousePos));
        return true;
    }
    if(key.isKeyCode(65) && key.getModifiers().isCommandDown()) {
        for(auto* child : boxes) {
            dragger.setSelected(child, true);
        }
        
        for(auto* child : connections) {
            child->isSelected = true;
            child->repaint();
        }
        
        return true;
    }
    if(key.getTextCharacter() == 'b') {
        boxes.add(new Box(this, "bng", lastMousePos));
        return true;
    }
    if(key.getTextCharacter() == 'm') {
        boxes.add(new Box(this, "msg", lastMousePos));
        return true;
    }
    if(key.getTextCharacter() == 'i') {
        boxes.add(new Box(this, "nbx", lastMousePos));
        return true;
    }
    if(key.getTextCharacter() == 'f') {
        boxes.add(new Box(this, "floatatom", lastMousePos));
        return true;
    }
    if(key.getTextCharacter() == 't') {
        boxes.add(new Box(this, "tgl", lastMousePos));
        return true;
    }
    if(key.getTextCharacter() == 's') {
        boxes.add(new Box(this, "vsl", lastMousePos));
        return true;
    }
    
    if(key.getKeyCode() == KeyPress::backspaceKey) {
        removeSelection();
        return true;
    }
    // cmd-c
    if(key.getModifiers().isCommandDown() && key.isKeyCode(67)) {
        copySelection();
        return true;
    }
    // cmd-v
    if(key.getModifiers().isCommandDown() && key.isKeyCode(86)) {
        pasteSelection();
        return true;
    }
    // cmd-x
    if(key.getModifiers().isCommandDown() && key.isKeyCode(88)) {
        
        copySelection();
        removeSelection();
        
        return true;
    }
    // cmd-d
    if(key.getModifiers().isCommandDown() && key.isKeyCode(68)) {
        
        duplicateSelection();
        return true;
    }
    
    // cmd-shift-z
    if(key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.isKeyCode(90)) {
        
        redo();
        return true;
    }
    // cmd-z
    if(key.getModifiers().isCommandDown() && key.isKeyCode(90)) {
        
        undo();
        return true;
    }
    
    return false;
}

void Canvas::copySelection() {
    
    for(auto* sel : dragger.getLassoSelection()) {
        if(auto* box = dynamic_cast<Box*>(sel)) {
            patch.selectObject(box->pdObject.get());
        }
    }
    
    patch.copy();
    
    patch.deselectAll();
    
}

void Canvas::pasteSelection() {
    patch.paste();
    synchronise();
}

void Canvas::duplicateSelection() {
    
    
    for(auto* sel : dragger.getLassoSelection()) {
        if(auto* box = dynamic_cast<Box*>(sel)) {
            patch.selectObject(box->pdObject.get());
        }
    }
    
    patch.duplicate();
    patch.deselectAll();
    
    synchronise();
}

void Canvas::removeSelection() {
    
    main.stopTimer();
    
    Array<pd::Object*> objects;
    for(auto* sel : dragger.getLassoSelection()) {
        if(auto* box = dynamic_cast<Box*>(sel)) {
            if(box->pdObject) {
                patch.selectObject(box->pdObject.get());
                objects.add(box->pdObject.get());
            }
        }
    }
    
    patch.removeSelection();
    
    for(auto& con : connections) {
        if(con->isSelected) {
            if(!(objects.contains(con->outObj->get()) || objects.contains(con->inObj->get()))) {
                patch.removeConnection(con->outObj->get(), con->outIdx,  con->inObj->get(), con->inIdx);
            }
        }
    }
    
    dragger.deselectAll();    
    
    synchronise();
    
    main.startTimer(guiUpdateMs);
    main.updateUndoState();
}


Array<Edge*> Canvas::getAllEdges() {
    Array<Edge*> allEdges;
    for(auto* box : boxes) {
        for(auto* edge : box->edges)
            allEdges.add(edge);
    };
    
    return allEdges;
}

Array<Canvas*> Canvas::findCanvas(t_canvas* cnv)
{
    Array<Canvas*> result;
    // For objects like drawpolygon that can draw on the canvas!

    if(patch.getPointer() == cnv) {
        result.add(this);
    }

    for(auto* box : boxes) {
        if(box->graphics && box->graphics->getGUI().getType() == pd::Type::Subpatch) {

            
        }
        if(box->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
            auto subresult = box->graphics->getCanvas()->findCanvas(cnv);
            result.addArray(subresult);
        }
    }

    return result;
}

void Canvas::undo() {
    patch.undo();
    synchronise();
    main.updateUndoState();
}

void Canvas::redo() {
    patch.redo();
    synchronise();
    main.updateUndoState();

}


// Called from subpatcher objects to close all tabs that refer to that subpatcher
void Canvas::closeAllInstances()
{
    auto& tabbar = main.getTabbar();
    for(int n = 0; n < tabbar.getNumTabs(); n++) {
        auto* cnv = main.getCanvas(n);
        if(cnv && cnv->patch == patch && &cnv->main == &main) {
            tabbar.removeTab(n);
            main.canvases.removeObject(cnv);
        }
    }
    
    if(tabbar.getNumTabs() > 1) {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    }
    else {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }
}


void Canvas::checkBounds() {
    
    // Check new bounds
    int minX = 0;
    int minY = 0;
    int maxX = getWidth() - getX();
    int maxY = getHeight() - getY();

    for(auto obj : boxes) {
        maxX = std::max<int>(maxX, (int)obj->getX() + obj->getWidth());
        maxY = std::max<int>(maxY, (int)obj->getY() + obj->getHeight());
        minX = std::min<int>(minX, (int)obj->getX());
        minY = std::min<int>(minY, (int)obj->getY());
    }
    
    //if(!isGraph) {
        setTopLeftPosition(-minX, -minY);
        setSize(maxX - minX, maxY - minY);
    //}
    
    
    
    
    if(graphArea) {
        auto [x, y, w, h] = patch.getBounds();

        
        graphArea->setBounds(x, y, w, h);
    }
}
