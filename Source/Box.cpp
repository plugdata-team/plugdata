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

Box::Box(Canvas* parent, const String& name, Point<int> position) : resizer(this, &constrainer)
{
    cnv = parent;

    initialise();
    setTopLeftPosition(position);

    setType(name);

    // Open editor for undefined objects
    if (name.isEmpty())
    {
        showEditor();
    }
}

Box::Box(pd::Object* object, Canvas* parent, const String& name, Point<int> position) : pdObject(object), resizer(this, &constrainer)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);

    setType(name, true);
}

Box::~Box()
{
}

void Box::initialise()
{
    addMouseListener(cnv, true);  // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    // Updates lock/unlock mode
    locked.referTo(cnv->pd->locked);
    commandLocked.referTo(cnv->pd->commandLocked);

    setBufferedToImage(true);

    locked.addListener(this);
    commandLocked.addListener(this);

    onTextChange = [this]()
    {
        String newText = getText();
        setType(newText);
    };

    addAndMakeVisible(resizer);
}

void Box::valueChanged(Value& v)
{
    if (lastTextValue != textValue.toString()) setText(textValue.toString(), sendNotification);

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
        graphics->lock(locked == true || commandLocked == true);
    }

    resizer.setVisible(locked == false);

    resized();
    repaint();
}

bool Box::hitTest(int x, int y)
{
    return getLocalBounds().reduced(2).contains(x, y);
}

void Box::mouseEnter(const MouseEvent& e)
{
    isOver = true;
    repaint();
}

void Box::mouseExit(const MouseEvent& e)
{
    isOver = false;
    repaint();
}

void Box::setType(const String& newType, bool exists)
{
    // Change box type
    setText(newType, dontSendNotification);
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

            // Synchronise to make sure connections are preserved correctly
            // Asynchronous because it could possibly delete this object
            MessageManager::callAsync([this]() { cnv->synchronise(false); });
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
            width = font.getStringWidth(newType) + 27;
        }
        else
        {
            width += 8;
        }
    }
    else
    {
        width = font.getStringWidth(newType) + 27;
    }

    // Update inlets/outlets if it's not in a graph
    if (!cnv->isGraph) updatePorts();

    int largestSide = std::max(numInputs, numOutputs);

    // Make sure we have enough space for inlets/outlets
    if (width - 12 < largestSide * 16)
    {
        width = (largestSide * 16) + 12;
    }

    if (pdObject)
    {
        // Create graphics for the object if necessary
        graphics.reset(GUIComponent::createGui(type, this, !exists));

        if (graphics)
        {
            graphics->lock(locked == true);
            graphics->updateValue();
        }

        if (graphics && graphics->getGui().getType() == pd::Type::Comment)
        {
            setSize(width, 34);
            setEditable(true);
            hideLabel = false;
        }
        else if (graphics && !graphics->fakeGui())
        {
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(w + 8, h + 8);
            graphics->resized();
            graphics->toBack();
            hideLabel = true;
            setEditable(false);
        }
        else if (!type.isEmpty())
        {
            hideLabel = false;
            setEditable(true);
            setSize(width, 34);
        }
    }
    else
    {
        hideLabel = false;
        setSize(width, 34);
    }

    if (type.isEmpty())
    {
        hideLabel = false;
        setEditable(true);
        setSize(100, 34);
    }

    // Hide "comment" in front of name
    if (newType.startsWith("comment "))
    {
        setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }

    if (graphics && !graphics->fakeGui())
    {
        resizer.setBorderThickness({0, 0, 5, 5});
    }
    else
    {
        resizer.setBorderThickness({0, 0, 0, 5});
    }

    // graphical objects manage their own size limits
    if (!graphics) constrainer.setSizeLimits(25, getHeight(), 350, getHeight());

    cnv->main.updateUndoState();
}

void Box::paint(Graphics& g)
{
    auto rect = getLocalBounds().reduced(6);

    auto baseColour = findColour(TextButton::buttonColourId);
    auto outlineColour = findColour(ComboBox::outlineColourId);

    bool selected = cnv->isSelected(this);

    if (pdObject && pdObject->getType() == pd::Type::Invalid && !getCurrentTextEditor())
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
        if (locked == false && (isOver || selected))
        {
            g.setColour(selected ? findColour(Slider::thumbColourId) : findColour(ComboBox::outlineColourId));
            g.drawRect(rect.toFloat(), 0.5f);
        }
    }
    // Draw for all other objects
    else
    {
        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }

    if (!hideLabel && !editor)
    {
        g.setColour(findColour(Label::textColourId));
        g.setFont(font);

        auto textArea = border.subtractedFrom(rect);

        g.drawFittedText(getText(), textArea, justification, jmax(1, static_cast<int>((static_cast<float>(textArea.getHeight()) / font.getHeight()))), minimumHorizontalScale);

        g.setColour(findColour(Label::outlineColourId));
    }
}

