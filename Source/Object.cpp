/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Object.h"
#include <memory>

#include "Canvas.h"
#include "Connection.h"
#include "Iolet.h"
#include "LookAndFeel.h"

extern "C"
{
#include <m_pd.h>
#include <m_imp.h>
}

Object::Object(Canvas* parent, const String& name, Point<int> position) : cnv(parent)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    if (cnv->attachNextObjectToMouse)
    {
        cnv->attachNextObjectToMouse = false;
        attachedToMouse = true;
        startTimer(16);
    }

    initialise();

    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty())
    {
        setSize(100, height);
    }
    else
    {
        setType(name);
    }

    // Open the text editor of a new object if it has one
    // Don't do this if the object is attached to the mouse
    // Param objects are an exception where we don't want to open on mouse-down
    if (attachedToMouse && !name.startsWith("param"))
    {
        createEditorOnMouseDown = true;
    }
    else if(!attachedToMouse)
    {
        showEditor();
    }
}

Object::Object(void* object, Canvas* parent)
{
    cnv = parent;

    initialise();

    setType("", object);
}

Object::~Object()
{
    if(!cnv->isBeingDeleted) {
        // Ensure there's no pointer to this object in the selection
        cnv->setSelected(this, false);
    }
    
    if (attachedToMouse)
    {
        stopTimer();
    }
}

Rectangle<int> Object::getObjectBounds()
{
    return getBounds().reduced(margin) - cnv->canvasOrigin;
}

void Object::setObjectBounds(Rectangle<int> bounds)
{
    setBounds(bounds.expanded(margin) + cnv->canvasOrigin);
}

void Object::initialise()
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

    originalBounds.setBounds(0, 0, 0, 0);
}

void Object::timerCallback()
{
    auto pos = cnv->getMouseXYRelative();
    if (pos != getBounds().getCentre())
    {
        setCentrePosition(cnv->viewport->getViewArea().getConstrainedPoint(pos));
    }
}

void Object::valueChanged(Value& v)
{
    // Hide certain objects in GOP
    resized();

    if (gui)
    {
        gui->lock(locked == var(true) || commandLocked == var(true));
    }

    resized();
    repaint();
}

bool Object::hitTest(int x, int y)
{
    if(gui && !gui->canReceiveMouseEvent(x, y)) {
        return false;
    }
    
    // Mouse over object
    if (getLocalBounds().reduced(margin).contains(x, y))
    {
        return true;
    }

    // Mouse over iolets
    for (auto* iolet : iolets)
    {
        if (iolet->getBounds().contains(x, y)) return true;
    }

    // Mouse over corners
    if (cnv->isSelected(this))
    {
        for (auto& corner : getCorners())
        {
            if (corner.contains(x, y)) return true;
        }
    }

    return false;
}


// To make iolets show/hide
void Object::mouseEnter(const MouseEvent& e)
{
    repaint();
}

void Object::mouseExit(const MouseEvent& e)
{
    repaint();
}

void Object::mouseMove(const MouseEvent& e)
{
    if (!cnv->isSelected(this) || locked == var(true))
    {
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        return;
    }

    auto corners = getCorners();
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

void Object::updateBounds()
{
    if (gui)
    {
        cnv->pd->setThis();
        gui->updateBounds();
    }
    resized();
}

void Object::setType(const String& newType, void* existingObject)
{
    // Change object type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    void* objectPtr;
    // "exists" indicates that this object already exists in pd
    // When setting exists to true, the gui needs to be assigned already
    if (!existingObject)
    {
        auto* pd = &cnv->patch;
        if (gui)
        {
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections()) cnv->connections.removeObject(connection);

            objectPtr = pd->renameObject(getPointer(), newType);

            // Synchronise to make sure connections are preserved correctly
            // Asynchronous because it could possibly delete this object
            MessageManager::callAsync([cnv = SafePointer(cnv)]() {
                if(cnv) cnv->synchronise(false);
            });
        }
        else
        {
            auto rect = getObjectBounds();
            objectPtr = pd->createObject(newType, rect.getX(), rect.getY());
        }
    }
    else
    {
        objectPtr = existingObject;
    }

    // Create gui for the object
    gui.reset(GUIObject::createGui(objectPtr, this));

    if (gui)
    {
        gui->lock(locked == var(true));
        gui->updateValue();
        gui->addMouseListener(this, true);
        addAndMakeVisible(gui.get());
    }

    // Update inlets/outlets
    updatePorts();
    updateBounds();

    cnv->main.updateCommandStatus();
}

