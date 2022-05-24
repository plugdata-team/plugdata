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

extern "C"
{
#include <m_pd.h>
#include <m_imp.h>
}

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
        toFront(false);
        if (attachedToMouse)
        {
            createEditorOnMouseDown = true;
        }
        else
        {
            showEditor();
        }
    }
    else
    {
        setType(name);
    }
    if (gui)
    {
        gui->showEditor();
    }
}

Box::Box(void* object, Canvas* parent)
{
    cnv = parent;

    initialise();

    setType("", object);
}

Box::~Box()
{
    if (attachedToMouse)
    {
        stopTimer();
    }
}

Rectangle<int> Box::getObjectBounds()
{
    return getBounds().reduced(margin) - cnv->canvasOrigin;
}

void Box::setObjectBounds(Rectangle<int> bounds)
{
    setBounds(bounds.expanded(margin) + cnv->canvasOrigin);
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
        setCentrePosition(cnv->viewport->getViewArea().getConstrainedPoint(pos));
    }
}

void Box::valueChanged(Value& v)
{
    // Hide certain objects in GOP
    if ((cnv->isGraph || cnv->presentationMode == var(true)) && gui && gui->hideInGraph())
    {
        setVisible(false);
    }
    else
    {
        setVisible(true);
        resized();
    }

    if (gui)
    {
        gui->lock(locked == var(true) || commandLocked == var(true));
    }

    resized();
    repaint();
}

bool Box::hitTest(int x, int y)
{
    int hitMargin = cnv->isSelected(this) ? margin - 5 : margin;

    // Mouse over object
    if (getLocalBounds().reduced(margin).contains(x, y))
    {
        return true;
    }

    // Mouse over edges
    for (auto* edge : edges)
    {
        if (edge->getBounds().contains(x, y)) return true;
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

void Box::mouseEnter(const MouseEvent& e)
{
    repaint();
}

void Box::mouseExit(const MouseEvent& e)
{
    repaint();
}

bool Box::isOver()
{
    if (isMouseOverOrDragging())
    {
        return true;
    }
    if (gui && gui->isMouseOver())
    {
        return true;
    }

    return false;
}

void Box::mouseMove(const MouseEvent& e)
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

void Box::updateBounds()
{
    if (gui)
    {
        cnv->pd->setThis();
        gui->updateBounds();
    }
    resized();
}

void Box::setType(const String& newType, void* existingObject)
{
    // Change box type
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    void* objectPtr = nullptr;
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
            MessageManager::callAsync([this]() { cnv->synchronise(false); });
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
        addAndMakeVisible(gui.get());
    }

    // Update inlets/outlets
    updateBounds();
    updatePorts();

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
    bool selected = cnv->isSelected(this);

    if (selected && !cnv->isGraph)
    {
        g.setColour(findColour(PlugDataColour::highlightColourId));

        g.saveState();
        g.excludeClipRegion(getLocalBounds().reduced(margin + 1));

        // Draw resize edges when selected
        for (auto& rect : getCorners())
        {
            g.fillRoundedRectangle(rect, 2.0f);
        }
        g.restoreState();
    }

    if (gui && !gui->drawOutline()) return;

    auto outlineColour = findColour(PlugDataColour::canvasOutlineColourId);
    float thickness = 1.0f;
    if (attachedToMouse)
    {
        outlineColour = Colours::lightgreen;
        thickness = 2.0f;
    }
    else if (selected && !cnv->isGraph)
    {
        outlineColour = findColour(PlugDataColour::highlightColourId);
    }

    g.setColour(outlineColour);
    g.drawRoundedRectangle(rect.toFloat(), 2.0f, thickness);
}

void Box::resized()
{
    if (gui)
    {
        gui->setBounds(getLocalBounds().reduced(margin));
    }

    if (newObjectEditor)
    {
        newObjectEditor->setBounds(getLocalBounds().reduced(margin));
    }

    int edgeSize = 12;
    const int edgeHitBox = 4;
    const int borderWidth = 14;

    if (getWidth() < 32 && (numInputs > 1 || numOutputs > 1))
    {
        edgeSize = 9;
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
            const float ratio = (bounds.getWidth() - edgeSize) / (float)(total - 1);
            edge->setBounds(bounds.getX() + ratio * position, yPosition, edgeSize, edgeSize);
        }

        index++;
    }
}

