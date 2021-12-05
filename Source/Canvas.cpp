#include "Canvas.h"
#include "Box.h"
#include "Connection.h"
#include "PluginEditor.h"
//#include "../../Source/Library.hpp"

//==============================================================================
Canvas::Canvas(ValueTree tree, PlugDataPluginEditor* parent) : ValueTreeObject(tree), main(parent)
{
    setSize (600, 400);
    
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
    for(auto* cnv : getAllInstances()) {
        cnv->synchronise();
    }
}

void Canvas::synchronise() {
    
    main->pd.waitForStateUpdate();
    dragger.deselectAll();
    
    patch.setCurrent();
    
    Array<Box*> removedBoxes;
    
    const auto currentObjects = patch.getObjects(getProperty(Identifiers::isGraph));
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
            if(abs(x - (int)exists->getProperty(Identifiers::boxX)) > 6.0f) {
                exists->setProperty(Identifiers::boxX, x * zoom);
            }
            if(abs(y - (int)exists->getProperty(Identifiers::boxY)) > 6.0f) {
                exists->setProperty(Identifiers::boxY, y * zoom);
            }
        }
        else {
            auto [x, y, w, h] = object.getBounds();
            auto box_tree = ValueTree(Identifiers::box);
            auto name = String(object.getText());
            
            box_tree.setProperty(Identifiers::exists, true, nullptr);
            box_tree.setProperty(Identifiers::boxX, x * zoom, nullptr);
            box_tree.setProperty(Identifiers::boxY, y * zoom, nullptr);
            
            auto* new_box = appendChild<Box>(box_tree);
            
            auto type = pd::Gui::getType(object.getPointer(), name.toStdString());
            if(type != pd::Type::Undefined) {
                new_box->pdObject = std::make_unique<pd::Gui>(object.getPointer(), &patch, &main->pd);
            }
            else {
                new_box->pdObject = std::make_unique<pd::Object>(object);
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
            auto gui_simplify = [](String& target, const String& selector) {
                if(target.startsWith(selector)) {
                    target = selector;
                }
            };
            
            gui_simplify(name, "bng");
            gui_simplify(name, "tgl");
            gui_simplify(name, "nbx");
            gui_simplify(name, "hsl");
            gui_simplify(name, "vsl");
            gui_simplify(name, "hradio");
            gui_simplify(name, "vradio");
            
            new_box->setProperty(Identifiers::boxName, name);
            
            if(new_box->graphics && new_box->graphics->getCanvas() && (type == pd::Type::GraphOnParent || type == pd::Type::Subpatch)) {
                //new_box->graphics->getCanvas()->synchronise();
                //patch.setCurrent();
            }
            
            new_box->setProperty(Identifiers::exists, false);
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
    int max_x = getWidth();
    int max_y = getHeight();
    for(auto obj : findChildrenOfClass<Box>()) {
            max_x = std::max<int>(max_x, (int)obj->getProperty(Identifiers::boxX) + 200);
            max_y = std::max<int>(max_y, (int)obj->getProperty(Identifiers::boxY) + 200);
    }
    
    setSize(max_x, max_y);
    
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
    int canvas_idx = 0;
    for(auto& line : lines) {
        
        if(line.startsWith("#N canvas")) canvas_idx++;
        if(line.startsWith("#X restore")) canvas_idx--;
        
        if(line.startsWith("#X connect") && canvas_idx == 0) {
            auto segments = StringArray::fromTokens(line.fromFirstOccurrenceOf("#X connect ", false, false), " ", "\"");
            
            int end_box   = segments[0].getIntValue();
            int outlet    = segments[1].getIntValue();
            int start_box = segments[2].getIntValue();
            int inlet     = segments[3].getIntValue();
            
            auto start_edges = findChildOfClass<Box>(start_box)->findChildrenOfClass<Edge>();
            auto end_edges = findChildOfClass<Box>(end_box)->findChildrenOfClass<Edge>();
            
            auto new_connection = ValueTree(Identifiers::connection);
            
            String start_id = start_edges[inlet]->getProperty(Identifiers::edgeID);
            String end_id = end_edges[findChildOfClass<Box>(end_box)->numInputs + outlet]->getProperty(Identifiers::edgeID);
            
            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
            
            new_connection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            appendChild(new_connection);
            
            // Return to normal operation
            new_connection.setProperty(Identifiers::exists, false, nullptr);
        }
    }

    patch.deselectAll();

}

void Canvas::createPatch() {
    // Create the pd save file
    auto temp_patch = File::createTempFile(".pd");
    temp_patch.replaceWithText(main->defaultPatch);
    
    
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to create the gui
    main->pd.openPatch(temp_patch.getParentDirectory().getFullPathName().toStdString(), temp_patch.getFileName().toStdString());
    
    patch = main->pd.getPatch();
    synchronise();
}

void Canvas::loadPatch(pd::Patch& patch) {
    
    this->patch = patch;
    
    synchronise();
}




void Canvas::mouseDown(const MouseEvent& e)
{
    if(getProperty(Identifiers::isGraph)) return;
    
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
        auto& lasso_selection = dragger.getLassoSelection();
        bool has_selection = lasso_selection.getNumSelected();
        bool multiple = lasso_selection.getNumSelected() > 1;
        
        bool is_subpatch = has_selection && ((lasso_selection.getSelectedItem(0)->graphics &&  lasso_selection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Type::GraphOnParent) || lasso_selection.getSelectedItem(0)->textLabel.getText().startsWith("pd"));
        
        popupMenu.clear();
        popupMenu.addItem(1, "Open", !multiple && is_subpatch);
        popupMenu.addSeparator();
        popupMenu.addItem(4,"Cut", has_selection);
        popupMenu.addItem(5,"Copy", has_selection);
        popupMenu.addItem(6,"Duplicate", has_selection);
        popupMenu.addItem(7,"Delete", has_selection);
        popupMenu.setLookAndFeel(&getLookAndFeel());
        
        auto callback = [this, &lasso_selection](int result) {
            if(result < 1) return;
            
            switch (result) {
                case 1: {
                    
                    auto* cnv = lasso_selection.getSelectedItem(0)->graphics.get()->getCanvas();
                    auto& tabbar = main->getTabbar();
                    
                    for(int n = 0; n < tabbar.getNumTabs(); n++) {
                        auto* tab_canvas = main->getCanvas(n);
                        
                        if(tab_canvas->patch == cnv->patch) {
                            tabbar.setCurrentTabIndex(n);
                            return;
                        }
                    }
                    
                    auto tree = ValueTree(Identifiers::canvas);
                    tree.setProperty(Identifiers::isGraph, false, nullptr);
                    tree.setProperty("Title", lasso_selection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false), nullptr);
                    
                    
                    auto* new_cnv = main->appendChild<Canvas>(tree);
                    auto patch_copy = cnv->patch;
                    new_cnv->loadPatch(patch_copy);
                    new_cnv->isMainPatch = false;
                    
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
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(box), ModalCallbackFunction::create(callback));
        }
        else if(auto* box = dynamic_cast<Box*>(e.originalComponent)) {
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
        }
        else if(auto* gui = dynamic_cast<GUIComponent*>(e.originalComponent)) {
            auto* box = gui->box;
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&box->textLabel), ModalCallbackFunction::create(callback));
        }
        else {
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
        auto all_edges = getAllEdges();
        
        Edge* nearest_edge = nullptr;
        
        for(auto& edge : all_edges) {
            
            auto bounds = edge->getCanvasBounds().expanded(150, 150);
            if(bounds.contains(pos)) {
                if(!nearest_edge) nearest_edge = edge;
                
                auto old_pos = nearest_edge->getCanvasBounds().getCentre();
                auto new_pos = bounds.getCentre();
                nearest_edge = new_pos.getDistanceFrom(pos) < old_pos.getDistanceFrom(pos) ? edge : nearest_edge;
            }
        }
        
        if(nearest_edge) nearest_edge->createConnection();
        
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
    
    patch.moveObjects(objects, round((float)dx / zoom), round((float)dy / zoom));
}

