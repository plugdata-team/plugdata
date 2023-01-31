/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Object.h"
#include <memory>

#include "Canvas.h"
#include "SuggestionComponent.h"
#include "Connection.h"
#include "Iolet.h"
#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "Utility/ObjectBoundsConstrainer.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
#include <x_libpd_extra_utils.h>
}

Object::Object(Canvas* parent, String const& name, Point<int> position)
    : cnv(parent)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    if (cnv->attachNextObjectToMouse) {
        cnv->attachNextObjectToMouse = false;
        attachedToMouse = true;
        startTimer(16);
    }

    initialise();

    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty()) {
        setSize(100, height);
    } else {
        setType(name);
    }

    // Open the text editor of a new object if it has one
    // Don't do this if the object is attached to the mouse
    // Param objects are an exception where we don't want to open on mouse-down
    if (attachedToMouse && !name.startsWith("param") && !static_cast<bool>(locked.getValue())) {
        createEditorOnMouseDown = true;
    } else if (!attachedToMouse && !static_cast<bool>(locked.getValue())) {
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
    if (attachedToMouse) {
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
    constrainer = std::make_unique<ObjectBoundsConstrainer>();
    
    addMouseListener(cnv, true); // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    // Updates lock/unlock mode
    locked.referTo(cnv->locked);
    commandLocked.referTo(cnv->pd->commandLocked);
    presentationMode.referTo(cnv->presentationMode);
    hvccMode.referTo(cnv->editor->hvccMode);

    presentationMode.addListener(this);
    locked.addListener(this);
    commandLocked.addListener(this);
    hvccMode.addListener(this);

    originalBounds.setBounds(0, 0, 0, 0);
    constrainer->setMinimumSize(12, 12);
}

void Object::timerCallback()
{
    auto pos = cnv->getMouseXYRelative();
    if (pos != getBounds().getCentre()) {
        auto viewArea = cnv->viewport->getViewArea() / static_cast<float>(cnv->editor->zoomScale.getValue());
        setCentrePosition(viewArea.getConstrainedPoint(pos));
    }
}

void Object::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(hvccMode)) {
        if (gui) {
            auto typeName = String::fromUTF8(libpd_get_object_class_name(gui->ptr));
            // Check hvcc compatibility
            bool isSubpatch = gui ? gui->getPatch() != nullptr : false;
            isHvccCompatible = !static_cast<bool>(hvccMode.getValue()) || isSubpatch || hvccObjects.contains(typeName);

            if (!isHvccCompatible) {
                cnv->pd->logWarning(String("Warning: object \"" + typeName + "\" is not supported in Compiled Mode").toRawUTF8());
            }

            repaint();
        }

        return;
    }

    // else it was a lock/unlock action
    // Hide certain objects in GOP
    // resized();

    if (gui) {
        gui->lock(locked == var(true) || commandLocked == var(true));
    }

    repaint();
}

bool Object::hitTest(int x, int y)
{
    if (gui && !gui->canReceiveMouseEvent(x, y)) {
        return false;
    }

    // Mouse over object
    if (getLocalBounds().reduced(margin).contains(x, y)) {
        return true;
    }

    // Mouse over iolets
    for (auto* iolet : iolets) {
        if (iolet->getBounds().contains(x, y))
            return true;
    }
    
    // Mouse over corners
    if (cnv->isSelected(this)) {
        
        for (auto& corner : getCorners()) {
            if (corner.contains(x, y))
                return true;
        }
        
        return getLocalBounds().reduced(1, margin).contains(x, y);
    }

    return false;
}

// To make iolets show/hide
void Object::mouseEnter(MouseEvent const& e)
{
    repaint();
}

void Object::mouseExit(MouseEvent const& e)
{
    repaint();
}

void Object::mouseMove(MouseEvent const& e)
{
    if (!cnv->isSelected(this) || locked == var(true) || commandLocked == var(true)) {
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        return;
    }
    
    resizeZone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), Point<int>(e.x, e.y));
    
    validResizeZone = resizeZone.getZoneFlags() != ResizableBorderComponent::Zone::centre;
    
    setMouseCursor(validResizeZone ? resizeZone.getMouseCursor() : MouseCursor::NormalCursor);
    updateMouseCursor();
}

void Object::updateBounds()
{
    if (gui) {
        cnv->pd->setThis();
        gui->updateBounds();
    }
    
    if(newObjectEditor) {
        textEditorTextChanged(*newObjectEditor);
    }
    
    resized();
}

