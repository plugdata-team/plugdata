/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Object.h"

#include "Canvas.h"
#include "SuggestionComponent.h"
#include "Connection.h"
#include "Iolet.h"
#include "Constants.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ObjectGrid.h"
#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"

#include "Pd/Patch.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
#include <x_libpd_extra_utils.h>
}

Object::Object(Canvas* parent, String const& name, Point<int> position)
    : cnv(parent)
    , ds(parent->dragState)
    , gui(nullptr)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    if (cnv->attachNextObjectToMouse) {
        cnv->attachNextObjectToMouse = false;
        attachedToMouse = true;
        startTimer(1, 16);
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
    if (attachedToMouse && !name.startsWith("param") && !getValue<bool>(locked)) {
        createEditorOnMouseDown = true;
    } else if (!attachedToMouse && !getValue<bool>(locked)) {
        showEditor();
    }
}

Object::Object(void* object, Canvas* parent)
    : ds(parent->dragState)
    , gui(nullptr)
{
    cnv = parent;

    initialise();

    setType("", object);
}

Object::~Object()
{
    if (attachedToMouse) {
        stopTimer(1);
    }

    cnv->selectedComponents.removeChangeListener(this);
}

Rectangle<int> Object::getObjectBounds()
{
    return getBounds().reduced(margin) - cnv->canvasOrigin;
}

Rectangle<int> Object::getSelectableBounds()
{
    if (gui) {
        return gui->getSelectableBounds() + cnv->canvasOrigin;
    }

    return getBounds().reduced(margin);
}

void Object::setObjectBounds(Rectangle<int> bounds)
{
    setBounds(bounds.expanded(margin) + cnv->canvasOrigin);
}

void Object::initialise()
{
    cnv->addAndMakeVisible(this);

    cnv->selectedComponents.addChangeListener(this);

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

    updateOverlays(cnv->getOverlays());
}

void Object::timerCallback(int timerID)
{
    switch (timerID){
    case 1: {
        auto pos = cnv->getMouseXYRelative();
        if (pos != getBounds().getCentre()) {
            auto viewArea = cnv->viewport->getViewArea() / getValue<float>(cnv->zoomScale);
            setCentrePosition(viewArea.getConstrainedPoint(pos));
        }
        break;
    }
    case 2: {
        activeStateAlpha -= 0.16f;
        repaint();
        if (activeStateAlpha <= 0.0f) {
            activeStateAlpha = 0.0f;
            stopTimer(2);
        }
        break;
    }
    default:
        break;
    }
}

void Object::changeListenerCallback(ChangeBroadcaster* source)
{
    if (auto selectedItems = dynamic_cast<SelectedItemSet<WeakReference<Component>>*>(source))
        setSelected(selectedItems->isSelected(this));
}

void Object::setSelected(bool shouldBeSelected)
{
    if (selectedFlag != shouldBeSelected) {
        selectedFlag = shouldBeSelected;
        repaint();
    }
    
    if(!shouldBeSelected && Object::consoleTarget == this)
    {
        Object::consoleTarget = nullptr;
        repaint();
    }
    
}

bool Object::isSelected() const
{
    return selectedFlag;
}

void Object::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(hvccMode)) {

        isHvccCompatible = checkIfHvccCompatible();
        
        if (gui && !isHvccCompatible) {
            cnv->pd->logWarning(String("Warning: object \"" + gui->getType() + "\" is not supported in Compiled Mode").toRawUTF8());
        }

        repaint();
    }
    else if (v.refersToSameSourceAs(cnv->presentationMode)) {
        // else it was a lock/unlock/presentation mode action
        // Hide certain objects in GOP
        setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));
    }
    else if (v.refersToSameSourceAs(cnv->locked) || v.refersToSameSourceAs(cnv->commandLocked)) {
        if (gui) {
            gui->lock(cnv->isGraph || locked == var(true) || commandLocked == var(true));
        }
    }

    repaint();
}

bool Object::checkIfHvccCompatible()
{
    if (gui) {
        auto typeName = gui->getType();
        // Check hvcc compatibility
        bool isSubpatch = gui->getPatch() != nullptr;
        
        return !getValue<bool>(hvccMode) || isSubpatch || hvccObjects.contains(typeName);
    }
    
    return true;
}

