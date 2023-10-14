#pragma once

#include <JuceHeader.h>
#include "ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
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
    Component* editor;
    Image dragImage;
    float scale = 0.5f;
    float animatedScale = 0.0f;
    
    static inline std::unique_ptr<ObjectClickAndDrop> instance = nullptr;
    
public:
    ObjectClickAndDrop(ObjectDragAndDrop* target)
    {
        objectString = target->getObjectString();
        editor = target->findParentComponentOfClass<AudioProcessorEditor>();
    
        if(ProjectInfo::canUseSemiTransparentWindows())
        {
            addToDesktop(ComponentPeer::windowIsTemporary);
        }
        else {
            editor->addChildComponent(this);
        }
        
        setVisible(true);
        setAlwaysOnTop(true);
        startTimerHz(60);
        
        
        auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(target);
        dragImage = offlineObjectRenderer->patchToTempImage(target->getObjectString(), 2.0f).image;
        repaint();
        
        // If we don't set the position immediately, we can prevent the mouse event that created this object from triggering the mouseDown function
        MessageManager::callAsync([_this = SafePointer(this)](){
            if(_this) {
                _this->setSize(_this->dragImage.getWidth(), _this->dragImage.getHeight());
                _this->setCentrePosition(Desktop::getMousePosition());
            }
        });
    }
    
    void paint(Graphics& g) override
    {
        g.drawImageTransformed(dragImage, AffineTransform::scale(scale));
    }
    
    static void attachToMouse(ObjectDragAndDrop* parent)
    {
        instance = std::make_unique<ObjectClickAndDrop>(parent);
    }
    
    void timerCallback() override
    {
        auto mousePosition = Desktop::getMousePosition();
        
        auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, mousePosition));
        if(!underMouse)
        {
            scale = 0.5f;
            repaint();
        }
        else if(auto* cnv = dynamic_cast<Canvas*>(underMouse))
        {
            scale = getValue<float>(cnv->zoomScale) / 2.0f;
        }
        else if(auto* cnv = underMouse->findParentComponentOfClass<Canvas>())
        {
            scale = getValue<float>(cnv->zoomScale) / 2.0f;
        }
        else {
            scale = 0.5f;
        }

        if(animatedScale != scale)
        {
            animatedScale = jmap(0.5f, animatedScale, scale);
            setSize(dragImage.getWidth() * animatedScale, dragImage.getHeight() * animatedScale);
            repaint();
        }
        
        if(ProjectInfo::canUseSemiTransparentWindows()) {
            setCentrePosition(mousePosition);
        }
        else {
            setCentrePosition(editor->getLocalPoint(nullptr, mousePosition));
        }
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, e.getScreenPosition()));
        if(underMouse)
        {
            if(auto* cnv = dynamic_cast<Canvas*>(underMouse))
            {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, dragImage.getWidth() / 2, dragImage.getHeight() / 2);
            }
            else if(auto* cnv = underMouse->findParentComponentOfClass<Canvas>())
            {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, dragImage.getWidth() / 2, dragImage.getHeight() / 2);
            }
        }
        
        instance.reset(nullptr);
    }
    
};