Array<Rectangle<float>> Object::getCorners() const
{
    auto rect = getLocalBounds().reduced(margin);
    const float offset = 2.0f;

    Array<Rectangle<float>> corners = {Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
                                       Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset)};

    return corners;
}

void Object::paintOverChildren(Graphics& g)
{
    if (attachedToMouse)
    {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets)
        {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::lightgreen);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), 2.0f, 2.0f);

        g.restoreState();
    }
}

void Object::paint(Graphics& g)
{
    if (cnv->isSelected(this) && !cnv->isGraph)
    {
        g.setColour(findColour(PlugDataColour::highlightColourId));

        g.saveState();
        g.excludeClipRegion(getLocalBounds().reduced(margin + 1));

        // Draw resize iolets when selected
        for (auto& rect : getCorners())
        {
            g.fillRoundedRectangle(rect, 2.0f);
        }
        g.restoreState();
    }
}

void Object::resized()
{
    setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));

    if (gui)
    {
        gui->setBounds(getLocalBounds().reduced(margin));
    }

    if (newObjectEditor)
    {
        newObjectEditor->setBounds(getLocalBounds().reduced(margin));
    }

    int ioletSize = 13;
    int ioletHitBox = 4;
    int borderWidth = 14;

    if (getWidth() < 45 && (numInputs > 1 || numOutputs > 1))
    {
        borderWidth = 9;
        ioletSize = 10;
    }

    auto inletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, inletBounds.getWidth() - (ioletHitBox * numInputs) - borderWidth))
    {
        inletBounds.removeFromLeft(spaceToRemove);
        inletBounds.removeFromRight(spaceToRemove);
    }

    auto outletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, outletBounds.getWidth() - (ioletHitBox * numOutputs) - borderWidth))
    {
        outletBounds.removeFromLeft(spaceToRemove);
        outletBounds.removeFromRight(spaceToRemove);
    }

    int index = 0;
    for (auto& iolet : iolets)
    {
        const bool isInlet = iolet->isInlet;
        const int position = index < numInputs ? index : index - numInputs;
        const int total = isInlet ? numInputs : numOutputs;
        const float yPosition = (isInlet ? (margin + 1) : getHeight() - margin) - ioletSize / 2.0f;

        const auto bounds = isInlet ? inletBounds : outletBounds;

        if (total == 1 && position == 0)
        {
            int xPosition = getWidth() < 40 ? getLocalBounds().getCentreX() - ioletSize / 2.0f : bounds.getX();
            iolet->setBounds(xPosition, yPosition, ioletSize, ioletSize);
        }
        else if (total > 1)
        {
            const float ratio = (bounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
            iolet->setBounds(bounds.getX() + ratio * position, yPosition, ioletSize, ioletSize);
        }

        index++;
    }
}