void Object::setType(String const& newType, void* existingObject)
{
    // Change object type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    void* objectPtr;
    // "exists" indicates that this object already exists in pd
    // When setting exists to true, the gui needs to be assigned already
    if (!existingObject || cnv->patch.objectWasDeleted(existingObject)) {
        auto* pd = &cnv->patch;
        if (gui) {
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections())
                cnv->connections.removeObject(connection);

            objectPtr = pd->renameObject(getPointer(), newType);

            // Synchronise to make sure connections are preserved correctly
            // Asynchronous because it could possibly delete this object
            MessageManager::callAsync([cnv = SafePointer(cnv)]() {
                if (cnv)
                    cnv->synchronise(false);
            });
        } else {
            auto rect = getObjectBounds();
            objectPtr = pd->createObject(newType, rect.getX(), rect.getY());
        }
    } else {
        objectPtr = existingObject;
    }

    // Create gui for the object
    gui.reset(ObjectBase::createGui(objectPtr, this));

    if (gui) {
        gui->lock(locked == var(true));
        gui->addMouseListener(this, true);
        addAndMakeVisible(gui.get());
    }

    auto typeName = String::fromUTF8(libpd_get_object_class_name(objectPtr));
    // Check hvcc compatibility
    bool isSubpatch = gui ? gui->getPatch() != nullptr : false;
    isHvccCompatible = !static_cast<bool>(hvccMode.getValue()) || isSubpatch || hvccObjects.contains(typeName);

    if (!isHvccCompatible) {
        cnv->pd->logWarning(String("Warning: object \"" + typeName + "\" is not supported in Compiled Mode").toRawUTF8());
    }

    // Update inlets/outlets
    updateIolets();
    updateBounds();

    // Auto patching
    if (static_cast<bool>(!attachedToMouse && cnv->editor->autoconnect.getValue()) && numInputs && cnv->lastSelectedObject && cnv->lastSelectedObject->numOutputs) {
        auto outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        auto inlet = iolets[0];
        if (outlet->isSignal == inlet->isSignal) {
            cnv->connections.add(new Connection(cnv, outlet, inlet, nullptr));
        }
    }
    if (cnv->lastSelectedConnection) {
        // if 1 connection is selected, connect the new object in middle of connection
        auto outobj = cnv->lastSelectedConnection->outobj;
        auto inobj = cnv->lastSelectedConnection->inobj;
        auto outlet = outobj->iolets[outobj->numInputs + cnv->lastSelectedConnection->outIdx];
        auto inlet = inobj->iolets[cnv->lastSelectedConnection->inIdx];
        if ((outlet->isSignal == iolets[0]->isSignal) && (inlet->isSignal == iolets[this->numInputs]->isSignal)) {
            cnv->connections.add(new Connection(cnv, outlet, iolets[0], nullptr));
            cnv->connections.add(new Connection(cnv, iolets[this->numInputs], inlet, nullptr));
            // remove the previous connection
            cnv->patch.removeConnection(outobj->getPointer(), cnv->lastSelectedConnection->outIdx, inobj->getPointer(), cnv->lastSelectedConnection->inIdx, cnv->lastSelectedConnection->getPathState());
            cnv->connections.removeObject(cnv->lastSelectedConnection);
        }
    }
    cnv->lastSelectedObject = nullptr;
    cnv->lastSelectedConnection = nullptr;

    cnv->editor->updateCommandStatus();
}

Array<Rectangle<float>> Object::getCorners() const
{
    auto rect = getLocalBounds().reduced(margin);
    float const offset = 2.0f;

    Array<Rectangle<float>> corners = { Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
        Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset) };

    return corners;
}

void Object::paintOverChildren(Graphics& g)
{
    if (isSearchTarget) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::violet);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), PlugDataLook::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (!isHvccCompatible) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::orange);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), PlugDataLook::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (attachedToMouse) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::lightgreen);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), PlugDataLook::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (indexShown) {
        int halfHeight = 5;

        auto text = String(cnv->objects.indexOf(this));
        int textWidth = Fonts::getMonospaceFont().withHeight(10).getStringWidth(text) + 5;
        int left = std::min<int>(getWidth() - (1.5 * margin), getWidth() - textWidth);

        auto indexBounds = Rectangle<int>(left, (getHeight() / 2) - halfHeight, getWidth() - left, halfHeight * 2);

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
        g.fillRoundedRectangle(indexBounds.toFloat(), 2.0f);

        PlugDataLook::drawStyledText(g, text, indexBounds, findColour(PlugDataColour::objectSelectedOutlineColourId).contrasting(), Monospace, 10, Justification::centred);
    }
}

