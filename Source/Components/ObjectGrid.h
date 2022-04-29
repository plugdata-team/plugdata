#pragma once
#include <JuceHeader.h>

class Box;
struct ObjectGrid : public DrawablePath
{
    enum GridType
    {
        NotSnappedToGrid,
        HorizontalSnap,
        VerticalSnap,
        ConnectionSnap,
        BestSizeSnap,
    };
    
    ObjectGrid();

    Point<int> forceSnap(GridType t, Box* toDrag, Point<int> dragOffset);
    
    Point<int> handleMouseDrag(Box* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds);
    Point<int> handleMouseUp(Point<int> dragOffset);
    
    static constexpr int range = 4;
    
private:

    enum SnapOrientation
    {
        SnappedLeft,
        SnappedCentre,
        SnappedRight
    };
    
    
    GridType type = ObjectGrid::NotSnappedToGrid;
    SnapOrientation orientation;
    int idx;
    Point<int> position;
    SafePointer<Component> start;
    SafePointer<Component> end;
    
    Point<int> setState(GridType type, int idx, Point<int> position, Component* start, Component* end);
    void updateMarker();
    void clear();
};
