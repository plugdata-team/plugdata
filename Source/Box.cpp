/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include "Connection.h"
#include "PluginEditor.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"


//==============================================================================
Box::Box(Canvas* parent, String name, Point<int> position) : textLabel(this, parent->dragger), dragger(parent->dragger)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);
    setType(name);
    
    if(name.isEmpty()) {
        textLabel.showEditor();
    }
}

Box::Box(pd::Object* object, Canvas* parent, String name, Point<int> position) : textLabel(this, parent->dragger), pdObject(object), dragger(parent->dragger)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);
    setType(name, true);
}

Box::~Box()
{
}

void Box::initialise() {
    addMouseListener(cnv, true);
    cnv->addAndMakeVisible(this);
    
    setSize (55, 31);
    
    auto& [minW, minH, maxW, maxH] = defaultLimits;
    restrainer.setSizeLimits(minW, minH, maxW, maxH);
    
    //resizer.reset(new ResizableBorderComponent(this, &restrainer));
    
    // Uncomment to enable resizing
    // doesn't work right yet!
    //addAndMakeVisible(resizer.get());
    
    addAndMakeVisible(&textLabel);
    
    textLabel.toBack();
    
    textLabel.onTextChange = [this]() {
        String newText = textLabel.getText();
        setType(newText);
    };
}


void Box::mouseMove(const MouseEvent& e) {
    findParentComponentOfClass<Canvas>()->repaint();
}


void Box::setType (String newType, bool exists)
{
    textLabel.setText(newType, dontSendNotification);
    
    String arguments = newType.fromFirstOccurrenceOf(" ", false, false);
    String type = newType.upToFirstOccurrenceOf(" ", false, false);
    
    if(!exists) {
        auto* pd = &cnv->patch;
        
        // Pd doesn't normally allow changing between gui and non-gui objects

        if(pdObject && graphics) {
            pd->removeObject(pdObject.get());
            pdObject = pd->createObject(newType, getX(), getY());
        }
        else if(pdObject) {
            pdObject = pd->renameObject(pdObject.get(), newType);
        }
        else {
            pdObject = pd->createObject(newType, getX(),  getY());
        }
    }
    else if (!exists) {
        pdObject = nullptr;
    }
    
    if(!cnv->isGraph) updatePorts();
    
    auto& [minW, minH, maxW, maxH] = defaultLimits;
    
    auto bestWidth = textLabel.getFont().getStringWidth(newType) + 25;
    restrainer.setSizeLimits(std::max(getHeight(), bestWidth), minH, std::max(700, bestWidth * 2), maxH);
    
    if(pdObject) {
        graphics.reset(GUIComponent::createGui(type, this));
        
        if(graphics && graphics->getGUI().getType() == pd::Type::Comment) {
            setSize(bestWidth, 31);
        }
        else if(graphics && !graphics->fakeGUI()) {
            auto [minW, minH, maxW, maxH] = graphics->getSizeLimits();
            restrainer.setSizeLimits(minW, minH, maxW, maxH);
            
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(w + 8, h + 27);
            graphics->toBack();
        }
        else {
            setSize(bestWidth, 31);
        }
    }
    else {
        setSize(bestWidth, 31);
    }
    
    if(type.isEmpty()) {
        setSize (100, 31);
    }
    
    if(newType.startsWith("comment ")) {
        textLabel.setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }
    
    cnv->main.updateUndoState();

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
    
    
    bool hideLabel = graphics && !graphics->fakeGUI() && (cnv->main.pd.mainTree.getProperty(Identifiers::hideHeaders) || cnv->isGraph);
    
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
        if(!hideLabel) {
            g.setColour(baseColour);
            g.fillRoundedRectangle(rect.toFloat().withTrimmedBottom(getHeight() - 32), 2.0f);
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }
    

}

void Box::moved()
{
    for(auto& edge : edges) {
        edge->sendMovedResizedMessages(true, true);
    }
}


void Box::resized()
{
    bool hideLabel = graphics && !graphics->fakeGUI() && (cnv->main.pd.mainTree.getProperty(Identifiers::hideHeaders) || cnv->isGraph);
    
    // Hidden header mode: gui objects become undraggable
    if(hideLabel) {
        textLabel.setVisible(false);
        auto [w, h] = graphics->getBestSize();
        setSize(w + 8, h + 8);
        graphics->setBounds(4, 4, w , h);
        
    }
    else {
        textLabel.setVisible(true);
        textLabel.setBounds(4, 4, getWidth() - 8, 22);
        auto bestWidth = textLabel.getFont().getStringWidth(textLabel.getText()) + 25;
        
        if(graphics && graphics->getGUI().getType() == pd::Type::Comment) {
            int num_lines = std::max(StringArray::fromTokens(textLabel.getText(), "\n", "\'").size(), 1);
            setSize(bestWidth + 30, (num_lines * 17) + 14);
            textLabel.setBounds(getLocalBounds().reduced(4));
        }
        else if(graphics)  {
            auto [w, h] = graphics->getBestSize();
            setSize(std::max(bestWidth, w + 8), h + 27);
            graphics->setBounds(4, 26, getWidth() - 8, getHeight() - 30);
        }
    }
    
    if(resizer) {
        resizer->setBounds(getLocalBounds());
    }
    
    int index = 0;
    for(auto& edge : edges) {
        
        //auto& state = edge->getObjectState();
        bool isInput = edge->isInput;
        
        int position = index < numInputs ? index : index - numInputs;
        
        int total = isInput ? numInputs : numOutputs;
            
        float newY = isInput ? 4 : getHeight() - 4;
        float newX = position * ((getWidth() - 32) / (total - 1 + (total == 1))) + 16;
        
        edge->setCentrePosition(newX, newY);
        edge->setSize(8, 8);
        
        index++;
    }
}


void Box::updatePorts() {
    
    int oldnumInputs = 0;
    int oldnumOutputs = 0;
    
    for(auto& edge : edges) {
        edge->isInput ? oldnumInputs++ : oldnumOutputs++;
    }
    
    numInputs = 0;
    numOutputs = 0;
    
    if(pdObject) {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }

    // TODO: Make sure that this logic is exactly the same as PD
    while(numInputs < oldnumInputs) {
        edges.remove(oldnumInputs - 1);
        oldnumInputs--;
    }
    while(numInputs > oldnumInputs) {
        edges.insert(oldnumInputs, new Edge(this, true));
        oldnumInputs++;
    }
    
    while(numOutputs < oldnumOutputs) {
        edges.remove(oldnumOutputs - 1);
        oldnumOutputs--;
    }
    while(numOutputs > oldnumOutputs) {
        edges.insert(numInputs + oldnumOutputs, new Edge(this, false));
        oldnumOutputs++;
    }
    
    int numIn = 0;
    int numOut = 0;
    
    for(int i = 0; i < numInputs + numOutputs; i++) {
        auto* edge = edges[i];
        
        bool input = edge->isInput;
        
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);
       
        
        /* this removes the connection if it's illegal
         disabled because regular pd doesn't even do this!!
         bool wasSignal = edge.getProperty(Identifiers::edgeSignal);
         
        if((!input && isSignal && !wasSignal) || (input && isSignal && !wasSignal)) {
            removeChild(edge);
            addChild(edge, i);
        } */
        
        //edge->isInput = input;
        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;
        
        numIn += input;
        numOut += !input;
    }

    
    resized();
}
