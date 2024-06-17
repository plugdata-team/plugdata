/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"

class BouncingViewportAttachment : public MouseListener
    , public Timer {
public:
    explicit BouncingViewportAttachment(Viewport* vp)
        : viewport(vp)
    {
        viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
        viewport->addMouseListener(this, true);
    }

    virtual ~BouncingViewportAttachment() = default;

public:
    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        // Protect against receiving the same even twice
        if (e.eventTime == lastScrollTime)
            return;

        lastScrollTime = e.eventTime;

        auto deltaY = rescaleMouseWheelDistance(wheel.deltaY);

        wasSmooth = wheel.isSmooth;

        int inertialThreshold = 50;
        bool isLargeInertialEvent = wheel.isInertial && std::abs(deltaY) > inertialThreshold;
        bool isSmallIntertialEvent = wheel.isInertial && std::abs(deltaY) <= inertialThreshold;

        wasInterialEvent = isLargeInertialEvent;

        // So far, we only really need vertical scrolling
        if (viewport->isVerticalScrollBarShown() && !isSmallIntertialEvent) {

            auto area = viewport->getViewArea();
            auto componentBounds = viewport->getViewedComponent()->getBounds();

            float factor = wheel.isInertial ? 0.02f : 0.1f;
            // If we scroll too far ahead or back, add the amount to the offset
            if (area.getY() - deltaY < componentBounds.getY()) {
                offset.y += (deltaY - area.getY()) * factor;
            } else if (area.getBottom() - deltaY > componentBounds.getHeight()) {
                offset.y += (deltaY - (area.getBottom() - componentBounds.getHeight())) * factor;
            }

            offset.y = std::clamp(offset.y, -50.0f, 50.0f);

            startTimerHz(60);
        }

        update();
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
        holder->setTransform(holder->getTransform().withAbsoluteTranslation(offset.x, offset.y));
    }

    bool isInsideScrollGesture()
    {
#if JUCE_MAC
        return OSUtils::ScrollTracker::isScrolling();
#else
        return false;
#endif
    }

    void timerCallback() override
    {
        if ((!wasSmooth || !isInsideScrollGesture()) && !wasInterialEvent) {
            offset.x *= 0.85f;
            offset.y *= 0.85f;

            if (offset.x > -1 && offset.x < 1 && offset.y > -1 && offset.y < 1) {
                offset = { 0, 0 };
                stopTimer();
            }
        }

        update();
    }

    bool wasInterialEvent = false;
    Viewport* viewport;
    bool wasSmooth = false;
    Point<float> offset = { 0, 0 };
    Time lastScrollTime;
};

class BouncingViewport : public Viewport {
public:
    BouncingViewport()
        : bouncer(this)
    {
    }

private:
    BouncingViewportAttachment bouncer;
};
