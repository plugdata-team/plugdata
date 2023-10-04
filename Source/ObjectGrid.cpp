/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "ObjectGrid.h"
#include "Object.h"
#include "Canvas.h"
#include "Connection.h"

ObjectGrid::ObjectGrid(Component* parent)
{
    for (auto& line : gridLines) {
        parent->addAndMakeVisible(line);

        line.setStrokeThickness(1);
        line.setAlwaysOnTop(true);
    }

    gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
    gridType = SettingsFile::getInstance()->getProperty<int>("grid_type");
    gridSize = SettingsFile::getInstance()->getProperty<int>("grid_size");
}

int ObjectGrid::applySnap(SnapOrientation direction, int pos, Component* s, Component* e, bool horizontal)
{
    orientation[horizontal] = direction;
    snapped[horizontal] = true;

    if (horizontal) {
        snappedPosition.x = pos;
    } else {
        snappedPosition.y = pos;
    }

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
        auto b = b1.getY() >= b2.getY() ? b1 : b2;
        auto l = b1.getX() < b2.getX() ? b1 : b2;
        auto r = b1.getX() >= b2.getX() ? b1 : b2;

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
    if (horizontal) {
        snappedPosition.x = 0;
    } else {
        snappedPosition.y = 0;
    }
    start[horizontal] = nullptr;
    end[horizontal] = nullptr;

    updateMarker();
}

