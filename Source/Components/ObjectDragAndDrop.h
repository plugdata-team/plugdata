#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/OfflineObjectRenderer.h"
#include "PluginEditor.h"
#include "Canvas.h"

class ObjectDragAndDrop final : public Component
    , public Timer {

    static inline std::unique_ptr<ObjectDragAndDrop> instance = nullptr;
    PluginEditor* editor;

    String object;
    Image dragImage;
    Image dragInvalidImage;
    float scale = 0.5f;
    float animatedScale = 0.0f;
    ImageComponent imageComponent;

    bool dropState = false;
    bool startedWithDrag = false;

    Rectangle<int> animationStartBounds, animationEndBounds;
    VBlankAnimatorUpdater updater { this };
    Animator zoomAnimator = ValueAnimatorBuilder { }
                            .withDurationMs(150)
                            .withEasing(Easings::createEaseInOut())
                            .withValueChangedCallback([this](float v) {
                                auto start = std::make_tuple(animationStartBounds.getX(), animationStartBounds.getY(), animationStartBounds.getWidth(), animationStartBounds.getHeight());
                                auto end = std::make_tuple(animationEndBounds.getX(), animationEndBounds.getY(), animationEndBounds.getWidth(), animationEndBounds.getHeight());
                                auto [x, y, w, h] = makeAnimationLimits(start, end).lerp(v);
                                imageComponent.setBounds(x, y, w, h);
                                if (imageComponent.getAlpha() < 1.0f)
                                    imageComponent.setAlpha(v);
                            })
                            .build();
    Canvas* canvas = nullptr;

public:
    explicit ObjectDragAndDrop(PluginEditor* editor, String const& objectName)
        : editor(editor), object(objectName)
    {
        startedWithDrag = Desktop::getInstance().getMainMouseSource().isDragging();
        setWantsKeyboardFocus(true);

        addToDesktop(ComponentPeer::windowIsTemporary, OSUtils::getDesktopParentPeer(editor));

        setAlwaysOnTop(true);

        dragImage = OfflineObjectRenderer::patchToMaskedImage(objectName, 3.0f).image;
        dragInvalidImage = OfflineObjectRenderer::patchToMaskedImage(objectName, 3.0f, true).image;

        if(ProjectInfo::canUseSemiTransparentWindows()) {
            // Make it larger so we don't accidentally lose track
            setSize(std::max(80, dragImage.getWidth() * 3), std::max(80, dragImage.getHeight() * 3));
        }
        else {
            setSize(dragImage.getWidth(), dragImage.getHeight());
        }

        addAndMakeVisible(imageComponent);
        imageComponent.setInterceptsMouseClicks(false, false);

        imageComponent.setImage(dragImage);
        auto const imageComponentBounds = getLocalBounds().withSizeKeepingCentre(0, 0);
        imageComponent.setBounds(imageComponentBounds);
        imageComponent.setAlpha(0.0f);

        auto const screenPos = Desktop::getMousePosition();
        setCentrePosition(screenPos);
        startTimerHz(90);
        setVisible(true);

        setOpaque(false);
        updater.addAnimator(zoomAnimator);
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress::escapeKey) {
            setVisible(false);
            instance.reset(nullptr);
            return true;
        }

        return false;
    }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::StandardCursorType::DraggingHandCursor;
    }

    // Calling this on mouseDown/mouseDrag will attach the object in drag mode, it will be pasted on mouseup
    // Calling this on mouseUp will attach the object in click mode, it won't be pasted until the user clicks again
    static void attachToMouse(PluginEditor* parent, String const& object)
    {
        if(instance) return;

        instance = std::make_unique<ObjectDragAndDrop>(parent, object);
        instance->grabKeyboardFocus();
    }

    void timerCallback() override
    {
        auto const screenPos = Desktop::getMousePosition();
        Component* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, screenPos));
        Canvas* foundCanvas = nullptr;

        if (!underMouse) {
            scale = 1.0f;
        } else if (auto* cnv = dynamic_cast<Canvas*>(underMouse)) {
            foundCanvas = cnv;
            scale = getValue<float>(cnv->zoomScale);
        } else if (auto* cnv = underMouse->findParentComponentOfClass<Canvas>()) {
            foundCanvas = cnv;
            scale = getValue<float>(cnv->zoomScale);
        } else if (auto* cnv = editor->getTabComponent().getCanvasAtScreenPosition(screenPos)) {
            foundCanvas = cnv;
            scale = getValue<float>(foundCanvas->zoomScale);
        } else {
            scale = 1.0f;
        }

        if (foundCanvas && foundCanvas != canvas) {
            canvas = foundCanvas;
            editor->getTabComponent().setActiveSplit(canvas);
        }

        // swap the image to show if the current drop position will result in adding a new object
        auto const newDropState = foundCanvas != nullptr;
        if (dropState != newDropState) {
            dropState = newDropState;
            imageComponent.setImage(dropState ? dragImage : dragInvalidImage);
        }

        if (!approximatelyEqual<float>(animatedScale, scale)) {
            animatedScale = scale;
            auto const newWidth = dragImage.getWidth() / 3.0f * animatedScale;
            auto const newHeight = dragImage.getHeight() / 3.0f * animatedScale;
            animationStartBounds = imageComponent.getBounds();
            animationEndBounds = getLocalBounds().withSizeKeepingCentre(newWidth, newHeight);
            zoomAnimator.start();
        }

        setCentrePosition(screenPos);

        auto mms = Desktop::getInstance().getMainMouseSource();
        auto* draggedComponent = mms.getComponentUnderMouse();
        if(draggedComponent)
        {
            draggedComponent->setMouseCursor(MouseCursor::StandardCursorType::DraggingHandCursor);
        }
        if(draggedComponent == this && startedWithDrag && !mms.isDragging())
        {
            paste(mms.getScreenPosition().roundToInt());
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        editor->grabKeyboardFocus();
    }

    void mouseUp(MouseEvent const& e) override
    {
        // This is nicer, but also makes sure that getComponentAt doesn't return this object
        paste(e.getScreenPosition());
    }

    void paste(Point<int> screenPosition)
    {
        setVisible(false);
        if (auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, screenPosition))) {
            auto const width = dragImage.getWidth() / 3.0f;
            auto const height = dragImage.getHeight() / 3.0f;

            if (auto* cnv = dynamic_cast<Canvas*>(underMouse)) {
                cnv->dragAndDropPaste(object, cnv->getLocalPoint(nullptr, screenPosition) - cnv->canvasOrigin, width, height);
            } else if (auto* cnv = underMouse->findParentComponentOfClass<Canvas>()) {
                cnv->dragAndDropPaste(object, cnv->getLocalPoint(nullptr, screenPosition) - cnv->canvasOrigin, width, height);
            }
        }

        instance.reset(nullptr);
    }
};
