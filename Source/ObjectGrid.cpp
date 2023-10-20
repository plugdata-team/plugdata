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

ObjectGrid::ObjectGrid(Canvas* cnv) {
    
    gridEnabled = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
    gridType = SettingsFile::getInstance()->getProperty<int>("grid_type");
    gridSize = SettingsFile::getInstance()->getProperty<int>("grid_size");
    
    cnv->addAndMakeVisible(gridLines[0]);
    cnv->addAndMakeVisible(gridLines[1]);
    
    gridLines[0].setAlwaysOnTop(true);
    gridLines[1].setAlwaysOnTop(true);
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

Point<int> ObjectGrid::performMove(Object* toDrag, Point<int> dragOffset)
{
    if (ModifierKeys::getCurrentModifiers().isShiftDown() || gridType == 0) {
        clearIndicators();
        return dragOffset;
    }
    
    auto [snapGrid, snapEdges, snapCentres] = std::tuple<bool, bool, bool> { gridType & 1, gridType & 2, gridType & 4 };
    
    auto snappable = getSnappableObjects(toDrag);
    
    Point<int> distance;
    Line<int> verticalIndicator, horizontalIndicator;
    bool connectionSnapped = false;
    
    // Check for straight connections to snap to
    if(snapEdges) {
        for (auto* connection : toDrag->getConnections()) {
            
            if(connection->inobj == toDrag)
            {
                if(!snappable.contains(connection->outobj)) continue;
                
                auto outletBounds = connection->outobj->getBounds() + connection->outlet->getPosition();
                auto inletBounds = connection->inobj->originalBounds + dragOffset + connection->inlet->getPosition();
                outletBounds = outletBounds.withSize(12, 12);
                inletBounds = inletBounds.withSize(8, 8);
                if(outletBounds.getY() > inletBounds.getY()) continue;
                
                auto snapDistance = inletBounds.getX() - outletBounds.getX();
                if(std::abs(snapDistance) < connectionTolerance)  {
                    distance.x = snapDistance;
                    horizontalIndicator = {outletBounds.getX() - 2, outletBounds.getBottom() + 4, outletBounds.getX() - 2, inletBounds.getY() - 4};
                    connectionSnapped = true;
                }
                break;
            }
            else if(connection->outobj == toDrag)
            {
                if(!snappable.contains(connection->inobj)) continue;
                
                auto inletBounds = connection->inobj->getBounds() + connection->inlet->getPosition();
                auto outletBounds = connection->outobj->originalBounds + dragOffset + connection->outlet->getPosition();
                outletBounds = outletBounds.withSize(12, 12);
                inletBounds = inletBounds.withSize(8, 8);
                if(outletBounds.getY() > inletBounds.getY()) continue;
                
                auto snapDistance = inletBounds.getX() - outletBounds.getX();
                if(std::abs(snapDistance) < connectionTolerance)  {
                    distance.x = -snapDistance;
                    horizontalIndicator = {inletBounds.getX() - 2, outletBounds.getBottom() + 4, inletBounds.getX() - 2, inletBounds.getY() - 4};
                    connectionSnapped = true;
                }
                break;
            }
        }
    }
    
    auto desiredBounds = toDrag->originalBounds.reduced(Object::margin) + dragOffset;
    auto actualBounds = toDrag->getBounds().reduced(Object::margin);
    
    bool objectSnapped = false;
    // Check for relative object snap
    for(auto* object : snappable) {
        auto b1 = object->getBounds().reduced(Object::margin);
        auto topDiff = b1.getY() - desiredBounds.getY();
        auto bottomDiff = b1.getBottom() - desiredBounds.getBottom();
        auto leftDiff = b1.getX() - desiredBounds.getX();
        auto rightDiff = b1.getRight() - desiredBounds.getRight();
        
        auto vCentreDiff = b1.getCentreY() - desiredBounds.getCentreY();
        auto hCentreDiff = b1.getCentreX() - desiredBounds.getCentreX();
        
        if (snapEdges && std::abs(topDiff) < objectTolerance) {
            verticalIndicator = getObjectIndicatorLine(Top, b1, actualBounds.withY(b1.getY()));
            distance.y = topDiff;
            objectSnapped = true;
        }
        else if (snapEdges && std::abs(bottomDiff) < objectTolerance) {
            verticalIndicator = getObjectIndicatorLine(Bottom, b1, actualBounds.withBottom(b1.getBottom()));
            distance.y = bottomDiff;
            objectSnapped = true;
        }
        else if (snapCentres && std::abs(vCentreDiff) < objectTolerance) {
            verticalIndicator = getObjectIndicatorLine(VerticalCentre, b1, actualBounds.withCentre({actualBounds.getCentreX(), b1.getCentreY()}));
            distance.y = vCentreDiff;
            objectSnapped = true;
        }
        
        // Skip horizontal snap if we've already found a connection snap
        if(!connectionSnapped) {
            if (snapEdges && std::abs(leftDiff) < objectTolerance) {
                horizontalIndicator = getObjectIndicatorLine(Left, b1, actualBounds.withX(b1.getX()));
                distance.x = leftDiff;
                objectSnapped = true;
            }
            else if (snapEdges && std::abs(rightDiff) < objectTolerance) {
                horizontalIndicator = getObjectIndicatorLine(Right, b1, actualBounds.withRight(b1.getRight()));
                distance.x = rightDiff;
                objectSnapped = true;
            }
            else if (snapCentres && std::abs(hCentreDiff) < objectTolerance) {
                horizontalIndicator = getObjectIndicatorLine(HorizontalCentre, b1, actualBounds.withCentre({b1.getCentreX(), actualBounds.getCentreY()}));
                distance.x = hCentreDiff;
                objectSnapped = true;
            }
        }
    }
    
    // Snap to absolute grid
    if (snapGrid && !objectSnapped && !connectionSnapped) {
        Point<int> newPos = toDrag->originalBounds.reduced(Object::margin).getPosition() + dragOffset;

        newPos.setX(floor(newPos.getX() / static_cast<float>(gridSize) + 1) * gridSize);
        newPos.x += (toDrag->cnv->canvasOrigin.x % gridSize) - 1;
        dragOffset.x = newPos.x - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
        
        newPos.setY(floor(newPos.getY() / static_cast<float>(gridSize) + 1) * gridSize);
        newPos.y += (toDrag->cnv->canvasOrigin.y % gridSize) - 1;
        dragOffset.y = newPos.y - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
    }

    if(objectSnapped || connectionSnapped) {
        auto lineScale = 0.5f / std::max(1.0f, getValue<float>(toDrag->cnv->zoomScale));
        setIndicator(0, verticalIndicator, lineScale);
        setIndicator(1, horizontalIndicator, lineScale);
        return dragOffset + distance;
    }
    
    clearIndicators();
    
    return dragOffset;
}

Point<int> ObjectGrid::performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds)
{
    if (ModifierKeys::getCurrentModifiers().isShiftDown() || gridType == 0) {
        clearIndicators();
        return dragOffset;
    }
    
    auto [snapGrid, snapEdges, snapCentres] = std::tuple<bool, bool, bool> { gridType & 1, gridType & 2, gridType & 4 };
    
    auto limits = [&]() -> Rectangle<int> {
        return { Canvas::infiniteCanvasSize, Canvas::infiniteCanvasSize };
    }();
    
    auto resizeZone = toDrag->resizeZone;
    auto isDraggingTop = resizeZone.isDraggingTopEdge();
    auto isDraggingBottom = resizeZone.isDraggingBottomEdge();
    auto isDraggingLeft = resizeZone.isDraggingLeftEdge();
    auto isDraggingRight = resizeZone.isDraggingRightEdge();
    
    auto nonClippedBounds = newResizeBounds;
    
    // Not great that we need to do this, but otherwise we don't really know the object bounds for sure
    toDrag->getConstrainer()->checkBounds(newResizeBounds, toDrag->originalBounds, limits,
                                          isDraggingTop, isDraggingLeft, isDraggingBottom, isDraggingRight);

    // Returns non-zero if the object has a fixed ratio
    auto ratio = toDrag->getConstrainer()->getFixedAspectRatio();

    // In case of a fixed ratio, calculate the dominant drag direction
    auto wDiff = nonClippedBounds.getWidth() - toDrag->originalBounds.getWidth();
    auto hDiff = nonClippedBounds.getHeight() - toDrag->originalBounds.getHeight();
    bool draggingWidth =  wDiff > (hDiff * ratio);

    auto desiredBounds = newResizeBounds.reduced(Object::margin);
    auto actualBounds = toDrag->getBounds().reduced(Object::margin);
    
    bool snapped = false;
    Line<int> verticalIndicator, horizontalIndicator;
    Point<int> distance;
    
    if(snapEdges) {
        // Check for objects to relative snap to
        for(auto* object : getSnappableObjects(toDrag)) {
            auto b1 = object->getBounds().reduced(Object::margin);
            auto topDiff = b1.getY() - desiredBounds.getY();
            auto bottomDiff = b1.getBottom() - desiredBounds.getBottom();
            auto leftDiff = b1.getX() - desiredBounds.getX();
            auto rightDiff = b1.getRight() - desiredBounds.getRight();
            
            if (isDraggingTop && std::abs(topDiff) < objectTolerance) {
                verticalIndicator = getObjectIndicatorLine(Top, b1, actualBounds.withY(b1.getY()));
                if(draggingWidth && ratio != 0)
                {
                    if(isDraggingRight) distance.x = topDiff * ratio;
                    if(isDraggingLeft) distance.x = -topDiff * ratio;
                }
                else {
                    distance.y = topDiff;
                }
                snapped = true;
            }
            else if (isDraggingBottom && std::abs(bottomDiff) < objectTolerance) {
                verticalIndicator = getObjectIndicatorLine(Bottom, b1, actualBounds.withBottom(b1.getBottom()));
                if(draggingWidth && ratio != 0)
                {
                    if(isDraggingRight) distance.x = bottomDiff * ratio;
                    if(isDraggingLeft) distance.x = -bottomDiff * ratio;
                }
                else {
                    distance.y = bottomDiff;
                }
                snapped = true;
            }
            if(approximatelyEqual(ratio, 0.0) || !snapped) {
                if (isDraggingLeft && std::abs(leftDiff) < objectTolerance) {
                    horizontalIndicator = getObjectIndicatorLine(Left, b1, actualBounds.withX(b1.getX()));
                    if(!draggingWidth && ratio != 0)
                    {
                        if(isDraggingBottom) distance.y = -leftDiff / ratio;
                        if(isDraggingTop) distance.y = leftDiff / ratio;
                    }
                    else {
                        distance.x = leftDiff;
                    }
                    snapped = true;
                }
                else if (isDraggingRight && std::abs(rightDiff) < objectTolerance) {
                    horizontalIndicator = getObjectIndicatorLine(Right, b1, actualBounds.withRight(b1.getRight()));
                    if(!draggingWidth && ratio != 0)
                    {
                        if(isDraggingBottom) distance.y = rightDiff / ratio;
                        if(isDraggingTop) distance.y = -rightDiff / ratio;
                    }
                    else {
                        distance.x = rightDiff;
                    }
                    snapped = true;
                }
            }
        }
        
        if(snapped) {
            auto lineScale = 0.5f / std::max(1.0f, getValue<float>(toDrag->cnv->zoomScale));
            setIndicator(0, verticalIndicator, lineScale);
            setIndicator(1, horizontalIndicator, lineScale);
            return dragOffset + distance;
        }
    }
    
    if(snapGrid)
    {
           Point<int> newPosTopLeft = toDrag->originalBounds.reduced(Object::margin).getTopLeft() + dragOffset;
           Point<int> newPosBotRight = toDrag->originalBounds.reduced(Object::margin).getBottomRight() + dragOffset;

            if (isDraggingTop) {
                auto newY = roundToInt(newPosTopLeft.getY() / gridSize + 1) * gridSize;
                dragOffset.y = newY - toDrag->originalBounds.reduced(Object::margin).getY() - gridSize;
                dragOffset.y += (toDrag->cnv->canvasOrigin.y % gridSize) + 1;
            }
            if (isDraggingBottom) {
                auto newY = roundToInt(newPosBotRight.getY() / gridSize + 1) * gridSize;
                dragOffset.y = newY - toDrag->originalBounds.reduced(Object::margin).getBottom() - gridSize;
                dragOffset.y += (toDrag->cnv->canvasOrigin.y % gridSize) + 1;
            }
           if (isDraggingLeft) {
               auto newX = roundToInt(newPosTopLeft.getX() / gridSize + 1) * gridSize;
               dragOffset.x = newX - toDrag->originalBounds.reduced(Object::margin).getX() - gridSize;
               dragOffset.x += (toDrag->cnv->canvasOrigin.x % gridSize) + 1;
           }

           if (isDraggingRight) {
               auto newX = roundToInt(newPosBotRight.getX() / gridSize + 1) * gridSize;
               dragOffset.x = newX - toDrag->originalBounds.reduced(Object::margin).getRight() - gridSize;
               dragOffset.x += (toDrag->cnv->canvasOrigin.x % gridSize) + 1;
           }
    }

    clearIndicators();
    
    return dragOffset;
}

