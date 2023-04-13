/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/SettingsFile.h"

class Object;
class Canvas;
class ObjectGrid : public SettingsFileListener {

public:
    ObjectGrid(Component* parent);

    void clearAll();

    Point<int> performMove(Object* toDrag, Point<int> dragOffset);
    Point<int> performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds);
    Point<int> performFixedResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds, Rectangle<int> nonClippedBounds);

    // TODO: Can we save gridSize in patch instead of Settings File??
    //  TS: No we can't!
    int gridSize;

    static constexpr int range = 7;
    static constexpr int tolerance = 5;

    static Array<Object*> getSnappableObjects(Object* cnv);

private:
    enum SnapOrientation {
        SnappedLeft,
        SnappedCentre,
        SnappedRight,
        SnappedConnection
    };

    bool widthResizeSnapped = false;
    bool horizontalResizeSnapped = false;

    bool snapped[2] = { false, false };

    SnapOrientation orientation[2];
    Point<int> snappedPosition;
    Component::SafePointer<Component> start[2];
    Component::SafePointer<Component> end[2];
    DrawablePath gridLines[2];

    int applySnap(SnapOrientation orientation, int position, Component* start, Component* end, bool horizontal);
    void updateMarker();
    void clear(bool horizontal);

    bool isAlreadySnapped(bool horizontal, Point<int>& dragOffset);

    void propertyChanged(String name, var value) override;

    std::tuple<bool, bool, bool> getSnapConfiguration();

    int gridType;
    bool gridEnabled;
};

/* better system to use in the future?
struct ObjectGrid2 : public SettingsFileListener {

    ObjectGrid2::ObjectGrid2(Canvas* cnv) {
        cnv->addAndMakeVisible(gridLines[0]);
        cnv->addAndMakeVisible(gridLines[1]);
    }

    Point<int> ObjectGrid2::performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds)
    {

        auto limits = [&]() -> Rectangle<int> {
            if (auto* parent = toDrag->getParentComponent())
                return { parent->getWidth(), parent->getHeight() };

            const auto globalBounds = toDrag->localAreaToGlobal(newResizeBounds - toDrag->getPosition());

            if (auto* display = Desktop::getInstance().getDisplays().getDisplayForPoint(globalBounds.getCentre()))
                return toDrag->getLocalArea(nullptr, display->userArea) + toDrag->getPosition();

            const auto max = std::numeric_limits<int>::max();
            return { max, max };
        }();

        auto resizeZone = toDrag->resizeZone;
        auto nonClippedBounds = newResizeBounds;
        // Not great that we need to do this, but otherwise we don't really know the object bounds for sure
        toDrag->constrainer->checkBounds(newResizeBounds, toDrag->originalBounds, limits,
            resizeZone.isDraggingTopEdge(), resizeZone.isDraggingLeftEdge(),
            resizeZone.isDraggingBottomEdge(), resizeZone.isDraggingRightEdge());


        auto snappable = ObjectGrid::getSnappableObjects(toDrag);

        auto ratio = toDrag->constrainer->getFixedAspectRatio();

        auto oldBounds = toDrag->originalBounds;

        auto wDiff = nonClippedBounds.getWidth() - oldBounds.getWidth();
        auto hDiff = nonClippedBounds.getHeight() - oldBounds.getHeight();
        bool draggingWidth =  wDiff > (hDiff * ratio);

        auto b2 = toDrag->getBounds().reduced(Object::margin);

        for(auto* object : snappable) {

            auto b1 = object->getBounds().reduced(Object::margin);
            auto topDiff = b1.getY() - b2.getY();
            auto bottomDiff = b1.getBottom() - b2.getBottom();
            auto leftDiff = b1.getX() - b2.getX();
            auto rightDiff = b1.getRight() - b2.getRight();

            bool snapped = true;
            int distance = 0;
            Point<int> start, end;
            if (resizeZone.isDraggingTopEdge() && std::abs(topDiff) < ObjectGrid::tolerance) {
                start = leftDiff > 0 ? b2.getTopLeft() : b1.getTopLeft();
                end = leftDiff > 0 ? b1.getTopRight() : b2.getTopRight();
                distance = topDiff;
            }
            else if (resizeZone.isDraggingBottomEdge() && std::abs(bottomDiff) < ObjectGrid::tolerance) {
                start = leftDiff > 0 ? b2.getBottomLeft() : b1.getBottomLeft();
                end = leftDiff > 0 ? b1.getBottomRight() : b2.getBottomRight();
                distance = bottomDiff;
            }
            else if (resizeZone.isDraggingLeftEdge() && std::abs(leftDiff) < ObjectGrid::tolerance) {
                start = topDiff > 0 ? b2.getTopLeft() : b1.getTopLeft();
                end = topDiff > 0 ? b1.getBottomLeft() : b2.getBottomLeft();
                distance = leftDiff;
            }
            else if (resizeZone.isDraggingRightEdge() && std::abs(rightDiff) < ObjectGrid::tolerance) {
                start = topDiff > 0 ? b2.getTopRight() : b1.getTopRight();
                end = topDiff > 0 ? b1.getBottomRight() : b2.getBottomRight();
                distance = rightDiff;
            }
            else {
                snapped = false;
            }

            if(snapped) {
                setPath(0, start, end);
                std::cout << "Snapped" << std::endl;
                return draggingWidth ? dragOffset.withX(dragOffset.x + distance) : dragOffset.withY(dragOffset.y + distance);
            }
        }

        std::cout << "Not snapped" << std::endl;

        clearPath(0);

        return dragOffset;
    }


    void ObjectGrid2::clearPath(int idx) {
        gridLines[idx].setPath(std::move(Path()));
    }

    void ObjectGrid2::setPath(int idx, Point<int> start, Point<int> end) {
        auto& lnf = LookAndFeel::getDefaultLookAndFeel();
        gridLines[idx].setStrokeFill(FillType(lnf.findColour(PlugDataColour::gridLineColourId)));
        gridLines[idx].setStrokeThickness(1);
        gridLines[idx].setAlwaysOnTop(true);

        Path toDraw;
        toDraw.addLineSegment(Line<float>(start.toFloat(), end.toFloat()), 1.0f);
        gridLines[idx].setPath(toDraw);
    }


    DrawablePath gridLines[2];
    Point<int> lastSnapPosition;
};
*/
