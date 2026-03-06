/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"

class BouncingViewportAttachment final : public MouseListener
    , public Timer
    , public SettingsFileListener {
public:
    explicit BouncingViewportAttachment(Viewport* vp)
        : viewport(vp)
    {

        if (SettingsFile::getInstance()->isUsingTouchMode()) {
            viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
        } else {
            viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
        }

        viewport->addMouseListener(this, true);
    }

    ~BouncingViewportAttachment() override = default;

    void settingsChanged(String const& name, var const& value) override
    {
        if (name == "touch_mode") {
            if (static_cast<bool>(value)) {
                viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
            } else {
                viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
            }
        }
    }

    bool doesMouseEventComponentBlockViewportDrag(Component const* eventComp)
    {
        for (auto c = eventComp; c != nullptr && c != viewport; c = c->getParentComponent())
            if (c->getViewportIgnoreDragFlag())
                return true;

        return false;
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!isBounceable || viewport->getScrollOnDragMode() == Viewport::ScrollOnDragMode::never) {
            wasSmooth = false;
            return;
        }

        if (e.source.getIndex() != 0 || !e.mods.isLeftButtonDown() || e.eventTime == lastScrollTime)
            return;

        auto const now = e.eventTime;
        auto const deltaMs = (now - lastScrollTime).inMilliseconds();
        lastScrollTime = now;

        // So far, we only really need vertical scrolling
        if (!doesMouseEventComponentBlockViewportDrag(e.eventComponent) && viewport->isVerticalScrollBarShown()) {
            auto const area = viewport->getViewArea();
            auto const componentBounds = viewport->getViewedComponent()->getBounds();

            float constexpr factor = 0.1f;
            float const dragDistance = e.getEventRelativeTo(viewport).getDistanceFromDragStartY();
            float const deltaY = dragDistance - lastDragDistance;
            // If we scroll too far ahead or back, add the amount to the offset
            if (area.getY() - deltaY < componentBounds.getY()) {
                verticalOverscroll += (deltaY - area.getY()) * factor;
            } else if (area.getBottom() - deltaY > componentBounds.getHeight()) {
                verticalOverscroll += (deltaY - (area.getBottom() - componentBounds.getHeight())) * factor;
            }
            verticalOverscroll = std::clamp(verticalOverscroll, -50.0f, 50.0f);

            if (deltaMs > 0) {
                float const newVelocityY = (e.position.y - lastDragPosition) / (float)deltaMs;
                velocity = velocity * 0.4f + newVelocityY * 0.6f;
            }

            startTimerHz(60);
            lastDragDistance = dragDistance;
            wasSmooth = true;
            lastDragPosition = e.position.y;
            isDecaying = false;
        }

        update();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.source.getIndex() != 0 || !e.mods.isLeftButtonDown())
            return;

        lastDragDistance = 0.0f;

        if (std::abs(velocity) > 0.1f) {
            isDecaying = true;
        } else {
            velocity = {};
        }

        startTimerHz(60);
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        if (!isBounceable)
            return;
        // Protect against receiving the same even twice
        if (e.eventTime == lastScrollTime)
            return;

        lastScrollTime = e.eventTime;

        auto const deltaY = rescaleMouseWheelDistance(wheel.deltaY);

        wasSmooth = wheel.isSmooth;

        int constexpr inertialThreshold = 50;
        bool const isLargeInertialEvent = wheel.isInertial && std::abs(deltaY) > inertialThreshold;
        bool const isSmallIntertialEvent = wheel.isInertial && std::abs(deltaY) <= inertialThreshold;

        isInterialEvent = isLargeInertialEvent;

        // So far, we only really need vertical scrolling
        if (viewport->isVerticalScrollBarShown() && !isSmallIntertialEvent) {
            auto const area = viewport->getViewArea();
            auto const componentBounds = viewport->getViewedComponent()->getBounds();

            float const factor = wheel.isInertial ? 0.02f : 0.1f;
            // If we scroll too far ahead or back, add the amount to the vertical overscroll
            if (area.getY() - deltaY < componentBounds.getY()) {
                verticalOverscroll += (deltaY - area.getY()) * factor;
            } else if (area.getBottom() - deltaY > componentBounds.getHeight()) {
                verticalOverscroll += (deltaY - (area.getBottom() - componentBounds.getHeight())) * factor;
            }

            verticalOverscroll = std::clamp(verticalOverscroll, -50.0f, 50.0f);
            startTimerHz(60);
        }

        update();
    }

    void setBounce(bool const shouldBounce)
    {
        isBounceable = shouldBounce;
    }

private:
    static int rescaleMouseWheelDistance(float distance) noexcept
    {
        if (distance == 0.0f)
            return 0;

        distance *= 224.0f;

        return roundToInt(distance < 0 ? jmin(distance, -1.0f)
                                       : jmax(distance, 1.0f));
    }

    void update()
    {
        auto* holder = viewport->getChildComponent(0);
        if (!holder)
            return;
        holder->setTransform(holder->getTransform().withAbsoluteTranslation(0.0f, verticalOverscroll));
    }

    bool isInsideScrollGesture()
    {
#if JUCE_MAC
        return OSUtils::ScrollTracker::isPerformingGesture();
#else
        if(SettingsFile::getInstance()->isUsingTouchMode()) {
            auto* component = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
            return component && !doesMouseEventComponentBlockViewportDrag(component) && component->isMouseButtonDown();
        }
        else {
            return false;
        }

#endif
    }

    void timerCallback() override
    {
        if (isDecaying && std::abs(velocity) > 0.01f) {
            float constexpr frameMs = 1000.0f / 60.0f;
            float const scrollDelta = -velocity * frameMs;
            auto const current = viewport->getViewPositionY();
            viewport->setViewPosition(viewport->getViewPositionX(), current + (int)scrollDelta);

            velocity *= decayFactor;
            if (std::abs(velocity) < 0.01f) {
                velocity = 0.0f;
                isDecaying = false;
            }
        } else {
            isDecaying = false;
        }

        if ((!wasSmooth || !isInsideScrollGesture()) && !isInterialEvent) {
            verticalOverscroll *= 0.85f;

            if (verticalOverscroll > -1 && verticalOverscroll < 1) {
                verticalOverscroll = 0.0f;
            }
        }

        if(velocity == 0.0f && verticalOverscroll == 0.0f)
        {
            stopTimer();
        }

        update();
    }

    Viewport* viewport;
    Time lastScrollTime;
    Time lastDragTime;

    float verticalOverscroll = 0.0f;
    float lastDragDistance = 0.0f;
    float velocity = 0.0f;
    float lastDragPosition = 0.0f;
    float decayFactor = 0.92f;

    bool isDecaying:1 = false;
    bool isInterialEvent:1 = false;
    bool wasSmooth:1 = false;
    bool isBounceable:1 = true;
};

class BouncingViewport : public Viewport {
public:
    BouncingViewport()
        : bouncer(this)
    {
    }

    void setBounce(bool const shouldBounce)
    {
        bouncer.setBounce(shouldBounce);
    }

private:
    BouncingViewportAttachment bouncer;
};