// Calculates the path of the grid lines
Line<int> ObjectGrid::getObjectIndicatorLine(Side side, Rectangle<int> b1, Rectangle<int> b2)
{
    switch (side) {
        case Left:
            if (b1.getY() > b2.getY()) {
                return {b2.getTopLeft(), b1.getBottomLeft()};
            } else {
                return {b1.getTopLeft(), b2.getBottomLeft()};
            }
        case Right:
            if (b1.getY() > b2.getY()) {
                return {b2.getTopRight(), b1.getBottomRight()};
            } else {
                return {b1.getTopRight(), b2.getBottomRight()};
            }
        case Top:
            if (b1.getX() > b2.getX()) {
                return {b2.getTopLeft(), b1.getTopRight()};
            } else {
                return {b1.getTopLeft(), b2.getTopRight()};
            }
        case Bottom:
            if (b1.getX() > b2.getX()) {
                return {b2.getBottomLeft(), b1.getBottomRight()};
            } else {
                return {b1.getBottomLeft(), b2.getBottomRight()};
            }
        case VerticalCentre:
            if (b1.getX() > b2.getX()) {
                
                return {b2.getX(), b2.getCentreY(), b1.getRight(), b1.getCentreY()};
            } else {
                return {b1.getX(), b1.getCentreY(), b2.getRight(), b2.getCentreY()};
            }
        case HorizontalCentre:
            if (b1.getY() > b2.getY()) {
                return {b2.getCentreX(), b2.getY(), b1.getCentreX(), b1.getBottom()};
            } else {
                return {b1.getCentreX(), b1.getY(), b2.getCentreX(), b2.getBottom()};
            }
    }
}

void ObjectGrid::clearIndicators()
{
    gridLineAnimator.fadeOut(&gridLines[0], 100);
    gridLineAnimator.fadeOut(&gridLines[1], 100);
    
    gridLines[0].setPath(Path());
    gridLines[1].setPath(Path());
}

void ObjectGrid::setIndicator(int idx, Line<int> line, float scale) {
    auto lineIsEmpty = line.getLength() == 0;
    if(gridLines[idx].isVisible() && lineIsEmpty) {
        gridLineAnimator.fadeOut(&gridLines[idx], 100);
    }
    
    auto& lnf = LookAndFeel::getDefaultLookAndFeel();
    gridLines[idx].setStrokeFill(FillType(lnf.findColour(PlugDataColour::gridLineColourId)));
    gridLines[idx].setStrokeThickness(1);
    gridLines[idx].toFront(false);

    Path toDraw;
    toDraw.addLineSegment(line.toFloat(), scale);
    gridLines[idx].setPath(toDraw);
    
    if(!gridLines[idx].isVisible() && !lineIsEmpty) {
        gridLineAnimator.fadeIn(&gridLines[idx], 75);
    }
}
