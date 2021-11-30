#include "Canvas.h"
#include "Box.h"
#include "Connection.h"
#include "MainComponent.h"
//#include "../../Source/Library.hpp"

//==============================================================================
Canvas::Canvas(ValueTree tree, MainComponent* parent) : ValueTreeObject(tree), main(parent)
{
    
    setSize (600, 400);
    
    
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha((float)0.3));
    
    addKeyListener(this);
    
    setWantsKeyboardFocus(true);
    
    viewport.setViewedComponent(this, false);
    
    int max_x = 600;
    int max_y = 400;
    for(auto obj : tree) {
        if(obj.hasProperty("X")) {
            max_x = std::max(max_x, (int)obj.getProperty("X"));
            max_y = std::max(max_y, (int)obj.getProperty("Y"));
        }
    }
    
    setSize(max_x + 500, max_y + 500);
    
    rebuildObjects();
    
}


Canvas::~Canvas()
{
    removeAllChildren();
    removeKeyListener(this);
    ValueTreeObject::getState().removeAllChildren(nullptr);
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

void Canvas::synchronise() {
    

    main->pd.waitForStateUpdate();
    
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

    // Go through all the boxes in the pd patch, and add GUI components for them
    for(auto& object : patch.getObjects(getState().getProperty(Identifiers::is_graph))) {
        
        auto box = ValueTree(Identifiers::box);
        
        // Setting the exists flag tells the box that this refers to an objects that already exists in the pd patch
        box.setProperty(Identifiers::exists, true, nullptr);
        box.setProperty(Identifiers::box_x, object.getBounds()[0] + 1, nullptr);
        box.setProperty(Identifiers::box_y, object.getBounds()[1] + 1, nullptr);
        
        getState().appendChild(box, nullptr);
        
        auto* last_box = findChildrenOfClass<Box>().getLast();
        last_box->pd_object = static_cast<t_pd*>(object.getPointer());
        boxes.add(last_box); // lets hope this preserves the object order...
        
        String name = object.getText();
        
        if(object.getName() == "message") {
            name = "msg " + name;
        }
        
        if(object.getName() == "gatom" && name.containsOnly("0123456789.")) {
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

        box.setProperty(Identifiers::box_name, name, nullptr);
        
        // Return to normal operation
        box.setProperty(Identifiers::exists, false, nullptr);
    }
    
    if(getState().getProperty(Identifiers::is_graph)) return;
    
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
            
            String start_id = start_edges[inlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            String end_id = end_edges[boxes[end_box]->numInputs + outlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            
            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
            
            new_connection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            getState().appendChild(new_connection, nullptr);
            
            // Return to normal operation
            new_connection.setProperty(Identifiers::exists, false, nullptr);
        }
        
    }
    
}

void Canvas::loadPatch(String patch) {
    
    
    // Create the pd save file
    auto temp_patch = File::createTempFile(".pd");
    temp_patch.replaceWithText(patch);
    
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to add GUI components
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
    if(getState().getProperty(Identifiers::is_graph)) return;
    
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
        
        bool is_subpatch = has_selection && ((lasso_selection.getSelectedItem(0)->graphics &&  lasso_selection.getSelectedItem(0)->graphics->getGUI().getType() == pd::Gui::Type::GraphOnParent) || lasso_selection.getSelectedItem(0)->textLabel.getText().startsWith("pd"));
        
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
                    
                    auto tree = ValueTree(Identifiers::canvas);
                    tree.setProperty(Identifiers::is_graph, false, nullptr);
                    tree.setProperty("Title", lasso_selection.getSelectedItem(0)->textLabel.getText().fromLastOccurrenceOf("pd ", false, false), nullptr);
                    main->getState().appendChild(tree, nullptr);
                    
                    auto* new_cnv = main->findChildrenOfClass<Canvas>().getLast();
                    new_cnv->loadPatch(cnv->patch);
                    
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
        
        
        popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent (e.originalComponent), ModalCallbackFunction::create(callback));

    }
}

Edge* Canvas::findEdgeByID(String ID) {
    for(auto& box : findChildrenOfClass<Box>()) {
        auto tree = box->getState().getChildWithProperty(Identifiers::edge_id, ID);
        if(auto* result = box->findObjectForTree<Edge>(tree))
            return result;
    };
    
    return nullptr;
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    if(getState().getProperty(Identifiers::is_graph)) return;
    
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
    if(getState().getProperty(Identifiers::is_graph)) return false;
    
    if(key.getTextCharacter() == 'n') {
        auto box = ValueTree(Identifiers::box);
        getState().appendChild(box, nullptr);
        
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
            patch.selectObject(box->pd_object);
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
            patch.selectObject(box->pd_object);
        }
    }
    
    patch.duplicate();
    patch.deselectAll();
    
    synchronise();
}

void Canvas::removeSelection() {
    for(auto& sel : dragger.getLassoSelection()) {
        removeMouseListener(sel);
        dynamic_cast<Box*>(sel)->remove();
    }
    
    for(auto& con : findChildrenOfClass<Connection>()) {
        if(con->isSelected) {
            removeMouseListener(con);
           
            patch.removeConnection(con->outObj, con->outIdx,  con->inObj, con->inIdx);
            getState().removeChild(con->getState(), nullptr);
        }
    }
    
    dragger.deselectAll();
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
    synchronise();
    main->valueTreeChanged();
}

void Canvas::redo() {
    patch.redo();
    synchronise();
    main->valueTreeChanged();
}
