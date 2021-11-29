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

void Canvas::load_state() {
    // Clear canvas
    for(auto& box : findChildrenOfClass<Box>()) {
        // set clear_pd to false: no need to clean up pd's objects, pd will take care of that
        box->remove_box(false);
    }
    
    // Removing all boxes should automatically remove all connections
    jassert(findChildrenOfClass<Connection>().size() == 0);
    
    String patch = main->pd.getCanvasContent();
    
    Array<Box*> boxes;
    
    // Go through all the boxes in the pd patch, and add GUI components for them
    for(auto& object : main->pd.getPatch().getObjects()) {
        
        auto box = ValueTree(Identifiers::box);
        
        // Setting the exists flag tells the box that this refers to an objects that already exists in the pd patch
        box.setProperty(Identifiers::exists, true, nullptr);
        box.setProperty(Identifiers::box_x, object.getBounds()[0], nullptr);
        box.setProperty(Identifiers::box_y, object.getBounds()[1], nullptr);
        
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
    
    auto lines = StringArray::fromLines(patch);
    
    // Connections are easier to read from the save file than to extract from the loaded patch
    // These connections already exists in libpd, we just need to add GUI components for them
    for(auto& line : lines) {
        if(line.startsWith("#X connect")) {
            auto segments = StringArray::fromTokens(line.fromFirstOccurrenceOf("#X connect ", false, false), " ", "\"");
            
            int end_box   = segments[0].getIntValue();
            int outlet    = segments[1].getIntValue();
            int start_box = segments[2].getIntValue();
            int inlet     = segments[3].getIntValue();
            
            auto start_edges = boxes[start_box]->findChildrenOfClass<Edge>();
            auto end_edges = boxes[end_box]->findChildrenOfClass<Edge>();
            
            auto new_connection = ValueTree(Identifiers::connection);
            
            String start_id = start_edges[inlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            String end_id = end_edges[boxes[end_box]->total_in + outlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            
            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
            
            new_connection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            getState().appendChild(new_connection, nullptr);
            
            // Return to normal operation
            new_connection.setProperty(Identifiers::exists, false, nullptr);
        }
        
    }
    
}

void Canvas::load_patch(String patch) {
    dragger.deselectAll();
    
    // Clear canvas
    for(auto& box : findChildrenOfClass<Box>()) {
        // set clear_pd to false: no need to clean up pd's objects, pd will take care of that
        box->remove_box(false);
    }
    
    // Removing all boxes should automatically remove all connections
    jassert(findChildrenOfClass<Connection>().size() == 0);
    
    // Create the pd save file
    auto temp_patch = File::createTempFile(".pd");
    temp_patch.replaceWithText(patch);
    
    // Load the patch into libpd
    // This way we don't have to parse the patch manually (which is complicated for arrays, subpatches, etc.)
    // Instead we can load the patch and iterate through it to add GUI components
    main->pd.openPatch(temp_patch.getParentDirectory().getFullPathName().toStdString(), temp_patch.getFileName().toStdString());
    
    Array<Box*> boxes;
    
    // Go through all the boxes in the pd patch, and add GUI components for them
    for(auto& object : main->pd.getPatch().getObjects()) {
        
        auto box = ValueTree(Identifiers::box);
        
        // Setting the exists flag tells the box that this refers to an objects that already exists in the pd patch
        box.setProperty(Identifiers::exists, true, nullptr);
        box.setProperty(Identifiers::box_x, object.getBounds()[0], nullptr);
        box.setProperty(Identifiers::box_y, object.getBounds()[1], nullptr);
        
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
    
    auto lines = StringArray::fromLines(patch);
    
    // Connections are easier to read from the save file than to extract from the loaded patch
    // These connections already exists in libpd, we just need to add GUI components for them
    for(auto& line : lines) {
        if(line.startsWith("#X connect")) {
            auto segments = StringArray::fromTokens(line.fromFirstOccurrenceOf("#X connect ", false, false), " ", "\"");
            
            int end_box   = segments[0].getIntValue();
            int outlet    = segments[1].getIntValue();
            int start_box = segments[2].getIntValue();
            int inlet     = segments[3].getIntValue();
            
            auto start_edges = boxes[start_box]->findChildrenOfClass<Edge>();
            auto end_edges = boxes[end_box]->findChildrenOfClass<Edge>();
            
            auto new_connection = ValueTree(Identifiers::connection);
            
            String start_id = start_edges[inlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            String end_id = end_edges[boxes[end_box]->total_in + outlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
            
            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
            
            new_connection.setProperty(Identifiers::exists, true, nullptr); // let the connection know that this connection already exists
            
            getState().appendChild(new_connection, nullptr);
            
            // Return to normal operation
            new_connection.setProperty(Identifiers::exists, false, nullptr);
        }
        
    }
    
    
}

void Canvas::mouseDown(const MouseEvent& e)
{
    if(!ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
        auto* source = e.originalComponent;
        
        drag_start_position = e.getMouseDownPosition();
        
        if(dynamic_cast<Connection*>(source)) {
            lasso.beginLasso(e.getEventRelativeTo(this), &dragger);
        }
        else if(source == this){
            Edge::connecting_edge = nullptr;
            lasso.beginLasso(e, &dragger);
            
            for(auto& con : findChildrenOfClass<Connection>()) {
                con->is_selected = false;
                con->repaint();
            }
        }
    }
    else
    {
        auto& lasso_selection = dragger.getLassoSelection();
        bool has_selection = lasso_selection.getNumSelected();
        bool multiple = lasso_selection.getNumSelected() > 1;
        
        popupmenu.clear();
        popupmenu.addItem(1, "Open", has_selection && !multiple && !lasso_selection.getSelectedItem(0)->findChildrenOfClass<Canvas>().isEmpty());
        popupmenu.addSeparator();
        popupmenu.addItem(4,"Cut", has_selection);
        popupmenu.addItem(5,"Copy", has_selection);
        popupmenu.addItem(6,"Duplicate", has_selection);
        popupmenu.addItem(7,"Delete", has_selection);
        popupmenu.setLookAndFeel(&getLookAndFeel());
    
        auto callback = [this, &lasso_selection](int result) {
            if(result < 1) return;
            
            switch (result) {
                case 1: {
                auto* new_cnv = lasso_selection.getSelectedItem(0)->findChildrenOfClass<Canvas>()[0];
                main->add_tab(new_cnv);
                    break;
                }
                case 4:
                SystemClipboard::copyTextToClipboard(copy_selection());
                remove_selection();
                    break;
                case 5:
                SystemClipboard::copyTextToClipboard(copy_selection());
                    break;
                case 6: {
                auto copy = copy_selection();
                paste_selection(copy);
                    break;
                }
                case 7:
                remove_selection();
                    break;
            }
        };
        
        
        popupmenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent (e.originalComponent), ModalCallbackFunction::create(callback));

    }
}

Edge* Canvas::find_edge_by_id(String ID) {
    for(auto& box : findChildrenOfClass<Box>()) {
        auto tree = box->getState().getChildWithProperty(Identifiers::edge_id, ID);
        if(auto* result = box->findObjectForTree<Edge>(tree))
            return result;
    };
    
    return nullptr;
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    auto* source = e.originalComponent;
    
    if(dynamic_cast<Connection*>(source)) {
        lasso.dragLasso(e.getEventRelativeTo(this));
    }
    else if(source == this){
        Edge::connecting_edge = nullptr;
        lasso.dragLasso(e);
        
        for(auto& con : findChildrenOfClass<Connection>()) {
            
            Line<int> path(con->start->get_canvas_bounds().getCentre(), con->end->get_canvas_bounds().getCentre());
            
            bool intersect = false;
            for(float i = 0; i < 1; i += 0.005) {
                if(lasso.getBounds().contains(path.getPointAlongLineProportionally(i))) {
                    intersect = true;
                }
            }
            
            if(!con->is_selected && intersect) {
                con->is_selected = true;
                con->repaint();
            }
            else if(con->is_selected && !intersect) {
                con->is_selected = false;
                con->repaint();
            }
        }
    }
    
    if(connecting_with_drag) repaint();
}

void Canvas::mouseUp(const MouseEvent& e)
{
    
    if(connecting_with_drag) {
        auto pos = e.getEventRelativeTo(this).getPosition();
        auto all_edges = get_all_edges();
        
        Edge* nearest_edge = nullptr;
        
        for(auto& edge : all_edges) {
            
            auto bounds = edge->get_canvas_bounds().expanded(150, 150);
            if(bounds.contains(pos)) {
                if(!nearest_edge) nearest_edge = edge;
                
                auto old_pos = nearest_edge->get_canvas_bounds().getCentre();
                auto new_pos = bounds.getCentre();
                nearest_edge = new_pos.getDistanceFrom(pos) < old_pos.getDistanceFrom(pos) ? edge : nearest_edge;
            }
        }

        if(nearest_edge) nearest_edge->create_connection();
        
        Edge::connecting_edge = nullptr;
        connecting_with_drag = false;
    }
    lasso.endLasso();
}

//==============================================================================
void Canvas::paintOverChildren (Graphics& g)
{
    if(Edge::connecting_edge && Edge::connecting_edge->box->cnv == this) {
        Point<float> mouse_pos = getMouseXYRelative().toFloat();
        Point<int> edge_pos =  Edge::connecting_edge->get_canvas_bounds().getPosition();
        
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
    
    if(key.getTextCharacter() == 'n') {
        auto box = ValueTree(Identifiers::box);
        getState().appendChild(box, nullptr);
        return true;
    }
    
    if(key.getKeyCode() == KeyPress::backspaceKey) {
        remove_selection();
        return true;
    }
    // cmd-c
    if(key.getModifiers().isCommandDown() && key.isKeyCode(67)) {
        
        SystemClipboard::copyTextToClipboard(copy_selection());
        
        return true;
    }
    // cmd-v
    if(key.getModifiers().isCommandDown() && key.isKeyCode(86)) {
        paste_selection(SystemClipboard::getTextFromClipboard());
        return true;
    }
    // cmd-x
    if(key.getModifiers().isCommandDown() && key.isKeyCode(88)) {
        
        SystemClipboard::copyTextToClipboard(copy_selection());
        remove_selection();
        
        return true;
    }
    // cmd-d
    if(key.getModifiers().isCommandDown() && key.isKeyCode(68)) {
        
        String copied = copy_selection();
        paste_selection(copied);
        
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

String Canvas::copy_selection() {
    ValueTree copy_tree = ValueTree("Canvas");
    Array<std::pair<Connection*, ValueTree>> found_connections;

    for(auto& con : findChildrenOfClass<Connection>()) {
        int num_points_found = 0;
        for(auto& box : findChildrenOfClass<Box>()) {
            if(!dragger.isSelected(box)) continue;
            for(auto& edge : box->findChildrenOfClass<Edge>()) {
                if(con->start == edge || con->end == edge) {
                    num_points_found++;
                }
            }
        }
        
        if(num_points_found > 1) {
            ValueTree connection_tree("Connection");
            connection_tree.copyPropertiesFrom(con->getState(), nullptr);
            found_connections.add({con, connection_tree});
        }
    }
        
    Array<std::pair<Box*, ValueTree&>> found_boxes;
    for(auto& box : findChildrenOfClass<Box>()) {
        if(dragger.isSelected(box)) {
            ValueTree box_tree(Identifiers::box);
            box_tree.copyPropertiesFrom(box->getState(), nullptr);
            
            found_boxes.add({box, box_tree});
            
            for(auto& edge : box->findChildrenOfClass<Edge>()) {
                Uuid new_id;
                auto edge_copy = edge->ValueTreeObject::getState().createCopy();

                for(auto& [con, con_tree] : found_connections) {
                    if(con_tree.getProperty(Identifiers::start_id) == edge_copy.getProperty(Identifiers::edge_id) || con_tree.getProperty(Identifiers::end_id) == edge_copy.getProperty(Identifiers::edge_id))
                    {
                        bool start_id = con_tree.getProperty(Identifiers::start_id) == edge_copy.getProperty(Identifiers::edge_id);
                        con_tree.setProperty(start_id ? Identifiers::start_id : Identifiers::end_id , new_id.toString(), nullptr);
                    }
                }
                edge_copy.setProperty(Identifiers::edge_id, new_id.toString(), nullptr);
                box_tree.appendChild(edge_copy, nullptr);
            }
            copy_tree.appendChild(box_tree, nullptr);
        }
    }
    
    for(auto& [con, con_tree] : found_connections) {
        copy_tree.appendChild(con_tree, nullptr);
    }

    return copy_tree.toXmlString();
}

void Canvas::remove_selection() {
    for(auto& sel : dragger.getLassoSelection()) {
        removeMouseListener(sel);
        dynamic_cast<Box*>(sel)->remove_box();
    }
    
    for(auto& con : findChildrenOfClass<Connection>()) {
        if(con->is_selected) {
            removeMouseListener(con);
           
            get_pd()->removeConnection(con->out_obj, con->out_idx,  con->in_obj, con->in_idx);
            getState().removeChild(con->getState(), nullptr);
        }
    }
    
    dragger.deselectAll();
}

void Canvas::paste_selection(String to_paste) {
    String text = to_paste;
    
    auto tree = ValueTree::fromXml(text);
    for(auto child : tree) {
        if(child.hasProperty(Identifiers::box_x)) {
            int old_x = child.getProperty(Identifiers::box_x);
            int old_y = child.getProperty(Identifiers::box_y);
            child.setProperty(Identifiers::box_x, old_x + 30, nullptr);
            child.setProperty(Identifiers::box_y, old_y + 30, nullptr);
        }
        
        getState().appendChild(child.createCopy(), nullptr);
    }
}

PlugData* Canvas::get_pd()
{
    return &main->pd;
}


Array<Edge*> Canvas::get_all_edges() {
    Array<Edge*> all_edges;
    for(auto* box : findChildrenOfClass<Box>()) {
       for(auto* edge : box->findChildrenOfClass<Edge>())
           all_edges.add(edge);
    };
    
    return all_edges;
}

void Canvas::undo() {
    main->pd.undo();
    load_state();
    main->valueTreeChanged();
}

void Canvas::redo() {
    main->pd.redo();
    load_state();
    main->valueTreeChanged();
}
