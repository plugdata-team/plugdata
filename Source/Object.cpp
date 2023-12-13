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
#include "Components/SuggestionComponent.h"
#include "Connection.h"
#include "Iolet.h"
#include "Constants.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ObjectGrid.h"
#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"
#include "Heavy/CompatibleObjects.h"

#include "Pd/Patch.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
}

Object::Object(Canvas* parent, String const& name, Point<int> position)
    : cnv(parent)
    , gui(nullptr)
    , ds(parent->dragState)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    initialise();

    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty()) {
        setSize(100, height);
    } else {
        setType(name);
    }

    if (!getValue<bool>(locked)) {
        showEditor();
    }
}

Object::Object(pd::WeakReference object, Canvas* parent)
    : gui(nullptr)
    , ds(parent->dragState)
{
    cnv = parent;

    initialise();

    setType("", std::move(object));
}

Object::~Object()
{
    hideEditor(); // Make sure the editor is not still open, that could lead to issues with listeners attached to the editor (i.e. suggestioncomponent)
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

void Object::timerCallback()
{
    activeStateAlpha -= 0.16f;
    repaint();
    if (activeStateAlpha <= 0.0f) {
        activeStateAlpha = 0.0f;
        stopTimer();
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

    if (!shouldBeSelected && Object::consoleTarget == this) {
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
    } else if (v.refersToSameSourceAs(cnv->presentationMode)) {
        // else it was a lock/unlock/presentation mode action
        // Hide certain objects in GOP
        setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));
    } else if (v.refersToSameSourceAs(cnv->locked) || v.refersToSameSourceAs(cnv->commandLocked)) {
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

        return !getValue<bool>(hvccMode) || isSubpatch || HeavyCompatibleObjects::getAllCompatibleObjects().contains(typeName);
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

    int zone = 0;
    auto b = getLocalBounds().toFloat().reduced(margin - 2);
    if (b.contains(e.position)
        && !b.reduced(7).contains(e.position)) {
        auto corners = getCorners();
        auto minW = jmax(b.getWidth() / 10.0f, jmin(10.0f, b.getWidth() / 3.0f));
        auto minH = jmax(b.getHeight() / 10.0f, jmin(10.0f, b.getHeight() / 3.0f));

        if (corners[0].contains(e.position) || corners[1].contains(e.position) || (e.position.x < jmax(7.0f, minW) && b.getX() > 0.0f))
            zone |= ResizableBorderComponent::Zone::left;
        else if (corners[2].contains(e.position) || corners[3].contains(e.position) || (e.position.x >= b.getWidth() - jmax(7.0f, minW)))
            zone |= ResizableBorderComponent::Zone::right;

        if (corners[0].contains(e.position) || corners[3].contains(e.position) || (e.position.y < jmax(7.0f, minH)))
            zone |= ResizableBorderComponent::Zone::top;
        else if (corners[1].contains(e.position) || corners[2].contains(e.position) || (e.position.y >= b.getHeight() - jmax(7.0f, minH)))
            zone |= ResizableBorderComponent::Zone::bottom;
    }

    resizeZone = static_cast<ResizableBorderComponent::Zone>(zone);
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
    patch->startUndoSequence("Resize");

    for (auto& [object, bounds] : newObjectSizes) {
        if (object->gui)
            object->gui->setPdBounds(bounds);
    }

    canvas_dirty(patchPtr, 1);

    patch->endUndoSequence("Resize");

    MessageManager::callAsync([cnv = SafePointer(this->cnv)] {
        if (cnv)
            cnv->editor->updateCommandStatus();
    });

    cnv->pd->unlockAudioThread();
}
void Object::updateBounds()
{
    // only update if we have a gui and the object isn't been moved by the user
    // otherwise PD hasn't been informed of the new position 'while' we are dragging
    // so we don't need to update the bounds when an object is being interacted with
    if (gui && !isObjectMouseActive) {
        // Get the bounds of the object in Pd
        auto newBounds = gui->getPdBounds();

        // Objects may return empty bounds if they are not a real object (like scalars)
        if (!newBounds.isEmpty())
            setObjectBounds(newBounds);
    }

    if (newObjectEditor) {
        textEditorTextChanged(*newObjectEditor);
    }
}

void Object::setType(String const& newType, pd::WeakReference existingObject)
{
    // Change object type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    pd::WeakReference objectPtr = nullptr;
    // "exists" indicates that this object already exists in pd
    // When setting exists to true, the gui needs to be assigned already
    if (!existingObject.isValid()) {
        auto* patch = &cnv->patch;
        if (gui) {
            // Clear connections to this object
            // They will be remade by the synchronise call later
            for (auto* connection : getConnections())
                cnv->connections.removeObject(connection);

            if (auto* checkedObject = pd::Interface::checkObject(getPointer())) {
                auto renamedObject = patch->renameObject(checkedObject, newType);
                objectPtr = pd::WeakReference(renamedObject, cnv->pd);
            }

            // Synchronise to make sure connections are preserved correctly
            cnv->synchronise();
        } else {
            auto rect = getObjectBounds();
            auto* newObject = patch->createObject(rect.getX(), rect.getY(), newType);
            objectPtr = pd::WeakReference(newObject, cnv->pd);
        }
    } else {
        objectPtr = existingObject;
    }
    if (!objectPtr.getRaw<t_gobj>()) {
        jassertfalse;
        return;
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
    resized(); // If bounds haven't changed, we'll still want to update gui and iolets bounds

    // Auto patching
    if (getValue<bool>(cnv->editor->autoconnect) && numInputs && cnv->lastSelectedObject && cnv->lastSelectedObject != this && cnv->lastSelectedObject->numOutputs) {
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

        auto* checkedOut = pd::Interface::checkObject(outobj->getPointer());
        auto* checkedIn = pd::Interface::checkObject(inobj->getPointer());

        if (checkedOut && checkedIn && (outlet->isSignal == iolets[0]->isSignal) && (inlet->isSignal == iolets[this->numInputs]->isSignal)) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet]() {
                cnv->connections.add(new Connection(cnv, outlet, iolets[0], nullptr));
                cnv->connections.add(new Connection(cnv, iolets[this->numInputs], inlet, nullptr));
            });
            // remove the previous connection
            cnv->patch.removeConnection(checkedOut, cnv->lastSelectedConnection->outIdx, checkedIn, cnv->lastSelectedConnection->inIdx, cnv->lastSelectedConnection->getPathState());
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
    // If autoconnect is about to happen, draw a fake inlet with a dotted outline
    if (getValue<bool>(cnv->editor->autoconnect) && isInitialEditorShown() && cnv->lastSelectedObject && cnv->lastSelectedObject != this && cnv->lastSelectedObject->numOutputs) {
        auto outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        auto fakeInletBounds = Rectangle<float>(16, 4, 8, 8);
        g.setColour(findColour(outlet->isSignal ? PlugDataColour::signalColourId : PlugDataColour::dataColourId).brighter());
        g.fillEllipse(fakeInletBounds);

        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawEllipse(fakeInletBounds, 1.0f);
    }

    if (isSearchTarget || consoleTarget == this) {
        g.saveState();

        // Don't draw line over iolets!
        for (auto& iolet : iolets) {
            g.excludeClipRegion(iolet->getBounds().reduced(2));
        }

        g.setColour(findColour(PlugDataColour::signalColourId));
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
    startTimer(1000 / ACTIVITY_UPDATE_RATE);

    // Because the timer is being reset when new messages come in
    // it will not trigger it's callback until it's free-running
    // so we manually call the repaint here if this happens
    repaint();
}

void Object::paint(Graphics& g)
{
    if ((showActiveState || isTimerRunning())) {
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
        innerArea.addRoundedRectangle(innerRect, Corners::objectCornerRadius);
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

#if JUCE_IOS
    int ioletSize = 15;
#else
    int ioletSize = 13;
#endif

    int ioletHitBox = 6;

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
            int xPosition = getWidth() < 50 ? getLocalBounds().getCentreX() - ioletSize / 2.0f : bounds.getX();
            iolet->setBounds(xPosition, yPosition, ioletSize, ioletSize);
        } else if (total > 1) {
            float const ratio = (bounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
            iolet->setBounds(bounds.getX() + ratio * position, yPosition, ioletSize, ioletSize);
        }

        index++;
    }

    if (!getLocalBounds().isEmpty() && activityOverlayImage.getBounds() != getLocalBounds()) {
        // Pre-render activity state overlay here since it'll always look the same for the same object size
        activityOverlayImage = Image(Image::ARGB, getWidth(), getHeight(), true);
        Graphics g(activityOverlayImage);
        g.saveState();

        g.excludeClipRegion(getLocalBounds().reduced(Object::margin + 1));

        Path objectShadow;
        objectShadow.addRoundedRectangle(getLocalBounds().reduced(Object::margin - 2), Corners::objectCornerRadius);
        StackShadow::renderDropShadow(g, objectShadow, findColour(PlugDataColour::dataColourId), 6, { 0, 0 }, 0);
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

        // Check child objects of subpatch for inlet/outlet messages
        for (auto obj : subpatch->getObjects()) {

            if (!obj.isValid())
                continue;

            String const name = pd::Interface::getObjectClassName(obj.getRaw<t_pd>());
            auto* checkedObject = pd::Interface::checkObject(obj.getRaw<t_pd>());
            if (name == "inlet" || name == "inlet~") {
                int size;
                char* str_ptr;
                pd::Interface::getObjectText(checkedObject, &str_ptr, &size);

                int x, y, w, h;
                pd::Interface::getObjectBounds(subpatchPtr, obj.getRaw<t_gobj>(), &x, &y, &w, &h);

                // Anything after the first space will be the comment
                auto const text = String::fromUTF8(str_ptr, size);
                inletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));
                freebytes(static_cast<void*>(str_ptr), static_cast<size_t>(size) * sizeof(char));
            }
            if (name == "outlet" || name == "outlet~") {
                int size;
                char* str_ptr;
                pd::Interface::getObjectText(checkedObject, &str_ptr, &size);

                int x, y, w, h;
                pd::Interface::getObjectBounds(subpatchPtr, obj.getRaw<t_gobj>(), &x, &y, &w, &h);

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

    if (auto* ptr = pd::Interface::checkObject(getPointer())) {
        numInputs = pd::Interface::numInlets(ptr);
        numOutputs = pd::Interface::numOutlets(ptr);
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

        bool isSignal = false;
        auto* patchableObject = pd::Interface::checkObject(getPointer());
        if (patchableObject && i < numInputs) {
            isSignal = pd::Interface::isSignalInlet(patchableObject, i);
        } else if (patchableObject) {
            isSignal = pd::Interface::isSignalOutlet(patchableObject, i - numInputs);
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
    // Only show right-click menu in locked mode if the object can be opened
    // We don't allow alt+click for popupmenus here, as that will conflict with some object behaviour, like for [range.hsl]
    if (e.mods.isRightButtonDown() && !cnv->editor->pluginMode) {
        PopupMenu::dismissAllActiveMenus();
        if (!getValue<bool>(locked))
            cnv->setSelected(this, true);
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
    cnv->patch.startUndoSequence("Drag");
    isInsideUndoSequence = true;
}

void Object::mouseUp(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    if (!ds.didStartDragging && !getValue<bool>(locked) && e.mods.isAltDown()) {
        // Show help file on alt+mouseup if object were not draged
        openHelpPatch();
    }

    auto objects = cnv->getSelectionOfType<Object>();
    for (auto* obj : objects)
        obj->isObjectMouseActive = false;

    if (ds.wasResized) {

        cnv->objectGrid.clearIndicators(false);

        applyBounds();

        ds.wasResized = false;
        originalBounds.setBounds(0, 0, 0, 0);
    } else {
        if (cnv->isGraph) {
            if (isInsideUndoSequence) {
                isInsideUndoSequence = false;
                cnv->patch.endUndoSequence("Drag");
            }
            return;
        }

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
            cnv->objectGrid.clearIndicators(false);
            applyBounds();
            ds.didStartDragging = false;
        }

        if (ds.objectSnappingInbetween) {
            auto* c = ds.connectionToSnapInbetween.getComponent();

            cnv->patch.startUndoSequence("Snap inbetween");

            auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
            auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());
            auto* checkedSnapped = pd::Interface::checkObject(ds.objectSnappingInbetween->getPointer());

            if (checkedOut && checkedIn && checkedSnapped) {
                cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());

                cnv->patch.createConnection(checkedOut, c->outIdx, checkedSnapped, 0);
                cnv->patch.createConnection(checkedSnapped, 0, checkedIn, c->inIdx);
            }

            cnv->patch.endUndoSequence("Snap inbetween");

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
    if (isInsideUndoSequence) {
        isInsideUndoSequence = false;
        cnv->patch.endUndoSequence("Drag");
    }
}

void Object::mouseDrag(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    if (cnv->objectRateReducer.tooFast())
        return;

    cnv->cancelConnectionCreation();

    if (e.mods.isMiddleButtonDown())
        return;

    beginDragAutoRepeat(25);

    if (validResizeZone && !originalBounds.isEmpty()) {

        auto draggedBounds = resizeZone.resizeRectangleBy(originalBounds, e.getOffsetFromDragStart());
        auto dragDistance = cnv->objectGrid.performResize(this, e.getOffsetFromDragStart(), draggedBounds);

        auto toResize = cnv->getSelectionOfType<Object>();

        for (auto* obj : toResize) {

            if (!obj->gui)
                continue;

            obj->isObjectMouseActive = true;

            // Create undo step when we start resizing
            if (!ds.wasResized) {
                auto* objPtr = static_cast<t_gobj*>(obj->getPointer());
                auto* cnv = obj->cnv;

                auto* patchPtr = cnv->patch.getPointer().get();
                if (!patchPtr)
                    continue;

                // Used for size changes, could also be used for properties
                pd::Interface::undoApply(patchPtr, objPtr);
            }

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
        int const minimumMovementToStartDrag = 5;

        // Ensure tiny movements don't start a drag.
        if (!ds.didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
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
                auto gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
                auto gridType = SettingsFile::getInstance()->getProperty<int>("grid_type");
                auto gridSize = gridEnabled && (gridType & 1) ? cnv->objectGrid.gridSize : 10;

                mouseDownObjectPositions.add(object->getPosition().translated(gridSize, gridSize));
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
                selected->isObjectMouseActive = true;
                i++;
            }

            if (isPositiveAndBelow(draggedIdx, selection.size())) {
                ds.componentBeingDragged = selection[draggedIdx];
            }
        }

        // FIXME: stop the mousedrag event from blocking the objects from redrawing, we shouldn't need to do this? JUCE bug?
        if (ds.componentBeingDragged) {
            for (auto* object : selection) {
                object->isObjectMouseActive = true;
                auto newPosition = object->originalBounds.getPosition() + dragDistance;

                object->setBufferedToImage(true);
                object->setTopLeftPosition(newPosition);
            }

            cnv->autoscroll(e.getEventRelativeTo(cnv->viewport.get()));
        }

        // This handles the "unsnap" action when you shift-drag a connected object
        if (e.mods.isShiftDown() && selection.size() == 1 && e.getDistanceFromDragStart() > 15) {
            auto* object = selection.getFirst();
            cnv->patch.startUndoSequence("Snap");

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
                    auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                    auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                    if (checkedOut && checkedIn) {
                        cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());

                        cnv->connections.add(new Connection(cnv, outlet, c->inlet, nullptr));
                        cnv->connections.removeObject(c);
                    }
                }

                auto* c = inputs[0];
                auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                if (checkedOut && checkedIn) {
                    cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                }
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            } else if (inputs.size() && outputs.size() == 1) {
                auto* inlet = outputs[0]->inlet.get();

                for (auto* c : inputs) {
                    auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                    auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());
                    if (checkedOut && checkedIn) {
                        cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                    }

                    cnv->connections.add(new Connection(cnv, c->outlet, inlet, nullptr));
                    cnv->connections.removeObject(c);
                }

                auto* c = outputs[0];
                auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                if (checkedOut && checkedIn) {
                    cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                }
                cnv->connections.removeObject(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            }

            cnv->patch.endUndoSequence("Snap");
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

// Returns true is the object is showing its initial editor, and doesn't have a GUI yet
bool Object::isInitialEditorShown()
{
    return newObjectEditor != nullptr;
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
        std::unique_ptr<TextEditor> outgoingEditor = nullptr;
        std::swap(outgoingEditor, newObjectEditor);

        cnv->hideSuggestions();

        outgoingEditor->removeListener(cnv->suggestor.get());

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

t_gobj* Object::getPointer() const
{
    return gui ? gui->ptr.getRaw<t_gobj>() : nullptr;
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
            if (reinterpret_cast<Component*>(cnv->suggestor.get())->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor) {
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

    if (auto* ptr = getPointer()) {

        auto file = cnv->pd->objectLibrary->findHelpfile(ptr, cnv->patch.getCurrentFile());

        if (!file.existsAsFile()) {
            cnv->pd->logMessage("Couldn't find help file");
            return;
        }

        cnv->pd->lockAudioThread();
        cnv->pd->loadPatch(file, cnv->editor, -1);
        cnv->pd->unlockAudioThread();

        return;
    }

    cnv->pd->logMessage("Couldn't find help file");
}