Point<int> ObjectGrid::performFixedResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds, Rectangle<int> nonClippedBounds)
{
    auto snappable = getSnappableObjects(toDrag);
    auto resizeZone = toDrag->resizeZone;

    auto* constrainer = toDrag->getConstrainer();
    auto ratio = constrainer ? constrainer->getFixedAspectRatio() : 0.0;

    auto oldBounds = toDrag->originalBounds;

    auto wDiff = nonClippedBounds.getWidth() - oldBounds.getWidth();
    auto hDiff = nonClippedBounds.getHeight() - oldBounds.getHeight();
    bool draggingWidth = wDiff > (hDiff * ratio);

    auto b2 = newResizeBounds.reduced(Object::margin);
    bool isSnapped = snapped[0] || snapped[1];

    if (isSnapped) {
        auto snappedPos = horizontalResizeSnapped ? snappedPosition.x : snappedPosition.y;
        auto dragPos = widthResizeSnapped ? dragOffset.x : dragOffset.y;

        if (std::abs(snappedPos - dragPos) > tolerance) {
            clear(0);
            clear(1);
            return dragOffset;
        }

        return { widthResizeSnapped ? snappedPos : dragOffset.x, widthResizeSnapped ? dragOffset.y : snappedPos };
    }

    auto snap = [this, b2, toDrag, draggingWidth, dragOffset, isSnapped, ratio](SnapOrientation direction, Object* object, Rectangle<int> b1, bool horizontal) mutable {
        if (isSnapped)
            return dragOffset;

        int delta;
        if (direction == SnappedLeft) {
            delta = horizontal ? b1.getX() - b2.getX() : b1.getY() - b2.getY();
        } else {
            delta = horizontal ? b1.getRight() - b2.getRight() : b1.getBottom() - b2.getBottom();
        }

        auto& snapTarget = draggingWidth ? dragOffset.x : dragOffset.y;

        if (draggingWidth && !horizontal) {
            delta *= ratio;
        }
        if (!draggingWidth && horizontal) {
            if (horizontal ^ draggingWidth) {
                delta /= ratio;
            }
        }

        snapTarget += delta;

        applySnap(direction, snapTarget, object, toDrag, horizontal);
        widthResizeSnapped = draggingWidth;
        horizontalResizeSnapped = horizontal;

        MessageManager::callAsync([this]() {
            updateMarker();
        });

        return dragOffset;
    };

    for (auto* object : snappable) {
        auto b1 = object->getBounds().reduced(Object::margin);

        auto topDiff = b1.getY() - b2.getY();
        auto bottomDiff = b1.getBottom() - b2.getBottom();
        auto leftDiff = b1.getX() - b2.getX();
        auto rightDiff = b1.getRight() - b2.getRight();

        if (resizeZone.isDraggingTopEdge() && std::abs(topDiff) < tolerance) {
            return snap(SnappedLeft, object, b1, false);
        }
        if (resizeZone.isDraggingBottomEdge() && std::abs(bottomDiff) < tolerance) {
            return snap(SnappedRight, object, b1, false);
        }

        if (resizeZone.isDraggingLeftEdge() && std::abs(leftDiff) < tolerance) {
            return snap(SnappedLeft, object, b1, true);
        }
        if (resizeZone.isDraggingRightEdge() && std::abs(rightDiff) < tolerance) {
            return snap(SnappedRight, object, b1, true);
        }
    }

    return dragOffset;
}
Point<int> ObjectGrid::performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds)
{
    if (!gridEnabled)
        return dragOffset;

    auto [snapGrid, snapEdges, snapCentres] = std::tuple<bool, bool, bool> { gridType & 1, gridType & 2, gridType & 4 };

    if (ModifierKeys::getCurrentModifiers().isShiftDown() || gridType == 0) {
        clear(0);
        clear(1);
        return dragOffset;
    }

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
    auto* constrainer = toDrag->getConstrainer();

    if (constrainer) {
        // Not great that we need to do this, but otherwise we don't really know the object bounds for sure
        constrainer->checkBounds(newResizeBounds, toDrag->originalBounds, limits,
            resizeZone.isDraggingTopEdge(), resizeZone.isDraggingLeftEdge(),
            resizeZone.isDraggingBottomEdge(), resizeZone.isDraggingRightEdge());
    }

    auto b2 = newResizeBounds.reduced(Object::margin);
    auto snappable = getSnappableObjects(toDrag);

    // Snap to Objects
    if (snapEdges && constrainer && constrainer->getFixedAspectRatio() != 0.0) {
        dragOffset = performFixedResize(toDrag, dragOffset, newResizeBounds, nonClippedBounds);
    } else if (snapEdges) {
        bool alreadySnappedVertically = isAlreadySnapped(false, dragOffset);
        bool alreadySnappedHorizontally = isAlreadySnapped(true, dragOffset);

        if (!alreadySnappedVertically) {
            for (auto* object : snappable) {
                auto b1 = object->getBounds().reduced(Object::margin);

                if (snapEdges && resizeZone.isDraggingTopEdge() && std::abs(b1.getY() - b2.getY()) < tolerance) {
                    auto dy = b1.getY() - b2.getY();
                    dragOffset.y = applySnap(SnappedLeft, dy + dragOffset.y, object, toDrag, false);
                }
                if (snapEdges && resizeZone.isDraggingBottomEdge() && std::abs(b1.getBottom() - b2.getBottom()) < tolerance) {
                    auto dy = b1.getBottom() - b2.getBottom();
                    dragOffset.y = applySnap(SnappedRight, dy + dragOffset.y, object, toDrag, false);
                }
            }
        }

        if (!alreadySnappedHorizontally) {
            for (auto* object : snappable) {

                auto b1 = object->getBounds().reduced(Object::margin);

                if (snapEdges && resizeZone.isDraggingLeftEdge() && std::abs(b1.getX() - b2.getX()) < tolerance) {
                    auto dx = b1.getX() - b2.getX();
                    dragOffset.x = applySnap(SnappedLeft, dx + dragOffset.x, object, toDrag, true);
                }
                if (snapEdges && resizeZone.isDraggingRightEdge() && std::abs(b1.getRight() - b2.getRight()) < tolerance) {
                    auto dx = b1.getRight() - b2.getRight();
                    dragOffset.x = applySnap(SnappedRight, dx + dragOffset.x, object, toDrag, true);
                }
            }
        }

        MessageManager::callAsync([this]() {
            updateMarker();
        });
    }

    if (!snapGrid) {
        return dragOffset;
    }

    // Snap to Grid
    Point<int> newPosTopLeft = toDrag->originalBounds.reduced(Object::margin).getTopLeft() + dragOffset;
    Point<int> newPosBotRight = toDrag->originalBounds.reduced(Object::margin).getBottomRight() + dragOffset;

    if (resizeZone.isDraggingLeftEdge() && !isAlreadySnapped(true, dragOffset)) {
        newPosTopLeft.setX(roundToInt(newPosTopLeft.getX() / gridSize + 1) * gridSize);
        snappedPosition.x = newPosTopLeft.x - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
        snappedPosition.x += (toDrag->cnv->canvasOrigin.x % gridSize) + 1;
        dragOffset.x = snappedPosition.x;
    }
    if (resizeZone.isDraggingTopEdge() && !isAlreadySnapped(false, dragOffset)) {
        newPosTopLeft.setY(roundToInt(newPosTopLeft.getY() / gridSize + 1) * gridSize);
        snappedPosition.y = newPosTopLeft.y - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
        snappedPosition.y += (toDrag->cnv->canvasOrigin.y % gridSize) + 1;
        dragOffset.y = snappedPosition.y;
    }
    if (resizeZone.isDraggingRightEdge() && !isAlreadySnapped(true, dragOffset)) {
        newPosBotRight.setX(roundToInt(newPosBotRight.getX() / gridSize + 1) * gridSize);
        snappedPosition.x = newPosBotRight.x - toDrag->originalBounds.reduced(Object::margin).getRight() - gridSize;
        snappedPosition.x += (toDrag->cnv->canvasOrigin.x % gridSize) + 1;
        dragOffset.x = snappedPosition.x;
    }
    if (resizeZone.isDraggingBottomEdge() && !isAlreadySnapped(false, dragOffset)) {
        newPosBotRight.setY(roundToInt(newPosBotRight.getY() / gridSize + 1) * gridSize);
        snappedPosition.y = newPosBotRight.y - toDrag->originalBounds.reduced(Object::margin).getBottom() - gridSize;
        snappedPosition.y += (toDrag->cnv->canvasOrigin.y % gridSize) + 1;
        dragOffset.y = snappedPosition.y;
    }

    MessageManager::callAsync([this]() {
        updateMarker();
    });

    return dragOffset;
}

