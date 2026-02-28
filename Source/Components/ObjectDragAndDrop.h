#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/OfflineObjectRenderer.h"
#include "PluginEditor.h"
#include "Canvas.h"

class ObjectDragAndDrop : public Component {
public:
    explicit ObjectDragAndDrop(PluginEditor* e)
        : editor(e)
    {
    }

    virtual String getObjectString() = 0;

    virtual String getPatchStringName() { return String(); }

    virtual void dismiss(bool withAnimation) { }

    void lookAndFeelChanged() override
    {
        resetDragAndDropImage();
    }

    MouseCursor getMouseCursor() override
    {
        if (editor && editor->isDragAndDropActive())
            return MouseCursor::DraggingHandCursor;

        return MouseCursor::PointingHandCursor;
    }

    void resetDragAndDropImage()
    {
        dragImage.image = Image();
        errorImage.image = Image();
    }

    void setIsReordering(bool const isReordering)
    {
        reordering = isReordering;
    }

    void mouseDrag(MouseEvent const& e) override
    {
#if JUCE_IOS
        OSUtils::ScrollTracker::setAllowOneFingerScroll(false);
#endif
        if (reordering || e.getDistanceFromDragStart() < 5)
            return;

        if (!editor || editor->isDragAndDropActive())
            return;

        constexpr auto scale = 3.0f;
        if (dragImage.image.isNull() || errorImage.image.isNull()) {
            dragImage = OfflineObjectRenderer::patchToMaskedImage(getObjectString(), scale);
            errorImage = OfflineObjectRenderer::patchToMaskedImage(getObjectString(), scale, true);
        }

        dismiss(true);

        VarArray palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(getObjectString()));
        palettePatchWithOffset.add(var(getPatchStringName()));
        editor->startDragging(palettePatchWithOffset, this, ScaledImage(dragImage.image, scale), ScaledImage(errorImage.image, scale), nullptr, nullptr, true);
    }

private:
    bool reordering = false;
    PluginEditor* editor;
    ImageWithOffset dragImage;
    ImageWithOffset errorImage;
    friend class ObjectClickAndDrop;
};

class ObjectClickAndDrop final : public Component
    , public Timer {
    String objectString;
    String objectName;
    PluginEditor* editor;
    Image dragImage;
    Image dragInvalidImage;
    float scale = 0.5f;
    float animatedScale = 0.0f;
    ImageComponent imageComponent;

    static inline std::unique_ptr<ObjectClickAndDrop> instance = nullptr;

    bool dropState = false;

    Rectangle<int> animationStartBounds, animationEndBounds;
    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder {}
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
    explicit ObjectClickAndDrop(ObjectDragAndDrop* target)
        : editor(target->editor)
    {
        setWantsKeyboardFocus(true);

        objectString = target->getObjectString();
        objectName = target->getPatchStringName();

        addToDesktop(ComponentPeer::windowIsTemporary, OSUtils::getDesktopParentPeer(editor));

        setAlwaysOnTop(true);

        // FIXME: we should only ask a new mask image when the theme has changed so it's the correct colour
        dragImage = OfflineObjectRenderer::patchToMaskedImage(target->getObjectString(), 3.0f).image;
        dragInvalidImage = OfflineObjectRenderer::patchToMaskedImage(target->getObjectString(), 3.0f, true).image;

        // we set the size of this component / window 3x larger to match the max zoom of canavs (300%)
        setSize(dragImage.getWidth(), dragImage.getHeight());

        addAndMakeVisible(imageComponent);
        imageComponent.setInterceptsMouseClicks(false, false);

        imageComponent.setImage(dragImage);
        auto const imageComponentBounds = getLocalBounds().withSizeKeepingCentre(0, 0);
        imageComponent.setBounds(imageComponentBounds);
        imageComponent.setAlpha(0.0f);

        auto const screenPos = Desktop::getMousePosition();
        setCentrePosition(screenPos);
        startTimerHz(60);
        setVisible(true);

        setOpaque(false);
        updater.addAnimator(animator);
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

    static void attachToMouse(ObjectDragAndDrop* parent)
    {
        instance = std::make_unique<ObjectClickAndDrop>(parent);
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
            animator.start();
        }

        setCentrePosition(screenPos);
    }

    void mouseUp(MouseEvent const& e) override
    {
        // This is nicer, but also makes sure that getComponentAt doesn't return this object
        setVisible(false);
        // We don't need to check getSplitAtScreenPosition() here because we have set this component to invisible!

        if (auto* underMouse = editor->getComponentAt(editor->getLocalPoint(nullptr, e.getScreenPosition()))) {
            auto const width = dragImage.getWidth() / 3.0f;
            auto const height = dragImage.getHeight() / 3.0f;

            if (auto* cnv = dynamic_cast<Canvas*>(underMouse)) {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height, objectName);
            } else if (auto* cnv = underMouse->findParentComponentOfClass<Canvas>()) {
                cnv->dragAndDropPaste(objectString, e.getEventRelativeTo(cnv).getPosition() - cnv->canvasOrigin, width, height, objectName);
            }
        }

        instance.reset(nullptr);
    }
};