void Object::updatePorts()
{
    if (!getPointer()) return;

    // update inlets and outlets

    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto& iolet : iolets)
    {
        iolet->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (auto* ptr = pd::Patch::checkObject(getPointer()))
    {
        numInputs = libpd_ninlets(ptr);
        numOutputs = libpd_noutlets(ptr);
    }

    while (numInputs < oldNumInputs) iolets.remove(--oldNumInputs);
    while (numInputs > oldNumInputs) iolets.insert(oldNumInputs++, new Iolet(this, true));
    while (numOutputs < oldNumOutputs) iolets.remove(numInputs + (--oldNumOutputs));
    while (numOutputs > oldNumOutputs) iolets.insert(numInputs + (++oldNumOutputs), new Iolet(this, false));
    
    if(gui) {
        gui->setTooltip(cnv->pd->objectLibrary.getObjectDescriptions()[gui->getType()]);
    }

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++)
    {
        auto* iolet = iolets[i];
        bool input = iolet->isInlet;

        bool isSignal;
        if (i < numInputs)
        {
            isSignal = libpd_issignalinlet(pd::Patch::checkObject(getPointer()), i);
        }
        else
        {
            isSignal = libpd_issignaloutlet(pd::Patch::checkObject(getPointer()), i - numInputs);
        }

        iolet->ioletIdx = input ? numIn : numOut;
        iolet->isSignal = isSignal;
        iolet->setAlwaysOnTop(true);

        if (gui)
        {
            String tooltip = cnv->pd->objectLibrary.getInletOutletTooltip(gui->getType(), iolet->ioletIdx, input ? numInputs : numOutputs, input);
            iolet->setTooltip(tooltip);
        }

        // Don't show for graphs or presentation mode
        iolet->setVisible(!(cnv->isGraph || cnv->presentationMode == var(true)));
        iolet->repaint();

        numIn += input;
        numOut += !input;
    }

    resized();
}

void Object::mouseDown(const MouseEvent& e)
{
    if(!getLocalBounds().contains(e.getPosition())) return;
    
    if (!static_cast<bool>(locked.getValue()) && ModifierKeys::getCurrentModifiers().isAltDown())
    {
        openHelpPatch();
    }

    if (attachedToMouse)
    {
        attachedToMouse = false;
        stopTimer();
        repaint();

        auto object = SafePointer<Object>(this);
        // Tell pd about new position
        cnv->pd->enqueueFunction(
            [this, object]()
            {
                if (!object || !object->gui) return;
                gui->applyBounds();
            });

        if (createEditorOnMouseDown)
        {
            createEditorOnMouseDown = false;

            // Prevent losing focus because of click event
            MessageManager::callAsync([_this = SafePointer(this)]() { _this->showEditor(); });
        }

        return;
    }

    if (cnv->isGraph || cnv->presentationMode == var(true) || locked == var(true) || commandLocked == var(true)) {
        wasLockedOnMouseDown = true;
        return;
    }
        
    wasLockedOnMouseDown = false;
    
    for (auto& rect : getCorners())
    {
        if (rect.contains(e.position) && cnv->isSelected(this))
        {
            // Start resize
            resizeZone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), e.getPosition());

            if(resizeZone != ResizableBorderComponent::Zone(0)) {
                originalBounds = getBounds();
                return;
            }
        }
    }

    bool wasSelected = cnv->isSelected(this);
    
    cnv->handleMouseDown(this, e);
    
    //
    if(cnv->isSelected(this) != wasSelected) {
        selectionStateChanged = true;
    }
    
}

void Object::mouseUp(const MouseEvent& e)
{
    resizeZone = ResizableBorderComponent::Zone();

    if (wasLockedOnMouseDown) return;

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600)
    {
        cnv->connectingEdges.clear();
    }

    if (!originalBounds.isEmpty() && originalBounds.withPosition(0, 0) != getLocalBounds())
    {
        originalBounds.setBounds(0, 0, 0, 0);

        cnv->pd->enqueueFunction(
            [this, object = SafePointer<Object>(this), e]() mutable
            {
                if (!object || !gui) return;

                // Used for size changes, could also be used for properties
                auto* obj = static_cast<t_gobj*>(getPointer());
                auto* canvas = static_cast<t_canvas*>(cnv->patch.getPointer());
                libpd_undo_apply(canvas, obj);

                gui->applyBounds();

                // To make sure it happens after setting object bounds
                if (!cnv->viewport->getViewArea().contains(getBounds()))
                {
                    MessageManager::callAsync(
                        [object]()
                        {
                            if(object) object->cnv->checkBounds();
                        });
                }
            });
    }
    else
    {
        cnv->handleMouseUp(this, e);
    }
    
    if(gui && !selectionStateChanged && cnv->isSelected(this) && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isRightButtonDown()) {
        gui->showEditor();
    }
    
    selectionStateChanged = false;
}

