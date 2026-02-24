/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/CachedStringWidth.h"

#include "Object.h"

#include "Canvas.h"
#include "Components/SuggestionComponent.h"
#include "Connection.h"
#include "Iolet.h"
#include "Constants.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"
#include "ObjectGrid.h"
#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"
#include "Heavy/CompatibleObjects.h"

#include "Pd/Patch.h"

extern "C" {
#include <m_pd.h>
}

Object::Object(Canvas* parent, String const& name, Point<int> const position)
    : NVGComponent(this)
    , cnv(parent)
    , editor(parent->editor)
    , ds(parent->dragState)
{
    setTopLeftPosition(position - Point<int>(margin, margin));

    initialise();

    // Open editor for undefined objects
    // Delay the setting of the type to prevent creating an invalid object first
    if (name.isEmpty()) {
        setSize(58, height);
    } else {
        setType(name);
    }

    if (!getValue<bool>(locked)) {
        showEditor();
    }
}

Object::Object(pd::WeakReference object, Canvas* parent)
    : NVGComponent(this)
    , cnv(parent)
    , editor(parent->editor)
    , ds(parent->dragState)
{
    initialise();

    setType("", object);
}

Object::~Object()
{
    hideEditor(); // Make sure the editor is not still open, that could lead to issues with listeners attached to the editor (i.e. suggestioncomponent)
    cnv->selectedComponents.removeChangeListener(this);
}

Rectangle<int> Object::getObjectBounds() const
{
    return getBounds().reduced(margin) - cnv->canvasOrigin;
}

Rectangle<int> Object::getSelectableBounds() const
{
    if (gui) {
        return gui->getSelectableBounds() + cnv->canvasOrigin;
    }

    return getBounds().reduced(margin);
}

void Object::setObjectBounds(Rectangle<int> const bounds)
{
    setBounds(bounds.expanded(margin) + cnv->canvasOrigin);
}

void Object::initialise()
{
    cnv->objectLayer.addAndMakeVisible(this);

    cnv->selectedComponents.addChangeListener(this);

    // Updates lock/unlock mode
    locked.referTo(cnv->locked);
    commandLocked.referTo(cnv->pd->commandLocked);
    presentationMode.referTo(cnv->presentationMode);

    hvccMode.referTo(SettingsFile::getInstance()->getValueTree(), Identifier("hvcc_mode"), nullptr, false);
    patchDownwardsOnly.referTo(SettingsFile::getInstance()->getValueTree(), Identifier("patch_downwards_only"), nullptr);

    presentationMode.addListener(this);
    locked.addListener(this);
    commandLocked.addListener(this);

    originalBounds.setBounds(0, 0, 0, 0);

    setAccessible(false); // TODO: implement accessibility. We disable default, since it makes stuff slow on macOS
}

void Object::timerCallback()
{
    activeStateAlpha -= 0.06f;
    repaint();
    if (activeStateAlpha <= 0.0f) {
        activeStateAlpha = 0.0f;
        stopTimer();
    }
}

void Object::changeListenerCallback(ChangeBroadcaster* source)
{
    if (auto const selectedItems = dynamic_cast<SelectedItemSet<WeakReference<Component>>*>(source))
        setSelected(selectedItems->isSelected(this));
}

void Object::setSelected(bool const shouldBeSelected)
{
    if (selectedFlag != shouldBeSelected) {
        selectedFlag = shouldBeSelected;
        repaint();
    }
}

bool Object::isSelected() const
{
    return selectedFlag;
}

void Object::settingsChanged(String const& name, var const& value)
{
    if (name == "hvcc_mode") {
        if (gui) {
            if (value)
                isHvccCompatible = gui->checkHvccCompatibility();
            else
                isHvccCompatible = true;
        }
        repaint();
    }
}

void Object::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(cnv->presentationMode)) {
        // else it was a lock/unlock/presentation mode action
        // Hide certain objects in GOP
        setVisible(!((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph()));
    } else if (v.refersToSameSourceAs(cnv->locked) || v.refersToSameSourceAs(cnv->commandLocked)) {
        if (gui) {
            gui->lock(cnv->isGraph || locked == var(true) || commandLocked == var(true));
        }
    }
}

