/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Box.h"

#include <memory>

#include "Canvas.h"
#include "Connection.h"
#include "Edge.h"
#include "PluginEditor.h"

Box::Box(Canvas* parent, const String& name, Point<int> position)
{
    cnv = parent;

    setTopLeftPosition(position - Point<int>(margin, margin));

    initialise();
    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty())
    {
        setSize(100, height);
        showEditor();
    }
    else
    {
        setType(name);
    }
}

Box::Box(pd::Object* object, Canvas* parent, const String& name) : pdObject(object)
{
    cnv = parent;

    initialise();

    setType(name, true);
}

void Box::initialise()
{
    addMouseListener(cnv, true);  // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    // Updates lock/unlock mode
    locked.referTo(cnv->pd->locked);
    commandLocked.referTo(cnv->pd->commandLocked);
    presentationMode.referTo(cnv->presentationMode);

    presentationMode.addListener(this);
    locked.addListener(this);
    commandLocked.addListener(this);

    setBufferedToImage(true);

    originalBounds.setBounds(0, 0, 0, 0);
}

void Box::valueChanged(Value& v)
{
    /*
     if (currentText != textValue.toString()) {
     currentText = textValue.toString();
     setType(currentText);
     } */

    // Hide certain objects in GOP
    if ((cnv->isGraph || cnv->presentationMode == var(true)) && (!graphics || (graphics && (graphics->getGui().getType() == pd::Type::Message || graphics->getGui().getType() == pd::Type::Comment))))
    {
        setVisible(false);
    }
    else
    {
        setVisible(true);
        updatePorts();
        resized();
    }

    if (graphics)
    {
        graphics->lock(locked == var(true) || commandLocked == var(true));
    }

    resized();
    repaint();
}

bool Box::hitTest(int x, int y)
{
    return getLocalBounds().reduced(1).contains(x, y);
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

void Box::mouseMove(const MouseEvent& e)
{
    auto corners = getCorners();

    if (!cnv->isSelected(this) || locked == var(true)) return;

    for (auto& rect : corners)
    {
        if (rect.contains(e.position))
        {
            auto zone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), e.getPosition());

            setMouseCursor(zone.getMouseCursor());
            updateMouseCursor();
            return;
        }
    }

    setMouseCursor(MouseCursor::NormalCursor);
    updateMouseCursor();
}

void Box::updateBounds(bool newObject)
{
    auto type = currentText.upToFirstOccurrenceOf(" ", false, false);
    int width = 0;
    
    if (pdObject)
    {
        auto bounds = pdObject->getBounds() - Point<int>(margin, margin);
        bounds.translate(cnv->canvasOrigin.x, cnv->canvasOrigin.y);

        width = bounds.getWidth() + doubleMargin;

        if (bounds.getPosition().getDistanceFrom(getPosition()) >= 2.0f)
        {
            setTopLeftPosition(bounds.getPosition());
        }
    }
    // width didn't get assigned, or was zero
    if (width <= doubleMargin)
    {
        width = font.getStringWidth(currentText) + widthOffset;
    }

    if (graphics && !graphics->fakeGui())
    {
        addAndMakeVisible(graphics.get());
        auto [w, h] = graphics->getBestSize();
        setSize(w + doubleMargin, h + doubleMargin);
        graphics->resized();
        graphics->toBack();
        hideLabel = true;
        setEditable(false);
    }
    else if (!graphics || (graphics && graphics->fakeGui()))
    {
        setEditable(true);
        hideLabel = false;
        setSize(width, height);
    }

    if (type.isEmpty())
    {
        hideLabel = false;
        setEditable(true);
        setSize(100, height);
    }
    if (graphics && graphics->getGui().getType() == pd::Type::Comment && !getCurrentTextEditor())
    {
        auto bestWidth = font.getStringWidth(currentText) + widthOffset;
        int numLines = std::max(StringArray::fromTokens(currentText, "\n", "\'").size(), 1);
        setSize(bestWidth + doubleMargin, (numLines * 18) + doubleMargin);
    }

    resized();
}

