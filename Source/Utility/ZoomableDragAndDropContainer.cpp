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
#include "RateReducer.h"

#include "Constants.h"
#include "LookAndFeel.h"

// objects are only drag and dropped onto a canvas, so we dynamic cast straight away to see if the dragged object is from an object
#include "Components/ObjectDragAndDrop.h"

//==============================================================================
class ZoomableDragAndDropContainer::DragImageComponent final : public Component
    , public Timer {
public:
    DragImageComponent(ScaledImage const& im, ScaledImage const& invalidIm,
        var const& desc,
        Component* const sourceComponent,
        MouseInputSource const* draggingSource,
        ZoomableDragAndDropContainer& ddc,
        Point<int> const offset,
        bool const canZoom)
        : sourceDetails(desc, sourceComponent, Point<int>())
        , image(im)
        , invalidImage(invalidIm)
        , isZoomable(canZoom)
        , owner(ddc)
        , mouseDragSource(draggingSource->getComponentUnderMouse())
        , imageOffset(transformOffsetCoordinates(sourceComponent, offset))
        , originalInputSourceIndex(draggingSource->getIndex())
        , originalInputSourceType(draggingSource->getType())
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

        updater.addAnimator(animator);
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
        if (e.originalComponent != this && isOriginalInputSource(e.source)) {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener(this);

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            auto details = sourceDetails;
            auto const wasVisible = isVisible();
            setVisible(false);
            Component* unused;
            DragAndDropTarget* finalTarget = findTarget(currentScreenPos, details.localPosition, unused);

            if (wasVisible) // fade the component and remove it - it'll be deleted later by the timer callback
                dismissWithAnimation(finalTarget == nullptr);

            if (auto* parent = getParentComponent())
                parent->removeChildComponent(this);

            if (finalTarget != nullptr) {
                currentlyOverComp = nullptr;
                finalTarget->itemDropped(details);
            } else {
                owner.getTabComponent().createNewWindowFromTab(details.sourceComponent.get());
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
            auto const* newTarget = findTarget(currentScreenPos, sourceDetails.localPosition, target);

            auto const wasInvalid = static_cast<bool>(zoomImageComponent.getProperties()["invalid"]);
            if ((wasInvalid && target) || (!wasInvalid && !target)) {
                zoomImageComponent.getProperties().set("invalid", target == nullptr);
                zoomImageComponent.setImage(target ? image.getImage() : invalidImage.getImage());
            }

            if (isZoomable) {
                if (target == nullptr) {
                    updateScale(1.0f, true);
                    return;
                }
            }

            auto* tabbar = dynamic_cast<TabComponent*>(target);

            if (tabbar && isZoomable) {
                if (newTarget) {
                    if (tabbar->getCurrentCanvas()) {
                        auto const zoomScale = ::getValue<float>(tabbar->getCurrentCanvas()->zoomScale);
                        updateScale(zoomScale, true);
                        return;
                    }
                }

                if (tabbar->getScreenBounds().contains(currentScreenPos.toInt())) {
                    return;
                }
            }

            if (tabbar && e.getEventRelativeTo(tabbar).y < 30) {
                updateScale(0.0f, true);
            } else {
                updateScale(1.0f, true);
            }
        }
    }

    void updateLocation(bool const canDoExternalDrag, Point<int> const screenPos)
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
            auto const now = Time::getCurrentTime();

            if (getCurrentlyOver() != nullptr)
                lastTimeOverTarget = now;
            else if (now > lastTimeOverTarget + RelativeTime::milliseconds(700))
                checkForExternalDrag(details, screenPos.toInt());
        }

        forceMouseCursorUpdate();
    }

    void updateScale(float const newScale, bool const withAnimation)
    {
        if (approximatelyEqual<float>(newScale, previousScale))
            return;

        previousScale = newScale;

        auto const newWidth = image.getScaledBounds().getWidth() * newScale;
        auto const newHeight = image.getScaledBounds().getHeight() * newScale;
        auto const zoomedImageBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
        auto const fadeIn = newScale > 0.0f;

        if (withAnimation)
            animate(&zoomImageComponent, zoomImageComponent.getBounds(), zoomedImageBounds, fadeIn);
        else {
            if (animationTarget == &zoomImageComponent)
                animator.complete();
            zoomImageComponent.setBounds(zoomedImageBounds);
            zoomImageComponent.setAlpha(fadeIn ? 0.0f : 1.0f);
        }
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
    ScaledImage invalidImage;

    bool isZoomable = false;
    float previousScale = 1.0f;

    ImageComponent zoomImageComponent;

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
        auto const newWidth = image.getScaledBounds().getWidth();
        auto const newHeight = image.getScaledBounds().getHeight();
        auto const zoomedImageBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
        zoomImageComponent.setBounds(zoomedImageBounds);
    }

    static void forceMouseCursorUpdate()
    {
        Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    }

    DragAndDropTarget* getCurrentlyOver() const noexcept
    {
        return dynamic_cast<DragAndDropTarget*>(currentlyOverComp.get());
    }

    static Component* findDesktopComponentBelow(Point<int> const screenPos)
    {
        auto const& desktop = Desktop::getInstance();

        for (auto i = desktop.getNumComponents(); --i >= 0;) {
            auto* desktopComponent = desktop.getComponent(i);
            auto const dPoint = desktopComponent->getLocalPoint(nullptr, screenPos);

            if (auto* c = desktopComponent->getComponentAt(dPoint)) {
                auto cPoint = c->getLocalPoint(desktopComponent, dPoint);

                if (c->hitTest(cPoint.getX(), cPoint.getY()))
                    return c;
            }
        }

        return nullptr;
    }

    Point<int> transformOffsetCoordinates(Component const* const sourceComponent, Point<int> const offsetInSource) const
    {
        return getLocalPoint(sourceComponent, offsetInSource) - getLocalPoint(sourceComponent, Point<int>());
    }

    DragAndDropTarget* findTarget(Point<int> const screenPos, Point<int>& relativePos,
        Component*& resultComponent) const
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
            auto const details = sourceDetails;

            while (hit != nullptr) {
                if (auto* ddt = dynamic_cast<PluginEditor*>(hit)) {
                    hit = &ddt->getTabComponent();
                }
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

    void setNewScreenPos(Point<int> const currentPos)
    {
        auto newPos = currentPos - (isZoomable ? Point<int>() : imageOffset);

        if (auto const* p = getParentComponent())
            newPos = p->getLocalPoint(nullptr, newPos);

        if (isZoomable)
            setCentrePosition(newPos);
        else
            setTopLeftPosition(newPos);
    }

    void sendDragMove(DragAndDropTarget::SourceDetails const& details) const
    {
        if (auto* target = getCurrentlyOver())
            if (target->isInterestedInDragSource(details))
                target->itemDragMove(details);
    }

    void checkForExternalDrag(DragAndDropTarget::SourceDetails const& details, Point<int> const screenPos)
    {
        if (!hasCheckedForExternalDrag) {
            if (Desktop::getInstance().findComponentAt(screenPos) == nullptr) {
                hasCheckedForExternalDrag = true;

                if (ComponentPeer::getCurrentModifiersRealtime().isAnyMouseButtonDown()) {
                    StringArray files;
                    auto canMoveFiles = false;

                    if (owner.shouldDropFilesWhenDraggedExternally(details, files, canMoveFiles) && !files.isEmpty()) {
                        MessageManager::callAsync([this, files, canMoveFiles] {
                            DragAndDropContainer::performExternalDragDropOfFiles(files, canMoveFiles);
                            deleteSelf();
                        });

                        return;
                    }

                    String text;

                    if (owner.shouldDropTextWhenDraggedExternally(details, text) && text.isNotEmpty()) {
                        MessageManager::callAsync([this, text] {
                            DragAndDropContainer::performExternalDragDropOfText(text);
                            deleteSelf(); // Delete asynchronously so the stack can unwind
                        });
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
        if (shouldSnapBack && sourceDetails.sourceComponent != nullptr) {
            auto const target = sourceDetails.sourceComponent->localPointToGlobal(sourceDetails.sourceComponent->getLocalBounds().getCentre());
            auto const ourCentre = localPointToGlobal(getLocalBounds().getCentre());
            animate(this, getBounds(), getBounds() + (target - ourCentre), false);
        } else {
            animate(this, getBounds(), getBounds(), false);
        }
    }

    bool isOriginalInputSource(MouseInputSource const& sourceToCheck) const
    {
        return sourceToCheck.getType() == originalInputSourceType
            && sourceToCheck.getIndex() == originalInputSourceIndex;
    }

    void animate(Component* target, Rectangle<int> startBounds, Rectangle<int> endBounds, bool fadeIn)
    {
        target->setVisible(true);
        animator.complete();
        animationTarget = target;
        animationStartBounds = startBounds;
        animationEndBounds = endBounds;
        animationFadeIn = fadeIn;
        animator.start();
    }

    SafePointer<Component> animationTarget;
    bool animationFadeIn;
    Rectangle<int> animationStartBounds, animationEndBounds;
    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder {}
                            .withDurationMs(150)
                            .withEasing(Easings::createEaseInOut())
                            .withValueChangedCallback([this](float v) {
                                if (!animationTarget)
                                    return;
                                auto start = std::make_tuple(animationStartBounds.getX(), animationStartBounds.getY(), animationStartBounds.getWidth(), animationStartBounds.getHeight());
                                auto end = std::make_tuple(animationEndBounds.getX(), animationEndBounds.getY(), animationEndBounds.getWidth(), animationEndBounds.getHeight());
                                auto [x, y, w, h] = makeAnimationLimits(start, end).lerp(v);
                                animationTarget->setBounds(x, y, w, h);
                                if (animationFadeIn && getAlpha() < 1.0f)
                                    animationTarget->setAlpha(v);
                                else if (!animationFadeIn && getAlpha() > 0.0f)
                                    setAlpha(1.0f - v);
                            })
                            .build();

    JUCE_DECLARE_NON_COPYABLE(DragImageComponent)
};

//==============================================================================
ZoomableDragAndDropContainer::ZoomableDragAndDropContainer() = default;

ZoomableDragAndDropContainer::~ZoomableDragAndDropContainer() = default;

void ZoomableDragAndDropContainer::startDragging(var const& sourceDescription,
    Component* sourceComponent,
    ScaledImage const& dragImage,
    ScaledImage const& invalidImage,
    Point<int> const* imageOffsetFromMouse,
    MouseInputSource const* inputSourceCausingDrag,
    bool const canZoom)
{
    if (isAlreadyDragging(sourceComponent))
        return;

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

    auto const imageToUse = [&](ScaledImage const& inputImage) -> ImageAndOffset {
        if (inputImage.getImage().isValid())
            return { inputImage, imageOffsetFromMouse != nullptr ? dragImage.getScaledBounds().getConstrainedPoint(-imageOffsetFromMouse->toDouble()) : dragImage.getScaledBounds().getCentre() };

        constexpr auto scaleFactor = 2.0;
        auto image = sourceComponent->createComponentSnapshot(sourceComponent->getLocalBounds(), true, scaleFactor)
                         .convertedToFormat(Image::ARGB);
        image.multiplyAllAlphas(0.6f);

        auto const relPos = sourceComponent->getLocalPoint(nullptr, lastMouseDown).toDouble();
        auto const clipped = (image.getBounds().toDouble() / scaleFactor).getConstrainedPoint(relPos);

        Image const fade(Image::SingleChannel, image.getWidth(), image.getHeight(), true);
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

        Image const composite(Image::ARGB, image.getWidth(), image.getHeight(), true);
        Graphics compositeContext(composite);

        compositeContext.reduceClipRegion(fade, {});
        compositeContext.drawImageAt(image, 0, 0);

        return { ScaledImage(composite, scaleFactor), clipped };
    };

    auto* dragImageComponent = dragImageComponents.add(new DragImageComponent(imageToUse(dragImage).image, imageToUse(invalidImage).image, sourceDescription, sourceComponent,
        draggingSource, *this, imageToUse(dragImage).offset.roundToInt(), canZoom));

    if (!Desktop::canUseSemiTransparentWindows()) {
        dragImageComponent->setOpaque(true);
    }

    dragImageComponent->addToDesktop(ComponentPeer::windowIgnoresMouseClicks | ComponentPeer::windowIsTemporary, OSUtils::getDesktopParentPeer(sourceComponent->getTopLevelComponent()));

    dragImageComponent->sourceDetails.localPosition = sourceComponent->getLocalPoint(nullptr, lastMouseDown).toInt();
    dragImageComponent->updateLocation(false, lastMouseDown.toInt());

    // plugdata fix for bug in JUCE, Linux also exhibits the same issue as Windows
#if JUCE_WINDOWS || JUCE_LINUX
    // Under heavy load, the layered window's paint callback can often be lost by the OS,
    // so forcing a repaint at least once makes sure that the window becomes visible..
    if (auto* peer = dragImageComponent->getPeer())
        peer->performAnyPendingRepaintsNow();
#endif

#if JUCE_IOS
    dragImageComponent->setAlwaysOnTop(true);
#endif

    dragOperationStarted(dragImageComponent->sourceDetails);
    if (auto* topLevel = TopLevelWindow::getActiveTopLevelWindow()) {
        topLevel->repaint();
    }
}

bool ZoomableDragAndDropContainer::isDragAndDropActive() const
{
    return dragImageComponents.size() > 0;
}

ZoomableDragAndDropContainer* ZoomableDragAndDropContainer::findParentDragContainerFor(Component const* c)
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
}

MouseInputSource const* ZoomableDragAndDropContainer::getMouseInputSourceForDrag(Component const* sourceComponent,
    MouseInputSource const* inputSourceCausingDrag)
{
    if (inputSourceCausingDrag == nullptr) {
        auto minDistance = std::numeric_limits<float>::max();
        auto const& desktop = Desktop::getInstance();

        auto const centrePoint = sourceComponent ? sourceComponent->getScreenBounds().getCentre().toFloat() : Point<float>();
        auto const numDragging = desktop.getNumDraggingMouseSources();

        for (auto i = 0; i < numDragging; ++i) {
            if (auto const* ms = desktop.getDraggingMouseSource(i)) {
                auto const distance = ms->getScreenPosition().getDistanceSquaredFrom(centrePoint);

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

bool ZoomableDragAndDropContainer::isAlreadyDragging(Component const* component) const noexcept
{
    for (auto const* dragImageComp : dragImageComponents) {
        if (dragImageComp->sourceDetails.sourceComponent == component)
            return true;
    }

    return false;
}
