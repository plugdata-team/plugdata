/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/
#include "ZoomableDragAndDropContainer.h"
#include "Utility/RateReducer.h"

#include "Constants.h"
#include "LookAndFeel.h"

// this is to find if we are over a split, and if so, we zoom the dragged image to the canvas zoom value
#include "Tabbar/ResizableTabbedComponent.h"

// if the drag is over a tab bar, then we don't show the drag image
#include "Tabbar/Tabbar.h"

// to reset the active split to what it was originally
#include "Tabbar/TabBarButtonComponent.h"

// if we are inside the splitview, don't reset the scale of the dragged image, regardless of what we are over
#include "Tabbar/SplitView.h"

// objects are only drag and dropped onto a canvas, so we dynamic cast straight away to see if the dragged object is from an object
#include "ObjectDragAndDrop.h"

bool juce_performDragDropFiles(StringArray const&, bool const copyFiles, bool& shouldStop);
bool juce_performDragDropText(String const&, bool& shouldStop);

//==============================================================================
class ZoomableDragAndDropContainer::DragImageComponent : public Component
    , public Timer {
public:
    DragImageComponent(ScaledImage const& im,
        var const& desc,
        Component* const sourceComponent,
        MouseInputSource const* draggingSource,
        ZoomableDragAndDropContainer& ddc,
        Point<int> offset,
        bool canZoom)
        : sourceDetails(desc, sourceComponent, Point<int>())
        , image(im)
        , owner(ddc)
        , mouseDragSource(draggingSource->getComponentUnderMouse())
        , imageOffset(transformOffsetCoordinates(sourceComponent, offset))
        , originalInputSourceIndex(draggingSource->getIndex())
        , originalInputSourceType(draggingSource->getType())
        , isZoomable(canZoom)
    {
        if (dynamic_cast<ObjectDragAndDrop*>(sourceComponent))
            isObjectItem = true;

        zoomImageComponent.setImage(im.getImage());
        addAndMakeVisible(&zoomImageComponent);

        updateSize();

        if (mouseDragSource == nullptr)
            mouseDragSource = sourceComponent;

        mouseDragSource->addMouseListener(this, false);

        startTimer(200);

        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(true);
        setAlwaysOnTop(true);

        updateScale(0.0f, false);
    }

    ~DragImageComponent() override
    {
        owner.dragImageComponents.remove(owner.dragImageComponents.indexOf(this), false);

        if (mouseDragSource != nullptr) {
            mouseDragSource->removeMouseListener(this);

            if (auto* current = getCurrentlyOver())
                if (current->isInterestedInDragSource(sourceDetails))
                    current->itemDragExit(sourceDetails);
        }

        owner.dragOperationEnded(sourceDetails);
    }

    void mouseUp(MouseEvent const& e) override
    {
        previousTarget = nullptr;

        if (e.originalComponent != this && isOriginalInputSource(e.source)) {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener(this);

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            auto details = sourceDetails;
            DragAndDropTarget* finalTarget = nullptr;

            auto wasVisible = isVisible();
            setVisible(false);
            Component* unused;
            finalTarget = findTarget(currentScreenPos, details.localPosition, unused);

            if (wasVisible) // fade the component and remove it - it'll be deleted later by the timer callback
                dismissWithAnimation(finalTarget == nullptr);

            if (auto* parent = getParentComponent())
                parent->removeChildComponent(this);

            if (finalTarget != nullptr) {
                currentlyOverComp = nullptr;
                finalTarget->itemDropped(details);
            }
            else if(auto* tab = dynamic_cast<TabBarButtonComponent*>(details.sourceComponent.get())){
                if(ProjectInfo::isStandalone) owner.createNewWindow(tab);
            }
            // careful - this object could now be deleted..
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.originalComponent != this && isOriginalInputSource(e.source)) {
            if (rateReducer.tooFast())
                return;

            beginDragAutoRepeat(16);
            currentScreenPos = e.getScreenPosition();
            updateLocation(true, currentScreenPos);
            Component* target = nullptr;
            auto* newTarget = findTarget(currentScreenPos, sourceDetails.localPosition, target);

            if (isZoomable) {
                if (target == nullptr) {
                    updateScale(1.0f, true);
                    previousTarget = nullptr;
                    return;
                }
            }

            if (target == previousTarget) {
                return;
            } else {
                previousTarget = target;
            }

            if (isZoomable) {
                auto* split = dynamic_cast<ResizableTabbedComponent*>(target);
                if (newTarget) {
                    if (split && split->getTabComponent() && split->getTabComponent()->getCurrentCanvas()) {
                        auto zoomScale = ::getValue<float>(split->getTabComponent()->getCurrentCanvas()->zoomScale);
                        updateScale(zoomScale, true);
                        return;
                    }
                }
                if (auto splitView = owner.getSplitView()) {
                    // don't reset the scale of the dragged image if we are inside of the splitview area
                    // there are some objects (splitview resizer, and edges of canvas objects) that
                    // register as a target that's null.
                    // this is a fix for that
                    if (splitView->getScreenBounds().contains(currentScreenPos.toInt())) {
                        return;
                    }
                }
            }
            if (dynamic_cast<ButtonBar*>(target)) {
                updateScale(0.0f, true);
                return;
            }
            updateScale(1.0f, true);
        }
    }

    void updateLocation(bool const canDoExternalDrag, Point<int> screenPos)
    {
        auto details = sourceDetails;

        setNewScreenPos(screenPos);

        Component* newTargetComp;
        auto* newTarget = findTarget(screenPos.toInt(), details.localPosition, newTargetComp);

        setVisible(newTarget == nullptr || newTarget->shouldDrawDragImageWhenOver());

        maintainKeyboardFocusWhenPossible();

        if (newTargetComp != currentlyOverComp) {
            if (auto* lastTarget = getCurrentlyOver())
                if (details.sourceComponent != nullptr && lastTarget->isInterestedInDragSource(details))
                    lastTarget->itemDragExit(details);

            currentlyOverComp = newTargetComp;

            if (newTarget != nullptr
                && newTarget->isInterestedInDragSource(details))
                newTarget->itemDragEnter(details);
        }

        sendDragMove(details);

        if (canDoExternalDrag) {
            auto now = Time::getCurrentTime();

            if (getCurrentlyOver() != nullptr)
                lastTimeOverTarget = now;
            else if (now > lastTimeOverTarget + RelativeTime::milliseconds(700))
                checkForExternalDrag(details, screenPos.toInt());
        }

        forceMouseCursorUpdate();
    }

    void updateScale(float newScale, bool withAnimation)
    {
        if (!approximatelyEqual<float>(newScale, previousScale))
            return;

        previousScale = newScale;

        auto newWidth = image.getScaledBounds().getWidth() * newScale;
        auto newHeight = image.getScaledBounds().getHeight() * newScale;
        auto zoomedImageBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);

        auto& animator = Desktop::getInstance().getAnimator();

        auto finalAlpha = newScale <= 0.0f ? 0.0f : 1.0f;

        if (withAnimation)
            animator.animateComponent(&zoomImageComponent, zoomedImageBounds, finalAlpha, 150, false, 3.0f, 0.0f);
        else {
            animator.cancelAnimation(&zoomImageComponent, true);
            zoomImageComponent.setBounds(zoomedImageBounds);
            zoomImageComponent.setAlpha(finalAlpha);
        }
    }

    void updateImage(ScaledImage const& newImage)
    {
        image = newImage;
        updateSize();
        repaint();
    }

    void timerCallback() override
    {
        forceMouseCursorUpdate();

        if (sourceDetails.sourceComponent == nullptr) {
            deleteSelf();
        } else {
            for (auto& s : Desktop::getInstance().getMouseSources()) {
                if (isOriginalInputSource(s) && !s.isDragging()) {
                    if (mouseDragSource != nullptr)
                        mouseDragSource->removeMouseListener(this);

                    deleteSelf();
                    break;
                }
            }
        }
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress::escapeKey) {
            auto const wasVisible = isVisible();
            setVisible(false);

            if (wasVisible)
                dismissWithAnimation(true);

            deleteSelf();
            return true;
        }

        return false;
    }

    bool canModalEventBeSentToComponent(Component const* targetComponent) override
    {
        return targetComponent == mouseDragSource;
    }

    // (overridden to avoid beeps when dragging)
    void inputAttemptWhenModal() override { }

    DragAndDropTarget::SourceDetails sourceDetails;

    SmoothedValue<float> smoothedScale = 1.0f;