void Box::setType(const String& newType, bool exists)
{
    hideEditor();

    if (currentText != newType)
    {
        currentText = newType;
    }

    // Change box type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    // "exists" indicates that this object already exists in pd
    // When setting exists to true, the pdObject need to be assigned already
    if (!exists)
    {
        auto* pd = &cnv->patch;
        if (pdObject)
        {
            pdObject = pd->renameObject(pdObject.get(), newType);

            // Synchronise to make sure connections are preserved correctly
            // Asynchronous because it could possibly delete this object
            MessageManager::callAsync([this]() { cnv->synchronise(false); });
        }
        else
        {
            auto rect = getBounds() - cnv->canvasOrigin;
            pdObject = pd->createObject(newType, rect.getX() + margin, rect.getY() + margin);
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

    if (pdObject)
    {
        // Create graphics for the object if necessary
        graphics.reset(GUIComponent::createGui(type, this, !exists));

        if (graphics)
        {
            graphics->lock(locked == var(true));
            graphics->updateValue();
        }
    }

    // Update inlets/outlets
    updatePorts();
    updateBounds(!exists);

    // Hide "comment" in front of name
    if (newType.startsWith("comment "))
    {
        currentText = currentText.fromFirstOccurrenceOf("comment ", false, false);
    }

    // graphical objects manage their own size limits
    // For text object, make sure the width at least fits the text
    if (!graphics || (graphics->fakeGui() && graphics->getGui().getType() != pd::Type::Comment))
    {
        int ioletWidth = (std::max(numInputs, numOutputs) * doubleMargin) + 15;
        int textWidth = font.getStringWidth(newType) + widthOffset;

        int minimumWidth = std::max(textWidth, ioletWidth);
        constrainer.setSizeLimits(minimumWidth, Box::height, std::max(3000, minimumWidth), Box::height);
    }

    cnv->updateDrawables();
    cnv->main.updateCommandStatus();
}

Array<Rectangle<float>> Box::getCorners() const
{
    auto rect = getLocalBounds().reduced(margin);
    const float offset = 2.0f;

    Array<Rectangle<float>> corners = {Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
                                       Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset)};

    return corners;
}

void Box::paint(Graphics& g)
{
    auto rect = getLocalBounds().reduced(margin);
    auto outlineColour = findColour(ComboBox::outlineColourId);

    bool selected = cnv->isSelected(this);

    if (pdObject && pdObject->getType() == pd::Type::Invalid && !getCurrentTextEditor())
    {
        outlineColour = Colours::red;
    }
    else if (selected && !cnv->isGraph)
    {
        outlineColour = findColour(Slider::thumbColourId);
        g.setColour(outlineColour);

        // Draw resize edges when selected
        for (auto& rect : getCorners()) g.fillRoundedRectangle(rect, 2.0f);
    }

    if (!graphics || (graphics && graphics->fakeGui() && graphics->getGui().getType() != pd::Type::Comment))
    {
        
        g.setColour(cnv->backgroundColour);
        g.fillRect(getLocalBounds().reduced(margin));
    }

    // Draw comment style
    if (graphics && graphics->getGui().getType() == pd::Type::Comment)
    {
        if (locked == var(false) && (isOver || selected) && !cnv->isGraph)
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
        g.setColour(cnv->backgroundColour.contrasting());
        g.setFont(font);

        auto textArea = border.subtractedFrom(rect);

        g.drawFittedText(currentText, textArea, justification, jmax(1, static_cast<int>((static_cast<float>(textArea.getHeight()) / font.getHeight()))), minimumHorizontalScale);


    }
}

void Box::resized()
{
    if (graphics)
    {
        graphics->setBounds(getLocalBounds().reduced(margin));
    }

    constrainer.checkComponentBounds(this);

    if (auto* newEditor = getCurrentTextEditor())
    {
        newEditor->setBounds(getLocalBounds().reduced(margin));
    }

    const int edgeMargin = 18;
    const int doubleEdgeMargin = edgeMargin * 2;

    int index = 0;
    for (auto& edge : edges)
    {
        bool isInlet = edge->isInlet;
        int position = index < numInputs ? index : index - numInputs;
        int total = isInlet ? numInputs : numOutputs;

        float newY = isInlet ? margin : getHeight() - margin;
        float newX = position * ((getWidth() - doubleEdgeMargin) / (total - 1 + (total == 1))) + edgeMargin;

        edge->setCentrePosition(newX, newY);
        edge->setSize(12, 12);

        index++;
    }
}

void Box::updatePorts()
{
    // update inlets and outlets

    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto& edge : edges)
    {
        edge->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (pdObject)
    {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }

    while (numInputs < oldNumInputs) edges.remove(--oldNumInputs);
    while (numInputs > oldNumInputs) edges.insert(oldNumInputs++, new Edge(this, true));
    while (numOutputs < oldNumOutputs) edges.remove(numInputs + (--oldNumOutputs));
    while (numOutputs > oldNumOutputs) edges.insert(numInputs + (++oldNumOutputs), new Edge(this, false));

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++)
    {
        auto* edge = edges[i];
        bool input = edge->isInlet;
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);

        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;
        edge->setAlwaysOnTop(true);

        // Dont show for graphs or presentation mode
        edge->setVisible(!(cnv->isGraph || cnv->presentationMode == var(true)));
        edge->repaint();

        numIn += input;
        numOut += !input;
    }

    resized();
}

