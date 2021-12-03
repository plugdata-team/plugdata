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
    main->removeChild(getState());
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
    
    // Save all open tabs to restore later
    Array<std::pair<pd::Patch, String>> tabs;
    main->pd.waitForStateUpdate();
    
    int open_tab = main->getTabbar().getCurrentTabIndex();
    for(int n = 1; n < main->getTabbar().getNumTabs(); n++) {
        tabs.add({main->getCanvas(n)->patch, main->getTabbar().getTabbedButtonBar().getTabNames()[n]});
    }
    
    dragger.deselectAll();
    
    // Clear canvas
    for(auto& box : findChildrenOfClass<Box>()) {
        // set clear_pd to false: no need to clean up pd's objects, pd will take care of that
        box->remove(false);
    }
    
    // Removing all boxes should automatically remove all connections
    jassert(findChildrenOfClass<Connection>().size() == 0);
    
    String content = patch.getCanvasContent();
    
    Array<Box*> boxes;
    
    patch.setCurrent();
    
    // Go through all the boxes in the pd patch, and add GUI components for them
    for(auto& object : patch.getObjects(getProperty(Identifiers::isGraph))) {
        
        auto box = ValueTree(Identifiers::box);
        
        // Setting the exists flag tells the box that this refers to an objects that already exists in the pd patch
        box.setProperty(Identifiers::exists, true, nullptr);
        box.setProperty(Identifiers::boxX, object.getBounds()[0] * zoom, nullptr);
        box.setProperty(Identifiers::boxY, object.getBounds()[1] * zoom, nullptr);
        
        appendChild(box);
        
        String name = object.getText();
        
        auto* last_box = findChildrenOfClass<Box>().getLast();
        bool is_gui = pd::Gui::getType(object.getPointer(), name.toStdString()) != pd::Type::Undefined;
       
        if(is_gui) {
            last_box->pdObject = std::make_unique<pd::Gui>(object.getPointer(), &patch, &main->pd);
        }
        else {
            last_box->pdObject = std::make_unique<pd::Object>(object);
        }
       
        boxes.add(last_box); // lets hope this preserves the object order...
        
       
        
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
        
        box.setProperty(Identifiers::boxName, name, nullptr);
        
        // Return to normal operation
        box.setProperty(Identifiers::exists, false, nullptr);
    }
    
    if(getProperty(Identifiers::isGraph))
    {
        patch.deselectAll();
    
        
        return;
    }
    
    auto lines = StringArray::fromLines(content);
    lines.remove(0); // remove first canvas part
    
    // Connections are easier to read from the save file than to extract from the loaded patch
    // These connections already exists in libpd, we just need to add GUI components for them
    int canvas_idx = 0;
    for(auto& line : lines) {
        
        if(line.startsWith("#N canvas")) {
            canvas_idx++;
        }
        
        if(line.startsWith("#X restore")) {
            canvas_idx--;
        }
        
        if(line.startsWith("#X connect") && canvas_idx == 0) {
            auto segments = StringArray::fromTokens(line.fromFirstOccurrenceOf("#X connect ", false, false), " ", "\"");
            
            int end_box   = segments[0].getIntValue();
            int outlet    = segments[1].getIntValue();
            int start_box = segments[2].getIntValue();
            int inlet     = segments[3].getIntValue();
            
            auto start_edges = boxes[start_box]->findChildrenOfClass<Edge>();
            auto end_edges = boxes[end_box]->findChildrenOfClass<Edge>();
            
            auto new_connection = ValueTree(Identifiers::connection);
            
            String start_id = start_edges[inlet]->ValueTreeObject::getProperty(Identifiers::edgeID);
            String end_id = end_edges[boxes[end_box]->numInputs + outlet]->ValueTreeObject::getProperty(Identifiers::edgeID);
            
            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
            
            new_connection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            appendChild(new_connection);
            
            // Return to normal operation
            new_connection.setProperty(Identifiers::exists, false, nullptr);
        }
    }
    

    patch.deselectAll();
    
    // Check canvas bounds
    int max_x = 600;
    int max_y = 400;
    for(auto obj : findChildrenOfClass<Box>()) {
            max_x = std::max<int>(max_x, (int)obj->getProperty(Identifiers::boxX) + 200);
            max_y = std::max<int>(max_y, (int)obj->getProperty(Identifiers::boxY) + 200);
    }
    
    setSize(max_x, max_y);
    
    for(int n = 0; n < tabs.size(); n++) {
        bool found = false;
        for(auto& object : patch.getObjects(false)) {
            if(object.getPointer() == tabs[n].first.getPointer()) {
                found = true;
                break;
            }
        }
        
        // restore tab if it survived the sync
        // Kinda hacky, could be improved in the future
        if(found) {
            auto tree = ValueTree(Identifiers::canvas);
            tree.setProperty(Identifiers::isGraph, false, nullptr);
            tree.setProperty("Title", tabs[n].second, nullptr);
            main->appendChild(tree);
            
            auto* new_cnv = main->findChildrenOfClass<Canvas>().getLast();
            new_cnv->loadPatch(tabs.getReference(n).first);
            new_cnv->isMainPatch = false;
        }
    }
    
    main->getTabbar().setCurrentTabIndex(open_tab);
    
    // Create a comment for plugdata specific information
    String extra_info;
    for(auto* box : findChildrenOfClass<Box>()) {
        extra_info += String(box->getWidth()) + ":" +  String(box->getHeight()) + " ";
    }    
}


void Canvas::loadPatch(String patch) {
    
    String extra_info = patch.fromFirstOccurrenceOf("#X text plugdata_info:",false, false).upToFirstOccurrenceOf(";", false, false);
    
    // Create the pd save file
    auto temp_patch = File::createTempFile(".pd");
    temp_patch.replaceWithText(patch);
    
    
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to create the gui
    main->pd.openPatch(temp_patch.getParentDirectory().getFullPathName().toStdString(), temp_patch.getFileName().toStdString());
    
    this->patch = main->pd.getPatch();
    
    
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
                    main->appendChild(tree);
                    
                    auto* new_cnv = main->findChildrenOfClass<Canvas>().getLast();
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
        appendChild(box);
        
        // Open editor
        findChildrenOfClass<Box>().getLast()->textLabel.showEditor();
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
            removeMouseListener(con);
            
            if(!(objects.contains(con->outObj) || objects.contains(con->inObj))) {
                patch.removeConnection(con->outObj, con->outIdx,  con->inObj, con->inIdx);
            }
            removeChild(con->getState());
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
            
            auto state = cnv->getState();
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
