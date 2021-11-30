#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include "MainComponent.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"

void ClickLabel::mouseDown(const MouseEvent & e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if(canvas->getState().getProperty(Identifiers::is_graph)) return;
    
    is_down = true;
    dragger.handleMouseDown(box, e);
    
    
}

void ClickLabel::mouseUp(const MouseEvent & e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if(canvas->getState().getProperty(Identifiers::is_graph)) return;
    
    is_down = false;
    dragger.handleMouseUp(box, e);
    
    if(e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600) {
        Edge::connectingEdge = nullptr;
    }
    
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
                    if(&box->textLabel != this)
                        box->setTopLeftPosition(box->getX() - bounds.getX(), box->getY());
                    else
                        box->setTopLeftPosition(0, box->getY());
                }
                
            }
            if(bounds.getY() < 0) {
                for(auto& box : canvas->findChildrenOfClass<Box>()) {
                    if(&box->textLabel != this)
                        box->setTopLeftPosition(box->getX(), box->getY() - bounds.getY());
                    else
                        box->setTopLeftPosition(box->getX(), 0);
                }
            }
            
            
        }
    }
    
    if(auto* box = dynamic_cast<Box*>(getParentComponent())) {
        box->updatePosition();
    }
    
    
}

void ClickLabel::mouseDrag(const MouseEvent & e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if(canvas->getState().getProperty(Identifiers::is_graph)) return;
    
    dragger.handleMouseDrag(e);
}


//==============================================================================
Box::Box(Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multiDragger) : ValueTreeObject(tree), textLabel(this, multiDragger), dragger(multiDragger)
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
    
    textLabel.setEditable(false, true);
    textLabel.setJustificationType(Justification::centred);
    
    addAndMakeVisible(&textLabel);
    
    textLabel.toBack();
    
    textLabel.onTextChange = [this]() {
        String new_text = textLabel.getText();
        getState().setProperty(Identifiers::box_name, new_text, nullptr);
    };
    
    
    rebuildObjects();
    
    auto edges = findChildrenOfClass<Edge>();
    int total = edges.size();
    int numIn = 0;
    for(auto& edge : edges) {
        auto edge_tree = edge->ValueTreeObject::getState();
        numIn += (int)edge_tree.getProperty("Input");
    }
    numInputs = numIn;
    numOutputs = total - numIn;
    
    getState().sendPropertyChangeMessage(Identifiers::box_name);
    getState().sendPropertyChangeMessage(Identifiers::box_x);
}


Box::~Box()
{
}