bool Object::hitTest(int const x, int const y)
{
    if (::getValue<bool>(presentationMode)) {
        if (cnv->isPointOutsidePluginArea(cnv->getLocalPoint(this, Point<int>(x, y))))
            return false;
    }

    if (cnv->panningModifierDown())
        return false;

    // If the hit-test get's to here, and any of these are still true
    // return! Otherwise it will test non-existent iolets and return true!
    bool const blockIolets = presentationMode.getValue() || locked.getValue() || commandLocked.getValue();
    // Mouse over iolets
    for (auto const* iolet : iolets) {
        if (!blockIolets && iolet->getBounds().contains(x, y))
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
    drawIoletExpanded = true;
    if (!getValue<bool>(locked)) {
        repaint();
    }
}

void Object::mouseExit(MouseEvent const& e)
{
    // we need to reset the resizeZone & validResizeZone,
    // otherwise it can have an old zone already selected on re-entry
    resizeZone = ResizableBorderComponent::Zone(ResizableBorderComponent::Zone::centre);
    validResizeZone = false;
    drawIoletExpanded = false;
    if (!getValue<bool>(locked)) {
        repaint();
    }
}

void Object::mouseMove(MouseEvent const& e)
{
    using Zone = ResizableBorderComponent::Zone;
    using Direction = ObjectBase::ResizeDirection;
    
    if (!selectedFlag || locked == var(true) || commandLocked == var(true)) {
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        return;
    }

    int zone = 0;
    auto const b = getLocalBounds().toFloat().reduced(margin - 2);
    if (b.contains(e.position)
        && !b.reduced(7).contains(e.position)) {
        auto corners = getCorners();
        auto const minW = jmax(b.getWidth() / 10.0f, jmin(10.0f, b.getWidth() / 3.0f));
        auto const minH = jmax(b.getHeight() / 10.0f, jmin(10.0f, b.getHeight() / 3.0f));

        if (corners[0].contains(e.position) || corners[1].contains(e.position) || (e.position.x < jmax(7.0f, minW) && b.getX() > 0.0f))
            zone |= Zone::left;
        else if (corners[2].contains(e.position) || corners[3].contains(e.position) || e.position.x >= b.getWidth() - jmax(7.0f, minW))
            zone |= Zone::right;

        if (corners[0].contains(e.position) || corners[3].contains(e.position) || e.position.y < jmax(7.0f, minH))
            zone |= Zone::top;
        else if (corners[1].contains(e.position) || corners[2].contains(e.position) || e.position.y >= b.getHeight() - jmax(7.0f, minH))
            zone |= Zone::bottom;
    }
    
    auto allowedDirection = gui ? gui->getAllowedResizeDirections() : Direction::None;
    switch(allowedDirection)
    {
        case Direction::None: {
            zone = 0;
            break;
        }
        case Direction::HorizontalOnly: {
            zone &= (Zone::left | Zone::right);
            break;
        }
        case Direction::VerticalOnly: {
            zone &= (Zone::top | Zone::bottom);
            break;
        }
        case Direction::DiagonalOnly: {
            const bool hasHorizontal = zone & (Zone::left | Zone::right);
            const bool hasVertical   = zone & (Zone::top  | Zone::bottom);
            if (!(hasHorizontal && hasVertical))
                zone = 0;
            break;
        }
        default: break; // Allow any direction
    }

    resizeZone = static_cast<Zone>(zone);
    validResizeZone = resizeZone.getZoneFlags() != ResizableBorderComponent::Zone::centre && e.originalComponent == this && !(gui && gui->isEditorShown()) && !newObjectEditor;

    setMouseCursor(validResizeZone ? resizeZone.getMouseCursor() : MouseCursor::NormalCursor);
    updateMouseCursor();
}

void Object::applyBounds()
{
    UnorderedMap<Object*, Rectangle<int>> newObjectSizes;
    for (auto* obj : cnv->getSelectionOfType<Object>())
        newObjectSizes[obj] = obj->getObjectBounds();

    auto const positionOffset = gui ? getBounds().reduced(margin).getPosition() - cnv->canvasOrigin - gui->getPdBounds().getPosition() : Point<int>(0, 0);
    auto* patch = &cnv->patch;

    cnv->pd->lockAudioThread();

    if (ds.wasResized || ds.wasDragDuplicated) {
        patch->startUndoSequence("Resize");

        for (auto& [object, bounds] : newObjectSizes) {
            if (object->gui)
                object->gui->setPdBounds(bounds);
        }

        if (auto* patch = cnv->patch.getRawPointer()) {
            canvas_dirty(patch, 1);
        }

        patch->endUndoSequence("Resize");
    } else if (ds.didStartDragging) {
        patch->startUndoSequence("Move");
        SmallArray<t_gobj*> objects;
        for (auto const* obj : cnv->getSelectionOfType<Object>()) {
            if (auto* ptr = obj->getPointer()) {
                objects.add(ptr);
            }
        }

        cnv->patch.moveObjects(objects, positionOffset.x, positionOffset.y);
        patch->endUndoSequence("Move");
    }
    cnv->pd->unlockAudioThread();

    MessageManager::callAsync([editor = SafePointer(this->editor)] {
        if (editor)
            editor->updateCommandStatus();
    });
}
void Object::updateBounds()
{
    if (cnv->isGraph && gui && gui->hideInGraph())
        return;

    // only update if we have a gui and the object isn't been moved by the user
    // otherwise PD hasn't been informed of the new position 'while' we are dragging
    // so we don't need to update the bounds when an object is being interacted with
    if (gui && !isObjectMouseActive) {
        // Get the bounds of the object in Pd
        auto const newBounds = gui->getPdBounds();

        // Objects may return empty bounds if they are not a real object (like scalars)
        if (!newBounds.isEmpty())
            setObjectBounds(newBounds);
        else
            setTopLeftPosition(newBounds.getX() + margin + cnv->canvasOrigin.x, newBounds.getY() + margin + cnv->canvasOrigin.y);
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
            for (auto const* connection : getConnections())
                cnv->connections.remove_one(connection);

            if (auto* checkedObject = pd::Interface::checkObject(getPointer())) {
                auto const renamedObject = patch->renameObject(checkedObject, newType);
                objectPtr = pd::WeakReference(renamedObject, cnv->pd);
            }

            // Synchronise to make sure connections are preserved correctly
            cnv->synchronise();
        } else {
            auto const rect = getObjectBounds();
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

    isGemObject = is_gem_object(gui->getText().toRawUTF8());

    if (gui) {
        gui->initialise();
        gui->lock(cnv->isGraph || locked == var(true) || commandLocked == var(true));
        gui->addMouseListener(this, true);
        addAndMakeVisible(gui.get());
        if (hvccMode.get())
            isHvccCompatible = gui->checkHvccCompatibility();
        else
            isHvccCompatible = true;
    }

    // Update inlets/outlets
    updateIolets();
    updateBounds();
    resized(); // If bounds haven't changed, we'll still want to update gui and iolets bounds

    // Auto patching
    if (getValue<bool>(editor->autoconnect) && numInputs && cnv->lastSelectedObject && cnv->lastSelectedObject != this && cnv->lastSelectedObject->numOutputs) {
        auto* outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        auto* inlet = iolets[0];
        if (outlet->isSignal == inlet->isSignal) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([this, outlet, inlet] {
                cnv->connections.add(cnv, outlet, inlet, nullptr);
            });
        }
    }

    if (cnv->lastSelectedConnection && numInputs && numOutputs) {
        // if 1 connection is selected, connect the new object in middle of connection
        auto const outobj = cnv->lastSelectedConnection->outobj;
        auto const inobj = cnv->lastSelectedConnection->inobj;
        auto* outlet = outobj->iolets[outobj->numInputs + cnv->lastSelectedConnection->outIdx];
        auto* inlet = inobj->iolets[cnv->lastSelectedConnection->inIdx];

        auto* checkedOut = pd::Interface::checkObject(outobj->getPointer());
        auto* checkedIn = pd::Interface::checkObject(inobj->getPointer());

        if (checkedOut && checkedIn && outlet->isSignal == iolets[0]->isSignal && inlet->isSignal == iolets[this->numInputs]->isSignal) {
            // Call async to make sure the object is created before the connection
            MessageManager::callAsync([_this = SafePointer(this), this, outlet, inlet] {
                if (!_this)
                    return;
                cnv->connections.add(cnv, outlet, iolets[0], nullptr);
                cnv->connections.add(cnv, iolets[this->numInputs], inlet, nullptr);
            });
            // remove the previous connection
            cnv->patch.removeConnection(checkedOut, cnv->lastSelectedConnection->outIdx, checkedIn, cnv->lastSelectedConnection->inIdx, cnv->lastSelectedConnection->getPathState());
            cnv->connections.remove_one(cnv->lastSelectedConnection);
        }
    }
    cnv->lastSelectedObject = nullptr;

    cnv->lastSelectedConnection = nullptr;

    editor->updateCommandStatus();

    cnv->synchroniseSplitCanvas();
    cnv->pd->updateObjectImplementations();
}

SmallArray<Rectangle<float>> Object::getCorners() const
{
    auto const rect = getLocalBounds().reduced(margin);
    constexpr float offset = 2.0f;

    SmallArray<Rectangle<float>> corners = { Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
        Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset) };

    return corners;
}