void Box::updatePorts()
{
    if (!getPointer()) return;

    // update inlets and outlets

    int oldNumInputs = 0;
    int oldNumOutputs = 0;

    for (auto& edge : edges)
    {
        edge->isInlet ? oldNumInputs++ : oldNumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (auto* ptr = pd::Patch::checkObject(getPointer()))
    {
        numInputs = libpd_ninlets(ptr);
        numOutputs = libpd_noutlets(ptr);
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

        bool isSignal;
        if (i < numInputs)
        {
            isSignal = libpd_issignalinlet(pd::Patch::checkObject(getPointer()), i);
        }
        else
        {
            isSignal = libpd_issignaloutlet(pd::Patch::checkObject(getPointer()), i - numInputs);
        }

        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;
        edge->setAlwaysOnTop(true);

        if (gui)
        {
            String tooltip = cnv->pd->objectLibrary.getInletOutletTooltip(gui->getName(), edge->edgeIdx, input ? numInputs : numOutputs, input);
            edge->setTooltip(tooltip);
        }

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

        if (createEditorOnMouseDown)
        {
            createEditorOnMouseDown = false;
            // Prevent losing focus because of click event
            MessageManager::callAsync([this]() { showEditor(); });
        }
        else
        {
            auto box = SafePointer<Box>(this);
            // Tell pd about new position
            cnv->pd->enqueueFunction(
                [this, box]()
                {
                    if (!box || !box->gui) return;
                    gui->applyBounds();
                });
        }

        return;
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

    cnv->handleMouseUp(this, e);

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600)
    {
        cnv->connectingEdge = nullptr;
    }

    if (!originalBounds.isEmpty() && originalBounds.withPosition(0, 0) != getLocalBounds())
    {
        originalBounds.setBounds(0, 0, 0, 0);

        auto box = SafePointer<Box>(this);
        cnv->pd->enqueueFunction(
            [this, box]()
            {
                if (!box || !gui) return;

                auto b = getBounds() - cnv->canvasOrigin;
                b.reduce(margin, margin);

                // Used for size changes, could also be used for properties
                auto* obj = static_cast<t_gobj*>(getPointer());
                auto* canvas = static_cast<t_canvas*>(cnv->patch.getPointer());
                libpd_undo_apply(canvas, obj);

                gui->applyBounds();

                // To make sure it happens after setting object bounds
                if (!cnv->viewport->getViewArea().contains(getBounds()))
                {
                    MessageManager::callAsync([this]() { cnv->checkBounds(); });
                }
            });
    }

    selectionChanged = false;
}

void Box::mouseDrag(const MouseEvent& e)
{
    if (cnv->isGraph || cnv->presentationMode == var(true) || cnv->pd->locked == var(true)) return;

    if (resizeZone.isDraggingTopEdge() || resizeZone.isDraggingLeftEdge() || resizeZone.isDraggingBottomEdge() || resizeZone.isDraggingRightEdge())
    {
        Point<int> dragDistance = e.getOffsetFromDragStart();

        auto newBounds = resizeZone.resizeRectangleBy(originalBounds, dragDistance);
        setBounds(newBounds);
    }
    // Let canvas handle moving
    else
    {
        cnv->handleMouseDrag(e);
    }
}

void Box::showEditor()
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

void Box::hideEditor()
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

        if (auto* peer = getPeer()) peer->dismissPendingTextInput();

        outgoingEditor->setInputFilter(nullptr, false);

        cnv->hideSuggestions();

        auto newText = outgoingEditor->getText();
        outgoingEditor.reset();

        repaint();
        setType(newText);
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

void* Box::getPointer() const
{
    return gui ? gui->ptr : nullptr;
}

void Box::openNewObjectEditor()
{
    if (!newObjectEditor)
    {
        newObjectEditor = std::make_unique<TextEditor>();

        auto* editor = newObjectEditor.get();
        editor->applyFontToAllText(Font(15));

        copyAllExplicitColoursTo(*editor);
        editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
        editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
        editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

        editor->setAlwaysOnTop(true);

        editor->setMultiLine(false);
        editor->setReturnKeyStartsNewLine(false);
        editor->setBorder(BorderSize<int>{1, 7, 1, 2});
        editor->setIndents(0, 0);
        editor->setJustification(Justification::left);

        editor->onFocusLost = [this]()
        {
            // Necessary so the editor doesn't close when clicking on a suggestion
            if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true))
            {
                hideEditor();
            }
        };

        cnv->showSuggestions(this, editor);

        editor->setSize(10, 10);
        addAndMakeVisible(editor);
        editor->addListener(this);

        if (editor == nullptr)  // may be deleted by a callback
            return;

        resized();
        repaint();

        editor->grabKeyboardFocus();
    }
}

void Box::textEditorReturnKeyPressed(TextEditor& ed)
{
    if (newObjectEditor)
    {
        newObjectEditor->giveAwayKeyboardFocus();
    }
}

void Box::textEditorTextChanged(TextEditor& ed)
{
    // For resize-while-typing behaviour
    auto width = Font(15).getStringWidth(ed.getText()) + 25;

    if (width > getWidth())
    {
        setSize(width, getHeight());
    }
}

pd::Patch Box::getHelp() const
{
    static File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library");

    auto* ptr = getPointer();
    if (!ptr) return {nullptr, nullptr};

    auto* pdclass = pd_class(static_cast<t_pd*>(ptr));
    auto* name = class_gethelpname(pdclass);

    String firstName = String(name) + "-help.pd";
    String secondName = "help-" + String(name) + ".pd";

    auto findHelpPatch = [&firstName, &secondName](File searchDir) -> File
    {
        for (auto& fileIter : RangedDirectoryIterator(searchDir, true))
        {
            auto file = fileIter.getFile();
            if (file.getFileName() == firstName)
            {
                return file;
            }
            else if (file.getFileName() == secondName)
            {
                return file;
            }
        }

        return File();
    };

    // Paths to search
    // First, only search vanilla, then search all documentation
    std::vector<File> paths = {appDir.getChildFile("Documentation").getChildFile("5.reference"), appDir.getChildFile("Documentation")};

    for (auto& path : paths)
    {
        auto file = findHelpPatch(path);
        if (file.existsAsFile())
        {
            auto name = file.getFileName();
            auto fullPath = file.getParentDirectory().getFullPathName();
            sys_lock();
            auto* pdPatch = glob_evalfile(nullptr, gensym(name.toRawUTF8()), gensym(fullPath.toRawUTF8()));
            sys_unlock();
            return {pdPatch, cnv->pd, file.getChildFile(secondName)};
        }
    }

    return {nullptr, nullptr};
}