void Box::remove(bool clear_pd) {
    
    cnv->removeMouseListener(this);
    
    if(pd_object && clear_pd) {
        cnv->patch.removeObject(pd_object);
    }
    
    if(graphics && (graphics->getGUI().getType() == pd::Gui::Type::GraphOnParent || graphics->getGUI().getType() == pd::Gui::Type::Subpatch)) {
        
        // TODO: find the tabs and close them!!
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


void Box::setType (String new_type)
{
    String arguments = new_type.fromFirstOccurrenceOf(" ", false, false);
    String type = new_type.upToFirstOccurrenceOf(" ", false, false);

    // TODO: this bit can be improved
    // Wrap t_pd* in a pd::Object
    bool is_gui = GUIComponent::is_gui(type);
    
    if(type.isNotEmpty() && !getState().getProperty(Identifiers::exists)) {
        auto* pd = &cnv->patch;
        
        
        // Pd doesn't normally allow changing between gui and non-gui objects
        if((pd_object && graphics.get() != nullptr) || is_gui) {
            pd->removeObject(pd_object);
            pd_object = pd->createObject(new_type, getX(), getY());
            graphics.reset(nullptr);
        }
        else if(pd_object) {
            pd_object = pd->renameObject(pd_object, new_type);
        }
        else {
            pd_object = pd->createObject(new_type, getX(), getY());
        }
    }
    else if(!getState().getProperty(Identifiers::exists)) {
        pd_object = nullptr;
    }
    
    
    updatePorts();
    
    if(pd_object && is_gui) {
        graphics.reset(GUIComponent::create_gui(type, this));
    }
    
    if(graphics) {
        addAndMakeVisible(graphics.get());
        auto [w, h] = graphics->getBestSize();
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

    resized();
}

//==============================================================================
void Box::paint (Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);
    
    auto base_colour = findColour(TextButton::buttonColourId);
    
    bool is_over = getLocalBounds().contains(getMouseXYRelative());
    bool is_down = textLabel.is_down;
    
    if (is_down || is_over || dragger.isSelected(this))
        base_colour = base_colour.contrasting (is_down ? 0.2f : 0.05f);
    
    
    g.setColour(base_colour);
    g.fillRoundedRectangle(rect.toFloat(), 2.0f);
    
    g.setColour(findColour(ComboBox::outlineColourId));
    
    if(graphics && graphics->getGUI().getType() == pd::Gui::Type::Comment) {
        std::cout << "todo: comment style";
        
    }
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
}

void Box::moved()
{
    for(auto& edge : findChildrenOfClass<Edge>()) {
        edge->sendMovedResizedMessages(true, true);
    }
    
    if(pd_object) {
        
        cnv->patch.moveObject(pd_object, getX(), getY());
    }
}

void Box::updatePosition()
{
    getState().setPropertyExcludingListener(this, Identifiers::box_x, getX(), nullptr);
    getState().setPropertyExcludingListener(this, Identifiers::box_y, getY(), nullptr);
}

void Box::resized()
{
    textLabel.setBounds(4, 4, getWidth() - 8, 24);
    
    if(graphics) {
        graphics->setBounds(4, 28, getWidth() - 8, getHeight() - 32);
    }
    
    int index = 0;
    for(auto& edge : findChildrenOfClass<Edge>()) {
        
        auto& state = edge->ValueTreeObject::getState();
        bool is_input = state.getProperty(Identifiers::edge_in);
        
        int position = index < numInputs ? index : index - numInputs;
        
        int total = is_input ? numInputs : numOutputs;
            
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
        updatePosition();
    }
    if(property == Identifiers::box_name) {
        String newName = treeWhosePropertyHasChanged.getProperty(Identifiers::box_name);
        int newSize = textLabel.getFont().getStringWidth(newName) + (textLabel.getHeight() * 1.35f);
        setSize(std::max(50, newSize), getHeight());
        textLabel.setText(newName, NotificationType::dontSendNotification);
        setType(newName);
    }
}

void Box::updatePorts() {
    
    int oldnumInputs = 0;
    int oldnumOutputs = 0;
    
    for(auto& edge : findChildrenOfClass<Edge>()) {
        (bool)edge->ValueTreeObject::getState().getProperty("Input") ? oldnumInputs++ : oldnumOutputs++;
    }
    
    numInputs = 0;
    numOutputs = 0;
    
    t_object* validobject = nullptr;
    
    if(pd_object) {
        validobject = pd_checkobject(pd_object);
        if(validobject) {
            // Count number of inlets and outlets on the object returned
            numInputs = libpd_ninlets(validobject);
            numOutputs = libpd_noutlets(validobject);
        }
    }


    while(numInputs < oldnumInputs) {
        int idx = oldnumInputs-1;
        getState().removeChild(idx, nullptr);
        oldnumInputs--;
    }
    while(numInputs > oldnumInputs) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edge_in, true, nullptr);
        edge.setProperty(Identifiers::edge_id, id.toString(), nullptr);
        
        getState().addChild(edge, oldnumInputs, nullptr);
        oldnumInputs++;
    }
    
    while(numOutputs < oldnumOutputs) {
        int idx = oldnumOutputs-1;
        getState().removeChild(numInputs + idx, nullptr);
        oldnumOutputs--;
    }
    while(numOutputs > oldnumOutputs) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edge_in, false, nullptr);
        edge.setProperty(Identifiers::edge_id, id.toString(), nullptr);
        
        getState().addChild(edge, numInputs + oldnumOutputs, nullptr);
        oldnumOutputs++;
    }
    
    int num_in = 0;
    int num_out = 0;
    
    for(int i = 0; i < numInputs + numOutputs; i++) {
        auto edge = getState().getChild(i);
        if(!edge.isValid()) continue;
        
        bool input = edge.getProperty(Identifiers::edge_in);
        
        bool is_signal = validobject && (i < numInputs ? libpd_issignalinlet(validobject, i) : libpd_issignaloutlet(validobject, i - numInputs));
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
