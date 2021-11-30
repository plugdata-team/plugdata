/*
  ==============================================================================

    jcf_multi_selection.h
    Created: 25 Feb 2016 9:00:28am
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef JCF_MULTI_SELECTION_H_INCLUDED
#define JCF_MULTI_SELECTION_H_INCLUDED


#include <JuceHeader.h>
#include "gin_valuetreeobject.h"

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


template<typename T>
class MultiComponentDragger : public LassoSource<T*>
{
public:
    
    Canvas* canvas;
    MultiComponentDragger(Canvas* parent) {
        canvas = parent;
    }
    virtual ~MultiComponentDragger() {}

    void setConstrainBoundsToParent(bool shouldConstrainToParentSize,
                                    BorderSize<int> amountPermittedOffscreen_)
    {
        constrainToParent = shouldConstrainToParentSize;
        amountPermittedOffscreen = amountPermittedOffscreen_;
    }

    /**
     If this flag is set then the dragging behaviour when shift
     is held down will be constrained to the vertical or horizontal
     direction.  This the the behaviour of Microsoft PowerPoint.
     */
    void setShiftConstrainsDirection(bool constrainDirection)
    {
        shiftConstrainsDirection = constrainDirection;
    }

    /**
     * Adds a specified component as being selected.
     */
    void setSelected(T* component, bool shouldNowBeSelected)
    {
        /* Asserts here? This class is only designed to work for components that have a common parent. */
        jassert(selectedComponents.getNumSelected() == 0 || component->getParentComponent() == selectedComponents.getSelectedItem(0)->getParentComponent());

        bool isAlreadySelected = isSelected(component);
        
        if (! isAlreadySelected && shouldNowBeSelected)
            selectedComponents.addToSelection(component);
        
        if (isAlreadySelected && ! shouldNowBeSelected)
            removeSelectedComponent(component);
    }

    /** Toggles the selected status of a particular component. */
    void toggleSelection(T* component)
    {
        setSelected(component, ! isSelected(component));
    }
    
    /** 
     You should call this when the user clicks on the background of the
     parent component.
     */
    void deselectAll()
    {
        for (auto c: selectedComponents)
            if (c)
                c->repaint();
        
        selectedComponents.deselectAll();
    }
    
    /**
     Find out if a component is marked as selected.
     */
    bool isSelected(T* component) const
    {
        return std::find(selectedComponents.begin(),
                         selectedComponents.end(),
                         component) != selectedComponents.end();
    }
    
    /** 
     Call this from your components mouseDown event.
     */
    void handleMouseDown (T* component, const MouseEvent & e)
    {
        jassert (component != nullptr);

        if (! isSelected(component))
        {
            if (! (e.mods.isShiftDown() || e.mods.isCommandDown()))
                deselectAll();
            
            setSelected(component, true);
            didJustSelect = true;
        }
        
        if (component != nullptr)
            mouseDownWithinTarget = e.getEventRelativeTo (component).getMouseDownPosition();

        componentBeingDragged = component;

        totalDragDelta = {0, 0};

        constrainedDirection = noConstraint;
        
        component->repaint();
    }
    
    /**
     Call this from your components mouseUp event.
     */
    void handleMouseUp (T* component, const MouseEvent & e)
    {
        if (didStartDragging)
            didStartDragging = false;
        /* uncomment to deselect when clicking a selected component
        else
            if (!didJustSelect && isSelected(component))
                setSelected(component, false); */
        
        didJustSelect = false;
        
        component->repaint();
        for(auto& component : selectedComponents) {
            component->updatePosition();
        }
        
    }

    /**
     Call this from your components mouseDrag event.
     */
    void handleMouseDrag (const MouseEvent& e)
    {

        jassert (e.mods.isAnyMouseButtonDown()); // The event has to be a drag event!

        /** Ensure tiny movements don't start a drag. */
        if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag)
            return;

        didStartDragging = true;

        Point<int> delta = e.getEventRelativeTo (componentBeingDragged).getPosition() - mouseDownWithinTarget;

        if (constrainToParent)
        {
            auto targetArea = getAreaOfSelectedComponents() + delta;
            auto limit = componentBeingDragged->getParentComponent()->getBounds();

            amountPermittedOffscreen.subtractFrom(targetArea);

            if (targetArea.getX() < 0)
                delta.x -= targetArea.getX();

            if (targetArea.getY() < 0)
                delta.y -= targetArea.getY();

            if (targetArea.getBottom() > limit.getBottom())
                delta.y -= targetArea.getBottom() - limit.getBottom();

            if (targetArea.getRight() > limit.getRight())
                delta.x -= targetArea.getRight() - limit.getRight();
        }

        applyDirectionConstraints(e, delta);

        for (auto comp: selectedComponents)
        {
            if (comp != nullptr)
            {
                Rectangle<int> bounds (comp->getBounds());

                bounds += delta;

                comp->setBounds (bounds);
            }
        }
        totalDragDelta += delta;
    }

    SelectedItemSet<T*>& getLassoSelection()
    {
        raw_pointers.deselectAll();
        
        for(auto& selected : selectedComponents) {
            if(selected) {
                raw_pointers.addToSelection(selected);
            }
        }
        
        return raw_pointers;
    }
    
    Rectangle<int> getAreaOfSelectedComponents()
    {
        if (selectedComponents.getNumSelected() == 0)
            return Rectangle<int>(0, 0, 0, 0);
        
        
        Rectangle<int> a = selectedComponents.getSelectedItem(0)->getBounds();
        
        for (auto comp: selectedComponents)
            if (comp)
                a = a.getUnion(comp->getBounds());
        
        return a;
    }
    