private:
    ScaledImage image;

    bool isZoomable = false;

    Component* previousTarget = nullptr;
    float previousScale = 1.0f;

    ImageComponent zoomImageComponent;

    Point<int> oldScreenPos = { 0, 0 };

    ZoomableDragAndDropContainer& owner;
    WeakReference<Component> mouseDragSource, currentlyOverComp;
    Point<int> const imageOffset;
    Point<int> currentScreenPos;
    RateReducer rateReducer = RateReducer(60);
    bool hasCheckedForExternalDrag = false;
    Time lastTimeOverTarget;
    int originalInputSourceIndex;
    MouseInputSource::InputSourceType originalInputSourceType;
    bool canHaveKeyboardFocus = false;

    bool isObjectItem = false;

    void maintainKeyboardFocusWhenPossible()
    {
        auto const newCanHaveKeyboardFocus = isVisible();

        if (std::exchange(canHaveKeyboardFocus, newCanHaveKeyboardFocus) != newCanHaveKeyboardFocus)
            if (canHaveKeyboardFocus)
                grabKeyboardFocus();
    }

    void updateSize()
    {
        auto const bounds = image.getScaledBounds().toNearestIntEdges() * (isZoomable ? 3 : 1); // 3x is the largest scale of canvas
        setSize(bounds.getWidth(), bounds.getHeight());
        updateImageBounds();
    }

    void updateImageBounds()
    {
        auto newWidth = image.getScaledBounds().getWidth();
        auto newHeight = image.getScaledBounds().getHeight();
        auto zoomedImageBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
        zoomImageComponent.setBounds(zoomedImageBounds);
    }

    void forceMouseCursorUpdate()
    {
        Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    }

    DragAndDropTarget* getCurrentlyOver() const noexcept
    {
        return dynamic_cast<DragAndDropTarget*>(currentlyOverComp.get());
    }

    static Component* findDesktopComponentBelow(Point<int> screenPos)
    {
        auto& desktop = Desktop::getInstance();

        for (auto i = desktop.getNumComponents(); --i >= 0;) {
            auto* desktopComponent = desktop.getComponent(i);
            auto dPoint = desktopComponent->getLocalPoint(nullptr, screenPos);

            if (auto* c = desktopComponent->getComponentAt(dPoint)) {
                auto cPoint = c->getLocalPoint(desktopComponent, dPoint);

                if (c->hitTest(cPoint.getX(), cPoint.getY()))
                    return c;
            }
        }

        return nullptr;
    }

    Point<int> transformOffsetCoordinates(Component const* const sourceComponent, Point<int> offsetInSource) const
    {
        return getLocalPoint(sourceComponent, offsetInSource) - getLocalPoint(sourceComponent, Point<int>());
    }

    DragAndDropTarget* findTarget(Point<int> screenPos, Point<int>& relativePos,
        Component*& resultComponent)
    {
        // if the source DnD is from the Add Object Menu, deal with it differently
        if (isObjectItem) {
            auto* nextTarget = owner.findNextDragAndDropTarget(screenPos);

            if (auto* component = dynamic_cast<Component*>(nextTarget)) {
                relativePos = component->getLocalPoint(nullptr, screenPos);
                resultComponent = component; // oof
                return nextTarget;
            }
        } else {
            auto* hit = getParentComponent();

            if (hit == nullptr)
                hit = findDesktopComponentBelow(screenPos);
            else
                hit = hit->getComponentAt(hit->getLocalPoint(nullptr, screenPos));

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            auto details = sourceDetails;

            while (hit != nullptr) {
                if (auto* ddt = dynamic_cast<DragAndDropTarget*>(hit)) {
                    if (ddt->isInterestedInDragSource(details)) {
                        relativePos = hit->getLocalPoint(nullptr, screenPos);
                        resultComponent = hit;
                        return ddt;
                    }
                }

                hit = hit->getParentComponent();
            }
        }

        resultComponent = nullptr;
        return nullptr;
    }

    void setNewScreenPos(Point<int> currentPos)
    {
        auto newPos = currentPos - (isZoomable ? Point<int>() : imageOffset);

        if (auto* p = getParentComponent())
            newPos = p->getLocalPoint(nullptr, newPos);

        if (isZoomable)
            setCentrePosition(newPos);
        else
            setTopLeftPosition(newPos);
    }

    void sendDragMove(DragAndDropTarget::SourceDetails& details) const
    {
        if (auto* target = getCurrentlyOver())
            if (target->isInterestedInDragSource(details))
                target->itemDragMove(details);
    }

    void checkForExternalDrag(DragAndDropTarget::SourceDetails& details, Point<int> screenPos)
    {
        if (!hasCheckedForExternalDrag) {
            if (Desktop::getInstance().findComponentAt(screenPos) == nullptr) {
                hasCheckedForExternalDrag = true;

                if (ComponentPeer::getCurrentModifiersRealtime().isAnyMouseButtonDown()) {
                    StringArray files;
                    auto canMoveFiles = false;

                    if (owner.shouldDropFilesWhenDraggedExternally(details, files, canMoveFiles) && !files.isEmpty()) {
                        MessageManager::callAsync([=] { DragAndDropContainer::performExternalDragDropOfFiles(files, canMoveFiles); });
                        deleteSelf();
                        return;
                    }

                    String text;

                    if (owner.shouldDropTextWhenDraggedExternally(details, text) && text.isNotEmpty()) {
                        MessageManager::callAsync([=] { DragAndDropContainer::performExternalDragDropOfText(text); });
                        deleteSelf();
                        return;
                    }
                }
            }
        }
    }

    void deleteSelf()
    {
        delete this;
    }

    void dismissWithAnimation(bool const shouldSnapBack)
    {
        setVisible(true);
        auto& animator = Desktop::getInstance().getAnimator();

        if (shouldSnapBack && sourceDetails.sourceComponent != nullptr) {
            auto target = sourceDetails.sourceComponent->localPointToGlobal(sourceDetails.sourceComponent->getLocalBounds().getCentre());
            auto ourCentre = localPointToGlobal(getLocalBounds().getCentre());

            animator.animateComponent(this,
                getBounds() + (target - ourCentre),
                0.0f, 120,
                true, 1.0, 1.0);
        } else {
            animator.fadeOut(this, 120);
        }
    }

    bool isOriginalInputSource(MouseInputSource const& sourceToCheck)
    {
        return (sourceToCheck.getType() == originalInputSourceType
            && sourceToCheck.getIndex() == originalInputSourceIndex);
    }

    JUCE_DECLARE_NON_COPYABLE(DragImageComponent)
};