void Object::showIndex(bool shouldShowIndex)
{
    if (shouldShowIndex != indexShown) {
        indexShown = shouldShowIndex;
        repaint();
    }
}

void Object::paint(Graphics& g)
{
    if ((cnv->isSelected(this) && !cnv->isGraph) || newObjectEditor) {

        if (newObjectEditor) {

            g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), PlugDataLook::objectCornerRadius);

            g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), PlugDataLook::objectCornerRadius, 1.0f);
        }

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));

        g.saveState();
        g.excludeClipRegion(getLocalBounds().reduced(margin + 1));

        for (auto& rect : getCorners()) {
            g.fillRoundedRectangle(rect, PlugDataLook::objectCornerRadius);
        }
        g.restoreState();
    }
}

void Object::resized()
{
    setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));

    if (gui) {
        gui->setBounds(getLocalBounds().reduced(margin));
    }

    if (newObjectEditor) {
        newObjectEditor->setBounds(getLocalBounds().reduced(margin));
    }

    int ioletSize = 13;
    int ioletHitBox = 4;

    int maxIoletWidth = std::min(((getWidth() - doubleMargin) / std::max(numInputs, 1)) - 4, ((getWidth() - doubleMargin) / std::max(numOutputs, 1)) - 4);
    int maxIoletHeight = (getHeight() / 2.0f) - 3;

    ioletSize = std::max(std::min({ ioletSize, maxIoletWidth, maxIoletHeight }), 10);
    int borderWidth = jmap<float>(ioletSize, 10, 13, 9, 14);

    auto inletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, inletBounds.getWidth() - (ioletHitBox * numInputs) - borderWidth)) {
        inletBounds.removeFromLeft(spaceToRemove);
        inletBounds.removeFromRight(spaceToRemove);
    }

    auto outletBounds = getLocalBounds();
    if (auto spaceToRemove = jlimit<int>(0, borderWidth, outletBounds.getWidth() - (ioletHitBox * numOutputs) - borderWidth)) {
        outletBounds.removeFromLeft(spaceToRemove);
        outletBounds.removeFromRight(spaceToRemove);
    }

    int index = 0;
    for (auto& iolet : iolets) {
        bool const isInlet = iolet->isInlet;
        int const position = index < numInputs ? index : index - numInputs;
        int const total = isInlet ? numInputs : numOutputs;
        float const yPosition = (isInlet ? (margin + 1) : getHeight() - margin) - ioletSize / 2.0f;

        auto const bounds = isInlet ? inletBounds : outletBounds;

        if (total == 1 && position == 0) {
            int xPosition = getWidth() < 40 ? getLocalBounds().getCentreX() - ioletSize / 2.0f : bounds.getX();
            iolet->setBounds(xPosition, yPosition, ioletSize, ioletSize);
        } else if (total > 1) {
            float const ratio = (bounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
            iolet->setBounds(bounds.getX() + ratio * position, yPosition, ioletSize, ioletSize);
        }

        index++;
    }
}

void Object::updateTooltips()
{
    if (!gui)
        return;

    std::vector<std::pair<int, String>> inletMessages;
    std::vector<std::pair<int, String>> outletMessages;

    // Set object tooltip
    gui->setTooltip(cnv->pd->objectLibrary.getObjectDescriptions()[gui->getType()]);

    if (auto* subpatch = gui->getPatch()) {

        // Check child objects of subpatch for inlet/outlet messages
        for (auto* obj : subpatch->getObjects()) {

            const String name = libpd_get_object_class_name(obj);
            if (name == "inlet" || name == "inlet~") {

                int size;
                char* str_ptr;
                libpd_get_object_text(obj, &str_ptr, &size);

                int x, y, w, h;
                libpd_get_object_bounds(subpatch->getPointer(), obj, &x, &y, &w, &h);

                // Anything after the first space will be the comment
                auto const text = String::fromUTF8(str_ptr, size);
                inletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));
                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
            if (name == "outlet" || name == "outlet~") {
                int size;
                char* str_ptr;
                libpd_get_object_text(obj, &str_ptr, &size);

                int x, y, w, h;
                libpd_get_object_bounds(subpatch->getPointer(), obj, &x, &y, &w, &h);

                auto const text = String::fromUTF8(str_ptr, size);
                outletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));

                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
        }
    }

    auto sortFunc = [](std::pair<int, String>& a, std::pair<int, String>& b) {
        return a.first < b.first;
    };

    std::sort(inletMessages.begin(), inletMessages.end(), sortFunc);
    std::sort(outletMessages.begin(), outletMessages.end(), sortFunc);

    int numIn = 0;
    int numOut = 0;

    // Check pd library for pddp tooltips, those have priority
    auto ioletTooltips = cnv->pd->objectLibrary.getIoletTooltips(gui->getType(), gui->getText(), numInputs, numOutputs);

    for (int i = 0; i < iolets.size(); i++) {
        auto* iolet = iolets[i];

        auto& tooltip = ioletTooltips[!iolet->isInlet][iolet->isInlet ? i : i - numInputs];

        // Don't overwrite custom documentation
        if (tooltip.isNotEmpty()) {
            iolet->setTooltip(tooltip);
            continue;
        }

        if ((iolet->isInlet && numIn >= inletMessages.size()) || (!iolet->isInlet && numOut >= outletMessages.size()))
            continue;

        auto& [x, message] = iolet->isInlet ? inletMessages[numIn++] : outletMessages[numOut++];
        iolet->setTooltip(message);
    }
}

