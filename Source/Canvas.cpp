#include "Canvas.h"
#include "Box.h"
#include "Connection.h"
#include "PluginEditor.h"

//==============================================================================
Canvas::Canvas(ValueTree tree, PlugDataPluginEditor* parent) : ValueTreeObject(tree), main(parent)
{
    setSize (600, 400);
    
    setTransform(parent->transform);
    
    
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha((float)0.3));
    
    addKeyListener(this);
    
    setWantsKeyboardFocus(true);
    
    if(!getProperty(Identifiers::isGraph)) {
        viewport = new Viewport; // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);
    }
    
    rebuildObjects();
    
    main->startTimer(guiUpdateMs);
}


Canvas::~Canvas()
{
    popupMenu.setLookAndFeel(nullptr);
    Component::removeAllChildren();
    removeKeyListener(this);
    ValueTreeObject::removeAllChildren();
}

ValueTreeObject* Canvas::factory(const juce::Identifier & id, const juce::ValueTree & tree)
{
    if(id == Identifier(Identifiers::box)) {
        auto* box = new Box(this, tree, dragger);
        addAndMakeVisible(box);
        box->addMouseListener(this, true);
        return static_cast<ValueTreeObject*>(box);
    }
    if(id == Identifier(Identifiers::connection)){
        auto* connection = new Connection(this, tree);
        addAndMakeVisible(connection);
        connection->addMouseListener(this, true);
        return static_cast<ValueTreeObject*>(connection);
    }
    return static_cast<ValueTreeObject*>(nullptr);
}

void Canvas::synchroniseAll() {
    // Synchronise all canvases that refer to this patch
    for(auto* cnv : main->findChildrenOfClass<Canvas>(true)) {
        
        // Hacky check to see if the last synchronise action didn't delete the canvas...
        if(main->findChildrenOfClass<Canvas>(true).contains(cnv)) {
            if(cnv->patch == patch && cnv->main == main) {
                cnv->synchronise();
            }
        }
    }
}

void Canvas::synchronise() {
    main->stopTimer();
    setTransform(main->transform);
    
    main->pd.waitForStateUpdate();
    dragger.deselectAll();
    
    patch.setCurrent();
    
    Array<Box*> removedBoxes;
    
    auto* cs = main->pd.getCallbackLock();
    cs->tryEnter();
    auto currentObjects = patch.getObjects(getProperty(Identifiers::isGraph));
    cs->exit();
    
    auto currentBoxes = findChildrenOfClass<Box>();
    
    // Clear connections
    // Currently there's no way we can check if these are alive,
    // We can only recreate them later from pd's save file
    for(auto& connection : findChildrenOfClass<Connection>()) {
        removeChild(connection->getObjectState());
    }
    
    for(auto& object : currentObjects) {
        // Check if pd objects are already represented in the GUI
        Box* exists = nullptr;
        for(auto* box : currentBoxes) {
            if(box->pdObject && object == *box->pdObject) {
                exists = box;
                break;
            }
        }
        // If so, update position
        if(exists) {
            auto [x, y, h, w] = object.getBounds();
            // Ignore rounding errors because of zooming
            if(abs((x * zoomX) - (int)exists->getProperty(Identifiers::boxX)) > 6.0f) {
                exists->setProperty(Identifiers::boxX, x * zoomX);
            }
            if(abs((y * zoomY) - (int)exists->getProperty(Identifiers::boxY)) > 6.0f) {
                exists->setProperty(Identifiers::boxY, y * zoomY);
            }
        }
        else {
            auto [x, y, w, h] = object.getBounds();
            auto boxTree = ValueTree(Identifiers::box);
            auto name = String(object.getText());
            
            boxTree.setProperty(Identifiers::exists, true, nullptr);
            boxTree.setProperty(Identifiers::boxX, x * zoomX, nullptr);
            boxTree.setProperty(Identifiers::boxY, y * zoomY, nullptr);
            
            auto* newBox = appendChild<Box>(boxTree);
            
            auto type = pd::Gui::getType(object.getPointer(), name.toStdString());
            if(type != pd::Type::Undefined) {
                newBox->pdObject = std::make_unique<pd::Gui>(object.getPointer(), &patch, &main->pd);
            }
            else {
                newBox->pdObject = std::make_unique<pd::Object>(object);
            }
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
            
            newBox->setProperty(Identifiers::boxName, name);
            
            if(newBox->graphics && newBox->graphics->getCanvas() && (type == pd::Type::GraphOnParent || type == pd::Type::Subpatch)) {
                //newBox->graphics->getCanvas()->synchronise();
                //patch.setCurrent();
            }
            
            newBox->setProperty(Identifiers::exists, false);
        }
    }
    
    for(auto* box : currentBoxes) {
        bool exists = false;
        for(const auto& object : currentObjects) {
            if(box->pdObject && object == *box->pdObject) {
                exists = true;
                break;
            }
        }
        if(!exists) {
            removeChild(box->getObjectState());
        }
    }
    
    // Update for the newly added and removed objects
    currentBoxes = findChildrenOfClass<Box>();
    
    Array<ValueTree> newOrder;
    newOrder.resize(currentBoxes.size());
    
    // Fix order
    int n = 0;
    for(auto* box : currentBoxes) {
        auto iter = std::find_if(currentObjects.begin(), currentObjects.end(), [box](pd::Object obj){
            return obj == *box->pdObject;
        });
        
        newOrder.set(iter - currentObjects.begin(), getChild(n));
        n++;
    }

    TreeSorter sorter(&newOrder);
    getObjectState().sort(sorter, nullptr, false);
    
    // Check canvas bounds
    int maxX = getWidth();
    int maxY = getHeight();
    for(auto obj : findChildrenOfClass<Box>()) {
            maxX = std::max<int>(maxX, (int)obj->getProperty(Identifiers::boxX) + 200);
            maxY = std::max<int>(maxY, (int)obj->getProperty(Identifiers::boxY) + 200);
    }
    
    setSize(maxX, maxY);
    
    // Skip connections for graphs
    if(getProperty(Identifiers::isGraph))  {
        patch.deselectAll();
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
            
            auto startEdges = findChildOfClass<Box>(startBox)->findChildrenOfClass<Edge>();
            auto endEdges = findChildOfClass<Box>(endBox)->findChildrenOfClass<Edge>();
            
            auto newConnection = ValueTree(Identifiers::connection);
            
            String startID = startEdges[inlet]->getProperty(Identifiers::edgeID);
            String endID = endEdges[findChildOfClass<Box>(endBox)->numInputs + outlet]->getProperty(Identifiers::edgeID);
            
            newConnection.setProperty(Identifiers::startID, startID, nullptr);
            newConnection.setProperty(Identifiers::endID, endID, nullptr);
            
            newConnection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            appendChild(newConnection);
            
            // Return to normal operation
            newConnection.setProperty(Identifiers::exists, false, nullptr);
        }
    }

    patch.deselectAll();
    
    main->startTimer(guiUpdateMs);
}