void Box::mouseDown(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->presentationMode == var(true) || cnv->pd->locked == var(true)) return;

    bool isSelected = cnv->isSelected(this);

    for (auto& rect : getCorners())
    {
        if (rect.contains(e.position) && isSelected)
        {
            // Start resize
            resizeZone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), e.getPosition());

            originalBounds = getBounds();

            return;
        }
    }

    cnv->handleMouseDown(this, e);

    if (isSelected != cnv->isSelected(this))
    {
        selectionChanged = true;
    }
}

void Box::mouseUp(const MouseEvent& e)
{
    resizeZone = ResizableBorderComponent::Zone();

    if (cnv->isGraph || cnv->presentationMode == var(true) || cnv->pd->locked == var(true)) return;

    if (editSingleClick && isEnabled() && contains(e.getPosition()) && !(e.mouseWasDraggedSinceMouseDown() || e.mods.isPopupMenu()) && cnv->isSelected(this) && !selectionChanged)
    {
        showEditor();
    }

    cnv->handleMouseUp(this, e);

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600)
    {
        cnv->connectingEdge = nullptr;
    }

    if (!originalBounds.isEmpty() && originalBounds.withPosition(0, 0) != getLocalBounds())
    {
        cnv->pd->enqueueFunction(
            [this]()
            {
                auto b = getBounds() - cnv->canvasOrigin;
                b.reduce(margin, margin);
                pdObject->setBounds(b);
            });

        cnv->checkBounds();

        originalBounds.setBounds(0, 0, 0, 0);
    }

    selectionChanged = false;
}

void Box::mouseDrag(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->presentationMode == var(true) || cnv->pd->locked == var(true)) return;

    if (resizeZone.isDraggingTopEdge() || resizeZone.isDraggingLeftEdge() || resizeZone.isDraggingBottomEdge() || resizeZone.isDraggingRightEdge())
    {
        auto newBounds = resizeZone.resizeRectangleBy(originalBounds, e.getOffsetFromDragStart());

        constrainer.setBoundsForComponent(this, newBounds, resizeZone.isDraggingTopEdge(), resizeZone.isDraggingLeftEdge(), resizeZone.isDraggingBottomEdge(), resizeZone.isDraggingRightEdge());

        return;
    }
    cnv->handleMouseDrag(e);
}

void Box::showEditor()
{
    if (editor == nullptr)
    {
        editor = std::make_unique<TextEditor>(getName());
        editor->applyFontToAllText(font);
        // copyAllExplicitColoursTo(*editor);

        /*
        copyColourIfSpecified(*this, *editor, Label::textWhenEditingColourId, TextEditor::textColourId);
        copyColourIfSpecified(*this, *editor, Label::backgroundWhenEditingColourId, TextEditor::backgroundColourId);
        copyColourIfSpecified(*this, *editor, Label::outlineWhenEditingColourId, TextEditor::focusedOutlineColourId); */

        editor->setAlwaysOnTop(true);

        bool multiLine = pdObject && pdObject->getType() == pd::Type::Comment;

        // Allow multiline for comment objects
        editor->setMultiLine(multiLine, false);
        editor->setReturnKeyStartsNewLine(multiLine);
        editor->setBorder(border);
        editor->setJustification(Justification::centred);

        editor->onFocusLost = [this]() { hideEditor(); };

        if (!(graphics && graphics->getGui().getType() == pd::Type::Comment))
        {
            cnv->showSuggestions(this, editor.get());
        }

        editor->setSize(10, 10);
        addAndMakeVisible(editor.get());
        editor->setText(currentText, false);
        editor->addListener(this);

        if (editor == nullptr)  // may be deleted by a callback
            return;

        editor->setHighlightedRegion(Range<int>(0, currentText.length()));

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

        if (graphics && !graphics->fakeGui())
        {
            setVisible(false);
            resized();
        }

        cnv->hideSuggestions();

        auto newText = outgoingEditor->getText();

        bool changed;
        if (currentText != newText)
        {
            currentText = newText;
            repaint();
            changed = true;
        }
        else
        {
            changed = false;
        }

        outgoingEditor.reset();

        repaint();

        // update if the name has changed, or if pdobject is unassigned
        if (changed || !pdObject)
        {
            setType(newText);
        }

        if (graphics && graphics->getGui().getType() == pd::Type::Comment)
        {
            updateBounds(false);
        }
    }
}

TextEditor* Box::getCurrentTextEditor() const noexcept
{
    return editor.get();
}

void Box::setEditable(bool editable)
{
    editSingleClick = editable;

    setWantsKeyboardFocus(editSingleClick);
    setFocusContainerType(editSingleClick ? FocusContainerType::keyboardFocusContainer : FocusContainerType::none);
    invalidateAccessibilityHandler();
}

void Box::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (editor != nullptr)
    {
        editor->giveAwayKeyboardFocus();
    }
}