bool Object::hitTest(int x, int y)
{
    if (Canvas::panningModifierDown())
        return false;

    // Mouse over iolets
    for (auto* iolet : iolets) {
        if (iolet->getBounds().contains(x, y))
            return true;
    }

    if (selectedFlag) {

        for (auto& corner : getCorners()) {
            if (corner.contains(x, y))
                return true;
        }

        return getLocalBounds().reduced(margin).contains(x, y);
    }

    if (gui && !gui->canReceiveMouseEvent(x, y)) {
        return false;
    }

    // Mouse over object
    if (getLocalBounds().reduced(margin).contains(x, y)) {
        return true;
    }

    return false;
}

// To make iolets show/hide
void Object::mouseEnter(MouseEvent const& e)
{
    for (auto* iolet : iolets)
        iolet->repaint();
}

void Object::mouseExit(MouseEvent const& e)
{
    // we need to reset the resizeZone & validResizeZone,
    // otherwise it can have an old zone already selected on re-entry
    resizeZone = ResizableBorderComponent::Zone(ResizableBorderComponent::Zone::centre);
    validResizeZone = false;

    for (auto* iolet : iolets)
        iolet->repaint();
}

void Object::mouseMove(MouseEvent const& e)
{
    if (!selectedFlag || locked == var(true) || commandLocked == var(true)) {
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        return;
    }

    resizeZone = ResizableBorderComponent::Zone::fromPositionOnBorder(getLocalBounds().reduced(margin - 2), BorderSize<int>(5), Point<int>(e.x, e.y));

    validResizeZone = resizeZone.getZoneFlags() != ResizableBorderComponent::Zone::centre && e.originalComponent == this;

    setMouseCursor(validResizeZone ? resizeZone.getMouseCursor() : MouseCursor::NormalCursor);
    updateMouseCursor();
}