void Object::mouseDrag(const MouseEvent& e)
{
    if (wasLockedOnMouseDown) return;
    
    if (resizeZone.isDraggingTopEdge() || resizeZone.isDraggingLeftEdge() || resizeZone.isDraggingBottomEdge() || resizeZone.isDraggingRightEdge())
    {
        Point<int> dragDistance = e.getOffsetFromDragStart();

        auto newBounds = resizeZone.resizeRectangleBy(originalBounds, dragDistance);
        setBounds(newBounds);
        if(gui) gui->checkBounds();
        
    }
    // Let canvas handle moving
    else
    {
        cnv->handleMouseDrag(e);
    }
}

void Object::showEditor()
{
    if (!gui)
    {
        openNewObjectEditor();
    }
    else
    {
        gui->showEditor();
    }
}

void Object::hideEditor()
{
    if (gui)
    {
        gui->hideEditor();
    }
    else if (newObjectEditor)
    {
        WeakReference<Component> deletionChecker(this);
        std::unique_ptr<TextEditor> outgoingEditor;
        std::swap(outgoingEditor, newObjectEditor);

        outgoingEditor->setInputFilter(nullptr, false);

        cnv->hideSuggestions();

        // Get entered text, remove extra spaces at the end
        auto newText = outgoingEditor->getText().trimEnd();
        outgoingEditor.reset();

        repaint();
        setType(newText);
    }
}

Array<Connection*> Object::getConnections() const
{
    Array<Connection*> result;
    for (auto* con : cnv->connections)
    {
        for (auto* iolet : iolets)
        {
            if (con->inlet == iolet || con->outlet == iolet)
            {
                result.add(con);
            }
        }
    }
    return result;
}

void* Object::getPointer() const
{
    return gui ? gui->ptr : nullptr;
}

void Object::openNewObjectEditor()
{
    if (!newObjectEditor)
    {
        newObjectEditor = std::make_unique<TextEditor>();

        auto* editor = newObjectEditor.get();
        editor->applyFontToAllText(Font(14.5));

        copyAllExplicitColoursTo(*editor);
        editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
        editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
        editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

        // Allow cancelling object creation with escape
        editor->onEscapeKey = [this](){
            MessageManager::callAsync([_this = SafePointer(this)](){
                if(!_this) return;
                _this->cnv->objects.removeObject(_this.getComponent());
            });
        };
        editor->setAlwaysOnTop(true);
        editor->setMultiLine(false);
        editor->setReturnKeyStartsNewLine(false);
        editor->setBorder(BorderSize<int>{1, 7, 1, 2});
        editor->setIndents(0, 0);
        editor->setJustification(Justification::left);

        editor->onFocusLost = [this, editor]()
        {
            if(reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor) {
                editor->grabKeyboardFocus();
                 return;
            }
           
            hideEditor();
        };

        cnv->showSuggestions(this, editor);

        editor->setSize(10, 10);
        addAndMakeVisible(editor);
        editor->addListener(this);

        resized();
        repaint();

        editor->grabKeyboardFocus();
    }
}

void Object::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (newObjectEditor)
    {
        newObjectEditor->giveAwayKeyboardFocus();
    }
}

void Object::textEditorTextChanged(TextEditor& ed)
{
    // For resize-while-typing behaviour
    auto width = Font(14.5).getStringWidth(ed.getText()) + 25;

    if (width > getWidth())
    {
        setSize(width, getHeight());
    }
}

void Object::openHelpPatch() const
{
    cnv->pd->setThis();

    if (auto* ptr = static_cast<t_object*>(getPointer())) {
        
        auto file = cnv->pd->objectLibrary.findHelpfile(ptr);
        
        if(!file.existsAsFile()) {
            cnv->pd->logMessage("Couldn't find help file");
            return;
        }
        
        cnv->pd->enqueueFunction([this, file]() mutable {
            cnv->pd->loadPatch(file);
            
        });
        
        return;
    }
    
    cnv->pd->logMessage("Couldn't find help file");
}