private:

    void applyDirectionConstraints(const MouseEvent &e, Point<int> &delta)
    {
        
        if (shiftConstrainsDirection && e.mods.isShiftDown())
        {
            /* xy > 0 == movement mainly X direction, xy < 0 == movement mainly Y direction. */
            int xy = abs(totalDragDelta.x + delta.x) - abs(totalDragDelta.y + delta.y);

            /* big movements remove the lock to a particular axis */

            if (xy > minimumMovementToStartDrag)
                constrainedDirection = xAxisOnly;

            if (xy < -minimumMovementToStartDrag)
                constrainedDirection = yAxisOnly;

            if ((xy > 0 && constrainedDirection != yAxisOnly)
                ||
                (constrainedDirection == xAxisOnly))
            {
                delta.y = -totalDragDelta.y; /* move X direction only. */
                constrainedDirection = xAxisOnly;
            }
            else if ((xy <= 0 && constrainedDirection != xAxisOnly)
                     ||
                     constrainedDirection == yAxisOnly)
            {
                delta.x = -totalDragDelta.x; /* move Y direction only. */
                constrainedDirection = yAxisOnly;
            }
            else
            {
                delta = {0, 0};
            }
        }
        else
        {
            constrainedDirection = noConstraint;

        }
    }

    void removeSelectedComponent(T* component)
    {
        selectedComponents.deselect(component);
    }
    
    void findLassoItemsInArea (Array<T*> & itemsFound, const Rectangle<int>& area)
    {
        for(auto element : static_cast<ValueTreeObject*>(canvas)->findChildrenOfClass<T>())
        {
            if (area.intersects(element->getBounds()))
            {
                itemsFound.add(element);
                setSelected(element, true);
            }
            else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown())
            {
                setSelected(element, false);
            }
        }
    }
    
    enum
    {
        noConstraint,
        xAxisOnly,
        yAxisOnly
    } constrainedDirection;

    const int minimumMovementToStartDrag = 10;

    bool constrainToParent {false};
    bool shiftConstrainsDirection {false};

    bool didJustSelect {false};
    bool didStartDragging {false};

    Point<int> mouseDownWithinTarget;
    Point<int> totalDragDelta;

    Array<T*> temp_selection;
    
    SelectedItemSet<Component::SafePointer<T>> selectedComponents;
    SelectedItemSet<T*> raw_pointers;
    
    T* componentBeingDragged { nullptr };
    
    BorderSize<int> amountPermittedOffscreen;
};



#endif  // JCF_MULTI_SELECTION_H_INCLUDED
