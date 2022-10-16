#include "ObjectGrid.h"
#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "LookAndFeel.h"

ObjectGrid::ObjectGrid(Canvas* parent)
{
    for (auto& line : gridLines) {
        parent->addAndMakeVisible(line);

        line.setStrokeThickness(1);
        line.setAlwaysOnTop(true);
    }
}

Point<int> ObjectGrid::setState(bool isSnapped, int i, Point<int> pos, Component* s, Component* e, bool horizontal)
{
    snapped[horizontal] = isSnapped;
    idx[horizontal] = i;
    position[horizontal] = pos;
    start[horizontal] = s;
    end[horizontal] = e;
    updateMarker();

    return pos;
}

void ObjectGrid::updateMarker()
{
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
    snapped[horizontal] = NotSnappedToGrid;
    idx[horizontal] = 0;
    position[horizontal] = Point<int>();
    start[horizontal] = nullptr;
    end[horizontal] = nullptr;
    updateMarker();
}

Point<int> ObjectGrid::handleMouseDrag(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds)
{
    gridLines[0].setStrokeFill(FillType(toDrag->findColour(PlugDataColour::canvasActiveColourId)));
    gridLines[1].setStrokeFill(FillType(toDrag->findColour(PlugDataColour::canvasActiveColourId)));

    // Check for snap points on both axes
    dragOffset = performVerticalSnap(toDrag, dragOffset, viewBounds);
    dragOffset = performHorizontalSnap(toDrag, dragOffset, viewBounds);

    // Update grid line when snapped
    // Async to make sure the objects position gets updated first...
    MessageManager::callAsync([this]() {
        updateMarker();
    });

    return dragOffset;
}

Point<int> ObjectGrid::performVerticalSnap(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds)
{
    auto* cnv = toDrag->cnv;

    if (snapped[0]) {
        if (std::abs(position[0].y - dragOffset.y) > range) {
            clear(false);
            return dragOffset;
        }

        return { dragOffset.x, position[0].y };
    }

    for (auto* object : cnv->objects) {
        if (cnv->isSelected(object))
            continue; // don't look at selected objects

        if (!viewBounds.intersects(object->getBounds()))
            continue; // if the object is out of viewport bounds

        auto b1 = object->getBounds().reduced(Object::margin);
        auto b2 = toDrag->getBounds().withPosition(toDrag->mouseDownPos + dragOffset).reduced(Object::margin);

        start[0] = object;
        end[0] = toDrag;

        if (trySnap(b1.getY() - b2.getY())) {
            orientation[0] = SnappedLeft;
            return setState(true, totalSnaps, Point<int>(0, b1.getY() - b2.getY()) + dragOffset, object, toDrag, false);
        }
        if (trySnap(b1.getCentreY() - b2.getCentreY())) {
            orientation[0] = SnappedCentre;
            return setState(true, totalSnaps, Point<int>(0, b1.getCentreY() - b2.getCentreY()) + dragOffset, object, toDrag, false);
        }
        if (trySnap(b1.getBottom() - b2.getBottom())) {
            orientation[0] = SnappedRight;
            return setState(true, totalSnaps, Point<int>(0, b1.getBottom() - b2.getBottom()) + dragOffset, object, toDrag, false);
        }
    }

    return dragOffset;
}

Point<int> ObjectGrid::performHorizontalSnap(Object* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds)
{

    auto* cnv = toDrag->cnv;

    // Check if already snapped
    if (snapped[1]) {
        if (std::abs(position[1].x - dragOffset.x) > range) {
            clear(true);
            return dragOffset;
        }

        return { position[1].x, dragOffset.y };
    }

    // Find snap points based on connection alignment
    for (auto* connection : toDrag->getConnections()) {
        auto inletBounds = connection->inlet->getCanvasBounds();
        auto outletBounds = connection->outlet->getCanvasBounds();

        // Don't snap if the cord is upside-down
        if (inletBounds.getY() < outletBounds.getY())
            continue;

        auto recentDragOffset = (toDrag->mouseDownPos + dragOffset) - toDrag->getPosition();
        if (connection->inobj == toDrag) {
            // Skip if both objects are selected
            if (cnv->isSelected(connection->outobj))
                continue;
            inletBounds += recentDragOffset;
        } else {
            if (cnv->isSelected(connection->inobj))
                continue;
            outletBounds += recentDragOffset;
        }

        int snapDistance = inletBounds.getX() - outletBounds.getX();

        // Check if the inlet or outlet is being moved, and invert if needed
        if (connection->inobj == toDrag)
            snapDistance = -snapDistance;

        if (trySnap(snapDistance)) {
            orientation[1] = SnappedConnection;
            return setState(ConnectionSnap, totalSnaps, { snapDistance + dragOffset.x, dragOffset.y }, connection->outlet, connection->inlet, true);
        }

        // If we're close, don't snap for other reasons
        if (std::abs(snapDistance) < tolerance * 2.0f) {
            return dragOffset;
        }
    }

    for (auto* object : cnv->objects) {
        if (cnv->isSelected(object))
            continue; // don't look at selected objects

        if (!viewBounds.intersects(object->getBounds()))
            continue; // if the object is out of viewport bounds

        auto b1 = object->getBounds().reduced(Object::margin);
        auto b2 = toDrag->getBounds().withPosition(toDrag->mouseDownPos + dragOffset).reduced(Object::margin);

        start[1] = object;
        end[1] = toDrag;

        auto t = b1.getY() < b2.getY() ? b1 : b2;
        auto b = b1.getY() > b2.getY() ? b1 : b2;
        auto r = b1.getX() < b2.getX() ? b1 : b2;
        auto l = b1.getX() > b2.getX() ? b1 : b2;

        if (trySnap(b1.getX() - b2.getX())) {
            orientation[1] = SnappedLeft;
            return setState(true, totalSnaps, Point<int>(b1.getX() - b2.getX(), 0) + dragOffset, object, toDrag, true);
        }
        if (trySnap(b1.getCentreX() - b2.getCentreX())) {
            orientation[1] = SnappedCentre;
            return setState(true, totalSnaps, Point<int>(b1.getCentreX() - b2.getCentreX(), 0) + dragOffset, object, toDrag, true);
        }
        if (trySnap(b1.getRight() - b2.getRight())) {
            orientation[1] = SnappedRight;
            return setState(true, totalSnaps, Point<int>(b1.getRight() - b2.getRight(), 0) + dragOffset, object, toDrag, true);
        }
    }

    return dragOffset;
}

Point<int> ObjectGrid::handleMouseUp(Point<int> dragOffset)
{
    if (snapped[1]) {
        dragOffset.x = position[1].x;
        clear(1);
    }

    if (snapped[0]) {
        dragOffset.y = position[0].y;
        clear(0);
    }

    return dragOffset;
}

bool ObjectGrid::trySnap(int distance)
{
    if (std::abs(distance) < tolerance) {
        return true;
    }
    totalSnaps++;
    return false;
}