void Object::updateIolets()
{
    if (!getPointer())
        return;

    // update inlets and outlets
    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto& iolet : iolets) {
        iolet->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (auto* ptr = pd::Patch::checkObject(getPointer())) {
        numInputs = libpd_ninlets(ptr);
        numOutputs = libpd_noutlets(ptr);
    }

    while (numInputs < oldNumInputs)
        iolets.remove(--oldNumInputs);
    while (numInputs > oldNumInputs)
        iolets.insert(oldNumInputs++, new Iolet(this, true));
    while (numOutputs < oldNumOutputs)
        iolets.remove(numInputs + (--oldNumOutputs));
    while (numOutputs > oldNumOutputs)
        iolets.insert(numInputs + (++oldNumOutputs), new Iolet(this, false));

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++) {
        auto* iolet = iolets[i];
        bool input = iolet->isInlet;

        bool isSignal;
        if (i < numInputs) {
            isSignal = libpd_issignalinlet(pd::Patch::checkObject(getPointer()), i);
        } else {
            isSignal = libpd_issignaloutlet(pd::Patch::checkObject(getPointer()), i - numInputs);
        }

        iolet->ioletIdx = input ? numIn : numOut;
        iolet->isSignal = isSignal;
        iolet->repaint();

        numIn += input;
        numOut += !input;
    }

    updateTooltips();
    resized();
}