//==============================================================================
ZoomableDragAndDropContainer::ZoomableDragAndDropContainer() = default;

ZoomableDragAndDropContainer::~ZoomableDragAndDropContainer() = default;

void ZoomableDragAndDropContainer::startDragging(var const& sourceDescription,
    Component* sourceComponent,
    ScaledImage const& dragImage,
    bool allowDraggingToExternalWindows,
    Point<int> const* imageOffsetFromMouse,
    MouseInputSource const* inputSourceCausingDrag,
    bool canZoom)
{
    if (isAlreadyDragging(sourceComponent))
        return;

    if (!ProjectInfo::canUseSemiTransparentWindows()) {
        // If window transparency isn't supported, we should add it to the source component instead of to desktop
        // This can be accomplished by disabling cross-window dragging
        allowDraggingToExternalWindows = false;
    }

    auto* draggingSource = getMouseInputSourceForDrag(sourceComponent, inputSourceCausingDrag);

    if (draggingSource == nullptr || !draggingSource->isDragging()) {
        jassertfalse; // You must call startDragging() from within a mouseDown or mouseDrag callback!
        return;
    }

    auto const lastMouseDown = draggingSource->getLastMouseDownPosition();

    struct ImageAndOffset {
        ScaledImage image;
        Point<double> offset;
    };

    auto const imageToUse = [&]() -> ImageAndOffset {
        if (dragImage.getImage().isValid())
            return { dragImage, imageOffsetFromMouse != nullptr ? dragImage.getScaledBounds().getConstrainedPoint(-imageOffsetFromMouse->toDouble()) : dragImage.getScaledBounds().getCentre() };

        const auto scaleFactor = 2.0;
        auto image = sourceComponent->createComponentSnapshot(sourceComponent->getLocalBounds(), true, (float)scaleFactor)
                         .convertedToFormat(Image::ARGB);
        image.multiplyAllAlphas(0.6f);

        const auto relPos = sourceComponent->getLocalPoint(nullptr, lastMouseDown).toDouble();
        const auto clipped = (image.getBounds().toDouble() / scaleFactor).getConstrainedPoint(relPos);

        Image fade(Image::SingleChannel, image.getWidth(), image.getHeight(), true);
        Graphics fadeContext(fade);

        ColourGradient gradient;
        gradient.isRadial = true;
        gradient.point1 = clipped.toFloat() * scaleFactor;
        gradient.point2 = gradient.point1 + Point<float>(0.0f, scaleFactor * 400.0f);
        gradient.addColour(0.0, Colours::white);
        gradient.addColour(0.375, Colours::white);
        gradient.addColour(1.0, Colours::transparentWhite);

        fadeContext.setGradientFill(gradient);
        fadeContext.fillAll();

        Image composite(Image::ARGB, image.getWidth(), image.getHeight(), true);
        Graphics compositeContext(composite);

        compositeContext.reduceClipRegion(fade, {});
        compositeContext.drawImageAt(image, 0, 0);

        return { ScaledImage(composite, scaleFactor), clipped };
    }();

    auto* dragImageComponent = dragImageComponents.add(new DragImageComponent(imageToUse.image, sourceDescription, sourceComponent,
        draggingSource, *this, imageToUse.offset.roundToInt(), canZoom));

    if (allowDraggingToExternalWindows) {
        if (!Desktop::canUseSemiTransparentWindows())
            dragImageComponent->setOpaque(true);

        dragImageComponent->addToDesktop(ComponentPeer::windowIgnoresMouseClicks
            | ComponentPeer::windowIsTemporary);
    } else {
        if (auto* thisComp = dynamic_cast<Component*>(this)) {
            thisComp->addChildComponent(dragImageComponent);
        } else {
            jassertfalse; // Your ZoomableDragAndDropContainer needs to be a Component!
            return;
        }
    }

    dragImageComponent->sourceDetails.localPosition = sourceComponent->getLocalPoint(nullptr, lastMouseDown).toInt();
    dragImageComponent->updateLocation(false, lastMouseDown.toInt());

    // plugdata fix for bug in JUCE, Linux also exhibits the same issue as Windows
#if JUCE_WINDOWS || JUCE_LINUX
    // Under heavy load, the layered window's paint callback can often be lost by the OS,
    // so forcing a repaint at least once makes sure that the window becomes visible..
    if (auto* peer = dragImageComponent->getPeer())
        peer->performAnyPendingRepaintsNow();
#endif

    dragOperationStarted(dragImageComponent->sourceDetails);
}