Point<int> ObjectGrid::performMove(Object* toDrag, Point<int> dragOffset)
{
    if (!gridEnabled)
        return dragOffset;

    auto [snapGrid, snapEdges, snapCentres] = std::tuple<bool, bool, bool> { gridType & 1, gridType & 2, gridType & 4 };

    if (ModifierKeys::getCurrentModifiers().isShiftDown() || gridType == 0) {
        clear(0);
        clear(1);
        return dragOffset;
    }

    // Snap to Objects
    if (snapEdges || snapCentres) {
        auto snappable = getSnappableObjects(toDrag);
        auto b2 = (toDrag->originalBounds + dragOffset).reduced(Object::margin);

        if (!isAlreadySnapped(false, dragOffset)) {
            for (auto* object : snappable) {
                auto b1 = object->getBounds().reduced(Object::margin);

                start[0] = object;
                end[0] = toDrag;

                if (snapEdges && std::abs(b1.getY() - b2.getY()) < tolerance) {
                    dragOffset.y = applySnap(SnappedLeft, b1.getY() - b2.getY() + dragOffset.y, object, toDrag, false);
                    break;
                }
                if (snapCentres && std::abs(b1.getCentreY() - b2.getCentreY()) < tolerance) {
                    dragOffset.y = applySnap(SnappedCentre, b1.getCentreY() - b2.getCentreY() + dragOffset.y, object, toDrag, false);
                    break;
                }
                if (snapEdges && std::abs(b1.getBottom() - b2.getBottom()) < tolerance) {
                    dragOffset.y = applySnap(SnappedRight, b1.getBottom() - b2.getBottom() + dragOffset.y, object, toDrag, false);
                    break;
                }
            }
        }
        if (snapEdges && !isAlreadySnapped(true, dragOffset)) {

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
                    dragOffset.x = applySnap(SnappedConnection, snapDistance + dragOffset.x, connection->outlet, connection->inlet, true);
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

                if (snapEdges && std::abs(b1.getX() - b2.getX()) < tolerance) {
                    dragOffset.x = applySnap(SnappedLeft, b1.getX() - b2.getX() + dragOffset.x, object, toDrag, true);
                    break;
                }
                if (snapCentres && std::abs(b1.getCentreX() - b2.getCentreX()) < tolerance) {
                    dragOffset.x = applySnap(SnappedCentre, b1.getCentreX() - b2.getCentreX() + dragOffset.x, object, toDrag, true);
                    break;
                }
                if (snapEdges && std::abs(b1.getRight() - b2.getRight()) < tolerance) {
                    dragOffset.x = applySnap(SnappedRight, b1.getRight() - b2.getRight() + dragOffset.x, object, toDrag, true);
                    break;
                }
            }
        }

        MessageManager::callAsync([this]() {
            updateMarker();
        });

        if (!snapGrid) {
            return dragOffset;
        }
    }

    // Snap to Grid
    if (snapGrid) {
        Point<int> newPos = toDrag->originalBounds.reduced(Object::margin).getPosition() + dragOffset;
        if (!isAlreadySnapped(true, dragOffset)) {

            newPos.setX(floor(newPos.getX() / static_cast<float>(gridSize) + 1) * gridSize);
            newPos.x += (toDrag->cnv->canvasOrigin.x % gridSize) - 1;
            snappedPosition.x = newPos.x - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
        }
        if (!isAlreadySnapped(false, dragOffset)) {
            newPos.setY(floor(newPos.getY() / static_cast<float>(gridSize) + 1) * gridSize);
            newPos.y += (toDrag->cnv->canvasOrigin.y % gridSize) - 1;
            snappedPosition.y = newPos.y - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
        }

        MessageManager::callAsync([this]() {
            updateMarker();
        });

        return snappedPosition;
    }

    return dragOffset;
}

