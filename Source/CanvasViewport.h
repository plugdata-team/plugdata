/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/GlobalMouseListener.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport
{
    class MousePanner : public MouseListener
    {
    public:
        MousePanner(Viewport* v) : viewport(v){
        }
        
        void enablePanning(bool enabled) {
            if(auto* viewedComponent = viewport->getViewedComponent()) {
                if(enabled) {
                    viewedComponent->addMouseListener(this, true);
                }
                else {
                    viewedComponent->removeMouseListener(this);
                }
            }
        }
        
        // warning: this only works because Canvas::mouseDown gets called before the listener's mouse down
        // thus giving is a chance to attach the mouselistener on the middle-mouse click event
        void mouseDown(const MouseEvent& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
            downPosition = viewport->getViewPosition();
            
        }
        
        void mouseDrag(const MouseEvent& e) override
        {
            float scale = 1.0f;
            if(auto* viewedComponent = viewport->getViewedComponent())
            {
                scale = std::sqrt(std::abs(viewedComponent->getTransform().getDeterminant()));
            }
            
            viewport->setViewPosition(downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
        }
        
        void mouseUp(const MouseEvent& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
        }
        
    private:
        Viewport* viewport;
        Point<int> downPosition;
    };
    
    class FadingScrollbar : public ScrollBar, public ScrollBar::Listener
    {
        struct FadeTimer : private ::Timer
        {
            std::function<bool()> callback;
            
            void start(int interval, std::function<bool()> cb) {
                callback = cb;
                startTimer(interval);
            }
            
            void stop() {
                stopTimer();
            }
            
            void timerCallback() override
            {
                if(callback()) stopTimer();
            }
        };
        
        struct FadeAnimator : private ::Timer
        {
            FadeAnimator(Component* target) : targetComponent(target)
            {
            }
            
            void timerCallback() {
                auto alpha = targetComponent->getAlpha();
                if(alphaTarget > alpha) {
                    targetComponent->setAlpha(alpha + 0.2f);
                }
                else if(alphaTarget < alpha) {
                    float easedAlpha = pow(alpha,  1.0f / 2.0f);
                    easedAlpha -= 0.01f;
                    alpha = pow(easedAlpha, 2.0f);
                    if (alpha < 0.01f)
                        alpha = 0.0f;
                    targetComponent->setAlpha(alpha);
                } else {
                    stopTimer();
                }
            }
            
            void fadeIn() {
                alphaTarget = 1.0f;
                startTimerHz(60);
            }
            
            void fadeOut() {
                alphaTarget = 0.0f;
                startTimerHz(60);
            }
            
            Component* targetComponent;
            float alphaTarget = 0.0f;
        };
        
    public:
        FadingScrollbar(bool isVertical) : ScrollBar(isVertical)
        {
            ScrollBar::setVisible(true);
            addListener(this);
            fadeOut();
        }
        
        void fadeIn(bool fadeOutAfterInterval) {
            setVisible(true);
            animator.fadeIn();
            
            if(fadeOutAfterInterval) {
                fadeTimer.start(800, [this](){
                    fadeOut();
                    return true;
                });
            }
        }
        
        void fadeOut() {
            animator.fadeOut();
        }
        
        std::function<void()> onScroll = [](){};
        
    private:
        
        void scrollBarMoved (ScrollBar *scrollBarThatHasMoved, double newRangeStart) override
        {
            onScroll();
            fadeIn(true);
        }
        
        void mouseEnter(const MouseEvent& e) override
        {
            fadeIn(false);
        }
        
        void mouseExit(const MouseEvent& e) override
        {
            fadeOut();
        }
        
        // Don't allow the viewport to manage scrollbar visibility!
        void setVisible(bool shouldBeVisible) override
        {
        }
        
        FadeAnimator animator = FadeAnimator(this);
        FadeTimer fadeTimer;
    };
    
    struct ViewportPositioner : public Component::Positioner {
        ViewportPositioner(Viewport& comp)
        : Component::Positioner(comp)
        , inset(comp.getScrollBarThickness())
        {
        }
        
        void applyNewBounds(Rectangle<int> const& newBounds) override
        {
            auto& component = getComponent();
            if (newBounds != component.getBounds()) {
                component.setBounds(newBounds.withTrimmedRight(-inset).withTrimmedBottom(-inset));
            }
        }
        
        int inset;
    };

public:
    CanvasViewport()
    {
        recreateScrollbars();
        
        setPositioner(new ViewportPositioner(*this));
        adjustScrollbarBounds();
    }
    
    void lookAndFeelChanged() override
    {
        if(!vbar || !hbar) return;
        
        vbar->repaint();
        hbar->repaint();
    }
    
    void enableMousePanning(bool enablePanning)
    {
        panner.enablePanning(enablePanning);
    }
    
    void adjustScrollbarBounds()
    {
        if(!vbar || !hbar) return;
        
        auto thickness = getScrollBarThickness();
        
        auto contentArea = getLocalBounds().withTrimmedRight(thickness).withTrimmedBottom(thickness);
        
        vbar->setBounds(contentArea.removeFromRight(thickness));
        hbar->setBounds(contentArea.removeFromBottom(thickness));
    }
    
    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }
    
    void resized() override
    {
        Viewport::resized();
        adjustScrollbarBounds();
    }
    
    ScrollBar* createScrollBarComponent (bool isVertical) override
    {
        if(isVertical) {
            vbar = new FadingScrollbar(true);
            vbar->onScroll = [this](){
                onScroll();
            };
            return vbar;
        }
        else {
            hbar = new FadingScrollbar(false);
            hbar->onScroll = [this](){
                onScroll();
            };
            return hbar;
        }
    }
    
    std::function<void()> onScroll = [](){};
private:
    
    MousePanner panner = MousePanner(this);
    FadingScrollbar* vbar = nullptr;
    FadingScrollbar* hbar = nullptr;
};