void Object::mouseDown(MouseEvent const& e)
{
    // TODO: why would this ever happen??
    if (!getLocalBounds().contains(e.getPosition()))
        return;

    if (attachedToMouse) {
        attachedToMouse = false;
        stopTimer();
        repaint();

        // Tell pd about new position
        cnv->pd->enqueueFunction(
            [_this = SafePointer(this)]() {
                if ((!_this || !_this->gui) && !_this->cnv->patch.objectWasDeleted(_this->gui->ptr)) {
                    return;
                }

                _this->gui->applyBounds();
            });

        if (createEditorOnMouseDown) {
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

    if(resizeZone.getZoneFlags() != ResizableBorderComponent::Zone::centre) {
        originalBounds = getBounds();
        return;
    }

    bool wasSelected = cnv->isSelected(this);

    cnv->handleMouseDown(this, e);

    if (cnv->isSelected(this) != wasSelected) {
        selectionStateChanged = true;
    }
}

void Object::mouseUp(MouseEvent const& e)
{
    resizeZone = ResizableBorderComponent::Zone();

    if (wasLockedOnMouseDown)
        return;

    if (!cnv->didStartDragging && !static_cast<bool>(locked.getValue()) && e.mods.isAltDown()) {
        // Show help file on alt+mouseup if object were not draged
        openHelpPatch();
    }

    if (!originalBounds.isEmpty() && originalBounds.withPosition(0, 0) != getLocalBounds()) {
        originalBounds.setBounds(0, 0, 0, 0);

        cnv->pd->enqueueFunction(
            [_this = SafePointer<Object>(this), e]() mutable {
                if (!_this || !_this->gui)
                    return;

                auto* obj = static_cast<t_gobj*>(_this->getPointer());
                auto* cnv = _this->cnv;

                if (cnv->patch.objectWasDeleted(obj))
                    return;

                // Used for size changes, could also be used for properties
                libpd_undo_apply(cnv->patch.getPointer(), obj);

                _this->gui->applyBounds();

                // To make sure it happens after setting object bounds
                if (!cnv->viewport->getViewArea().contains(_this->getBounds())) {
                    MessageManager::callAsync(
                        [_this]() {
                            if (_this)
                                _this->cnv->checkBounds();
                        });
                }
            });
    } else {
        cnv->handleMouseUp(this, e);
    }

    if (gui && !selectionStateChanged && cnv->isSelected(this) && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isRightButtonDown()) {
        gui->showEditor();
    }

    selectionStateChanged = false;
}

void Object::mouseDrag(MouseEvent const& e)
{
    if (wasLockedOnMouseDown) return;

    cnv->cancelConnectionCreation();
    
    // Let canvas handle moving
    if (resizeZone.getZoneFlags() == ResizableBorderComponent::Zone::centre) {
        cnv->handleMouseDrag(e);
    } else if (validResizeZone && !originalBounds.isEmpty()) {
        const Rectangle<int> newBounds (resizeZone.resizeRectangleBy (originalBounds, e.getOffsetFromDragStart()));
        
        bool useConstrainer = gui && !gui->checkBounds(originalBounds - cnv->canvasOrigin, newBounds - cnv->canvasOrigin, resizeZone.isDraggingLeftEdge());
        
        if(useConstrainer)
        {
            constrainer->setBoundsForComponent (this, newBounds, resizeZone.isDraggingTopEdge(),
                                                    resizeZone.isDraggingLeftEdge(),
                                                    resizeZone.isDraggingBottomEdge(),
                                                resizeZone.isDraggingRightEdge());
        }
        
    }
}

void Object::showEditor()
{
    if (!gui) {
        openNewObjectEditor();
    } else {
        gui->showEditor();
    }
}

void Object::hideEditor()
{
    if (gui) {
        gui->hideEditor();
    } else if (newObjectEditor) {
        WeakReference<Component> deletionChecker(this);
        std::unique_ptr<TextEditor> outgoingEditor;
        std::swap(outgoingEditor, newObjectEditor);

        cnv->hideSuggestions();
        
        outgoingEditor->removeListener(cnv->suggestor);

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
    for (auto* con : cnv->connections) {
        for (auto* iolet : iolets) {
            if (con->inlet == iolet || con->outlet == iolet) {
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
    if (!newObjectEditor) {
        newObjectEditor = std::make_unique<TextEditor>();

        auto* editor = newObjectEditor.get();
        editor->applyFontToAllText(Font(15));

        copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, findColour(PlugDataColour::canvasTextColourId));
        editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        editor->setAlwaysOnTop(true);
        editor->setMultiLine(false);
        editor->setReturnKeyStartsNewLine(false);
        editor->setBorder(BorderSize<int>(1, 7, 1, 2));
        editor->setIndents(0, 0);
        editor->setJustification(Justification::centredLeft);

        editor->setBounds(getLocalBounds().reduced(Object::margin));
        editor->addListener(this);

        // Allow cancelling object creation with escape
        editor->onEscapeKey = [this]() {
            MessageManager::callAsync([_this = SafePointer(this)]() {
                if (!_this)
                    return;
                _this->cnv->hideSuggestions();
                _this->cnv->objects.removeObject(_this.getComponent());
                _this->cnv->lastSelectedObject = nullptr;
                _this->cnv->lastSelectedConnection = nullptr;
            });
        };

        addAndMakeVisible(editor);
        editor->grabKeyboardFocus();

        editor->onFocusLost = [this, editor]() {
            if (reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor) {
                editor->grabKeyboardFocus();
                return;
            }

            hideEditor();
        };

        cnv->showSuggestions(this, editor);

        resized();
        repaint();
    }
}

void Object::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (newObjectEditor) {
        newObjectEditor->giveAwayKeyboardFocus();
        cnv->grabKeyboardFocus();
    }
}

// For resize-while-typing behaviour
void Object::textEditorTextChanged(TextEditor& ed)
{
    String currentText;
    if(cnv->suggestor && !cnv->suggestor->getText().isEmpty()) {
        currentText = cnv->suggestor->getText();
    }
    else {
        currentText = ed.getText();
    }
    
    // For resize-while-typing behaviour
    auto width = Font(15).getStringWidth(currentText) + 14.0f;

    width += Object::doubleMargin;

    setSize(width, getHeight());
}

void Object::openHelpPatch() const
{
    cnv->pd->setThis();

    if (auto* ptr = static_cast<t_object*>(getPointer())) {
    
        auto file = cnv->pd->objectLibrary.findHelpfile(ptr, cnv->patch.getCurrentFile());

        if (!file.existsAsFile()) {
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
