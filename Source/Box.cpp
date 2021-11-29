#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include "MainComponent.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"

typedef struct _messresponder
{
    t_pd mr_pd;
    t_outlet *mr_outlet;
} t_messresponder;

typedef struct _message
{
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist *m_glist;
    t_clock *m_clock;
} t_message;

void ClickLabel::mouseDown(const MouseEvent & e)
{
    is_down = true;
    dragger.handleMouseDown(box, e);
}

void ClickLabel::mouseUp(const MouseEvent & e)
{
    is_down = false;
    dragger.handleMouseUp(box, e);
    
    if(e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600) {
        Edge::connecting_edge = nullptr;
    }
    
    Viewport* viewport = findParentComponentOfClass<Viewport>();
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    
    if(canvas) {
        
        auto pos = e.getEventRelativeTo(canvas).getPosition() - e.getPosition();
        auto bounds = Rectangle<int>(pos, pos + Point<int>(getParentWidth(), getParentHeight()));
        if(!canvas->getLocalBounds().contains(bounds)) {
            if(bounds.getRight() > canvas->getWidth())
                canvas->setSize(bounds.getRight() + 10, canvas->getHeight());
            
            if(bounds.getBottom() > canvas->getHeight())
                canvas->setSize(canvas->getWidth(), bounds.getBottom() + 10);
            
            
            if(bounds.getX() < 0.0f) {
                for(auto& box : canvas->findChildrenOfClass<Box>()) {
                    if(&box->text_label != this)
                        box->setTopLeftPosition(box->getX() - bounds.getX(), box->getY());
                    else
                        box->setTopLeftPosition(0, box->getY());
                }
                
            }
            if(bounds.getY() < 0) {
                for(auto& box : canvas->findChildrenOfClass<Box>()) {
                    if(&box->text_label != this)
                        box->setTopLeftPosition(box->getX(), box->getY() - bounds.getY());
                    else
                        box->setTopLeftPosition(box->getX(), 0);
                }
            }
            
            
        }
    }
    
    if(auto* box = dynamic_cast<Box*>(getParentComponent())) {
        box->update_position();
    }
    
    
}

void ClickLabel::mouseDrag(const MouseEvent & e)
{
    dragger.handleMouseDrag(e);
}


//==============================================================================
Box::Box(Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multi_dragger) : ValueTreeObject(tree), dragger(multi_dragger), text_label(this, multi_dragger)
{
    cnv = parent;
    
    if(!tree.hasProperty(Identifiers::box_x)) {
        auto pos = cnv->getMouseXYRelative();
        tree.setProperty(Identifiers::box_x, pos.getX(), nullptr);
        tree.setProperty(Identifiers::box_y, pos.getY(), nullptr);
    }
    
    if(!tree.hasProperty("ID")) {
        Uuid id;
        tree.setProperty("ID", id.toString(), nullptr);
    }
    
    setSize (100, 32);
    
    text_label.setEditable(false, true);
    text_label.setJustificationType(Justification::centred);
    
    addAndMakeVisible(&text_label);
    
    text_label.toBack();
    
    text_label.onTextChange = [this]() {
        String new_text = text_label.getText();
        getState().setProperty(Identifiers::box_name, new_text, nullptr);
    };
    
    
    
    rebuildObjects();
    
    auto edges = findChildrenOfClass<Edge>();
    int total = edges.size();
    int num_in = 0;
    for(auto& edge : edges) {
        auto edge_tree = edge->ValueTreeObject::getState();
        num_in += (int)edge_tree.getProperty("Input");
    }
    total_in = num_in;
    total_out = total - num_in;
    
    getState().sendPropertyChangeMessage(Identifiers::box_name);
    getState().sendPropertyChangeMessage(Identifiers::box_x);
}


Box::Box(t_pd* object, Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multi_dragger) : ValueTreeObject(tree), dragger(multi_dragger), text_label(this, multi_dragger)
{
    
}

Box::~Box()
{
}

void Box::remove_box() {
    if(pd_object) {
        cnv->get_pd()->removeObject(pd_object);
    }
    pd_object = nullptr;
    cnv->getState().removeChild(ValueTreeObject::getState(), nullptr);
}

ValueTreeObject* Box::factory (const juce::Identifier& id, const juce::ValueTree& tree)
{
    if(getState().indexOf(tree) < 0) return nullptr;
    
    if(id == Identifiers::edge) {
        auto* new_edge = new Edge(tree, this);
        addAndMakeVisible(new_edge);
        return static_cast<ValueTreeObject*>(new_edge);
    }
    if(id == Identifiers::canvas) {
        auto* canvas = new Canvas(tree, cnv->main);
        return static_cast<ValueTreeObject*>(canvas);
    }
    
    return nullptr;
}

void Box::mouseMove(const MouseEvent& e) {
    findParentComponentOfClass<Canvas>()->repaint();
}


void Box::set_type (String new_type)
{
    String arguments = new_type.fromFirstOccurrenceOf(" ", false, false);
    String type = new_type.upToFirstOccurrenceOf(" ", false, false);
    
    String obj_name = text_label.getText();
    
    if(type.isNotEmpty() && !getState().getProperty(Identifiers::exists)) {
        auto* pd = cnv->get_pd();
        
        if(pd_object) {
            pd_object = pd->renameObject(pd_object, obj_name);
        }
        else {
            pd_object = pd->createObject(obj_name, getX(), getY());
        }
    }
    else if(!getState().getProperty(Identifiers::exists)) {
        pd_object = nullptr;
    }
    
    update_ports();
    
    if(pd_object || true) { // temp
        graphics.reset(GUIComponent::create_gui(type, this));
    }
    
    if(graphics) {
        addAndMakeVisible(graphics.get());
        auto [w, h] = graphics->get_best_size();
        setSize(std::max(getWidth(), w + 8), h + 28);
        graphics->toBack();
    }
    else
    {
        setSize(getWidth(), 32);
    }

    
    if(type.isEmpty()) {
        setSize (100, 32);
    }
    
    // Handle subpatchers
    /*
    if(auto* subcnv = findChildOfClass<Canvas>(0)) {
        getState().removeChild(subcnv->getState(), nullptr);
    } */
    

    resized();
}