void Object::applyBounds()
{
    std::map<SafePointer<Object>, Rectangle<int>> newObjectSizes;
    for (auto* obj : cnv->getSelectionOfType<Object>())
        newObjectSizes[obj] = obj->getObjectBounds();

    auto* patch = &cnv->patch;

    auto* patchPtr = cnv->patch.getPointer().get();
    if (!patchPtr)
        return;

    cnv->pd->lockAudioThread();
    patch->startUndoSequence("resize");

    for (auto& [object, bounds] : newObjectSizes) {
        if (!object || !object->gui)
            return;

        auto* obj = static_cast<t_gobj*>(object->getPointer());
        auto* cnv = object->cnv;

        if (!obj) return;
        
        // Used for size changes, could also be used for properties
        libpd_undo_apply(patchPtr, obj);

        object->gui->setPdBounds(bounds);

        canvas_dirty(patchPtr, 1);
    }

    patch->endUndoSequence("resize");
    
    MessageManager::callAsync([cnv = SafePointer(this->cnv)]{
        if(cnv) cnv->editor->updateCommandStatus();
    });

    cnv->pd->unlockAudioThread();
}
void Object::updateBounds()
{
    if (gui) {
        // Get the bounds of the object in Pd
        auto newBounds = gui->getPdBounds();

        // Objects may return empty bounds if they are not a real object (like scalars)
        if (!newBounds.isEmpty())
            setObjectBounds(newBounds);
    }

    if (newObjectEditor) {
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
    if (!existingObject) {
        auto* patch = &cnv->patch;
        if (gui) {
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections())
                cnv->connections.removeObject(connection);

            objectPtr = patch->renameObject(getPointer(), newType);

            // Synchronise to make sure connections are preserved correctly
            cnv->synchronise();
        } else {
            auto rect = getObjectBounds();
            objectPtr = patch->createObject(rect.getX(), rect.getY(), newType);
        }
    } else {
        objectPtr = existingObject;
    }

    // Create gui for the object
    gui.reset(ObjectBase::createGui(objectPtr, this));

    if (gui) {
        gui->initialise();
        gui->lock(cnv->isGraph || locked == var(true) || commandLocked == var(true));
        gui->addMouseListener(this, true);
        addAndMakeVisible(gui.get());
    }

    isHvccCompatible = checkIfHvccCompatible();

    if (gui && !isHvccCompatible) {
        cnv->pd->logWarning(String("Warning: object \"" + gui->getType() + "\" is not supported in Compiled Mode").toRawUTF8());
    }

    // Update inlets/outlets
    updateIolets();
    updateBounds();

    // Auto patching
    if (!attachedToMouse && getValue<bool>(cnv->editor->autoconnect) && numInputs && cnv->lastSelectedObject && cnv->lastSelectedObject->numOutputs) {
        auto outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        auto inlet = iolets[0];
        if (outlet->isSignal == inlet->isSignal) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet]() {
                cnv->connections.add(new Connection(cnv, outlet, inlet, nullptr));
            });
        }
    }
    if (cnv->lastSelectedConnection && numInputs && numOutputs) {
        // if 1 connection is selected, connect the new object in middle of connection
        auto outobj = cnv->lastSelectedConnection->outobj;
        auto inobj = cnv->lastSelectedConnection->inobj;
        auto outlet = outobj->iolets[outobj->numInputs + cnv->lastSelectedConnection->outIdx];
        auto inlet = inobj->iolets[cnv->lastSelectedConnection->inIdx];
        if ((outlet->isSignal == iolets[0]->isSignal) && (inlet->isSignal == iolets[this->numInputs]->isSignal)) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet]() {
                cnv->connections.add(new Connection(cnv, outlet, iolets[0], nullptr));
                cnv->connections.add(new Connection(cnv, iolets[this->numInputs], inlet, nullptr));
            });
            // remove the previous connection
            cnv->patch.removeConnection(outobj->getPointer(), cnv->lastSelectedConnection->outIdx, inobj->getPointer(), cnv->lastSelectedConnection->inIdx, cnv->lastSelectedConnection->getPathState());
            cnv->connections.removeObject(cnv->lastSelectedConnection);
        }
    }
    cnv->lastSelectedObject = nullptr;
    cnv->lastSelectedConnection = nullptr;

    cnv->editor->updateCommandStatus();

    cnv->synchroniseSplitCanvas();
    cnv->pd->updateObjectImplementations();
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
    if (consoleTarget == this) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::darkorange);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    }
    if (isSearchTarget) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::violet);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (!isHvccCompatible) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::orange);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (attachedToMouse) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(Colours::lightgreen);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(Object::margin + 1.0f), Corners::objectCornerRadius, 2.0f);

        g.restoreState();
    } else if (indexShown) {
        int halfHeight = 5;

        auto text = String(cnv->objects.indexOf(this));
        int textWidth = Fonts::getMonospaceFont().withHeight(10).getStringWidth(text) + 5;
        int left = std::min<int>(getWidth() - (1.5 * margin), getWidth() - textWidth);

        auto indexBounds = Rectangle<int>(left, (getHeight() / 2) - halfHeight, getWidth() - left, halfHeight * 2);

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
        g.fillRoundedRectangle(indexBounds.toFloat(), 2.0f);

        Fonts::drawStyledText(g, text, indexBounds, findColour(PlugDataColour::objectSelectedOutlineColourId).contrasting(), Monospace, 10, Justification::centred);
    }
}

void Object::triggerOverlayActiveState()
{
    if (!showActiveState)
        return;

    if (rateReducer.tooFast())
        return;

    activeStateAlpha = 1.0f;
    startTimer(2, 1000 / ACTIVITY_UPDATE_RATE);

    // Because the timer is being reset when new messages come in
    // it will not trigger it's callback until it's free-running
    // so we manually call the repaint here if this happens
    MessageManager::callAsync([this](){
        repaint();
    });
}

