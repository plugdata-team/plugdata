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


Box::Box(Canvas* parent, const String& name, Point<int> position) : locked(parent->pd->locked), textLabel(this, *parent), resizer(this, &restrainer)
{
    cnv = parent;

    initialise();
    setTopLeftPosition(position);

    setBufferedToImage(true);

    setType(name);

    addChildComponent(resizer);

    setTopLeftPosition(position);

    // Open editor for undefined objects
    if (name.isEmpty())
    {
        textLabel.showEditor();
        resized();
    }

    // Updates lock/unlock mode
    changeListenerCallback(nullptr);
}

Box::Box(pd::Object* object, Canvas* parent, const String& name, Point<int> position) : pdObject(object), locked(parent->pd->locked), textLabel(this, *parent), resizer(this, &restrainer)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);

    setType(name, true);

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

    // Hide certain objects in GOP
    if (cnv->isGraph && (!graphics || (graphics && (graphics->getGui().getType() == pd::Type::Message || graphics->getGui().getType() == pd::Type::Comment))))
    {
        setVisible(false);
    }
    else
    {
        setVisible(true);
    }

    if (graphics)
    {
        graphics->lock(locked || cnv->pd->commandLocked);
    }

    resizer.setVisible(!locked);

    resized();
    repaint();
}

void Box::initialise()
{
    addMouseListener(this, true);
    addMouseListener(cnv, true);  // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    addAndMakeVisible(&textLabel);

    textLabel.toBack();

    textLabel.onTextChange = [this]()
    {
        String newText = textLabel.getText();
        setType(newText);
    };

    // Add listener for lock/unlock messages
    cnv->main.addChangeListener(this);
}

bool Box::hitTest(int x, int y)
{
    return getLocalBounds().reduced(2).contains(x, y);
}

void Box::mouseEnter(const MouseEvent& e)
{
    repaint();
}

void Box::mouseExit(const MouseEvent& e)
{
    repaint();
}

void Box::setType(const String& newType, bool exists)
{
    // Change box type
    textLabel.setText(newType, dontSendNotification);
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    // "exists" indicates that this object already exists in pd
    // When setting exists to true, you need to have assigned an object to the pdObject variable already
    if (!exists)
    {
        auto* pd = &cnv->patch;
        // Pd doesn't normally allow changing between gui and non-gui objects, so we force it
        if (pdObject)
        {
            pdObject = pd->renameObject(pdObject.get(), newType);
            cnv->synchronise(false);
        }
        else
        {
            pdObject = pd->createObject(newType, getX() - cnv->zeroPosition.x, getY() - cnv->zeroPosition.y);
        }
    }
    else
    {
        auto* ptr = pdObject->getPointer();
        // Reload GUI if it already exists
        if (pd::Gui::getType(ptr) != pd::Type::Undefined)
        {
            pdObject = std::make_unique<pd::Gui>(ptr, &cnv->patch, cnv->pd);
        }
        else
        {
            pdObject = std::make_unique<pd::Object>(ptr, &cnv->patch, cnv->pd);
        }
    }

    int width;
    if (exists)
    {
        width = pdObject->getWidth();
        if (width == 0)
        {
            width = textLabel.getFont().getStringWidth(newType) + 28;
        }
        else
        {
            width += 8;
        }
    }
    else
    {
        width = textLabel.getFont().getStringWidth(newType) + 28;
    }

    // Update inlets/outlets if it's not in a graph
    if (!cnv->isGraph) updatePorts();

    if (pdObject)
    {
        // Create graphics for the object if necessary
        graphics.reset(GUIComponent::createGui(type, this, !exists));

        if (graphics)
        {
            graphics->lock(locked);
            graphics->updateValue();
        }

        if (graphics && graphics->getGui().getType() == pd::Type::Comment)
        {
            setSize(width, 34);
            textLabel.setEditable(true);
        }
        else if (graphics && !graphics->fakeGui())
        {
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(w, h);
            graphics->toBack();
            textLabel.toBack();
            textLabel.setEditable(false);
        }
        else
        {
            textLabel.setEditable(true);
            setSize(width, 34);
        }
    }
    else
    {
        setSize(width, 34);
    }

    if (type.isEmpty())
    {
        setSize(100, 34);
    }

    // Hide "comment" in front of name
    if (newType.startsWith("comment "))
    {
        textLabel.setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }

    if (graphics && (graphics->getGui().isIEM() || graphics->getGui().getType() == pd::Type::Panel))
    {
        resizer.setBorderThickness({0, 0, 5, 5});
    }
    else if (graphics && graphics->getGui().getType() == pd::Type::GraphOnParent)
    {
        // Only allow resize on right, otherwise we have to edit the position
        resizer.setBorderThickness({0, 0, 0, 0});
    }
    else
    {
        resizer.setBorderThickness({0, 0, 0, 5});
    }

    // graphical objects manage their own size limits
    if (!graphics) restrainer.setSizeLimits(25, getHeight(), 350, getHeight());

    restrainer.checkComponentBounds(this);

    cnv->main.updateUndoState();

    resized();
}


