/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ObjectGrid.h"
#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "LookAndFeel.h"
#include "Utility/ObjectBoundsConstrainer.h"

ObjectGrid::ObjectGrid(Canvas* parent)
{
    for (auto& line : gridLines) {
        parent->addAndMakeVisible(line);

        line.setStrokeThickness(1);
        line.setAlwaysOnTop(true);
    }

    gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
}

Point<int> ObjectGrid::applySnap(SnapOrientation direction, Point<int> pos, Component* s, Component* e, bool horizontal)
{
    orientation[horizontal] = direction;
    snapped[horizontal] = true;
    snappedPosition = pos;
    start[horizontal] = s;
    end[horizontal] = e;
    updateMarker();

    return pos;
}

void ObjectGrid::updateMarker()
{
    auto& lnf = LookAndFeel::getDefaultLookAndFeel();
    gridLines[0].setStrokeFill(FillType(lnf.findColour(PlugDataColour::gridLineColourId)));
    gridLines[1].setStrokeFill(FillType(lnf.findColour(PlugDataColour::gridLineColourId)));

    if (!snapped[0]) {
        gridLines[0].setPath(std::move(Path()));
    }
    if (!snapped[1]) {
        gridLines[1].setPath(std::move(Path()));
    }
    if (!snapped[0] && !snapped[1]) {
        return;
    }

    Path toDraw;

    if (snapped[1] && orientation[1] == SnappedConnection && start[1] && end[1]) {

        auto b1 = start[1]->getParentComponent()->getBounds();
        auto b2 = end[1]->getParentComponent()->getBounds();

        b1.translate(start[1]->getX() - 2, 0);
        b2.translate(end[1]->getX() - 2, 0);

        toDraw.addLineSegment(Line<float>(b1.getX() - 2, b1.getBottom() + 2, b1.getX() - 2, b2.getY() - 2), 1.0f);
        gridLines[1].setPath(toDraw);
        return;
    }

    for (int i = 0; i < 2; i++) {
        if (!start[i] || !end[i] || !snapped[i])
            continue;

        auto b1 = start[i]->getBounds().reduced(Object::margin);
        auto b2 = end[i]->getBounds().reduced(Object::margin);

        auto t = b1.getY() < b2.getY() ? b1 : b2;
        auto b = b1.getY() > b2.getY() ? b1 : b2;
        auto l = b1.getX() < b2.getX() ? b1 : b2;
        auto r = b1.getX() > b2.getX() ? b1 : b2;

        auto gridLine = Line<float>();

        if (i == 0) {
            if (orientation[0] == SnappedLeft) {
                gridLine = Line<float>(l.getX(), t.getY(), r.getRight(), t.getY());
            } else if (orientation[0] == SnappedRight) {
                gridLine = Line<float>(l.getX(), t.getBottom(), r.getRight(), t.getBottom());
            } else { // snapped centre
                gridLine = Line<float>(l.getX(), t.getCentreY(), r.getRight(), t.getCentreY());
            }
        } else if (i == 1) {
            if (orientation[1] == SnappedLeft) {
                gridLine = Line<float>(l.getX(), t.getY(), l.getX(), b.getBottom());
            } else if (orientation[1] == SnappedRight) {
                gridLine = Line<float>(l.getRight(), t.getY(), l.getRight(), b.getBottom());
            } else { // snapped centre
                gridLine = Line<float>(l.getCentreX(), t.getY(), l.getCentreX(), b.getBottom());
            }
        }

        toDraw.addLineSegment(gridLine, 1.0f);
        gridLines[i].setPath(toDraw);
    }
}

void ObjectGrid::clear(bool horizontal)
{
    snapped[horizontal] = false;
    snappedPosition = Point<int>();
    start[horizontal] = nullptr;
    end[horizontal] = nullptr;
    updateMarker();
}