Array<Object*> ObjectGrid::getSnappableObjects(Object* draggedObject)
{
    auto& cnv = draggedObject->cnv;

    Array<Object*> snappable;

    auto scaleFactor = std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
    auto viewBounds = cnv->viewport.get()->getViewArea() / scaleFactor;

    for (auto* object : cnv->objects) {
        if (draggedObject == object || object->isSelected() || !viewBounds.intersects(object->getBounds()))
            continue; // don't look at dragged object, selected objects, or objects that are outside of view bounds

        snappable.add(object);
    }

    auto centre = draggedObject->getBounds().getCentre();

    std::sort(snappable.begin(), snappable.end(), [centre](Object* a, Object* b) {
        auto distA = a->getBounds().getCentre().getDistanceFrom(centre);
        auto distB = b->getBounds().getCentre().getDistanceFrom(centre);

        return distA < distB;
    });

    return snappable;
}

bool ObjectGrid::isAlreadySnapped(bool horizontal, Point<int>& dragOffset)
{
    if (horizontal && snapped[1]) {
        if (std::abs(snappedPosition.x - dragOffset.x) > range) {
            clear(true);
            return true;
        }
        dragOffset = { snappedPosition.x, dragOffset.y };
        return true;
    } else if (!horizontal && snapped[0]) {
        if (std::abs(snappedPosition.y - dragOffset.y) > range) {
            clear(false);
            return true;
        }
        dragOffset = { dragOffset.x, snappedPosition.y };
        return true;
    }

    return false;
}

void ObjectGrid::clearAll()
{
    clear(false);
    clear(true);
}

void ObjectGrid::propertyChanged(String const& name, var const& value)
{
    if (name == "grid_type") {
        gridType = static_cast<int>(value);
    }
    if (name == "grid_enabled") {
        gridEnabled = static_cast<int>(value);
    }
    if (name == "grid_size") {
        gridSize = static_cast<int>(value);
    }
}