void Box::paint(Graphics& g)
{
    auto rect = getLocalBounds().reduced(6);

    auto baseColour = findColour(TextButton::buttonColourId);
    auto outlineColour = findColour(ComboBox::outlineColourId);

    bool isOver = getLocalBounds().contains(getMouseXYRelative());

    bool selected = cnv->isSelected(this);

    if (pdObject && pdObject->getType() == pd::Type::Invalid && !textLabel.isBeingEdited())
    {
        outlineColour = Colours::red;
    }
    else if (selected)
    {
        outlineColour = findColour(Slider::thumbColourId);
    }

    if (!graphics || (graphics && graphics->fakeGui() && graphics->getGui().getType() != pd::Type::Comment))
    {
        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillRect(getLocalBounds().reduced(6));
    }

    // Draw comment style
    if (graphics && graphics->getGui().getType() == pd::Type::Comment)
    {
        if (locked || (!isOver && !selected))
            g.setColour(Colours::transparentBlack);
        else
            g.setColour(findColour(ComboBox::outlineColourId));

        g.drawRect(rect.toFloat(), 0.5f);
    }
    // Draw for all other objects
    else
    {
        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }
}

void Box::resized()
{
    if (graphics)
    {
        graphics->setBounds(getLocalBounds().reduced(6));
    }

    if (pdObject && (!graphics || !graphics->getGui().isIEM()))
    {
        pdObject->setWidth(getWidth() - 8);
    }

    auto bestWidth = textLabel.getFont().getStringWidth(textLabel.getText()) + 28;

    if (graphics && graphics->getGui().getType() == pd::Type::Comment && !textLabel.isBeingEdited())
    {
        int numLines = std::max(StringArray::fromTokens(textLabel.getText(), "\n", "\'").size(), 1);
        setSize(bestWidth + 30, (numLines * 17) + 14);
        textLabel.setBounds(getLocalBounds().reduced(6));
    }

    textLabel.setBounds(getLocalBounds().reduced(6));

    // Init size for empty objects
    if (textLabel.isBeingEdited())
    {
        if (textLabel.getCurrentTextEditor()->getText().isEmpty())
        {
            setSize(100, 34);
        }
    }

    resizer.setBounds(getLocalBounds().reduced(6));

    int index = 0;
    for (auto& edge : edges)
    {
        bool isInput = edge->isInput;
        int position = index < numInputs ? index : index - numInputs;
        int total = isInput ? numInputs : numOutputs;

        float newY = isInput ? 6 : getHeight() - 6;
        float newX = position * ((getWidth() - 32) / (total - 1 + (total == 1))) + 16;

        edge->setCentrePosition(newX, newY);
        edge->setSize(10, 10);

        index++;
    }

    resizer.toFront(false);
}

void Box::updatePorts()
{
    // update inlets and outlets

    int oldnumInputs = 0;
    int oldnumOutputs = 0;

    for (auto& edge : edges)
    {
        edge->isInput ? oldnumInputs++ : oldnumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (pdObject)
    {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }

    while (numInputs < oldnumInputs) edges.remove(--oldnumInputs);
    while (numInputs > oldnumInputs) edges.insert(oldnumInputs++, new Edge(this, true));
    while (numOutputs < oldnumOutputs) edges.remove(numInputs + (--oldnumOutputs));
    while (numOutputs > oldnumOutputs) edges.insert(numInputs + (++oldnumOutputs), new Edge(this, false));

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++)
    {
        auto* edge = edges[i];
        bool input = edge->isInput;
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);

        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;
        edge->setAlwaysOnTop(true);

        numIn += input;
        numOut += !input;
    }

    resized();
}
