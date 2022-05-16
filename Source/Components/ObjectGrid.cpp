#include "ObjectGrid.h"
#include "Box.h"
#include "Connection.h"
#include "Canvas.h"
#include "LookAndFeel.h"

ObjectGrid::ObjectGrid()
{
    setStrokeThickness(1);
    setAlwaysOnTop(true);
}

Point<int> ObjectGrid::setState(GridType t, int i, Point<int> pos, Component* s, Component* e)
{
    type = t;
    idx = i;
    position = pos;
    start = s;
    end = e;
    updateMarker();

    return pos;
}

void ObjectGrid::updateMarker()
{
    if (type == NotSnappedToGrid || !start || !end)
    {
        setPath(std::move(Path()));
        return;
    }

    Path toDraw;

    if (type == ConnectionSnap)
    {
        auto b1 = start->getParentComponent()->getBounds();
        auto b2 = end->getParentComponent()->getBounds();

        toDraw.addLineSegment(Line<float>(b1.getX() - 2, b1.getBottom() + 2, b1.getX() - 2, b2.getY() - 2), 1.0f);
        setPath(toDraw);
        return;
    }

    auto b1 = start->getBounds().reduced(Box::margin);
    auto b2 = end->getBounds().reduced(Box::margin);

    auto t = b1.getY() < b2.getY() ? b1 : b2;
    auto b = b1.getY() > b2.getY() ? b1 : b2;
    auto l = b1.getX() < b2.getX() ? b1 : b2;
    auto r = b1.getX() > b2.getX() ? b1 : b2;

    auto gridLine = Line<float>();

    if (type == VerticalSnap)
    {
        if (orientation == SnappedLeft)
        {
            gridLine = Line<float>(l.getX(), t.getY(), r.getRight(), t.getY());
        }
        else if (orientation == SnappedRight)
        {
            gridLine = Line<float>(l.getX(), t.getBottom(), r.getRight(), t.getBottom());
        }
        else
        {  // snapped centre
            gridLine = Line<float>(l.getX(), t.getCentreY(), r.getRight(), t.getCentreY());
        }
    }
    else
    {
        if (orientation == SnappedLeft)
        {
            gridLine = Line<float>(l.getX(), t.getY(), l.getX(), b.getBottom());
        }
        else if (orientation == SnappedRight)
        {
            gridLine = Line<float>(l.getRight(), t.getY(), l.getRight(), b.getBottom());
        }
        else
        {  // snapped centre
            gridLine = Line<float>(l.getCentreX(), t.getY(), l.getCentreX(), b.getBottom());
        }
    }

    toDraw.addLineSegment(gridLine, 1.0f);
    setPath(toDraw);
}

void ObjectGrid::clear()
{
    type = NotSnappedToGrid;
    idx = 0;
    position = Point<int>();
    start = nullptr;
    end = nullptr;
    updateMarker();
}