void Object::paint(Graphics& g)
{
    if ((showActiveState || isTimerRunning(2))) {
        g.setOpacity(activeStateAlpha);
        // show activation state glow
        g.drawImage(activityOverlayImage, getLocalBounds().toFloat());
        g.setOpacity(1.0f);
    }
    if ((selectedFlag && !cnv->isGraph) || newObjectEditor) {
        if (newObjectEditor) {

            g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), Corners::objectCornerRadius);

            g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().reduced(Object::margin + 1).toFloat(), Corners::objectCornerRadius, 1.0f);
        }

        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));

        g.saveState();
        // Make a rounded rectangle hole path:
        // We do this by creating a large rectangle path with inverted winding
        // and adding the inner rounded rectangle path
        // this creates one path that has a hole in the middle
        Path outerArea;
        outerArea.addRectangle(getLocalBounds());
        outerArea.setUsingNonZeroWinding(false);
        Path innerArea;
        auto innerRect = getLocalBounds().reduced(margin + 1);
        innerArea.addRoundedRectangle(innerRect,Corners::objectCornerRadius);
        outerArea.addPath(innerArea);

        // use the path with a hole in it to exclude the inner rounded rect from painting
        g.reduceClipRegion(outerArea);

        for (auto& rect : getCorners()) {
            g.fillRoundedRectangle(rect, Corners::objectCornerRadius);
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
    
    if(!getLocalBounds().isEmpty()) {
        // Pre-render activity state overlay here since it'll always look the same for the same object size
        activityOverlayImage = Image(Image::ARGB, getWidth(), getHeight(), true);
        Graphics g(activityOverlayImage);
        g.saveState();
        
        g.excludeClipRegion(getLocalBounds().reduced(Object::margin + 1));
        
        Path objectShadow;
        objectShadow.addRoundedRectangle(getLocalBounds().reduced(Object::margin - 2), Corners::objectCornerRadius);
        StackShadow::renderDropShadow(g, objectShadow, findColour(PlugDataColour::dataColourId), 5, { 0, 0 }, 0);
        g.restoreState();
    }
    
}