bool ZoomableDragAndDropContainer::isDragAndDropActive() const
{
    return dragImageComponents.size() > 0;
}

int ZoomableDragAndDropContainer::getNumCurrentDrags() const
{
    return dragImageComponents.size();
}

var ZoomableDragAndDropContainer::getCurrentDragDescription() const
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the getDragDescriptionForIndex() method instead!
    jassert(dragImageComponents.size() < 2);

    return dragImageComponents.size() != 0 ? dragImageComponents[0]->sourceDetails.description
                                           : var();
}

var ZoomableDragAndDropContainer::getDragDescriptionForIndex(int index) const
{
    if (!isPositiveAndBelow(index, dragImageComponents.size()))
        return {};

    return dragImageComponents.getUnchecked(index)->sourceDetails.description;
}

void ZoomableDragAndDropContainer::setCurrentDragImage(ScaledImage const& newImage)
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the setDragImageForIndex() method instead!
    jassert(dragImageComponents.size() < 2);

    dragImageComponents[0]->updateImage(newImage);
}

void ZoomableDragAndDropContainer::setDragImageForIndex(int index, ScaledImage const& newImage)
{
    if (isPositiveAndBelow(index, dragImageComponents.size()))
        dragImageComponents.getUnchecked(index)->updateImage(newImage);
}