String Object::getType(bool const withOriginPrefix) const
{
    if (gui && withOriginPrefix)
        return gui->getTypeWithOriginPrefix();
    if (gui)
        return gui->getType();
    return String();
}

void Object::triggerOverlayActiveState()
{
    if (rateReducer.tooFast())
        return;

    if (!cnv->shouldShowObjectActivity())
        return;

    activeStateAlpha = 1.0f;
    startTimer(1000 / 30.f);

    // Because the timer is being reset when new messages come in
    // it will not trigger it's callback until it's free-running
    // so we manually call the repaint here if this happens
    repaint();
}

void Object::lookAndFeelChanged()
{
    if (gui)
        gui->updateLabel();
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

    updateIoletGeometry();
}

void Object::updateIoletGeometry()
{

    int maxIoletWidth = std::min((getWidth() - doubleMargin) / std::max<int>(numInputs, 1) - 4, (getWidth() - doubleMargin) / std::max<int>(numOutputs, 1) - 4);
    int maxIoletHeight = getHeight() / 2.0f - 2;

    int ioletSize = PlugDataLook::ioletSize;

    ioletSize = std::max(std::min({ ioletSize, maxIoletWidth, maxIoletHeight }), 10);
    int const borderWidth = jmap<float>(ioletSize, 10, 13, 7, 12);

    // IOLET layout for vanilla style (iolets in corners of objects)
    if (PlugDataLook::getUseIoletSpacingEdge()) {
        auto vanillaIoletBounds = getLocalBounds();
        auto const marginOffset = (Corners::objectCornerRadius == 0.0f) * 2;
        vanillaIoletBounds.removeFromLeft(margin - marginOffset);
        vanillaIoletBounds.removeFromRight(margin - marginOffset);
        auto objectWidth = vanillaIoletBounds.getWidth() + 0.5f; // FIXME: the right most iolet looks not right otherwise

        int inletIndex = 0;
        int outletIndex = 0;
        for (auto const& iolet : iolets) {
            bool const isInlet = iolet->isInlet;
            float const yPosition = (isInlet ? margin + 1 : getHeight() - margin) - ioletSize / 2.0f;

            auto distributeIolets = [ioletSize, objectWidth, vanillaIoletBounds, yPosition](Iolet* iolet, int const ioletIndex, int const totalIolets) {
                auto const allOutLetWidth = totalIolets * ioletSize;
                auto const spacing = ioletIndex != 0 ? (objectWidth - allOutLetWidth) / static_cast<float>(totalIolets - 1) : 0;
                auto const ioletOffset = ioletIndex != 0 ? ioletSize * ioletIndex : 0;
                iolet->setBounds(vanillaIoletBounds.getX() + spacing * ioletIndex + ioletOffset, yPosition, ioletSize, ioletSize);
            };

            if (isInlet) {
                distributeIolets(iolet, inletIndex, numInputs);
                inletIndex++;
            } else {
                distributeIolets(iolet, outletIndex, numOutputs);
                outletIndex++;
            }
        }
        // DEFAULT iolet position style
    } else {
        constexpr int ioletHitBox = 6;
        auto inletBounds = getLocalBounds();
        if (auto const spaceToRemove = jlimit<int>(0, borderWidth, inletBounds.getWidth() - ioletHitBox * numInputs - borderWidth)) {
            inletBounds.removeFromLeft(spaceToRemove);
            inletBounds.removeFromRight(spaceToRemove);
        }

        auto outletBounds = getLocalBounds();
        if (auto const spaceToRemove = jlimit<int>(0, borderWidth, outletBounds.getWidth() - ioletHitBox * numOutputs - borderWidth)) {
            outletBounds.removeFromLeft(spaceToRemove);
            outletBounds.removeFromRight(spaceToRemove);
        }

        int index = 0;
        for (auto const& iolet : iolets) {
            bool const isInlet = iolet->isInlet;
            int const position = index < numInputs ? index : index - numInputs;
            int const total = isInlet ? numInputs : numOutputs;
            float const yPosition = (isInlet ? margin + 1 : getHeight() - margin) - ioletSize / 2.0f;

            auto const bounds = isInlet ? inletBounds : outletBounds;

            if (total == 1 && position == 0) {
                iolet->setBounds(getWidth() < 25 + ioletSize ? getLocalBounds().getCentreX() - (ioletSize - 1) / 2.0f : bounds.getX(),
                    yPosition, ioletSize, ioletSize);
            } else if (total > 1) {
                float const ratio = (bounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
                iolet->setBounds(bounds.getX() + ratio * position, yPosition, ioletSize, ioletSize);
            }

            index++;
        }
    }
}

void Object::updateTooltips()
{
    if (!gui || cnv->isGraph)
        return;

    auto const objectInfo = cnv->pd->objectLibrary->getObjectInfo(gui->getTypeWithOriginPrefix());

    StackArray<StringArray, 2> ioletTooltips;

    if (objectInfo.isValid()) {
        // Set object tooltip
        gui->setTooltip(objectInfo.getProperty("description").toString());
        // Check pd library for pddp tooltips, those have priority
        ioletTooltips = cnv->pd->objectLibrary->parseIoletTooltips(objectInfo.getChildWithName("iolets"), gui->getText(), numInputs, numOutputs);
    }

    // First clear all tooltips, so we can see later if it has already been set or not
    for (auto const iolet : iolets) {
        iolet->setTooltip("");
    }

    // Load tooltips from documentation files, these have priority
    for (int i = 0; i < iolets.size(); i++) {
        auto* iolet = iolets[i];

        auto& tooltip = ioletTooltips[!iolet->isInlet][iolet->isInlet ? i : i - numInputs];
        if (tooltip.startsWith("(gemlist)")) {
            iolet->isGemState = true;
            iolet->setTooltip("(gemlist)");
        } else {
            if (tooltip.isNotEmpty()) { // Don't overwrite custom documentation if there is no md documentation
                iolet->setTooltip(tooltip);
            }
            iolet->isGemState = false;
        }
    }

    SmallArray<std::pair<int, String>, 16> inletMessages;
    SmallArray<std::pair<int, String>, 16> outletMessages;

    if (auto const subpatch = gui->getPatch()) {
        cnv->pd->lockAudioThread();
        auto* subpatchPtr = subpatch->getPointer().get();

        // Check child objects of subpatch for inlet/outlet messages
        for (auto obj : subpatch->getObjects()) {

            if (!obj.isValid())
                continue;

            auto const name = String::fromUTF8((*obj.getRaw<t_pd>())->c_name->s_name);
            auto const* checkedObject = pd::Interface::checkObject(obj.getRaw<t_pd>());
            if (name == "inlet" || name == "inlet~") {
                int size;
                char* str_ptr;
                pd::Interface::getObjectText(checkedObject, &str_ptr, &size);

                int x, y, w, h;
                pd::Interface::getObjectBounds(subpatchPtr, obj.getRaw<t_gobj>(), &x, &y, &w, &h);

                // Anything after the first space will be the comment
                auto const text = String::fromUTF8(str_ptr, size);
                inletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));
                freebytes(str_ptr, static_cast<size_t>(size) * sizeof(char));
            } else if (name == "outlet" || name == "outlet~") {
                int size;
                char* str_ptr;
                pd::Interface::getObjectText(checkedObject, &str_ptr, &size);

                int x, y, w, h;
                pd::Interface::getObjectBounds(subpatchPtr, obj.getRaw<t_gobj>(), &x, &y, &w, &h);

                auto const text = String::fromUTF8(str_ptr, size);
                outletMessages.emplace_back(x, text.fromFirstOccurrenceOf(" ", false, false));
                freebytes(str_ptr, static_cast<size_t>(size) * sizeof(char));
            }
        }
        cnv->pd->unlockAudioThread();
    }

    if (inletMessages.empty() && outletMessages.empty())
        return;

    auto sortFunc = [](auto const& a, auto const& b) {
        return a.first < b.first;
    };

    inletMessages.sort(sortFunc);
    outletMessages.sort(sortFunc);

    int numIn = 0;
    int numOut = 0;

    for (auto const iolet : iolets) {
        if (iolet->getTooltip().isNotEmpty())
            continue;

        if ((iolet->isInlet && numIn >= inletMessages.size()) || (!iolet->isInlet && numOut >= outletMessages.size()))
            continue;

        auto& [x, message] = iolet->isInlet ? inletMessages[numIn++] : outletMessages[numOut++];
        iolet->setTooltip(message);
        iolet->isGemState = message.startsWith("(gemlist)");
    }
}