//==============================================================================
void Box::paint (Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);
    
    auto base_colour = findColour(TextButton::buttonColourId);
    
    bool is_over = getLocalBounds().contains(getMouseXYRelative());
    bool is_down = text_label.is_down;
    
    if (is_down || is_over || dragger.isSelected(this))
        base_colour = base_colour.contrasting (is_down ? 0.2f : 0.05f);
    
    
    g.setColour(base_colour);
    g.fillRoundedRectangle(rect.toFloat(), 2.0f);
    
    g.setColour(findColour(ComboBox::outlineColourId));
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    
}

void Box::moved()
{
    for(auto& edge : findChildrenOfClass<Edge>()) {
        edge->sendMovedResizedMessages(true, true);
    }
    
    if(pd_object) {
        cnv->get_pd()->moveObject(pd_object, getX(), getY());
    }
    
    
}

void Box::update_position()
{
    getState().setPropertyExcludingListener(this, Identifiers::box_x, getX(), nullptr);
    getState().setPropertyExcludingListener(this, Identifiers::box_y, getY(), nullptr);
}

void Box::resized()
{
    text_label.setBounds(4, 4, getWidth() - 8, 24);
    
    if(graphics) {
        graphics->setBounds(4, 28, getWidth() - 8, getHeight() - 32);
    }
    
    
    int index = 0;
    for(auto& edge : findChildrenOfClass<Edge>()) {
        
        auto& state = edge->ValueTreeObject::getState();
        bool is_input = state.getProperty(Identifiers::edge_in);
        
        int position = index < total_in ? index : index - total_in;
        
        bool sideways = false;
        
        int total = is_input ? total_in : total_out;
            
        float newY = is_input ? 4 : getHeight() - 4;
        float newX = position * ((getWidth() - 32) / (total - 1 + (total == 1))) + 16;
        
        edge->setCentrePosition(newX, newY);
        edge->setSize(8, 8);
        
        index++;
    }
}

void Box::valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property)
{
    if(treeWhosePropertyHasChanged != getState()) return;
    
    if(property == Identifiers::box_x || property == Identifiers::box_y) {
        setTopLeftPosition(getState().getProperty(Identifiers::box_x), getState().getProperty(Identifiers::box_y));
        update_position();
    }
    if(property == Identifiers::box_name) {
        String new_name = treeWhosePropertyHasChanged.getProperty(Identifiers::box_name);
        int new_size = text_label.getFont().getStringWidth(new_name) + (text_label.getHeight() * 1.35f);
        setSize(std::max(50, new_size), getHeight());
        text_label.setText(new_name, NotificationType::dontSendNotification);
        set_type(new_name);
    }
}

void Box::update_ports() {
    
    int old_total_in = 0;
    int old_total_out = 0;
    
    for(auto& edge : findChildrenOfClass<Edge>()) {
        (bool)edge->ValueTreeObject::getState().getProperty("Input") ? old_total_in++ : old_total_out++;
    }
    
    total_in = 0;
    total_out = 0;
    
    t_object* validobject = nullptr;
    
    if(pd_object) {
        validobject = pd_checkobject(pd_object);
        if(validobject) {
            // Count number of inlets and outlets on the object returned
            total_in = libpd_ninlets(validobject);
            total_out = libpd_noutlets(validobject);
        }
    }


    while(total_in < old_total_in) {
        int idx = old_total_in-1;
        getState().removeChild(idx, nullptr);
        old_total_in--;
    }
    while(total_in > old_total_in) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edge_in, true, nullptr);
        edge.setProperty(Identifiers::edge_id, id.toString(), nullptr);
        
        getState().addChild(edge, old_total_in, nullptr);
        old_total_in++;
    }
    
    while(total_out < old_total_out) {
        int idx = old_total_out-1;
        getState().removeChild(total_in + idx, nullptr);
        old_total_out--;
    }
    while(total_out > old_total_out) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edge_in, false, nullptr);
        edge.setProperty(Identifiers::edge_id, id.toString(), nullptr);
        
        getState().addChild(edge, total_in + old_total_out, nullptr);
        old_total_out++;
    }
    
    int num_in = 0;
    int num_out = 0;
    
    for(int i = 0; i < total_in + total_out; i++) {
        auto edge = getState().getChild(i);
        if(!edge.isValid()) continue;
        
        bool input = edge.getProperty(Identifiers::edge_in);
        
        bool is_signal = validobject && (i < total_in ? libpd_issignalinlet(validobject, i) : libpd_issignaloutlet(validobject, i - total_in));
        bool was_signal = edge.getProperty(Identifiers::edge_ctx);
        
        if(is_signal != was_signal) {
            getState().removeChild(edge, nullptr);
            getState().addChild(edge, i, nullptr);
        }
        
        edge.setProperty(Identifiers::edge_idx, input ? num_in : num_out, nullptr);
        edge.setProperty(Identifiers::edge_ctx, is_signal, nullptr);
        
        num_in += input;
        num_out += !input;
    }
    
    
    
    resized();
}