Point<int> ObjectGrid::handleMouseDrag(Box* toDrag, Point<int> dragOffset, Rectangle<int> viewBounds)
{
    constexpr int tolerance = range / 2;

    setStrokeFill(FillType(findColour(PlugDataColour::highlightColourId)));

    // If object was snapped last time
    if (type != NotSnappedToGrid)
    {
        // Check if we've dragged out of the ObjectGrid snap
        bool horizontalSnap = type == HorizontalSnap || type == ConnectionSnap;
        bool horizontalUnsnap = horizontalSnap && std::abs(position.x - dragOffset.x) > range;
        bool verticalUnsnap = !horizontalSnap && std::abs(position.y - dragOffset.y) > range;

        if (horizontalUnsnap || verticalUnsnap)
        {
            clear();
            return dragOffset;
        }

        updateMarker();

        // Otherwise replace drag distance with the drag distance when we first snapped
        if (horizontalSnap)
        {
            return {position.x, dragOffset.y};
        }
        else
        {
            return {dragOffset.x, position.y};
        }
    }

    auto* cnv = toDrag->cnv;

    int totalSnaps = 0;  // Keep idx of object snapped to recognise when we've changed to a different target
    auto trySnap = [this, &totalSnaps, &tolerance](int distance) -> bool
    {
        if (std::abs(distance) < tolerance)
        {
            return true;
        }
        totalSnaps++;
        return false;
    };

    // Find snap points based on connection alignment
    for (auto* connection : toDrag->getConnections())
    {
        auto inletBounds = connection->inlet->getCanvasBounds();
        auto outletBounds = connection->outlet->getCanvasBounds();

        // Don't snap if the cord is upside-down
        if (inletBounds.getY() < outletBounds.getY()) continue;

        auto recentDragOffset = (toDrag->mouseDownPos + dragOffset) - toDrag->getPosition();
        if (connection->inbox == toDrag)
        {
            // Skip if both objects are selected
            if (cnv->isSelected(connection->outbox)) continue;
            inletBounds += recentDragOffset;
        }
        else
        {
            if (cnv->isSelected(connection->inbox)) continue;
            outletBounds += recentDragOffset;
        }

        int snapDistance = inletBounds.getX() - outletBounds.getX();

        // Check if the inlet or outlet is being moved, and invert if needed
        if (connection->inbox == toDrag) snapDistance = -snapDistance;

        if (trySnap(snapDistance))
        {
            return setState(ConnectionSnap, totalSnaps, {snapDistance + dragOffset.x, dragOffset.y}, connection->outlet, connection->inlet);
        }

        // If we're close, don't snap for other reasons
        if (std::abs(snapDistance) < tolerance * 2.0f)
        {
            return dragOffset;
        }
    }

    // Find snap points based on box alignment
    for (auto* box : cnv->boxes)
    {
        if (cnv->isSelected(box)) continue;  // don't look at selected objects

        if (!viewBounds.intersects(box->getBounds())) continue;  // if the box is out of viewport bounds

        auto b1 = box->getBounds().reduced(Box::margin);
        auto b2 = toDrag->getBounds().withPosition(toDrag->mouseDownPos + dragOffset).reduced(Box::margin);

        start = box;
        end = toDrag;

        auto t = b1.getY() < b2.getY() ? b1 : b2;
        auto b = b1.getY() > b2.getY() ? b1 : b2;
        auto r = b1.getX() < b2.getX() ? b1 : b2;
        auto l = b1.getX() > b2.getX() ? b1 : b2;

        if (trySnap(b1.getX() - b2.getX()))
        {
            orientation = SnappedLeft;
            return setState(HorizontalSnap, totalSnaps, Point<int>(b1.getX() - b2.getX(), 0) + dragOffset, box, toDrag);
        }
        if (trySnap(b1.getCentreX() - b2.getCentreX()))
        {
            orientation = SnappedCentre;
            setState(HorizontalSnap, totalSnaps, Point<int>(b1.getCentreX() - b2.getCentreX(), 0) + dragOffset, box, toDrag);
            return position;
        }
        if (trySnap(b1.getRight() - b2.getRight()))
        {
            orientation = SnappedRight;
            setState(HorizontalSnap, totalSnaps, Point<int>(b1.getRight() - b2.getRight(), 0) + dragOffset, box, toDrag);
            return position;
        }

        if (trySnap(b1.getY() - b2.getY()))
        {
            orientation = SnappedLeft;
            setState(VerticalSnap, totalSnaps, Point<int>(0, b1.getY() - b2.getY()) + dragOffset, box, toDrag);
            return position;
        }
        if (trySnap(b1.getCentreY() - b2.getCentreY()))
        {
            orientation = SnappedCentre;
            setState(VerticalSnap, totalSnaps, Point<int>(0, b1.getCentreY() - b2.getCentreY()) + dragOffset, box, toDrag);
            return position;
        }
        if (trySnap(b1.getBottom() - b2.getBottom()))
        {
            orientation = SnappedRight;
            setState(VerticalSnap, totalSnaps, Point<int>(0, b1.getBottom() - b2.getBottom()) + dragOffset, box, toDrag);
            return position;
        }
    }

    return dragOffset;
}

Point<int> ObjectGrid::handleMouseUp(Point<int> dragOffset)
{
    if (type == HorizontalSnap || type == ConnectionSnap)
    {
        dragOffset.x = position.x;
    }
    else if (type == VerticalSnap)
    {
        dragOffset.y = position.y;
    }

    clear();
    return dragOffset;
}