ZoomableDragAndDropContainer* ZoomableDragAndDropContainer::findParentDragContainerFor(Component* c)
{
    return c != nullptr ? c->findParentComponentOfClass<ZoomableDragAndDropContainer>() : nullptr;
}

bool ZoomableDragAndDropContainer::shouldDropFilesWhenDraggedExternally(DragAndDropTarget::SourceDetails const&, StringArray&, bool&)
{
    return false;
}

bool ZoomableDragAndDropContainer::shouldDropTextWhenDraggedExternally(DragAndDropTarget::SourceDetails const&, String&)
{
    return false;
}

void ZoomableDragAndDropContainer::dragOperationStarted(DragAndDropTarget::SourceDetails const&) { }

void ZoomableDragAndDropContainer::dragOperationEnded(DragAndDropTarget::SourceDetails const& sourceComponent)
{
    // dragOperationEnded only runs when the DnD is unsuccessful
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(sourceComponent.sourceComponent.get())) {
        tab->setFocusForTabSplit();
    }
}

MouseInputSource const* ZoomableDragAndDropContainer::getMouseInputSourceForDrag(Component* sourceComponent,
    MouseInputSource const* inputSourceCausingDrag)
{
    if (inputSourceCausingDrag == nullptr) {
        auto minDistance = std::numeric_limits<float>::max();
        auto& desktop = Desktop::getInstance();

        auto centrePoint = sourceComponent ? sourceComponent->getScreenBounds().getCentre().toFloat() : Point<float>();
        auto numDragging = desktop.getNumDraggingMouseSources();

        for (auto i = 0; i < numDragging; ++i) {
            if (auto* ms = desktop.getDraggingMouseSource(i)) {
                auto distance = ms->getScreenPosition().getDistanceSquaredFrom(centrePoint);

                if (distance < minDistance) {
                    minDistance = distance;
                    inputSourceCausingDrag = ms;
                }
            }
        }
    }

    // You must call startDragging() from within a mouseDown or mouseDrag callback!
    jassert(inputSourceCausingDrag != nullptr && inputSourceCausingDrag->isDragging());

    return inputSourceCausingDrag;
}

DragAndDropTarget* ZoomableDragAndDropContainer::findNextDragAndDropTarget(Point<int> screenPos)
{
    return nullptr;
}

bool ZoomableDragAndDropContainer::isAlreadyDragging(Component* component) const noexcept
{
    for (auto* dragImageComp : dragImageComponents) {
        if (dragImageComp->sourceDetails.sourceComponent == component)
            return true;
    }

    return false;
}