void Canvas::createPatch() {
    // Create the pd save file
    auto tempPatch = File::createTempFile(".pd");
    tempPatch.replaceWithText(main->defaultPatch);
    
    ScopedLock lock(*main->pd.getCallbackLock());
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to create the gui
    main->pd.openPatch(tempPatch.getParentDirectory().getFullPathName().toStdString(), tempPatch.getFileName().toStdString());
    
    patch = main->pd.getPatch();
    synchronise();
}

void Canvas::loadPatch(pd::Patch& patch) {
    
    this->patch = patch;
    
    synchronise();
}


void Canvas::mouseDown(const MouseEvent& e)
{
    if(getProperty(Identifiers::isGraph))  {
        auto* box = findParentOfType<Box>();
        box->dragger.setSelected(box, true);
        return;
    }
    
    if(!ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
        auto* source = e.originalComponent;
        
        dragStartPosition = e.getMouseDownPosition();
        
        if(dynamic_cast<Connection*>(source)) {
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);
        }
        else if(source == this){
            Edge::connectingEdge = nullptr;
            lasso.beginLasso(e, &dragger);
            
            for(auto& con : findChildrenOfClass<Connection>()) {
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
                    auto* cnv = lassoSelection.getSelectedItem(0)->graphics.get()->getCanvas();
                    auto& tabbar = main->getTabbar();
                    
                    for(int n = 0; n < tabbar.getNumTabs(); n++) {
                        auto* tabCanvas = main->getCanvas(n);
                        
                        if(tabCanvas->patch == cnv->patch) {
                            tabbar.setCurrentTabIndex(n);
                            return;
                        }
                    }
                    
                    auto tree = ValueTree(Identifiers::canvas);
                    tree.setProperty(Identifiers::isGraph, false, nullptr);
                    
                    tree.setProperty("Title", lassoSelection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false), nullptr);
                    
                    auto* newCanvas = main->appendChild<Canvas>(tree);
                    auto patchCopy = cnv->patch;
                    newCanvas->loadPatch(patchCopy);                    
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

Edge* Canvas::findEdgeByID(String ID) {
    for(auto& box : findChildrenOfClass<Box>()) {
        auto tree = box->getChildWithProperty(Identifiers::edgeID, ID);
        if(auto* result = box->findObjectForTree<Edge>(tree))
            return result;
    };
    
    return nullptr;
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    if(getProperty(Identifiers::isGraph)) return;
    
    auto* source = e.originalComponent;
    
    if(dynamic_cast<Connection*>(source)) {
        lasso.dragLasso(e.getEventRelativeTo(this));
    }
    else if(source == this){
        Edge::connectingEdge = nullptr;
        lasso.dragLasso(e);
        
        for(auto& con : findChildrenOfClass<Connection>()) {
            if(!con->start || !con->end) {
                removeChild(con->getObjectState());
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
}

void Canvas::mouseUp(const MouseEvent& e)
{
    if(connectingWithDrag) {
        auto pos = e.getEventRelativeTo(this).getPosition();
        auto allEdges = getAllEdges();
        
        Edge* nearesEdge = nullptr;
        
        for(auto& edge : allEdges) {
            
            auto bounds = edge->getCanvasBounds().expanded(150, 150);
            if(bounds.contains(pos)) {
                if(!nearesEdge) nearesEdge = edge;
                
                auto oldPos = nearesEdge->getCanvasBounds().getCentre();
                auto newPos = bounds.getCentre();
                nearesEdge = newPos.getDistanceFrom(pos) < oldPos.getDistanceFrom(pos) ? edge : nearesEdge;
            }
        }
        
        if(nearesEdge) nearesEdge->createConnection();
        
        Edge::connectingEdge = nullptr;
        connectingWithDrag = false;
    }
    lasso.endLasso();
}

void Canvas::dragCallback(int dx, int dy)
{
    auto selection = dragger.getLassoSelection();
    auto objects = std::vector<pd::Object*>();
    
    for(auto* box : selection) {
        if(box->pdObject) {
            objects.push_back(box->pdObject.get());
        }
    }
    
    patch.moveObjects(objects, round((float)dx / zoomX), round((float)dy / zoomY));
}

//==============================================================================
void Canvas::paintOverChildren (Graphics& g)
{
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
    lastMousePos = e.getPosition();
    repaint();
}
void Canvas::resized()
{
    
}

bool Canvas::keyPressed(const KeyPress &key, Component *originatingComponent) {
    if(main->getCurrentCanvas() != this) return false;
    if(getProperty(Identifiers::isGraph)) return false;
    
    // Zoom in
    if(key.isKeyCode(61) && key.getModifiers().isCommandDown()) {        
        main->transform = main->transform.scaled(1.25f);
        setTransform(main->transform);
        return true;
    }
    // Zoom out
    if(key.isKeyCode(45) && key.getModifiers().isCommandDown()) {
        main->transform = main->transform.scaled(0.8f);
        setTransform(main->transform);
        return true;
    }
    // Key shortcuts for creating objects
    if(key.getTextCharacter() == 'n') {
        auto box = ValueTree(Identifiers::box);
        appendChild<Box>(box)->textLabel.showEditor();
        return true;
    }
    if(key.isKeyCode(65) && key.getModifiers().isCommandDown()) {
        for(auto* child : findChildrenOfClass<Box>()) {
            dragger.setSelected(child, true);
        }
        
        for(auto* child : findChildrenOfClass<Connection>()) {
            child->isSelected = true;
            child->repaint();
        }
        
        return true;
    }
    if(key.getTextCharacter() == 'b') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "bng", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
        return true;
    }
    if(key.getTextCharacter() == 'm') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "msg", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
        return true;
    }
    if(key.getTextCharacter() == 'i') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "nbx", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
        return true;
    }
    if(key.getTextCharacter() == 'f') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "floatatom", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
        return true;
    }
    if(key.getTextCharacter() == 't') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "tgl", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
        return true;
    }
    if(key.getTextCharacter() == 's') {
        auto box = ValueTree(Identifiers::box);
        box.setProperty(Identifiers::boxName, "vsl", nullptr);
        appendChild(box);
        box.setProperty(Identifiers::boxX, lastMousePos.x, nullptr);
        box.setProperty(Identifiers::boxY, lastMousePos.y, nullptr);
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
    
    main->stopTimer();
    
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
    
    for(auto& con : findChildrenOfClass<Connection>()) {
        if(con->isSelected) {
            if(!(objects.contains(con->outObj) || objects.contains(con->inObj))) {
                patch.removeConnection(con->outObj, con->outIdx,  con->inObj, con->inIdx);
            }
        }
    }
    
    dragger.deselectAll();    
    
    synchroniseAll();
    
    main->startTimer(guiUpdateMs);
}


Array<Edge*> Canvas::getAllEdges() {
    Array<Edge*> allEdges;
    for(auto* box : findChildrenOfClass<Box>()) {
        for(auto* edge : box->findChildrenOfClass<Edge>())
            allEdges.add(edge);
    };
    
    return allEdges;
}

void Canvas::undo() {
    patch.undo();
    synchroniseAll();
    main->valueTreeChanged();
}

void Canvas::redo() {
    patch.redo();
    synchroniseAll();
    main->valueTreeChanged();
}


// Called from subpatcher objects to close all tabs that refer to that subpatcher
void Canvas::closeAllInstances()
{
    auto& tabbar = main->getTabbar();
    for(int n = 0; n < tabbar.getNumTabs(); n++) {
        auto* cnv = main->getCanvas(n);
        if(cnv && cnv->patch == patch && cnv->main == main) {
            tabbar.removeTab(n);
            
            auto state = cnv->getObjectState();
            main->removeChild(state);
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
