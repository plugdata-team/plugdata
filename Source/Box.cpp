#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include "PluginEditor.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"

void ClickLabel::mouseDown(const MouseEvent & e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if(canvas->getProperty(Identifiers::isGraph)) return;
    
    isDown = true;
    dragger.handleMouseDown(box, e);
    
    
}

void ClickLabel::mouseUp(const MouseEvent & e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if(canvas->getProperty(Identifiers::isGraph)) return;
    
    isDown = false;
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
    if(canvas->getProperty(Identifiers::isGraph)) return;
    
    dragger.handleMouseDrag(e);
}


//==============================================================================
Box::Box(Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multiDragger) : ValueTreeObject(tree), textLabel(this, multiDragger), dragger(multiDragger)
{
    cnv = parent;
    
    if(!tree.hasProperty(Identifiers::boxX)) {
        auto pos = cnv->getMouseXYRelative();
        tree.setProperty(Identifiers::boxX, pos.getX(), nullptr);
        tree.setProperty(Identifiers::boxY, pos.getY(), nullptr);
    }
    
    if(!tree.hasProperty("ID")) {
        Uuid id;
        tree.setProperty("ID", id.toString(), nullptr);
    }
    
    setSize (100, 32);
    
    auto& [minW, minH, maxW, maxH] = defaultLimits;
    restrainer.setSizeLimits(minW, minH, maxW, maxH);
    
    resizer.reset(new ResizableBorderComponent(this, &restrainer));
    
    // Uncomment to enable resizing
    // doesn't work right yet!
    //addAndMakeVisible(resizer.get());
    
    textLabel.setEditable(false, true);
    textLabel.setJustificationType(Justification::centred);
    
    addAndMakeVisible(&textLabel);
    
    textLabel.toBack();
    
    textLabel.onTextChange = [this]() {
        String new_text = textLabel.getText();
        setProperty(Identifiers::boxName, new_text);
    };
    
    
    rebuildObjects();
    
    auto edges = findChildrenOfClass<Edge>();
    int total = edges.size();
    int numIn = 0;
    for(auto& edge : edges) {
        auto edge_tree = edge->getObjectState();
        numIn += (int)edge_tree.getProperty("Input");
    }
    numInputs = numIn;
    numOutputs = total - numIn;
    
    sendPropertyChangeMessage(Identifiers::boxName);
    sendPropertyChangeMessage(Identifiers::boxX);
    
    
}


Box::~Box()
{
}



ValueTreeObject* Box::factory (const juce::Identifier& id, const juce::ValueTree& tree)
{
    if(indexOf(tree) < 0) return nullptr;
    
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
    
    if(type.isNotEmpty() && !getProperty(Identifiers::exists)) {
        auto* pd = &cnv->patch;
        
        // Pd doesn't normally allow changing between gui and non-gui objects
        if(pdObject && (graphics.get() != nullptr || pdObject->getType() != pd::Type::Undefined) && pdObject->getType() != pd::Type::Comment) {
            pd->removeObject(pdObject.get());
            pdObject = pd->createObject(new_type, getX() / Canvas::zoom, getY() / Canvas::zoom);
        }
        else if(pdObject) {
            pdObject = pd->renameObject(pdObject.get(), new_type);
        }
        else {
            pdObject = pd->createObject(new_type, getX() / Canvas::zoom,  getY() / Canvas::zoom);
        }
    }
    else if(!getProperty(Identifiers::exists)) {
        pdObject = nullptr;
    }
    
    updatePorts();
    
    if(pdObject) {
        graphics.reset(GUIComponent::create_gui(type, this));
        
        if(graphics) {
            auto [minW, minH, maxW, maxH] = graphics->getSizeLimits();
            restrainer.setSizeLimits(minW, minH, maxW, maxH);
            
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(std::max(getWidth(), w + 8), h + 28);
            graphics->toBack();
        }
        else {
            setSize(getWidth(), 32);
            auto& [minW, minH, maxW, maxH] = defaultLimits;
        
            auto bestWidth = textLabel.getFont().getStringWidth(type) + getHeight();
            restrainer.setSizeLimits(bestWidth, minH, std::max(700, bestWidth * 2), maxH);
        }
    }
    else {
        setSize(getWidth(), 32);
        auto& [minW, minH, maxW, maxH] = defaultLimits;
        
        auto bestWidth = textLabel.getFont().getStringWidth(type) + getHeight();
        restrainer.setSizeLimits(bestWidth, minH, std::max(700, bestWidth * 2), maxH);
    }
    
    if(type.isEmpty()) {
        setSize (100, 32);
    }
    
    if(new_type.startsWith("comment ")) {
        textLabel.setText(new_type.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }

    resized();
}

//==============================================================================
void Box::paint (Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);
    
    auto baseColour = findColour(TextButton::buttonColourId);
    
    bool isOver = getLocalBounds().contains(getMouseXYRelative());
    bool isDown = textLabel.isDown;
    
    if (isDown || isOver || dragger.isSelected(this))
        baseColour = baseColour.contrasting (isDown ? 0.2f : 0.05f);
    
    if(graphics && graphics->getGUI().getType() == pd::Type::Comment) {
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawRect(rect.toFloat(), 0.5f);
    }
    else {
        g.setColour(baseColour);
        g.fillRoundedRectangle(rect.toFloat(), 2.0f);
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }
    

}

