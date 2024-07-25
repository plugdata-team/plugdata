/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"

class WindowDragger {
public:
    WindowDragger() = default;
    ~WindowDragger() = default;

    void startDraggingWindow(Component* componentToDrag,
        MouseEvent const& e)
    {
        jassert(componentToDrag != nullptr);
        jassert(e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

        if (componentToDrag != nullptr)
            mouseDownWithinTarget = e.getEventRelativeTo(componentToDrag).getMouseDownPosition();

#if JUCE_LINUX || JUCE_BSD
        auto* peer = componentToDrag->getPeer();
        peer->startHostManagedResize(e.getPosition(), ResizableBorderComponent::Zone(0));
#endif
    }

    void dragWindow(Component* componentToDrag,
        MouseEvent const& e,
        ComponentBoundsConstrainer* constrainer)
    {
        jassert(componentToDrag != nullptr);
        jassert(e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

        if (componentToDrag != nullptr) {
            auto bounds = componentToDrag->getBounds();
            auto peerBounds = componentToDrag->getPeer()->getBounds();
            // If the component is a window, multiple mouse events can get queued while it's in the same position,
            // so their coordinates become wrong after the first one moves the window, so in that case, we'll use
            // the current mouse position instead of the one that the event contains...
            bounds += componentToDrag->getLocalPoint(nullptr, e.source.getScreenPosition()).roundToInt() - mouseDownWithinTarget;

            bounds *= Desktop::getInstance().getGlobalScaleFactor();

            componentToDrag->getPeer()->setBounds(peerBounds.withPosition(bounds.getPosition()), false);
        }
    }

private:
    //==============================================================================
    Point<int> mouseDownWithinTarget;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowDragger)
};
