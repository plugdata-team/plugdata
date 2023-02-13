/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>
#include "Utility/SettingsFile.h"

class Object;
class Canvas;
class ObjectGrid : public SettingsFileListener {

public:
    ObjectGrid(Canvas* parent);
    // Point<int> handleMouseDrag(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds, Rectangle<int> resizeBounds = Rectangle<int>(0, 0, 0, 0));
    Point<int> handleMouseUp(Point<int> dragOffset);

    Point<int> performMove(Object* toDrag, Point<int> dragOffset);

    Point<int> performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds);

    int gridSize = 20;
    static constexpr int range = 5;
    static constexpr int tolerance = 3;

private:
    enum SnapOrientation {
        SnappedLeft,
        SnappedCentre,
        SnappedRight,
        SnappedConnection
    };

    bool snapped[2] = { false, false };

    SnapOrientation orientation[2];
    Point<int> snappedPosition;
    Component::SafePointer<Component> start[2];
    Component::SafePointer<Component> end[2];
    DrawablePath gridLines[2];

    int gridEnabled = 1;

    Point<int> applySnap(SnapOrientation orientation, Point<int> position, Component* start, Component* end, bool horizontal);
    void updateMarker();
    void clear(bool horizontal);

    Point<int> performVerticalSnap(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds, Rectangle<int> newResizeBounds);
    Point<int> performHorizontalSnap(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds, Rectangle<int> newResizeBounds);

    Point<int> performAbsoluteSnap(Object* toDrag, Point<int> dragOffset);

    Array<Object*> getSnappableObjects(Canvas* cnv);
    bool isAlreadySnapped(bool horizontal, Point<int>& dragOffset);

    void propertyChanged(String name, var value) override;
};