void Box::resized()
{
    if (graphics)
    {
        graphics->setBounds(getLocalBounds().reduced(6));
    }

    // Send current width to pd object
    if (pdObject)
    {
        pdObject->setSize(getWidth() - 8, getHeight() - 8);
    }

    constrainer.checkComponentBounds(this);

    auto bestWidth = font.getStringWidth(getText()) + 27;

    if (graphics && graphics->getGui().getType() == pd::Type::Comment && !getCurrentTextEditor())
    {
        int numLines = std::max(StringArray::fromTokens(getText(), "\n", "\'").size(), 1);
        setSize(bestWidth + 30, (numLines * 17) + 14);
    }

    if (auto* editor = getCurrentTextEditor())
    {
        editor->setBounds(getLocalBounds().reduced(6));
    }

    resizer.setBounds(getLocalBounds().reduced(5));
    resizer.toFront(false);

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
}

void Box::setText(const String& newText, NotificationType notification)
{
    hideEditor();

    if (lastTextValue != newText)
    {
        lastTextValue = newText;
        textValue = newText;
        repaint();

        if (notification != dontSendNotification && onTextChange != nullptr) onTextChange();
    }
}

String Box::getText(bool returnActiveEditorContents) const
{
    return (returnActiveEditorContents && editor) ? editor->getText() : textValue.toString();
}

void Box::mouseDown(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->pd->locked == true) return;

    cnv->handleMouseDown(this, e);
}

void Box::mouseUp(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->pd->locked == true) return;

    cnv->handleMouseUp(this, e);

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600)
    {
        cnv->connectingEdge = nullptr;
    }
}

void Box::mouseDrag(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->pd->locked == true) return;

    cnv->handleMouseDrag(e);
}

static void copyColourIfSpecified(Box& l, TextEditor& ed, int colourId, int targetColourId)
{
    if (l.isColourSpecified(colourId) || l.getLookAndFeel().isColourSpecified(colourId)) ed.setColour(targetColourId, l.findColour(colourId));
}

void Box::showEditor()
{
    if (editor == nullptr)
    {
        editor.reset(new TextEditor(getName()));
        editor->applyFontToAllText(font);
        copyAllExplicitColoursTo(*editor);

        copyColourIfSpecified(*this, *editor, Label::textWhenEditingColourId, TextEditor::textColourId);
        copyColourIfSpecified(*this, *editor, Label::backgroundWhenEditingColourId, TextEditor::backgroundColourId);
        copyColourIfSpecified(*this, *editor, Label::outlineWhenEditingColourId, TextEditor::focusedOutlineColourId);

        editor->setAlwaysOnTop(true);

        bool multiLine = pdObject && pdObject->getType() == pd::Type::Comment;

        // Allow multiline for comment objects
        editor->setMultiLine(multiLine, false);
        editor->setReturnKeyStartsNewLine(multiLine);
        editor->setBorder(border);
        editor->setJustification(Justification::centred);

        editor->onFocusLost = [this]()
        {
            if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true))
            {
                hideEditor();
            }
        };

        if (!(graphics && graphics->getGui().getType() == pd::Type::Comment))
        {
            cnv->showSuggestions(this, editor.get());
        }

        editor->setSize(10, 10);
        addAndMakeVisible(editor.get());
        editor->setText(getText(), false);
        editor->setKeyboardType(keyboardType);
        editor->addListener(this);

        if (editor == nullptr)  // may be deleted by a callback
            return;

        editor->setHighlightedRegion(Range<int>(0, textValue.toString().length()));

        resized();
        repaint();

        editor->grabKeyboardFocus();
    }
}

void Box::hideEditor()
{
    if (editor != nullptr)
    {
        WeakReference<Component> deletionChecker(this);
        std::unique_ptr<TextEditor> outgoingEditor;
        std::swap(outgoingEditor, editor);

        if (auto* peer = getPeer()) peer->dismissPendingTextInput();

        outgoingEditor->setInputFilter(nullptr, false);

        // Clear overridden lambda
        outgoingEditor->onTextChange = []() {};

        if (graphics && !graphics->fakeGui())
        {
            setVisible(false);
            resized();
        }

        cnv->hideSuggestions();

        auto newText = outgoingEditor->getText();

        bool changed;
        if (textValue.toString() != newText)
        {
            lastTextValue = newText;
            textValue = newText;
            repaint();

            changed = true;
        }
        else
        {
            changed = false;
        }

        outgoingEditor.reset();

        repaint();

        if (changed && onTextChange != nullptr) onTextChange();
    }
}

TextEditor* Box::getCurrentTextEditor() const noexcept
{
    return editor.get();
}

void Box::mouseDoubleClick(const MouseEvent& e)
{
    if (editDoubleClick && isEnabled() && !e.mods.isPopupMenu())
    {
        showEditor();
    }
}

void Box::setEditable(bool editable)
{
    editDoubleClick = editable;

    setWantsKeyboardFocus(editDoubleClick);
    setFocusContainerType(editDoubleClick ? FocusContainerType::keyboardFocusContainer : FocusContainerType::none);
    invalidateAccessibilityHandler();
}

void Box::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (editor != nullptr)
    {
        editor->giveAwayKeyboardFocus();
    }
}