Point<int> ObjectGrid::performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds)
{
    // Grid is disabled
    if (gridEnabled == 0 || ModifierKeys::getCurrentModifiers().isShiftDown()) { 
        return dragOffset;
    }

    // Snap to Objects
    if (gridEnabled == 1 || gridEnabled == 3) { 

        auto limits = [&]() -> Rectangle<int> {
            if (auto* parent = toDrag->getParentComponent())
                return { parent->getWidth(), parent->getHeight() };

            const auto globalBounds = toDrag->localAreaToGlobal(newResizeBounds - toDrag->getPosition());

            if (auto* display = Desktop::getInstance().getDisplays().getDisplayForPoint(globalBounds.getCentre()))
                return toDrag->getLocalArea(nullptr, display->userArea) + toDrag->getPosition();

            const auto max = std::numeric_limits<int>::max();
            return { max, max };
        }();

        auto snappable = getSnappableObjects(toDrag->cnv);
        auto resizeZone = toDrag->resizeZone;

        // Not great that we need to do this, but otherwise we don't really know the object bounds for sure
        toDrag->constrainer->checkBounds(newResizeBounds, toDrag->originalBounds, limits,
            resizeZone.isDraggingTopEdge(), resizeZone.isDraggingLeftEdge(),
            resizeZone.isDraggingBottomEdge(), resizeZone.isDraggingRightEdge());

        auto b2 = newResizeBounds.reduced(Object::margin);
        auto ratio = toDrag->constrainer->getFixedAspectRatio();

        if (!isAlreadySnapped(false, dragOffset)) {
            for (auto* object : snappable) {
                auto b1 = object->getBounds().reduced(Object::margin);

                if (std::abs(b1.getY() - b2.getY()) < tolerance) {
                    auto dy = b1.getY() - b2.getY();
                    auto dx = roundToInt(ratio * dy);
                    dragOffset = applySnap(SnappedLeft, Point<int>(0, dy) + dragOffset, object, toDrag, false);
                }
                if (std::abs(b1.getBottom() - b2.getBottom()) < tolerance) {
                    auto dy = b1.getBottom() - b2.getBottom();
                    auto dx = roundToInt(ratio * dy);
                    dragOffset = applySnap(SnappedRight, Point<int>(0, dy) + dragOffset, object, toDrag, false);
                }
            }
        }

        if (!isAlreadySnapped(true, dragOffset)) {
            for (auto* object : snappable) {

                auto b1 = object->getBounds().reduced(Object::margin);

                if (std::abs(b1.getX() - b2.getX()) < tolerance) {
                    auto dx = b1.getX() - b2.getX();
                    auto dy = roundToInt(ratio / dx);
                    dragOffset = applySnap(SnappedLeft, Point<int>(dx, 0) + dragOffset, object, toDrag, true);
                }
                if (std::abs(b1.getRight() - b2.getRight()) < tolerance) {
                    auto dx = b1.getRight() - b2.getRight();
                    auto dy = roundToInt(ratio / dx);
                    dragOffset = applySnap(SnappedRight, Point<int>(dx, 0) + dragOffset, object, toDrag, true);
                }
            }
        }

        MessageManager::callAsync([this]() {
            updateMarker();
        });

        if (gridEnabled == 1) {
            return dragOffset;
        }
    }

    // Snap to Grid
    if (gridEnabled == 2 || gridEnabled == 3) { 
        Point<int> newPos = toDrag->originalBounds.reduced(Object::margin).getPosition() + dragOffset;
        if (!isAlreadySnapped(true, dragOffset)) {
            newPos.setX(roundToInt(newPos.getX() / gridSize + 1) * gridSize);
            snappedPosition.x = newPos.x - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
        }
        if (!isAlreadySnapped(false, dragOffset)) {
            newPos.setY(roundToInt(newPos.getY() / gridSize + 1) * gridSize);
            snappedPosition.y = newPos.y - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
        }
        return snappedPosition;
    }
}