//==============================================================================
void Canvas::paintOverChildren (Graphics& g)
{
    if(Edge::connectingEdge && Edge::connectingEdge->box->cnv == this) {
        Point<float> mouse_pos = getMouseXYRelative().toFloat();
        Point<int> edge_pos =  Edge::connectingEdge->getCanvasBounds().getPosition();
        
        edge_pos += Point<int>(4, 4);
        
        Path path;
        path.startNewSubPath(edge_pos.toFloat());
        path.lineTo(mouse_pos);
        
        g.setColour(Colours::grey);
        g.strokePath(path, PathStrokeType(3.0f));
    }
    
}

void Canvas::mouseMove(const MouseEvent& e) {
    repaint();
}
void Canvas::resized()
{
    
}

bool Canvas::keyPressed(const KeyPress &key, Component *originatingComponent) {
    if(main->getCurrentCanvas() != this) return false;
    if(getProperty(Identifiers::isGraph)) return false;
    
    if(key.getTextCharacter() == 'n') {
        auto box = ValueTree(Identifiers::box);
        appendChild<Box>(box)->textLabel.showEditor();
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
    
    patch.setCurrent();
    
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
    
    patch.setCurrent();
    
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
    Array<Edge*> all_edges;
    for(auto* box : findChildrenOfClass<Box>()) {
        for(auto* edge : box->findChildrenOfClass<Edge>())
            all_edges.add(edge);
    };
    
    return all_edges;
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

Array<Canvas*> Canvas::getAllInstances()
{
    Array<Canvas*> all_instances;
    for(auto* cnv : main->findChildrenOfClass<Canvas>(true)) {
        if(cnv->patch == patch && cnv->main == main) {
            all_instances.add(cnv);
        }
    }
    
    return all_instances;
}


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
