#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include "Connection.h"
#include "PluginEditor.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"


//==============================================================================
Box::Box(Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multiDragger) : ValueTreeObject(tree), textLabel(this, multiDragger), dragger(multiDragger)
{
    cnv = parent;
    
    if(!tree.hasProperty(Identifiers::boxX)) {
        auto pos = cnv->getMouseXYRelative();
        tree.setProperty(Identifiers::boxX, pos.getX(), nullptr);
        tree.setProperty(Identifiers::boxY, pos.getY(), nullptr);
    }
    
    setSize (100, 32);
    
    auto& [minW, minH, maxW, maxH] = defaultLimits;
    restrainer.setSizeLimits(minW, minH, maxW, maxH);
    
    resizer.reset(new ResizableBorderComponent(this, &restrainer));
    
    // Uncomment to enable resizing
    // doesn't work right yet!
    //addAndMakeVisible(resizer.get());
    
    addAndMakeVisible(&textLabel);
    
    textLabel.toBack();
    
    textLabel.onTextChange = [this]() {
        String newText = textLabel.getText();
        setProperty(Identifiers::boxName, newText);
    };
    
    
    rebuildObjects();
    
    auto edges = findChildrenOfClass<Edge>();
    int total = edges.size();
    int numIn = 0;
    for(auto& edge : edges) {
        auto edgeTree = edge->getObjectState();
        numIn += (int)edgeTree.getProperty("Input");
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
        auto* newEdge = new Edge(tree, this);
        addAndMakeVisible(newEdge);
        return static_cast<ValueTreeObject*>(newEdge);
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


void Box::setType (String newType)
{
    String arguments = newType.fromFirstOccurrenceOf(" ", false, false);
    String type = newType.upToFirstOccurrenceOf(" ", false, false);
    
    if(type.isNotEmpty() && !getProperty(Identifiers::exists)) {
        auto* pd = &cnv->patch;
        
        // Pd doesn't normally allow changing between gui and non-gui objects
        if(pdObject && (graphics.get() != nullptr || pdObject->getType() != pd::Type::Undefined) && pdObject->getType() != pd::Type::Comment) {
            pd->removeObject(pdObject.get());
            pdObject = pd->createObject(newType, getX() / Canvas::zoomX, getY() / Canvas::zoomY);
        }
        else if(pdObject) {
            pdObject = pd->renameObject(pdObject.get(), newType);
        }
        else {
            pdObject = pd->createObject(newType, getX() / Canvas::zoomX,  getY() / Canvas::zoomY);
        }
    }
    else if(!getProperty(Identifiers::exists)) {
        pdObject = nullptr;
    }
    
    updatePorts();
    
    if(pdObject) {
        graphics.reset(GUIComponent::createGui(type, this));
        
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
    
    if(newType.startsWith("comment ")) {
        textLabel.setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }

    resized();
}

//==============================================================================
void Box::paint (Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);
    
    auto baseColour = findColour(TextButton::buttonColourId);
    auto outlineColour = findColour(ComboBox::outlineColourId);
    
    bool isOver = getLocalBounds().contains(getMouseXYRelative());
    bool isDown = textLabel.isDown;
    
    bool selected = dragger.isSelected(this);
    if (isDown || isOver || selected) {
        baseColour = baseColour.contrasting (isDown ? 0.2f : 0.05f);

    }
    if(selected) {
        outlineColour = MainLook::highlightColour;
    }
    
    if(graphics && graphics->getGUI().getType() == pd::Type::Comment) {
        g.setColour(outlineColour);
        g.drawRect(rect.toFloat(), 0.5f);
    }
    else {
        g.setColour(baseColour);
        g.fillRoundedRectangle(rect.toFloat(), 2.0f);

        g.setColour(outlineColour);
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
    bool hideLabel = graphics && graphics->getGUI().getType() != pd::Type::Comment && (cnv->main->getProperty(Identifiers::hideHeaders) || cnv->getProperty(Identifiers::isGraph));
    
    if(hideLabel) {
        textLabel.setVisible(false);
        auto [w, h] = graphics->getBestSize();
        graphics->setBounds(4, 4, std::max(getWidth() - 8, w), h);
        setSize(std::max(getWidth(), w + 8), h + 8);
    }
    else {
        textLabel.setVisible(true);
        textLabel.setBounds(4, 4, getWidth() - 8, 24);
        
        if(graphics)  {
            auto [w, h] = graphics->getBestSize();
            graphics->setBounds(4, 28, getWidth() - 8, getHeight() - 32);
            setSize(getWidth(), h + 28);
        }
        
        
    }
    
    if(resizer) {
        resizer->setBounds(getLocalBounds());
    }
    
    int index = 0;
    for(auto& edge : findChildrenOfClass<Edge>()) {
        
        auto& state = edge->getObjectState();
        bool isInput = state.getProperty(Identifiers::edgeIsInput);
        
        int position = index < numInputs ? index : index - numInputs;
        
        int total = isInput ? numInputs : numOutputs;
            
        float newY = isInput ? 4 : getHeight() - 4;
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

    // TODO: Make sure that this logic is exactly the same as PD
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
    
    int numIn = 0;
    int numOut = 0;
    
    for(int i = 0; i < numInputs + numOutputs; i++) {
        auto edge = getChild(i);
        if(!edge.isValid()) continue;
        
        bool input = edge.getProperty(Identifiers::edgeIsInput);
        
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);
       
        
        /* this removes the connection if it's illegal
         disabled because regular pd doesn't even do this!!
         bool wasSignal = edge.getProperty(Identifiers::edgeSignal);
         
        if((!input && isSignal && !wasSignal) || (input && isSignal && !wasSignal)) {
            removeChild(edge);
            addChild(edge, i);
        } */
        
        auto* edge_obj = findObjectForTree<Edge>(edge);
        for(auto& connection : edge_obj->getConnections()) {
            if(!connection) continue;
                
            if(connection->start == edge_obj) {
                connection->inObj = pdObject.get();
            }
            else {
                connection->outObj = pdObject.get();
            }
        }
        
        
        edge.setProperty(Identifiers::edgeIdx, input ? numIn : numOut, nullptr);
        edge.setProperty(Identifiers::edgeSignal, isSignal, nullptr);
        
        numIn += input;
        numOut += !input;
    }
    
    
    
    resized();
}
