#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
#include "../PluginEditor.h"
#include "Canvas.h"

class ObjectDragAndDrop : public Component {
public:
    ObjectDragAndDrop() { }

    virtual String getObjectString() = 0;

    virtual void dismiss(bool withAnimation) { }

    void lookAndFeelChanged() override
    {
        resetDragAndDropImage();
    }

    MouseCursor getMouseCursor() override
    {
        if ((dragContainer != nullptr) && dragContainer->isDragAndDropActive())
            return MouseCursor::DraggingHandCursor;

        return MouseCursor::PointingHandCursor;
    }

    void resetDragAndDropImage()
    {
        dragImage.image = Image();
        errorImage.image = Image();
    }

    void setIsReordering(bool isReordering)
    {
        reordering = isReordering;
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (reordering || e.getDistanceFromDragStart() < 5)
            return;

        dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        if (!dragContainer || dragContainer->isDragAndDropActive())
            return;

        auto scale = 3.0f;
        if (dragImage.image.isNull() || errorImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToMaskedImage(getObjectString(), scale);
            errorImage = offlineObjectRenderer->patchToMaskedImage(getObjectString(), scale, true);
        }

        dismiss(true);

        Array<var> palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(getObjectString()));
        dragContainer->startDragging(palettePatchWithOffset, this, ScaledImage(dragImage.image, scale), ScaledImage(errorImage.image, scale), true, nullptr, nullptr, true);
    }

private:
    bool reordering = false;
    ZoomableDragAndDropContainer* dragContainer = nullptr;
    ImageWithOffset dragImage;
    ImageWithOffset errorImage;
};

class ObjectClickAndDrop : public Component
    , public Timer {
    String objectString;
    PluginEditor* editor;
    Image dragImage;
    Image dragInvalidImage;
    float scale = 0.5f;
    float animatedScale = 0.0f;
    ImageComponent imageComponent;

    static inline std::unique_ptr<ObjectClickAndDrop> instance = nullptr;
    bool isOnEditor = false;

    bool dropState = false;

    ComponentAnimator animator;
    Canvas* canvas = nullptr;

public:
    ObjectClickAndDrop(ObjectDragAndDrop* target)
    {
        objectString = target->getObjectString();
        editor = target->findParentComponentOfClass<PluginEditor>();

        if (ProjectInfo::canUseSemiTransparentWindows()) {
            addToDesktop(ComponentPeer::windowIsTemporary);
        } else {
            isOnEditor = true;
            editor->addChildComponent(this);
        }

        setAlwaysOnTop(true);

        auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(target);
        // FIXME: we should only ask a new mask image when the theme has changed so it's the correct colour
        dragImage = offlineObjectRenderer->patchToMaskedImage(target->getObjectString(), 3.0f).image;
        dragInvalidImage = offlineObjectRenderer->patchToMaskedImage(target->getObjectString(), 3.0f, true).image;

        // we set the size of this component / window 3x larger to match the max zoom of canavs (300%)
        setSize(dragImage.getWidth(), dragImage.getHeight());

        addAndMakeVisible(imageComponent);
        imageComponent.setInterceptsMouseClicks(false, false);

        imageComponent.setImage(dragImage);
        auto imageComponentBounds = getLocalBounds().withSizeKeepingCentre(0, 0);
        imageComponent.setBounds(imageComponentBounds);
        imageComponent.setAlpha(0.0f);

        auto screenPos = Desktop::getMousePosition();
        setCentrePosition(isOnEditor ? getLocalPoint(nullptr, screenPos) : screenPos);
        startTimerHz(60);
        setVisible(true);

        setOpaque(false);
    }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::StandardCursorType::DraggingHandCursor;
    }

    static void attachToMouse(ObjectDragAndDrop* parent)
    {
        instance = std::make_unique<ObjectClickAndDrop>(parent);
    }

    void timerCallback() override
    {
        auto screenPos = Desktop::getMousePosition();
        auto mousePosition = Point<int>();
        Component* underMouse;

        if (isOnEditor) {
            mousePosition = editor->getLocalPoint(nullptr, screenPos);
            underMouse = editor->getComponentAt(mousePosition);
        } else {
            mousePosition = screenPos;
            underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, screenPos));
        }

        Canvas* foundCanvas = nullptr;

        if (!underMouse) {
            scale = 1.0f;
        } else if (auto* cnv = dynamic_cast<Canvas*>(underMouse)) {
            foundCanvas = cnv;
            scale = getValue<float>(cnv->zoomScale);
        } else if (auto* cnv = underMouse->findParentComponentOfClass<Canvas>()) {
            foundCanvas = cnv;
            scale = getValue<float>(cnv->zoomScale);
        } else if (auto* split = editor->splitView.getSplitAtScreenPosition(screenPos)) {
            // if we get here, this object is on the editor (not a window) - so we have to manually find the canvas
            if (auto* tabComponent = split->getTabComponent()) {
                foundCanvas = tabComponent->getCurrentCanvas();
                scale = getValue<float>(foundCanvas->zoomScale);
            } else {
                scale = 1.0f;
            }
        } else {
            scale = 1.0f;
        }

        if (foundCanvas && (foundCanvas != canvas)) {
            canvas = foundCanvas;
            for (auto* split : editor->splitView.splits) {
                if (canvas == split->getTabComponent()->getCurrentCanvas()) {
                    editor->splitView.setFocus(split);
                    break;
                }
            }
        }

        // swap the image to show if the current drop position will result in adding a new object
        auto newDropState = foundCanvas != nullptr;
        if (dropState != newDropState) {
            dropState = newDropState;
            imageComponent.setImage(dropState ? dragImage : dragInvalidImage);
        }

        if (!approximatelyEqual<float>(animatedScale, scale)) {
            animatedScale = scale;
            auto newWidth = dragImage.getWidth() / 3.0f * animatedScale;
            auto newHeight = dragImage.getHeight() / 3.0f * animatedScale;
            auto animatedBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
            animator.animateComponent(&imageComponent, animatedBounds, 1.0f, 150, false, 3.0f, 0.0f);
        }

        setCentrePosition(mousePosition);
    }

    void mouseDown(MouseEvent const& e) override
    {
        // This is nicer, but also makes sure that getComponentAt doesn't return this object
        setVisible(false);
        // We don't need to check getSplitAtScreenPosition() here because we have set this component to invisible!
        auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, e.getScreenPosition()));

        if (underMouse) {
            auto width = dragImage.getWidth() / 3.0f;
            auto height = dragImage.getHeight() / 3.0f;

            if (auto* cnv = dynamic_cast<Canvas*>(underMouse)) {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height);
            } else if (auto* cnv = underMouse->findParentComponentOfClass<Canvas>()) {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height);
            }
        }

        instance.reset(nullptr);
    }
};
