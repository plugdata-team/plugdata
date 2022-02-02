/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Box.h"
#include "Canvas.h"
#include "Connection.h"
#include "Edge.h"
#include "PluginEditor.h"

#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"

//==============================================================================
Box::Box(Canvas* parent, String name, Point<int> position)
    :
      locked(parent->pd->locked)
    , textLabel(this, *parent)
{
    cnv = parent;
    
    initialise();
    setTopLeftPosition(position);
    
    setType(name);
    
    if(graphics) {
        position.addXY(0, -22);
    }
    
    setTopLeftPosition(position);
    
    // Open editor for undefined objects
    if (name.isEmpty()) {
        textLabel.showEditor();
        resized();
    }
    
    // Updates lock/unlock mode
    changeListenerCallback(nullptr);
}

Box::Box(pd::Object* object, Canvas* parent, String name, Point<int> position)
    : pdObject(object)
    , locked(parent->pd->locked)
    , textLabel(this, *parent)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);
    
    
    setType(name, true);
    
    if(graphics) {
        position.addXY(0, -22);
    }
    
    setTopLeftPosition(position);
    
    // Updates lock/unlock mode
    changeListenerCallback(nullptr);
    
}

Box::~Box()
{
    cnv->main.removeChangeListener(this);
}

void Box::changeListenerCallback(ChangeBroadcaster* source)
{
    locked = cnv->pd->locked;
    // Called when locking/unlocking
    textLabel.setEditable(!locked);
    
    bool hideForGraph = !graphics || graphics->getGUI().getType() == pd::Type::Message || (graphics->fakeGUI() && graphics->getGUI().getType() != pd::Type::Comment);
    
    setVisible(!(cnv->isGraph && hideForGraph));
    
    // If the object has graphics, we hide the draggable name object
    if (graphics && !graphics->fakeGUI() && (locked || cnv->isGraph)) {
        textLabel.setVisible(false);
        if (resizer)
            resizer->setVisible(false);
            
    }
    else {
        if (resizer)
            resizer->setVisible(true);
    }
    
    resized();
    repaint();
}

void Box::initialise()
{
    addMouseListener(this, true);
    addMouseListener(cnv, true); // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    //setSize(90, 31);

    addAndMakeVisible(&textLabel);

    textLabel.toBack();

    textLabel.onTextChange = [this]() {
        String newText = textLabel.getText();
        setType(newText);
    };
    
    // Add listener for lock/unlock messages
    cnv->main.addChangeListener(this);
}

bool Box::hitTest(int x, int y) {
    if(graphics && !graphics->fakeGUI()) {
        bool overGraphics = graphics->getBounds().expanded(4).contains(x, y);
        bool overText = textLabel.isVisible() && textLabel.getBounds().expanded(4).contains(x, y);
        return overGraphics || overText;
    }
    else {
        return textLabel.getBounds().expanded(4).contains(x, y);
    }
}


void Box::mouseEnter(const MouseEvent& e) {

    if(graphics && !graphics->fakeGUI() && !locked) {
        textLabel.setVisible(true);
    }
    
    repaint();
    
}

void Box::mouseExit(const MouseEvent& e)  {
    if(graphics && !graphics->fakeGUI() && !textLabel.isBeingEdited()) {
        textLabel.setVisible(false);
    }
    
    repaint();
}

void Box::setType(String newType, bool exists)
{
    // Change box type
    textLabel.setText(newType, dontSendNotification);

    String arguments = newType.fromFirstOccurrenceOf(" ", false, false);
    String type = newType.upToFirstOccurrenceOf(" ", false, false);
    
    bool wasInvalid = pdObject && pdObject->getType() == pd::Type::Invalid;


    // Exists indicates that this object already exists in pd
    // When setting exists to true, you need to have assigned an object to the pdObject variable already
    if (!exists) {
        auto* pd = &cnv->patch;
        // Pd doesn't normally allow changing between gui and non-gui objects so we force it
        if (pdObject) {
            pdObject = pd->renameObject(pdObject.get(), newType);
            cnv->synchronise();
        } else {
            pdObject = pd->createObject(newType, getX() - cnv->zeroPosition.x, getY() - cnv->zeroPosition.y);
        }
    }
    else {
        auto* ptr = pdObject->getPointer();
        // Reload GUI if it already exists
        if(pd::Gui::getType(ptr) != pd::Type::Undefined && pd::Gui::getType(ptr) != pd::Type::Invalid) {
            pdObject.reset(new pd::Gui(ptr, &cnv->patch, cnv->pd));
        }
        else {
            pdObject.reset(new pd::Object(ptr, &cnv->patch, cnv->pd));
        }
    }

    int width;
    if(exists) {
        width = pdObject->getWidth();
        if(width == 0) {
            width = textLabel.getFont().getStringWidth(newType) + 25;
        }
        else {
            width += 8;
        }
    }
    else {
        width = textLabel.getFont().getStringWidth(newType) + 25;
    }
    
    // Update inlets/outlets if it's not in a graph
    if (!cnv->isGraph)
        updatePorts();
    
    if (pdObject) {
        // Create graphics for the object if necessary
        graphics.reset(GUIComponent::createGui(type, this));

        if (graphics && graphics->getGUI().getType() == pd::Type::Comment) {
            setSize(width, 31);
            textLabel.setVisible(true);
        } else if (graphics && !graphics->fakeGUI()) {
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(w + 8, h + 29);
            graphics->toBack();
            textLabel.setVisible(false);
        } else {
            textLabel.setVisible(true);
            setSize(width, 31);
        }
    } else {
        textLabel.setVisible(true);
        setSize(width, 31);
    }

    if (type.isEmpty()) {
        setSize(100, 31);
    }
    


    // Hide "comment" in front of name
    if (newType.startsWith("comment ")) {
        textLabel.setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }
    
    if(!resizer) {
        resizer.reset(new ResizableBorderComponent(this, &restrainer));
        addAndMakeVisible(resizer.get());
        resizer->toBack(); // make sure it's behind the edges
    }
    
    if (graphics && graphics->getGUI().isIEM()) {
        resizer->setBorderThickness({5, 5, 5, 5});
    }
    else {
        // Only allow resize on right, otherwise we have to edit the position
        resizer->setBorderThickness({0, 0, 0, 5});
    }
    
    // graphical objects manage their own size limits
    if(!graphics) restrainer.setSizeLimits(25, getHeight(), 350, getHeight());
    

    restrainer.checkComponentBounds(this);
    
    cnv->main.updateUndoState();

    resized();
}

