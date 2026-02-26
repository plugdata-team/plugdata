/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"

class BouncingViewportAttachment final : public MouseListener
    , public Timer, public SettingsFileListener {
public:
    explicit BouncingViewportAttachment(Viewport* vp)
        : viewport(vp)
    {
        if(SettingsFile::getInstance()->getProperty<bool>("touch_mode"))
        {
            viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
        }
        else {
            viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
        }
        
        viewport->addMouseListener(this, true);
    }

    ~BouncingViewportAttachment() override = default;
        
    void settingsChanged(String const& name, var const& value) override
    {
        if(name == "touch_mode")
        {
            if(static_cast<bool>(value))
            {
                viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
            }
            else {
                viewport->setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
            }
        }
    }
        
#if JUCE_IOS
    void mouseDown(MouseEvent const& e) override
    {
        OSUtils::ScrollTracker::setAllowOneFingerScroll(true);
    }
#else
    void mouseDrag(MouseEvent const& e) override
    {
        if (!isBounceable || viewport->getScrollOnDragMode() == Viewport::ScrollOnDragMode::never)
            return;
        
        if(e.source.getIndex() != 0 || !e.mods.isLeftButtonDown() || e.eventTime == lastScrollTime)
            return;
        
        lastScrollTime = e.eventTime;
        
        // So far, we only really need vertical scrolling
        if (viewport->isVerticalScrollBarShown()) {
            offset.y = std::clamp<float>(e.getDistanceFromDragStartY(), -50.0f, 50.0f);
        }

        update();
    }
#endif
    
    void mouseUp(MouseEvent const& e) override
    {
        if(e.source.getIndex() != 0 || !e.mods.isLeftButtonDown())
            return;
        
#if JUCE_IOS
        OSUtils::ScrollTracker::setAllowOneFingerScroll(false);
#else
        startTimerHz(60);
#endif
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

        wasInterialEvent = isLargeInertialEvent;

        // So far, we only really need vertical scrolling
        if (viewport->isVerticalScrollBarShown() && !isSmallIntertialEvent) {

            auto const area = viewport->getViewArea();
            auto const componentBounds = viewport->getViewedComponent()->getBounds();

            float const factor = wheel.isInertial ? 0.02f : 0.1f;
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
        holder->setTransform(holder->getTransform().withAbsoluteTranslation(offset.x, offset.y));
    }

    static bool isInsideScrollGesture()
    {
#if JUCE_MAC || JUCE_IOS
        return OSUtils::ScrollTracker::isPerformingGesture();
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
    bool isBounceable = true;
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
