/*
 // Copyright (c) 2015-2021 Jim Credland
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once

#include <JuceHeader.h>

class Canvas;
/**
 * MultiComponentDragger allows the user to select objects and drag them around the
 * screen.  Multiple objects can be selected and dragged at once.  The behaviour
 * is similar to Microsoft PowerPoint and probably lots of other applications.
 *
 * Holding down Command(Control) or Shift allows multiple selection.  Holding down
 * shift can optionally also constrain the objects movement to only the left or
 * right axis.
 *
 * The movement can be constrained to be within the bounds of the parent component.
 *
 * Objects directly attached to the desktop are not supported.
 *
 * Using: see handleMouseUp, handleMouseDown and handleMouseDrag
 *
 * You will probably also want to check isSelected() in your objects paint(Graphics &)
 * routine and ensure selected objects are highlighted.  Repaints are triggered
 * automatically if the selection status changes.
 *
 * @TODO: Add 'grid' support.
 */

template <typename T>
class MultiComponentDragger : public LassoSource<T*> {
 public:
  Canvas* canvas;
  OwnedArray<T>* selectable;

  MultiComponentDragger(Canvas* parent, OwnedArray<T>* selectableObjects) {
    canvas = parent;
    selectable = selectableObjects;
  }
  virtual ~MultiComponentDragger() = default;

  virtual void dragCallback(int dx, int dy) = 0;

  /**
   * Adds a specified component as being selected.
   */
  void setSelected(T* component, bool shouldNowBeSelected) {
    /* Asserts here? This class is only designed to work for components that have a common parent. */
    jassert(selectedComponents.getNumSelected() == 0 || component->getParentComponent() == selectedComponents.getSelectedItem(0)->getParentComponent());

    bool isAlreadySelected = isSelected(component);

    if (!isAlreadySelected && shouldNowBeSelected) {
      selectedComponents.addToSelection(component);
      component->repaint();
    }

    if (isAlreadySelected && !shouldNowBeSelected) {
      removeSelectedComponent(component);
      component->repaint();
    }
  }

  /**
   You should call this when the user clicks on the background of the
   parent component.
   */
  virtual void deselectAll() {
    for (auto c : selectedComponents)
      if (c) c->repaint();

    selectedComponents.deselectAll();
  }

  /**
   Find out if a component is marked as selected.
   */
  bool isSelected(T* component) const { return std::find(selectedComponents.begin(), selectedComponents.end(), component) != selectedComponents.end(); }

  /**
   Call this from your components mouseDown event.
   */
  void handleMouseDown(T* component, const MouseEvent& e) {
    jassert(component != nullptr);

    if (!isSelected(component)) {
      if (!(e.mods.isShiftDown() || e.mods.isCommandDown())) deselectAll();

      setSelected(component, true);
    }

    if (component != nullptr) mouseDownWithinTarget = e.getEventRelativeTo(component).getMouseDownPosition();

    componentBeingDragged = component;

    totalDragDelta = {0, 0};

    component->repaint();
  }

  /**
   Call this from your components mouseUp event.
   */
  void handleMouseUp(T* component, const MouseEvent& e) {
    if (didStartDragging) {
      dragCallback(totalDragDelta.x, totalDragDelta.y);
    }

    if (didStartDragging) didStartDragging = false;

    component->repaint();
  }

  /**
   Call this from your components mouseDrag event.
   */
  void handleMouseDrag(const MouseEvent& e) {
    jassert(e.mods.isAnyMouseButtonDown());  // The event has to be a drag event!

    /** Ensure tiny movements don't start a drag. */
    if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag) return;

    didStartDragging = true;

    Point<int> delta = e.getEventRelativeTo(componentBeingDragged).getPosition() - mouseDownWithinTarget;

    for (auto comp : selectedComponents) {
      if (comp != nullptr) {
        Rectangle<int> bounds(comp->getBounds());

        bounds += delta;

        comp->setBounds(bounds);
      }
    }
    totalDragDelta += delta;
  }

  SelectedItemSet<T*>& getLassoSelection() {
    rawPointers.deselectAll();

    for (auto& selected : selectedComponents) {
      if (selected) {
        rawPointers.addToSelection(selected);
      }
    }

    return rawPointers;
  }

 private:
  void removeSelectedComponent(T* component) { selectedComponents.deselect(component); }

  void findLassoItemsInArea(Array<T*>& itemsFound, const Rectangle<int>& area) {
    for (auto* element : *selectable) {
      if (area.intersects(element->getBounds())) {
        itemsFound.add(element);
        setSelected(element, true);
        element->repaint();
      } else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
        setSelected(element, false);
      }
    }
  }

  const int minimumMovementToStartDrag = 10;

  bool didStartDragging{false};

  Point<int> mouseDownWithinTarget;
  Point<int> totalDragDelta;

  SelectedItemSet<Component::SafePointer<T>> selectedComponents;
  SelectedItemSet<T*> rawPointers;

  T* componentBeingDragged{nullptr};
};