//==============================================================================
void Box::paint(Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);

    auto baseColour = findColour(TextButton::buttonColourId);
    auto outlineColour = findColour(ComboBox::outlineColourId);
    

    bool isOver = getLocalBounds().contains(getMouseXYRelative());

    bool selected = cnv->isSelected(this);

    bool hideLabel =  graphics && !graphics->fakeGUI() && (locked || !textLabel.isVisible());
    if(hideLabel) rect.removeFromTop(21);
    
    if(pdObject && pdObject->getType() == pd::Type::Invalid && !textLabel.isBeingEdited()) {
        outlineColour = Colours::red;
    }
    else if (selected) {
        outlineColour = MainLook::highlightColour;
    }
    
    if (isOver) {
        baseColour = baseColour.contrasting(0.05f);
    }

    // Draw comment style
    if (graphics && graphics->getGUI().getType() == pd::Type::Comment) {
        if(locked || (!isOver && !selected)) g.setColour(Colours::transparentBlack);
        else g.setColour(findColour(ComboBox::outlineColourId));
        
        g.drawRect(rect.toFloat(), 0.5f);
    }
    // Draw for all other objects
    else {
        if (!hideLabel && !(graphics && graphics->getGUI().getType() == pd::Type::GraphOnParent)) {
            g.setColour(baseColour);
            g.fillRoundedRectangle(rect.toFloat().withTrimmedBottom(getHeight() - 31), 2.0f);
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }
}

void Box::resized()
{
    if(graphics)  {
        graphics->setBounds(4, 25, getWidth() - 8, getHeight() - 29);
    }
    
    if(pdObject && (!graphics || !graphics->getGUI().isIEM())) {
        pdObject->setWidth(getWidth() - 8);
    }

    auto bestWidth = textLabel.getFont().getStringWidth(textLabel.getText()) + 25;

    if (graphics && graphics->getGUI().getType() == pd::Type::Comment && !textLabel.isBeingEdited()) {
        int num_lines = std::max(StringArray::fromTokens(textLabel.getText(), "\n", "\'").size(), 1);
        setSize(bestWidth + 30, (num_lines * 17) + 14);
        textLabel.setBounds(getLocalBounds().reduced(4));
    }
    else if(graphics && graphics->getGUI().getType() == pd::Type::Message && !graphics->getGUI().isAtom()) {
        textLabel.setBorderSize({2, 2, 2, 22});
    }
    else {
        textLabel.setBorderSize({2, 2, 2, 2});
        textLabel.setBounds(4, 4, getWidth() - 8, 23);
    }
    
    
 
    // Init size for empty objects
    if(textLabel.isBeingEdited()) {
        if(textLabel.getCurrentTextEditor()->getText().isEmpty()) {
            setSize(100, 31);
        }
    }

    if (resizer) {
        resizer->setBounds(getLocalBounds());
    }

    int index = 0;
    for (auto& edge : edges) {
        bool isInput = edge->isInput;
        int position = index < numInputs ? index : index - numInputs;
        int total = isInput ? numInputs : numOutputs;
        
        float newY = isInput ? 4 : getHeight() - 4;
        float newX = position * ((getWidth() - 32) / (total - 1 + (total == 1))) + 16;

        if(isInput && graphics && !graphics->fakeGUI()) newY += 22;
        edge->setCentrePosition(newX, newY);
        edge->setSize(8, 8);

        index++;
    }
    
}

void Box::updatePorts()
{
    // update inlets and outlets

    int oldnumInputs = 0;
    int oldnumOutputs = 0;

    for (auto& edge : edges) {
        edge->isInput ? oldnumInputs++ : oldnumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (pdObject) {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }

    // TODO: Make sure that this logic is exactly the same as PD
    while (numInputs < oldnumInputs) edges.remove(--oldnumInputs);
    while (numInputs > oldnumInputs) edges.insert(oldnumInputs++, new Edge(this, true));
    while (numOutputs < oldnumOutputs) edges.remove(numInputs + (--oldnumOutputs));
    while (numOutputs > oldnumOutputs) edges.insert(numInputs + (++oldnumOutputs), new Edge(this, false));

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++) {
        auto* edge = edges[i];
        bool input = edge->isInput;
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);

        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;

        numIn += input;
        numOut += !input;
    }

    resized();
}
