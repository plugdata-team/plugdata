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
#include "LookAndFeel.h"

Box::Box(Canvas* parent, const String& name, Point<int> position)
{
    cnv = parent;

    setTopLeftPosition(position - Point<int>(margin, margin));

    if (cnv->attachNextObjectToMouse)
    {
        cnv->attachNextObjectToMouse = false;
        startTimer(20);
        attachedToMouse = true;
    }

    initialise();
    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty())
    {
        setSize(100, height);
        showEditor();
        toFront(false);
    }
    else if (name == "msg")
    {
        setType(name);
        graphics->showEditor();
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

Box::~Box()
{
    if (attachedToMouse)
    {
        stopTimer();
    }
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

void Box::timerCallback()
{
    auto pos = cnv->getMouseXYRelative();
    if (pos != getBounds().getCentre())
    {
        setCentrePosition(cnv->getBounds().getConstrainedPoint(pos));
    }
}

void Box::valueChanged(Value& v)
{
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
    if (pdObject)
    {
        auto bounds = pdObject->getBounds() - Point<int>(margin, margin);
        bounds.translate(cnv->canvasOrigin.x, cnv->canvasOrigin.y);

        if (bounds.getPosition().getDistanceFrom(getPosition()) >= 2.0f)
        {
            setTopLeftPosition(bounds.getPosition());
        }

        if (graphics && !graphics->noGui())
        {
            addAndMakeVisible(graphics.get());
            auto b = pdObject->getBounds();

            int width = b.getWidth() <= 0 ? getBestTextWidth() : b.getWidth() + doubleMargin;

            setSize(width, b.getHeight() + doubleMargin);
            graphics->resized();
            graphics->toBack();
            hideLabel = true;
            setEditable(false);
        }
        else
        {
            setEditable(true);
            hideLabel = false;

            int width = bounds.getWidth() <= 0 ? getBestTextWidth() : bounds.getWidth() + doubleMargin;

            setSize(width, height);
        }
    }
    else
    {
        hideLabel = false;
        setEditable(true);
        setSize(100, Box::height);
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
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections()) cnv->connections.removeObject(connection);

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

        if (pdObject->getType() == pd::Type::Invalid)
        {
            setSize(100, getHeight());
        }
    }

    // Update inlets/outlets
    updatePorts();
    updateBounds(!exists);

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
    auto outlineColour = findColour(PlugDataColour::canvasOutlineColourId);

    bool selected = cnv->isSelected(this);

    float thickness = 1.0f;
    if (attachedToMouse)
    {
        outlineColour = Colours::lightgreen;
        thickness = 2.0f;
    }
    else if (pdObject && pdObject->getType() == pd::Type::Invalid && !getCurrentTextEditor())
    {
        outlineColour = Colours::red;
        if (selected) outlineColour = outlineColour.brighter(1.3f);
    }
    else if (selected && !cnv->isGraph)
    {
        outlineColour = findColour(PlugDataColour::highlightColourId);
        g.setColour(outlineColour);

        // Draw resize edges when selected
        for (auto& rect : getCorners()) g.fillRoundedRectangle(rect, 2.0f);
    }

    if (!graphics || (graphics && graphics->noGui()))
    {
        g.setColour(findColour(ResizableWindow::backgroundColourId));
        g.fillRect(getLocalBounds().reduced(margin));
    }

    // Draw comment style
    if (graphics && graphics->getGui().getType() == pd::Type::Comment)
    {
        if (locked == var(false) && (isOver || selected) && !cnv->isGraph)
        {
            g.setColour(selected ? findColour(PlugDataColour::highlightColourId) : findColour(PlugDataColour::canvasOutlineColourId));
            g.drawRect(rect.toFloat(), 0.5f);
        }
    }
    // Draw for all other objects
    else
    {
        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, thickness);
    }

    if (!hideLabel && !editor)
    {
        g.setColour(findColour(ResizableWindow::backgroundColourId).contrasting());
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

    // graphical objects manage their own size limits
    // For text object, make sure the width at least fits the text
    if (!graphics || graphics->noGui())
    {
        // Estimate the amount of with needed to fit inlets and outlets
        int ioletWidth = std::max(numInputs, numOutputs) * 15;

        // Width of text
        int textWidth = getBestTextWidth();

        // Round width of text objects to pd's resolution, make sure that the ideal size is one of the options
        auto roundToTextWidth = [this](int width, int stringWidth)
        {
            int multiple = glist_fontwidth(cnv->patch.getPointer());
            int offset = stringWidth % multiple;
            return (((width + multiple - 1) / multiple) * multiple) - offset;
        };

        // Calculate height based on number of lines, and width
        int width = std::max(getWidth(), ioletWidth + doubleMargin);
        int height = Box::height + ((getNumLines(currentText, width - 13, font) - 1) * 28);

        // Recursive resize is a bit tricky, but since these variables are very predictable,
        // it won't be a problem
        if (getWidth() != width || getHeight() != height)
        {
            setSize(width, height);
        }
    }

    if (auto* newEditor = getCurrentTextEditor())
    {
        newEditor->setBounds(getLocalBounds().reduced(margin));
    }

    int edgeSize = 12;
    const int edgeHitBox = 8;
    const int borderWidth = 14;

    if (getWidth() < 35)
    {
        edgeSize = std::max(edgeSize - (35 - getWidth()), 10);
    }

    auto inletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, inletBounds.getWidth() - (edgeHitBox * numInputs) - borderWidth))
    {
        inletBounds.removeFromLeft(spaceToRemove);
        inletBounds.removeFromRight(spaceToRemove);
    }

    auto outletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, outletBounds.getWidth() - (edgeHitBox * numOutputs) - borderWidth))
    {
        outletBounds.removeFromLeft(spaceToRemove);
        outletBounds.removeFromRight(spaceToRemove);
    }

    int index = 0;
    for (auto& edge : edges)
    {
        const bool isInlet = edge->isInlet;
        const int position = index < numInputs ? index : index - numInputs;
        const int total = isInlet ? numInputs : numOutputs;
        const float yPosition = (isInlet ? margin : getHeight() - margin) - edgeSize / 2.0f;

        const auto bounds = isInlet ? inletBounds : outletBounds;

        if (total == 1 && position == 0)
        {
            int xPosition = getWidth() < 40 ? getLocalBounds().getCentreX() - edgeSize / 2.0f : bounds.getX();
            edge->setBounds(xPosition, yPosition, edgeSize, edgeSize);
        }
        else if (total > 1)
        {
            const double ratio = (bounds.getWidth() - edgeSize) / (double)(total - 1);
            edge->setBounds(bounds.getX() + ratio * position, yPosition, edgeSize, edgeSize);
        }

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

        String tooltip = cnv->pd->objectLibrary.getInletOutletTooltip(currentText, edge->edgeIdx, input ? numInputs : numOutputs, input);
        edge->setTooltip(tooltip);

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
    if (attachedToMouse)
    {
        attachedToMouse = false;
        stopTimer();
        repaint();
    }

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

    if (wasResized) cnv->grid.handleMouseUp(Point<int>());

    cnv->handleMouseUp(this, e);

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600)
    {
        cnv->connectingEdge = nullptr;
    }

    if (!originalBounds.isEmpty() && originalBounds.withPosition(0, 0) != getLocalBounds())
    {
        originalBounds.setBounds(0, 0, 0, 0);

        cnv->pd->enqueueFunction(
            [this]()
            {
                auto b = getBounds() - cnv->canvasOrigin;
                b.reduce(margin, margin);
                pdObject->setBounds(b);

                // To make sure it happens after setting object bounds
                if (!cnv->viewport->getViewArea().contains(getBounds()))
                {
                    MessageManager::callAsync([this]() { cnv->checkBounds(); });
                }
            });
    }
    else if (!cnv->viewport->getViewArea().contains(getBounds()))
    {
        cnv->checkBounds();
    }

    selectionChanged = false;
}