void Box::moved()
{
    for(auto& edge : findChildrenOfClass<Edge>()) {
        edge->sendMovedResizedMessages(true, true);
    }
}

void Box::updatePosition()
{
    setPropertyExcludingListener(this, Identifiers::boxX, getX());
    setPropertyExcludingListener(this, Identifiers::boxY, getY());
}

void Box::resized()
{
    textLabel.setBounds(4, 4, getWidth() - 8, 24);
    
    if(graphics) {
        graphics->setBounds(4, 28, getWidth() - 8, getHeight() - 32);
    }
    
    if(resizer) {
        resizer->setBounds(getLocalBounds());
    }
    
    int index = 0;
    for(auto& edge : findChildrenOfClass<Edge>()) {
        
        auto& state = edge->getObjectState();
        bool is_input = state.getProperty(Identifiers::edgeIsInput);
        
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
    if(treeWhosePropertyHasChanged != getObjectState()) return;
    
    if(property == Identifiers::boxX || property == Identifiers::boxY) {
        setTopLeftPosition(getProperty(Identifiers::boxX), getProperty(Identifiers::boxY));
        updatePosition();
    }
    if(property == Identifiers::boxName) {
        String newName = treeWhosePropertyHasChanged.getProperty(Identifiers::boxName);
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
        (bool)edge->getProperty("Input") ? oldnumInputs++ : oldnumOutputs++;
    }
    
    numInputs = 0;
    numOutputs = 0;
    
    if(pdObject) {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }


    while(numInputs < oldnumInputs) {
        int idx = oldnumInputs-1;
        removeChild(idx);
        oldnumInputs--;
    }
    while(numInputs > oldnumInputs) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edgeIsInput, true, nullptr);
        edge.setProperty(Identifiers::edgeID, id.toString(), nullptr);
        
        addChild(edge, oldnumInputs);
        oldnumInputs++;
    }
    
    while(numOutputs < oldnumOutputs) {
        int idx = oldnumOutputs-1;
        removeChild(numInputs + idx);
        oldnumOutputs--;
    }
    while(numOutputs > oldnumOutputs) {
        Uuid id;
        ValueTree edge = ValueTree(Identifiers::edge);
        edge.setProperty(Identifiers::edgeIsInput, false, nullptr);
        edge.setProperty(Identifiers::edgeID, id.toString(), nullptr);
        
        addChild(edge, numInputs + oldnumOutputs);
        oldnumOutputs++;
    }
    
    int num_in = 0;
    int num_out = 0;
    
    for(int i = 0; i < numInputs + numOutputs; i++) {
        auto edge = getChild(i);
        if(!edge.isValid()) continue;
        
        bool input = edge.getProperty(Identifiers::edgeIsInput);
        
        bool is_signal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);
        bool was_signal = edge.getProperty(Identifiers::edgeSignal);
        
        if(is_signal != was_signal) {
            removeChild(edge);
            addChild(edge, i);
        }
        
        edge.setProperty(Identifiers::edgeIdx, input ? num_in : num_out, nullptr);
        edge.setProperty(Identifiers::edgeSignal, is_signal, nullptr);
        
        num_in += input;
        num_out += !input;
    }
    
    
    
    resized();
}