void Object::updateIolets()
{
    if (!getPointer())
        return;

    // update inlets and outlets
    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto const* iolet : iolets) {
        iolet->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (auto const* ptr = pd::Interface::checkObject(getPointer())) {
        numInputs = pd::Interface::numInlets(ptr);
        numOutputs = pd::Interface::numOutlets(ptr);
    }

    // Looking up tooltips takes a bit of time, so we make sure we're not constantly updating them for no reason
    bool const tooltipsNeedUpdate = gui->getPatch() != nullptr || numInputs != oldNumInputs || numOutputs != oldNumOutputs || isGemObject;

    for (auto* iolet : iolets) {
        if (gui && !iolet->isInlet) {
            iolet->setHidden(gui->hideOutlet());
        } else if (gui && iolet->isInlet) {
            iolet->setHidden(gui->hideInlet());
        }
    }

    while (numInputs < oldNumInputs)
        iolets.remove_at(--oldNumInputs);
    while (numInputs > oldNumInputs)
        iolets.insert(oldNumInputs++, this, true);
    while (numOutputs < oldNumOutputs)
        iolets.remove_at(numInputs + --oldNumOutputs);
    while (numOutputs > oldNumOutputs)
        iolets.insert(numInputs + oldNumOutputs++, this, false);

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++) {
        auto* iolet = iolets[i];
        bool const input = iolet->isInlet;

        bool isSignal = false;
        auto const* patchableObject = pd::Interface::checkObject(getPointer());
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

    if (tooltipsNeedUpdate)
        updateTooltips();
    resized();
}