void Object::updateTooltips()
{
    if (!gui)
        return;

    auto objectInfo = cnv->pd->objectLibrary->getObjectInfo(gui->getType());

    // Set object tooltip
    gui->setTooltip(objectInfo.getProperty("description").toString());

    // Check pd library for pddp tooltips, those have priority
    auto ioletTooltips = cnv->pd->objectLibrary->parseIoletTooltips(objectInfo.getChildWithName("iolets"), gui->getText(), numInputs, numOutputs);

    // First clear all tooltips, so we can see later if it has already been set or not
    for (auto iolet : iolets) {
        iolet->setTooltip("");
    }

    // Load tooltips from documentation files, these have priority
    for (int i = 0; i < iolets.size(); i++) {
        auto* iolet = iolets[i];

        auto& tooltip = ioletTooltips[!iolet->isInlet][iolet->isInlet ? i : i - numInputs];

        // Don't overwrite custom documentation
        if (tooltip.isNotEmpty()) {
            iolet->setTooltip(tooltip);
        }
    }

    std::vector<std::pair<int, String>> inletMessages;
    std::vector<std::pair<int, String>> outletMessages;

    cnv->pd->lockAudioThread();
    if (auto subpatch = gui->getPatch()) {
        auto* subpatchPtr = subpatch->getPointer().get();

        // if(!subpatchPtr) return;

        // Check child objects of subpatch for inlet/outlet messages
        for (auto* obj : subpatch->getObjects()) {

            const String name = libpd_get_object_class_name(obj);
            if (name == "inlet" || name == "inlet~") {

                int size;
                char* str_ptr;
                libpd_get_object_text(obj, &str_ptr, &size);

                int x, y, w, h;
                libpd_get_object_bounds(subpatchPtr, obj, &x, &y, &w, &h);

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
                libpd_get_object_bounds(subpatchPtr, obj, &x, &y, &w, &h);

                auto const text = String::fromUTF8(str_ptr, size);
                outletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));

                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
        }
    }
    cnv->pd->unlockAudioThread();

    if (inletMessages.empty() && outletMessages.empty())
        return;

    auto sortFunc = [](std::pair<int, String>& a, std::pair<int, String>& b) {
        return a.first < b.first;
    };

    std::sort(inletMessages.begin(), inletMessages.end(), sortFunc);
    std::sort(outletMessages.begin(), outletMessages.end(), sortFunc);

    int numIn = 0;
    int numOut = 0;

    for (auto iolet : iolets) {
        if (iolet->getTooltip().isNotEmpty())
            continue;

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

    for (auto* iolet : iolets) {
        iolet->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (cnv->patch.objectWasDeleted(getPointer())) {
        iolets.clear();
        return;
    }

    if (auto* ptr = pd::Patch::checkObject(getPointer())) {
        numInputs = libpd_ninlets(ptr);
        numOutputs = libpd_noutlets(ptr);
    }

    for (auto* iolet : iolets) {
        if (gui && !iolet->isInlet) {
            iolet->setHidden(gui->hideOutlets());
        } else if (gui && iolet->isInlet) {
            iolet->setHidden(gui->hideInlets());
        }
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
        stopTimer(1);
        repaint();
        
        if (createEditorOnMouseDown) {
            createEditorOnMouseDown = false;

            // Prevent losing focus because of click event
            MessageManager::callAsync([_this = SafePointer(this)]() { _this->showEditor(); });
        }

        return;
    }

    // Only show right-click menu in locked mode if the object can be opened
    // We don't allow alt+click for popupmenus here, as that will conflict with some object behaviour, like for [range.hsl]
    if (e.mods.isRightButtonDown()) {
        PopupMenu::dismissAllActiveMenus();
        if(!getValue<bool>(locked)) cnv->setSelected(this, true);
        Dialogs::showCanvasRightClickMenu(cnv, this, e.getScreenPosition());
        return;
    }
    
    if (cnv->isGraph || getValue<bool>(presentationMode) || getValue<bool>(locked) || getValue<bool>(commandLocked)) {
        wasLockedOnMouseDown = true;
        return;
    }

    wasLockedOnMouseDown = false;

    if (e.mods.isShiftDown()) {
        // select multiple objects
        ds.wasSelectedOnMouseDown = selectedFlag;
    } else if (!selectedFlag) {
        cnv->deselectAll();
    }

    cnv->setSelected(this, true);


    ds.componentBeingDragged = this;

    for (auto* object : cnv->getSelectionOfType<Object>()) {
        object->originalBounds = object->getBounds();
    }

    repaint();

    ds.canvasDragStartPosition = cnv->getPosition();

    if (!selectedFlag) {
        selectionStateChanged = true;
    }

    cnv->updateSidebarSelection();
}

void Object::mouseUp(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    if (!ds.didStartDragging && !getValue<bool>(locked) && e.mods.isAltDown()) {
        // Show help file on alt+mouseup if object were not draged
        openHelpPatch();
    }

    if (ds.wasResized) {

        cnv->objectGrid.clearAll();

        applyBounds();

        ds.wasResized = false;
        originalBounds.setBounds(0, 0, 0, 0);
    } else {
        if (cnv->isGraph)
            return;

        if (e.mods.isShiftDown() && ds.wasSelectedOnMouseDown && !ds.didStartDragging) {
            // Unselect object if selected
            cnv->setSelected(this, false);
            repaint();
        } else if (!e.mods.isShiftDown() && !e.mods.isAltDown() && selectedFlag && !ds.didStartDragging && !e.mods.isRightButtonDown()) {

            // Don't run normal deselectAll, that would clear the sidebar inspector as well
            // We'll update sidebar selection later in this function
            cnv->selectedComponents.deselectAll();

            cnv->setSelected(this, true);
        }

        cnv->updateSidebarSelection();

        if (ds.didStartDragging) {
            cnv->objectGrid.clearAll();
            applyBounds();
            ds.didStartDragging = false;
        }

        if (ds.objectSnappingInbetween) {
            auto* c = ds.connectionToSnapInbetween.getComponent();

            cnv->patch.startUndoSequence("SnapInbetween");

            cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

            cnv->patch.createConnection(c->outobj->getPointer(), c->outIdx, ds.objectSnappingInbetween->getPointer(), 0);
            cnv->patch.createConnection(ds.objectSnappingInbetween->getPointer(), 0, c->inobj->getPointer(), c->inIdx);

            cnv->patch.endUndoSequence("SnapInbetween");

            ds.objectSnappingInbetween->iolets[0]->isTargeted = false;
            ds.objectSnappingInbetween->iolets[ds.objectSnappingInbetween->numInputs]->isTargeted = false;
            ds.objectSnappingInbetween = nullptr;

            cnv->synchronise();
        }

        if (ds.wasDragDuplicated) {
            cnv->patch.endUndoSequence("Duplicate");
            ds.wasDragDuplicated = false;
        }

        for (auto* object : cnv->getSelectionOfType<Object>()) {
            object->setBufferedToImage(false);
            object->repaint();
        }

        ds.componentBeingDragged = nullptr;

        repaint();
    }

    if (ds.wasDragDuplicated) {
        cnv->patch.endUndoSequence("Duplicate");
        ds.wasDragDuplicated = false;
    }

    if (gui && selectedFlag && !selectionStateChanged && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isRightButtonDown()) {
        gui->showEditor();
    }

    selectionStateChanged = false;
}

void Object::mouseDrag(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    cnv->cancelConnectionCreation();

    if (e.mods.isMiddleButtonDown())
        return;

    if (validResizeZone && !originalBounds.isEmpty()) {

        auto draggedBounds = resizeZone.resizeRectangleBy(originalBounds, e.getOffsetFromDragStart());
        auto dragDistance = cnv->objectGrid.performResize(this, e.getOffsetFromDragStart(), draggedBounds);

        auto toResize = cnv->getSelectionOfType<Object>();

        for (auto* obj : toResize) {

            if (!obj->gui)
                continue;

            auto const newBounds = resizeZone.resizeRectangleBy(obj->originalBounds, dragDistance);
            
            
            if (auto* constrainer = obj->getConstrainer()) {
                
                constrainer->setBoundsForComponent(obj, newBounds, resizeZone.isDraggingTopEdge(),
                    resizeZone.isDraggingLeftEdge(),
                    resizeZone.isDraggingBottomEdge(),
                    resizeZone.isDraggingRightEdge());
            }
        }
        
        ds.wasResized = true;
    } else if (!cnv->isGraph) {
        // Ensure tiny movements don't start a drag.
        if (!ds.didStartDragging && e.getDistanceFromDragStart() < cnv->minimumMovementToStartDrag)
            return;

        if (!ds.didStartDragging) {
            ds.didStartDragging = true;
        }

        auto canvasMoveOffset = ds.canvasDragStartPosition - cnv->getPosition();

        auto selection = cnv->getSelectionOfType<Object>();

        auto dragDistance = e.getOffsetFromDragStart() + canvasMoveOffset;

        if (ds.componentBeingDragged) {
            dragDistance = cnv->objectGrid.performMove(this, dragDistance);
        }

        // alt+drag will duplicate selection
        if (!ds.wasDragDuplicated && e.mods.isAltDown()) {

            Array<Point<int>> mouseDownObjectPositions; // Stores object positions for alt + drag

            // Single for undo for duplicate + move
            cnv->patch.startUndoSequence("Duplicate");

            // Sort selection indexes to match pd indexes
            std::sort(selection.begin(), selection.end(),
                [this](auto* a, auto* b) -> bool {
                    return cnv->objects.indexOf(a) < cnv->objects.indexOf(b);
                });

            int draggedIdx = selection.indexOf(ds.componentBeingDragged.getComponent());

            // Store origin object positions
            for (auto object : selection) {
                mouseDownObjectPositions.add(object->getPosition().translated(10, 10));
            }

            // Duplicate once
            ds.wasDragDuplicated = true;
            cnv->duplicateSelection();
            cnv->cancelConnectionCreation();

            selection = cnv->getSelectionOfType<Object>();

            // Sort selection indexes to match pd indexes
            std::sort(selection.begin(), selection.end(),
                [this](auto* a, auto* b) -> bool {
                    return cnv->objects.indexOf(a) < cnv->objects.indexOf(b);
                });

            int i = 0;
            for (auto* selected : selection) {
                selected->originalBounds = selected->getBounds().withPosition(mouseDownObjectPositions[i]);
                i++;
            }

            if (isPositiveAndBelow(draggedIdx, selection.size())) {
                ds.componentBeingDragged = selection[draggedIdx];
            }
        }

        // FIXME: stop the mousedrag event from blocking the objects from redrawing, we shouldn't need to do this? JUCE bug?
        if (!cnv->objectRateReducer.tooFast() && ds.componentBeingDragged) {
            for (auto* object : selection) {

                auto newPosition = object->originalBounds.getPosition() + dragDistance;

                object->setBufferedToImage(true);
                object->setTopLeftPosition(newPosition);
            }

            if (cnv->autoscroll(e.getEventRelativeTo(cnv->viewport.get()))) {
                beginDragAutoRepeat(25);
            }
        }

        // This handles the "unsnap" action when you shift-drag a connected object
        if (e.mods.isShiftDown() && selection.size() == 1 && e.getDistanceFromDragStart() > 15) {
            auto* object = selection.getFirst();

            Array<Connection*> inputs, outputs;
            for (auto* connection : cnv->connections) {
                if (connection->inlet == object->iolets[0]) {
                    inputs.add(connection);
                }
                if (connection->outlet == object->iolets[object->numInputs]) {
                    outputs.add(connection);
                }
            }

            // Two cases that are allowed: either 1 input and multiple outputs,
            // or 1 output and multiple inputs
            if (inputs.size() == 1 && outputs.size()) {
                auto* outlet = inputs[0]->outlet.get();

                for (auto* c : outputs) {
                    cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                    cnv->connections.add(new Connection(cnv, outlet, c->inlet, nullptr));
                    cnv->connections.removeObject(c);
                }

                auto* c = inputs[0];
                cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            } else if (inputs.size() && outputs.size() == 1) {
                auto* inlet = outputs[0]->inlet.get();

                for (auto* c : inputs) {
                    cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());

                    cnv->connections.add(new Connection(cnv, c->outlet, inlet, nullptr));
                    cnv->connections.removeObject(c);
                }

                auto* c = outputs[0];
                cnv->patch.removeConnection(c->outobj->getPointer(), c->outIdx, c->inobj->getPointer(), c->inIdx, c->getPathState());
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            }
        }

        // Behaviour for shift-dragging objects over
        if (ds.objectSnappingInbetween) {
            if (ds.connectionToSnapInbetween->intersectsObject(ds.objectSnappingInbetween)) {
                return;
            }

            // If we're here, it's not snapping anymore
            ds.objectSnappingInbetween->iolets[0]->isTargeted = false;
            ds.objectSnappingInbetween->iolets[ds.objectSnappingInbetween->numInputs]->isTargeted = false;
            ds.objectSnappingInbetween = nullptr;
        }

        if (e.mods.isShiftDown() && selection.size() == 1) {
            auto* object = selection.getFirst();
            if (object->numInputs && object->numOutputs) {
                bool intersected = false;
                for (auto* connection : cnv->connections) {
                    if (connection->intersectsObject(object)) {
                        object->iolets[0]->isTargeted = true;
                        object->iolets[object->numInputs]->isTargeted = true;
                        object->iolets[0]->repaint();
                        object->iolets[object->numInputs]->repaint();
                        ds.connectionToSnapInbetween = connection;
                        ds.objectSnappingInbetween = object;
                        intersected = true;
                        break;
                    }
                }

                if (!intersected) {
                    object->iolets[0]->isTargeted = false;
                    object->iolets[object->numInputs]->isTargeted = false;
                    object->iolets[0]->repaint();
                    object->iolets[object->numInputs]->repaint();
                }
            }
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
    for (auto* iolet : iolets) {
        result.addArray(iolet->getConnections());
    }

    return result;
}

void* Object::getPointer() const
{
    return gui ? gui->ptr.getRaw<void>() : nullptr;
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
                auto* cnv = _this->cnv; // Copy pointer because _this will get deleted
                cnv->hideSuggestions();
                cnv->objects.removeObject(_this.getComponent());
                cnv->lastSelectedObject = nullptr;
                cnv->lastSelectedConnection = nullptr;
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
        cnv->grabKeyboardFocus();
    }
}

void Object::updateOverlays(int overlay)
{
    if (cnv->isGraph)
        return;

    bool indexWasShown = indexShown;
    indexShown = overlay & Overlay::Index;
    showActiveState = overlay & Overlay::ActivationState;

    if (indexWasShown != indexShown) {
        repaint();
    }
}

// For resize-while-typing behaviour
void Object::textEditorTextChanged(TextEditor& ed)
{
    String currentText;
    if (cnv->suggestor && !cnv->suggestor->getText().isEmpty()) {
        currentText = cnv->suggestor->getText();
    } else {
        currentText = ed.getText();
    }

    // For resize-while-typing behaviour
    auto width = Font(15).getStringWidth(currentText) + 14.0f;

    width += Object::doubleMargin;

    setSize(width, getHeight());
}

ComponentBoundsConstrainer* Object::getConstrainer() const
{
    if (gui) {
        return gui->getConstrainer();
    }

    return nullptr;
}

void Object::openHelpPatch() const
{
    cnv->pd->setThis();

    if (auto* ptr = static_cast<t_object*>(getPointer())) {

        auto file = cnv->pd->objectLibrary->findHelpfile(ptr, cnv->patch.getCurrentFile());

        if (!file.existsAsFile()) {
            cnv->pd->logMessage("Couldn't find help file");
            return;
        }

        cnv->pd->lockAudioThread();
        cnv->pd->loadPatch(file);
        cnv->pd->unlockAudioThread();

        return;
    }

    cnv->pd->logMessage("Couldn't find help file");
}