void Box::mouseDrag(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->presentationMode == var(true) || cnv->pd->locked == var(true)) return;

    if (resizeZone.isDraggingTopEdge() || resizeZone.isDraggingLeftEdge() || resizeZone.isDraggingBottomEdge() || resizeZone.isDraggingRightEdge())
    {
        wasResized = true;
        Point<int> dragDistance = e.getOffsetFromDragStart();

        int distance = resizeZone.resizeRectangleBy(originalBounds, dragDistance).getWidth() - getBestTextWidth();
        if (abs(distance) < ObjectGrid::range)
        {
            cnv->grid.setSnapped(ObjectGrid::BestSizeSnap, this, {dragDistance.x - distance, dragDistance.y});
        }

        if (static_cast<bool>(cnv->gridEnabled.getValue()))
        {
            dragDistance = cnv->grid.handleMouseDrag(this, dragDistance, cnv->viewport->getViewArea());
        }

        auto newBounds = resizeZone.resizeRectangleBy(originalBounds, dragDistance);

        setBounds(newBounds);

        return;
    }
    // Let canvas handle moving
    else
    {
        cnv->handleMouseDrag(e);
    }
}

void Box::showEditor()
{
    if (editor == nullptr)
    {
        editor = std::make_unique<TextEditor>(getName());
        editor->applyFontToAllText(font);

        copyAllExplicitColoursTo(*editor);
        editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
        editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
        editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

        editor->setAlwaysOnTop(true);

        editor->setMultiLine(false);
        editor->setReturnKeyStartsNewLine(false);
        editor->setBorder(border);
        editor->setIndents(0, 0);
        editor->setJustification(justification);

        editor->onFocusLost = [this]()
        {
            // Necessary so the editor doesn't close when clicking on a suggestion
            if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true))
            {
                hideEditor();
            }
        };

        cnv->showSuggestions(this, editor.get());

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

        // not needed?
        outgoingEditor->setInputFilter(nullptr, false);

        if (graphics && !graphics->noGui())
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
    }
}

Array<Connection*> Box::getConnections() const
{
    Array<Connection*> result;
    for (auto* con : cnv->connections)
    {
        for (auto* edge : edges)
        {
            if (con->inlet == edge || con->outlet == edge)
            {
                result.add(con);
            }
        }
    }
    return result;
}

TextEditor* Box::getCurrentTextEditor() const noexcept
{
    return editor.get();
}

int Box::getBestTextWidth()
{
    return std::max<float>(round(font.getStringWidthFloat(currentText) + 30.5f), 50);
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

void Box::textEditorTextChanged(TextEditor& ed)
{
    currentText = ed.getText();
    // For resize-while-typing behaviour
    auto width = getBestTextWidth();

    if (width > getWidth())
    {
        setSize(width, getHeight());
    }
}