void Object::mouseDown(MouseEvent const& e)
{
    // Only show right-click menu in locked mode if the object can be opened
    // We don't allow alt+click for popupmenus here, as that will conflict with some object behaviour, like for [range.hsl]
    if (e.mods.isRightButtonDown() && !cnv->isGraph && !(gui && gui->isEditorShown())) {
        PopupMenu::dismissAllActiveMenus();
        if (!getValue<bool>(locked)) {
            if (!e.mods.isAnyModifierKeyDown() && !e.mods.isRightButtonDown())
                cnv->deselectAll();
            cnv->setSelected(this, true);
        }
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
        ds.duplicateOffset = { 0, 0 };
        ds.lastDuplicateOffset = { 0, 0 };
        ds.wasDuplicated = false;
    } else if (!selectedFlag) {
        cnv->deselectAll();
        ds.duplicateOffset = { 0, 0 };
        ds.lastDuplicateOffset = { 0, 0 };
        ds.wasDuplicated = false;
    }

    cnv->setSelected(this, true);

    ds.componentBeingDragged = this;

    for (auto* object : cnv->getSelectionOfType<Object>()) {
        object->originalBounds = object->getBounds();
    }

    bool const overEdgeNotCorner = resizeZone.isDraggingTopEdge() + resizeZone.isDraggingLeftEdge() + resizeZone.isDraggingBottomEdge() + resizeZone.isDraggingRightEdge() == 1;

    for (auto toResize = cnv->getSelectionOfType<Object>(); auto const* obj : toResize) {
        if (!obj->gui)
            continue;

        bool isDraggingOneEdge = false;
        if (obj->gui->canEdgeOverrideAspectRatio() && overEdgeNotCorner) {
            isDraggingOneEdge = true;
        }

        if (auto* constrainer = obj->getConstrainer()) {
            if (gui && gui->canEdgeOverrideAspectRatio()) {
                if (isDraggingOneEdge)
                    constrainer->setFixedAspectRatio(0);
                else
                    constrainer->setFixedAspectRatio(static_cast<float>(obj->getObjectBounds().getWidth()) / obj->getObjectBounds().getHeight());
            }
        }
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

    auto objects = cnv->getSelectionOfType<Object>();
    for (auto* obj : objects)
        obj->isObjectMouseActive = false;

    if (ds.wasResized) {
        cnv->objectGrid.clearIndicators(false);
        for (auto* connection : getConnections())
            connection->pushPathState(true);

        applyBounds();
        cnv->patch.endUndoSequence("Resize");

        ds.wasResized = false;
        originalBounds.setBounds(0, 0, 0, 0);
    } else {
        if (cnv->isGraph) {
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

        if (ds.didStartDragging) {
            for (auto* connection : getConnections())
                connection->pushPathState(true);

            applyBounds();
            cnv->patch.endUndoSequence("Drag");
            cnv->objectGrid.clearIndicators(false);
            ds.didStartDragging = false;
        }

        cnv->updateSidebarSelection();

        if (ds.objectSnappingInbetween) {
            auto const* c = ds.connectionToSnapInbetween.getComponent();

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

        for (auto* object : cnv->getSelectionOfType<Object>()) {
            object->repaint();
        }

        ds.componentBeingDragged = nullptr;

        repaint();
    }

    if (ds.wasDragDuplicated) {
        cnv->patch.endUndoSequence("Duplicate");
        ds.wasDragDuplicated = false;
    }

    if (gui && selectedFlag && !selectionStateChanged && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isRightButtonDown() && !e.mods.isAnyModifierKeyDown()) {
        gui->showEditor();
    }

    selectionStateChanged = false;
    cnv->needsSearchUpdate = true;
}

void Object::mouseDrag(MouseEvent const& e)
{
    if (wasLockedOnMouseDown || (gui && gui->isEditorShown()))
        return;

    if (cnv->objectRateReducer.tooFast())
        return;

#if JUCE_MAC || JUCE_WINDOWS
    beginDragAutoRepeat(25); // Doing this leads to terrible performance on Linux, unfortunately
#endif

    cnv->cancelConnectionCreation();

    if (e.mods.isMiddleButtonDown())
        return;

    if (validResizeZone && !originalBounds.isEmpty()) {
        auto const draggedBounds = resizeZone.resizeRectangleBy(originalBounds, e.getOffsetFromDragStart());
        auto const dragDistance = cnv->objectGrid.performResize(this, e.getOffsetFromDragStart(), draggedBounds);

        auto toResize = cnv->getSelectionOfType<Object>();

        if (!ds.wasResized) {
            cnv->patch.startUndoSequence("Resize");
        }
        for (auto* obj : toResize) {

            if (!obj->gui)
                continue;

            obj->isObjectMouseActive = true;

            // Create undo step when we start resizing
            if (!ds.wasResized) {
                auto* objPtr = obj->getPointer();
                auto const* cnv = obj->cnv;

                if (auto patchPtr = cnv->patch.getPointer()) {
                    pd::Interface::undoApply(patchPtr.get(), objPtr);
                }
            }

            auto const newBounds = resizeZone.resizeRectangleBy(obj->originalBounds, dragDistance);

            if (auto* constrainer = obj->getConstrainer()) {
                constrainer->setBoundsForComponent(obj, newBounds, resizeZone.isDraggingTopEdge(), resizeZone.isDraggingLeftEdge(), resizeZone.isDraggingBottomEdge(), resizeZone.isDraggingRightEdge());
            }
        }

        ds.wasResized = true;
    } else if (!cnv->isGraph) {
        int constexpr minimumMovementToStartDrag = 5;

        // Ensure tiny movements don't start a drag.
        if (!ds.didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
            return;

        if (!ds.didStartDragging) {
            cnv->patch.startUndoSequence("Drag");
            ds.didStartDragging = true;
        }

        auto const canvasMoveOffset = ds.canvasDragStartPosition - cnv->getPosition();
        auto selection = cnv->getSelectionOfType<Object>();
        auto dragDistance = e.getOffsetFromDragStart() + canvasMoveOffset;

        if (ds.componentBeingDragged) {
            dragDistance = cnv->objectGrid.performMove(this, dragDistance);
            ds.duplicateOffset = ds.wasDuplicated ? dragDistance : Point<int>(0, 0);
        }

        // alt+drag will duplicate selection
        if (!ds.wasDragDuplicated && e.mods.isAltDown()) {
            SmallArray<Point<int>> mouseDownObjectPositions; // Stores object positions for alt + drag

            // Single for undo for duplicate + move
            cnv->patch.startUndoSequence("Duplicate");

            // Sort selection indexes to match pd indexes
            selection.sort([this](auto const* a, auto const* b) -> bool {
                return cnv->objects.index_of(a) < cnv->objects.index_of(b);
            });

            int const draggedIdx = selection.index_of(ds.componentBeingDragged.getComponent());

            // Store origin object positions
            for (auto const object : selection) {
                auto const gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
                auto const gridType = SettingsFile::getInstance()->getProperty<int>("grid_type");
                auto const gridSize = gridEnabled && gridType & 1 ? cnv->objectGrid.gridSize : 10;

                mouseDownObjectPositions.add(object->getPosition().translated(gridSize, gridSize));
            }

            // Duplicate once
            ds.wasDragDuplicated = true;
            cnv->duplicateSelection();
            cnv->cancelConnectionCreation();

            selection = cnv->getSelectionOfType<Object>();

            // Sort selection indexes to match pd indexes
            selection.sort([this](auto* a, auto* b) -> bool {
                return cnv->objects.index_of(a) < cnv->objects.index_of(b);
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
                auto const newPosition = object->originalBounds.getPosition() + dragDistance;

                object->setTopLeftPosition(newPosition);
            }

            if (cnv->viewport) {
                cnv->autoscroll(e.getEventRelativeTo(cnv->viewport.get()));
            }
        }

        // This handles the "unsnap" action when you shift-drag a connected object
        if (e.mods.isShiftDown() && selection.size() == 1 && e.getDistanceFromDragStart() > 15) {
            auto* object = selection.front();
            cnv->patch.startUndoSequence("Snap");

            SmallArray<Connection*> inputs, outputs;
            for (auto* connection : cnv->connections) {
                if (object->iolets.size() && connection->inlet == object->iolets[0]) {
                    inputs.add(connection);
                }
                if (object->numOutputs && connection->outlet == object->iolets[object->numInputs]) {
                    outputs.add(connection);
                }
            }

            // Two cases that are allowed: either 1 input and multiple outputs,
            // or 1 output and multiple inputs
            if (inputs.size() == 1 && outputs.size()) {
                auto outlet = inputs[0]->outlet;

                for (auto* c : outputs) {
                    auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                    auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                    if (checkedOut && checkedIn) {
                        cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                        cnv->connections.remove_one(c);
                        cnv->connections.add(cnv, outlet, c->inlet, nullptr);
                    }
                }

                auto const* c = inputs[0];
                auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                if (checkedOut && checkedIn) {
                    cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                }
                cnv->connections.remove_one(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            } else if (inputs.size() && outputs.size() == 1) {
                auto inlet = outputs[0]->inlet;

                for (auto* c : inputs) {
                    auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                    auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());
                    if (checkedOut && checkedIn) {
                        cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                    }

                    cnv->connections.add(cnv, c->outlet, inlet, nullptr);
                    cnv->connections.remove_one(c);
                }

                auto const* c = outputs[0];
                auto* checkedOut = pd::Interface::checkObject(c->outobj->getPointer());
                auto* checkedIn = pd::Interface::checkObject(c->inobj->getPointer());

                if (checkedOut && checkedIn) {
                    cnv->patch.removeConnection(checkedOut, c->outIdx, checkedIn, c->inIdx, c->getPathState());
                }
                cnv->connections.remove_one(c);

                object->iolets[0]->isTargeted = false;
                object->iolets[object->numInputs]->isTargeted = false;
                object->iolets[0]->repaint();
                object->iolets[object->numInputs]->repaint();

                ds.objectSnappingInbetween = nullptr;
            }

            cnv->patch.endUndoSequence("Snap");
        }

        // Behaviour for shift-dragging objects over
        if (ds.objectSnappingInbetween && !ds.objectSnappingInbetween->iolets.empty()) {
            if (ds.connectionToSnapInbetween->intersectsRectangle(ds.objectSnappingInbetween->iolets[0]->getCanvasBounds())) {
                return;
            }

            // If we're here, it's not snapping anymore
            ds.objectSnappingInbetween->iolets[0]->isTargeted = false;
            ds.objectSnappingInbetween->iolets[ds.objectSnappingInbetween->numInputs]->isTargeted = false;
            ds.objectSnappingInbetween = nullptr;
        }

        if (e.mods.isShiftDown() && selection.size() == 1) {
            auto* object = selection.front();
            if (object->numInputs && object->numOutputs && !object->iolets.empty()) {
                bool intersected = false;
                for (auto* connection : cnv->connections) {
                    if (connection->intersectsRectangle(object->iolets[0]->getCanvasBounds()) && !iolets.contains(connection->inlet) && !iolets.contains(connection->outlet)) {
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

void Object::render(NVGcontext* nvg)
{
    auto const lb = getLocalBounds();
    auto const b = lb.reduced(margin);

    if (cnv->shouldShowObjectActivity() && !approximatelyEqual(activeStateAlpha, 0.0f)) {
        auto glowColour = cnv->dataCol;
        glowColour.a = static_cast<uint8_t>(activeStateAlpha * 255);
        nvgSmoothGlow(nvg, lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), glowColour, nvgRGBA(0, 0, 0, 0), Corners::objectCornerRadius, 1.1f);
    }

    if (selectedFlag && showHandles) {
        auto& resizeHandleImage = cnv->resizeHandleImage;
        int angle = 360;
        for (auto& corner : getCorners()) {
            NVGScopedState scopedState(nvg);
            // Rotate around centre
            nvgTranslate(nvg, corner.getCentreX(), corner.getCentreY());
            nvgRotate(nvg, degreesToRadians<float>(angle));
            nvgTranslate(nvg, -4.5f, -4.5f);

            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, 9, 9);
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, 0, 0, 9, 9, 0, resizeHandleImage.getImageId(), cnv->selectedOutlineCol));
            nvgFill(nvg);
            angle -= 90;
        }
    }

    if (gui && gui->isTransparent() && !getValue<bool>(locked) && !cnv->isGraph) {
        nvgFillColor(nvg, cnv->transparentObjectBackgroundCol);
        nvgFillRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
    }

    nvgTranslate(nvg, margin, margin);

    if (gui) {
        gui->render(nvg);
    }

    if (newObjectEditor) {
        nvgDrawRoundedRect(nvg, 0, 0, b.getWidth(), b.getHeight(), cnv->textObjectBackgroundCol, isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);
        Graphics g(*editor->getNanoLLGC());
        newObjectEditor->paintEntireComponent(g, true);
    }

    // If autoconnect is about to happen, draw a fake inlet with a dotted outline
    if (isInitialEditorShown() && cnv->lastSelectedObject && cnv->lastSelectedObject != this && cnv->lastSelectedObject->numOutputs && getValue<bool>(editor->autoconnect)) {
        auto const* outlet = cnv->lastSelectedObject->iolets[cnv->lastSelectedObject->numInputs];
        SmallArray fakeInletBounds = PlugDataLook::getUseIoletSpacingEdge() ? SmallArray { -8.0f, -3.0f, 18.0f, 7.0f } : SmallArray { 8.5f, -3.5f, 8.0f, 8.0f };
        nvgBeginPath(nvg);
        if (PlugDataLook::getUseSquareIolets()) {
            nvgRect(nvg, fakeInletBounds[0] + fakeInletBounds[2] * 0.5f, fakeInletBounds[1] + fakeInletBounds[3] * 0.5f, fakeInletBounds[2] * 0.5f, fakeInletBounds[3] * 0.5f);
        } else {
            nvgEllipse(nvg, fakeInletBounds[0] + fakeInletBounds[2] * 0.5f, fakeInletBounds[1] + fakeInletBounds[3] * 0.5f, fakeInletBounds[2] * 0.5f, fakeInletBounds[3] * 0.5f);
        }

        nvgFillColor(nvg, outlet->isSignal ? cnv->sigColBrighter : cnv->dataColBrighter);
        nvgFill(nvg);

        nvgStrokeColor(nvg, cnv->objectOutlineCol);
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }

    nvgTranslate(nvg, -margin, -margin);

    if (!isHvccCompatible) {
        NVGScopedState scopedState(nvg);
        nvgBeginPath(nvg);
        nvgStrokeColor(nvg, nvgRGBA(255, 127, 0.0f, 255)); // orange
        nvgStrokeWidth(nvg, 1.0f);
        nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
        nvgStroke(nvg);
    } else if (cnv->shouldShowIndex()) {
        constexpr int halfHeight = 5;

        auto const text = std::to_string(cnv->objects.index_of(this));
        int const textWidth = 6 + text.length() * 4;
        auto const indexBounds = b.withSizeKeepingCentre(b.getWidth() + doubleMargin, halfHeight * 2).removeFromRight(textWidth);

        auto const fillColour = cnv->selectedOutlineCol;
        nvgDrawRoundedRect(nvg, indexBounds.getX(), indexBounds.getY(), indexBounds.getWidth(), indexBounds.getHeight(), fillColour, fillColour, 2.0f);

        nvgFontSize(nvg, 8.0f);
        nvgFontFace(nvg, "Inter-Regular");
        nvgTextAlign(nvg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
        nvgFillColor(nvg, cnv->indexTextCol);
        nvgText(nvg, indexBounds.getCentreX(), indexBounds.getCentreY(), text.c_str(), nullptr);
    }

    renderIolets(nvg);
}

void Object::renderIolets(NVGcontext* nvg)
{
    if (cnv->isGraph)
        return;

    if (getValue<bool>(locked) || !drawIoletExpanded) {
        auto const clipBounds = getLocalBounds().reduced(Object::margin);
        nvgIntersectScissor(nvg, clipBounds.getX(), clipBounds.getY(), clipBounds.getWidth(), clipBounds.getHeight());
    } else if (patchDownwardsOnly) {
        auto const clipBounds = getLocalBounds().reduced(Object::margin);
        nvgIntersectScissor(nvg, clipBounds.getX(), clipBounds.getY(), clipBounds.getWidth(), clipBounds.getHeight() + Object::doubleMargin);
    }

    auto lastPosition = Point<int>();
    for (auto* iolet : iolets) {
        nvgTranslate(nvg, iolet->getX() - lastPosition.x, iolet->getY() - lastPosition.y);

        if (iolet->isTargeted) {
            nvgSave(nvg);
            nvgResetScissor(nvg);
        }

        iolet->render(nvg);
        lastPosition = iolet->getPosition();

        if (iolet->isTargeted) {
            nvgRestore(nvg);
        }
    }
}

void Object::renderLabel(NVGcontext* nvg)
{
    if (gui) {
        for (auto* label : gui->labels) {
            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, label->getX(), label->getY());
            if (label->isVisible()) {
                label->renderLabel(nvg, gui->getImageScale());
            }
        }
    }
}

// Returns true is the object is showing its initial editor, and doesn't have a GUI yet
bool Object::isInitialEditorShown() const
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

        // Get entered text, remove extra spaces at the end
        auto newText = outgoingEditor->getText().trimEnd();

        newText = newText.replace("\n", " ");
        newText = newText.replace(";", " ;");

        outgoingEditor.reset();

        repaint();
        setType(newText);
    }
}

SmallArray<Connection*> Object::getConnections() const
{
    SmallArray<Connection*> result;
    for (auto const* iolet : iolets) {
        result.add_array(iolet->getConnections());
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
        editor->applyFontToAllText(Font(FontOptions(15)));

        copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, PlugDataColours::canvasTextColour);
        editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        editor->setScrollToShowCursor(false);
        editor->setAlwaysOnTop(true);
        editor->setMultiLine(true);
        editor->setReturnKeyStartsNewLine(false);
        editor->setScrollbarsShown(false);
        editor->setBorder(BorderSize<int>(1, 6, 1, 1));
        editor->setIndents(0, 0);
        editor->setJustification(Justification::centredLeft);

        editor->setBounds(getLocalBounds().reduced(Object::margin));
        editor->addListener(this);
        editor->addKeyListener(this);

        // Allow cancelling object creation with escape
        editor->onEscapeKey = [this] {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (!_this)
                    return;
                auto* cnv = _this->cnv; // Copy pointer because _this will get deleted
                cnv->hideSuggestions();
                cnv->objects.remove_one(_this.getComponent());
                cnv->lastSelectedObject = nullptr;

                cnv->lastSelectedConnection = nullptr;
            });
        };

        addAndMakeVisible(editor);
        editor->grabKeyboardFocus();

        editor->onFocusLost = [this, editor] {
            if (cnv->suggestor->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor) {
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

bool Object::keyPressed(KeyPress const& key, Component* component)
{
    if (auto* editor = newObjectEditor.get()) {
        if (key.getKeyCode() == KeyPress::returnKey && key.getModifiers().isShiftDown()) {
            int caretPosition = editor->getCaretPosition();
            auto text = editor->getText();

            if (!editor->getHighlightedRegion().isEmpty())
                return false;
            if (text[caretPosition - 1] == ';') {
                text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
                caretPosition += 1;
            } else {
                text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);
                caretPosition += 2;
            }

            editor->setText(text);
            editor->setCaretPosition(caretPosition);
            cnv->hideSuggestions();

            return true;
        }
    }

    return false;
}

// For resize-while-typing behaviour
void Object::textEditorTextChanged(TextEditor& ed)
{
    if (cnv->suggestor)
        cnv->suggestor->updateSuggestions(ed.getText());

    String currentText;
    if (cnv->suggestor && !cnv->suggestor->getText().isEmpty() && !ed.getText().containsChar('\n')) {
        currentText = cnv->suggestor->getText();
    } else {
        currentText = ed.getText();
    }

    // For resize-while-typing behaviour
    auto newWidth = CachedStringWidth<15>::calculateStringWidth(currentText) + 14.0f;
    newWidth += Object::doubleMargin;

    auto const numLines = StringArray::fromLines(currentText.trimEnd()).size();
    auto const newHeight = std::max(numLines * 15 + 5 + Object::doubleMargin, height);
    setSize(newWidth, newHeight);
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
        auto const file = pd::Library::findHelpfile(ptr, cnv->patch.getCurrentFile());

        if (!file.existsAsFile()) {
            cnv->pd->logMessage("Couldn't find help file");
            return;
        }

        editor->getTabComponent().openHelpPatch(URL(file));

        return;
    }

    cnv->pd->logMessage("Couldn't find help file");
}
