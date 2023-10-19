#pragma once

#include <JuceHeader.h>
#include "ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
#include "../PluginEditor.h"
#include "Canvas.h"

class ObjectDragAndDrop : public Component
{
public:
    ObjectDragAndDrop(){};
    
    virtual String getObjectString() = 0;

    virtual void dismiss(bool withAnimation){};

    void lookAndFeelChanged() override
    {
        resetDragAndDropImage();
    }

    void resetDragAndDropImage()
    {
        dragImage.image = Image();
    }

    void setIsReordering(bool isReordering)
    {
        reordering = isReordering;
    }

    void mouseDrag(MouseEvent const& e) override final
    {
        if (reordering || e.getDistanceFromDragStart() < 5)
            return;

        auto* dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        if (!dragContainer || dragContainer->isDragAndDropActive())
            return;

        auto scale = 2.0f;
        if (dragImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToTempImage(getObjectString(), scale);
        }

        dismiss(true);

        Array<var> palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(getObjectString()));
        dragContainer->startDragging(palettePatchWithOffset, this, ScaledImage(dragImage.image, scale), true, nullptr, nullptr, true);
    }

private:
    bool reordering = false;
    ImageWithOffset dragImage;
};

class ObjectClickAndDrop : public Component, public Timer
{
    String objectString;
    PluginEditor* editor;
    Image dragImage;
    float scale = 0.5f;
    float animatedScale = 0.0f;
    ImageComponent imageComponent;
    
    static inline std::unique_ptr<ObjectClickAndDrop> instance = nullptr;
    bool isOnEditor = false;

    ComponentAnimator animator;
    
public:
    ObjectClickAndDrop(ObjectDragAndDrop* target)
    {
        objectString = target->getObjectString();
        editor = target->findParentComponentOfClass<PluginEditor>();

        if(ProjectInfo::canUseSemiTransparentWindows())
        {
            addToDesktop(ComponentPeer::windowIsTemporary);
        }
        else {
            isOnEditor = true;
            editor->addChildComponent(this);
        }

        setAlwaysOnTop(true);

        auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(target);
        dragImage = offlineObjectRenderer->patchToTempImage(target->getObjectString(), 3.0f).image;

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

        if(!underMouse)
        {
            scale = 1.0f;
        }
        else if(auto* cnv = dynamic_cast<Canvas*>(underMouse))
        {
            scale = getValue<float>(cnv->zoomScale);
        }
        else if(auto* cnv = underMouse->findParentComponentOfClass<Canvas>())
        {
            scale = getValue<float>(cnv->zoomScale);
        }
        else if (auto* split = editor->splitView.getSplitAtScreenPosition(screenPos))
        {
            // if we get here, this object is on the editor (not a window) - so we have to manually find the canvas
            scale = getValue<float>(split->getTabComponent()->getCurrentCanvas()->zoomScale);
        }
        else {
            scale = 1.0f;
        }

        if(animatedScale != scale)
        {
            animatedScale = scale;
            auto newWidth = dragImage.getWidth() / 3.0f * animatedScale;
            auto newHeight = dragImage.getHeight() / 3.0f * animatedScale;
            auto animatedBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
            animator.animateComponent(&imageComponent, animatedBounds, 1.0f, 150, false, 3.0f, 0.0f);
        }

        setCentrePosition(mousePosition);
    }

    void mouseDown(const MouseEvent& e) override
    {
        // This is nicer, but also makes sure that getComponentAt doesn't return this object
        setVisible(false);
       // We don't need to check getSplitAtScreenPosition() here because we have set this component to invisible!
        auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, e.getScreenPosition()));
        
        if(underMouse)
        {
            auto width = dragImage.getWidth() / 3.0f;
            auto height = dragImage.getHeight() / 3.0f;

            if(auto* cnv = dynamic_cast<Canvas*>(underMouse))
            {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height);
            }
            else if(auto* cnv = underMouse->findParentComponentOfClass<Canvas>())
            {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height);
            }
        }

        instance.reset(nullptr);
    }
    
};
