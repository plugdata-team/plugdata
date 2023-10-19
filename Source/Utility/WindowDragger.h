/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"

class WindowDragger
{
public:

    WindowDragger() = default;
    ~WindowDragger() = default;

    void startDraggingWindow (Component* componentToDrag,
                                 const MouseEvent& e)
    {
      jassert (componentToDrag != nullptr);
      jassert (e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

      if (componentToDrag != nullptr)
          mouseDownWithinTarget = e.getEventRelativeTo (componentToDrag).getMouseDownPosition();

        ResizableBorderComponent::Zone(0);
#if JUCE_LINUX
      auto* peer = componentToDraw->getPeer();
        
        peer->startHostManagedResize(peer->localToGlobal(mouseDownWithinTarget), ResizableBorderComponent::Zone(0));
      // This will tell X11 to act as if the titlebar is being dragged, and can make window dragging behave better.
      // This will sometimes also work on XWayland, but not always
      OSUtils::hostManagedX11WindowMove(componentToDrag, {mouseDownWithinTarget.x, mouseDownWithinTarget.y, 0, 0});
#endif
}

    void dragWindow (Component* componentToDrag,
                        const MouseEvent& e,
                        ComponentBoundsConstrainer* constrainer)
    {
      jassert (componentToDrag != nullptr);
      jassert (e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

      if (componentToDrag != nullptr)
      {
          auto bounds = componentToDrag->getBounds();

          // If the component is a window, multiple mouse events can get queued while it's in the same position,
          // so their coordinates become wrong after the first one moves the window, so in that case, we'll use
          // the current mouse position instead of the one that the event contains...
          if (componentToDrag->isOnDesktop())
              bounds += componentToDrag->getLocalPoint (nullptr, e.source.getScreenPosition()).roundToInt() - mouseDownWithinTarget;
          else
              bounds += e.getEventRelativeTo (componentToDrag).getPosition() - mouseDownWithinTarget;

          componentToDrag->getPeer()->setBounds (bounds, false);
      }

    }

private:
    //==============================================================================
    Point<int> mouseDownWithinTarget;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowDragger)
};