Point<int> ObjectGrid::performMove(Object* toDrag, Point<int> dragOffset)
{
    // Grid is disabled
    if (gridEnabled == 0 || ModifierKeys::getCurrentModifiers().isShiftDown()) {
        return dragOffset;
    }

    // Snap to Objects
    if (gridEnabled == 1 || gridEnabled == 3) {
        auto snappable = getSnappableObjects(toDrag->cnv);
        auto b2 = (toDrag->originalBounds + dragOffset).reduced(Object::margin);

        if (!isAlreadySnapped(false, dragOffset)) {

            for (auto* object : snappable) {
                auto b1 = object->getBounds().reduced(Object::margin);

                start[0] = object;
                end[0] = toDrag;

                if (std::abs(b1.getY() - b2.getY()) < tolerance) {
                    dragOffset = applySnap(SnappedLeft, Point<int>(0, b1.getY() - b2.getY()) + dragOffset, object, toDrag, false);
                    break;
                }
                if (std::abs(b1.getCentreY() - b2.getCentreY()) < tolerance) {
                    dragOffset = applySnap(SnappedCentre, Point<int>(0, b1.getCentreY() - b2.getCentreY()) + dragOffset, object, toDrag, false);
                    break;
                }
                if (std::abs(b1.getBottom() - b2.getBottom()) < tolerance) {
                    dragOffset = applySnap(SnappedRight, Point<int>(0, b1.getBottom() - b2.getBottom()) + dragOffset, object, toDrag, false);
                    break;
                }
            }
        }

        if (!isAlreadySnapped(true, dragOffset)) {

            // Find snap points based on connection alignment
            for (auto* connection : toDrag->getConnections()) {
                auto inletBounds = connection->inlet->getCanvasBounds();
                auto outletBounds = connection->outlet->getCanvasBounds();

                // Don't snap if the cord is upside-down
                if (inletBounds.getY() < outletBounds.getY())
                    continue;

                auto recentDragOffset = (toDrag->originalBounds.getPosition() + dragOffset) - toDrag->getPosition();
                if (connection->inobj == toDrag) {
                    // Skip if both objects are selected
                    if (!snappable.contains(connection->outobj))
                        continue;
                    inletBounds += recentDragOffset;
                } else {
                    if (!snappable.contains(connection->inobj))
                        continue;
                    outletBounds += recentDragOffset;
                }

                int snapDistance = inletBounds.getX() - outletBounds.getX();

                // Check if the inlet or outlet is being moved, and invert if needed
                if (connection->inobj == toDrag)
                    snapDistance = -snapDistance;

                if (std::abs(snapDistance) < tolerance) {
                    dragOffset = applySnap(SnappedConnection, { snapDistance + dragOffset.x, dragOffset.y }, connection->outlet, connection->inlet, true);
                    break;
                }

                // If we're close, don't snap for other reasons
                if (std::abs(snapDistance) < tolerance * 2.0f) {
                    dragOffset = dragOffset;
                }
            }
        }

        if (!isAlreadySnapped(true, dragOffset)) {
            for (auto* object : snappable) {

                auto b1 = object->getBounds().reduced(Object::margin);

                start[1] = object;
                end[1] = toDrag;

                if (std::abs(b1.getX() - b2.getX()) < tolerance) {
                    dragOffset = applySnap(SnappedLeft, Point<int>(b1.getX() - b2.getX(), 0) + dragOffset, object, toDrag, true);
                    break;
                }
                if (std::abs(b1.getCentreX() - b2.getCentreX()) < tolerance) {
                    dragOffset = applySnap(SnappedCentre, Point<int>(b1.getCentreX() - b2.getCentreX(), 0) + dragOffset, object, toDrag, true);
                    break;
                }
                if (std::abs(b1.getRight() - b2.getRight()) < tolerance) {
                    dragOffset = applySnap(SnappedRight, Point<int>(b1.getRight() - b2.getRight(), 0) + dragOffset, object, toDrag, true);
                    break;
                }
            }
        }

        MessageManager::callAsync([this]() {
            updateMarker();
        });

        if (gridEnabled == 1) {
            return dragOffset;
        }
    }

    // Snap to Grid
    if (gridEnabled == 2 || gridEnabled == 3) {
        Point<int> newPos = toDrag->originalBounds.reduced(Object::margin).getPosition() + dragOffset;
        if (!isAlreadySnapped(true, dragOffset)) {
            newPos.setX(roundToInt(newPos.getX() / gridSize + 1) * gridSize);
            snappedPosition.x = newPos.x - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
        }
        if (!isAlreadySnapped(false, dragOffset)) {
            newPos.setY(roundToInt(newPos.getY() / gridSize + 1) * gridSize);
            snappedPosition.y = newPos.y - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
        }
        return snappedPosition;
    }
}

Array<Object*> ObjectGrid::getSnappableObjects(Canvas* cnv)
{
    Array<Object*> snappable;

    auto viewBounds = reinterpret_cast<Viewport*>(cnv->viewport)->getViewArea();

    for (auto* object : cnv->objects) {
        if (cnv->isSelected(object) || !viewBounds.intersects(object->getBounds()))
            continue; // don't look at selected objects, or objects that are outside of view bounds

        snappable.add(object);
    }

    return snappable;
}

bool ObjectGrid::isAlreadySnapped(bool horizontal, Point<int>& dragOffset)
{
    if (horizontal && snapped[1]) {
        if (std::abs(snappedPosition.x - dragOffset.x) > range) {
            clear(true);
            return true;
        }
        dragOffset = { snappedPosition.x, snappedPosition.y };
        return true;
    } else if (!horizontal && snapped[0]) {
        if (std::abs(snappedPosition.y - dragOffset.y) > range) {
            clear(false);
            return true;
        }
        dragOffset = { snappedPosition.x, snappedPosition.y };
        return true;
    }

    return false;
}

Point<int> ObjectGrid::handleMouseUp(Point<int> dragOffset)
{
    // dragOffset = snappedPosition;
    if (gridEnabled == 1 && !ModifierKeys::getCurrentModifiers().isShiftDown()) {
        if (snapped[1]) {
            dragOffset.x = snappedPosition.x;
            clear(1);
        }
        if (snapped[0]) {
            dragOffset.y = snappedPosition.y;
            clear(0);
        }
    } else if ((gridEnabled == 2 || gridEnabled == 3) && !ModifierKeys::getCurrentModifiers().isShiftDown()) {
        dragOffset = snappedPosition;
        if (snapped[1]) {
            clear(1);
        }
        if (snapped[0]) {
            clear(0);
        }
    }
    return dragOffset;
}

void ObjectGrid::propertyChanged(String name, var value)
{
    if (name == "grid_enabled") {
        gridEnabled = static_cast<int>(value);
    }
}
