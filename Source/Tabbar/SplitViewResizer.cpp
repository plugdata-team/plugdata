/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "SplitViewResizer.h"
#include "ResizableTabbedComponent.h"

SplitViewResizer::SplitViewResizer(ResizableTabbedComponent* originalComponent, ResizableTabbedComponent* newComponent, Split::SplitMode mode, int flipped)
{
    setAlwaysOnTop(true);

    splitMode = mode;
    resizerPosition = 0.5f;
    auto resizerRightPostion = 1.0f;
    auto resizerLeftPosition = 0.0f;

    if (auto originalRight = originalComponent->resizerRight) {
        resizerRightPostion = originalRight->resizerPosition;
    }

    if (auto originalLeft = originalComponent->resizerLeft) {
        resizerLeftPosition = originalLeft->resizerPosition;
    }

    resizerPosition = (resizerRightPostion - resizerLeftPosition) * 0.5f + resizerLeftPosition;

    if (flipped == 1) {
        splits[0] = originalComponent;
        splits[1] = newComponent;
    } else if (flipped == 0) {
        splits[0] = newComponent;
        splits[1] = originalComponent;
    }
    setWantsKeyboardFocus(false);
}

MouseCursor SplitViewResizer::getMouseCursor()
{
    switch (splitMode) {
    case Split::SplitMode::Horizontal:
        return MouseCursor::LeftRightResizeCursor;
    case Split::SplitMode::Vertical:
        return MouseCursor::UpDownResizeCursor;
    default:
        return MouseCursor::NormalCursor;
    }
}

bool SplitViewResizer::hitTest(int x, int y)
{
    if (splitMode == Split::SplitMode::None)
        return false;
    return resizeArea.contains(Point<int>(x, y));
}

void SplitViewResizer::resized()
{
    if (splitMode == Split::SplitMode::None)
        return;

    if (splitMode == Split::SplitMode::Horizontal) {
        resizeArea.setBounds(getWidth() * resizerPosition, 0, thickness, getHeight());
    } else {
        resizeArea.setBounds(0, getHeight() * resizerPosition, getWidth(), thickness);
    }

#if (ENABLE_SPLIT_RESIZER_DEBBUGING == 1)
    repaint();
#endif
}

void SplitViewResizer::paint(Graphics& g)
{
#if (ENABLE_SPLIT_RESIZER_DEBBUGING == 1)
    if (splitMode == Split::SplitMode::None)
        return;

    g.setColour(Colours::red);
    g.fillRect(resizeArea);
#endif
}

void SplitViewResizer::mouseDown(MouseEvent const& e)
{
    dragPositionX = e.position.x;
    dragPositionY = e.getPosition().getY();

    leftBounds = getParentComponent()->getBounds().getX();
}

bool SplitViewResizer::setResizerPosition(float newPosition, bool checkLeft)
{
    auto left = splits[0]->resizerLeft;
    auto right = splits[1]->resizerRight;

    auto leftPos = 0.0f;
    auto rightPos = 1.0f;

    if (left)
        leftPos = left->resizerPosition;
    if (right)
        rightPos = right->resizerPosition;

    // check calculated sise of all resizers, if they are less than min width, don't resize.
    // if we hit a resizer that is null, this means we are at an edge

    // check all to left
    if ((newPosition / getWidth()) - leftPos < (minimumWidth / getWidth()) && checkLeft == true) {

        if (splits[0]->resizerLeft) {
            hitEdge = splits[0]->resizerLeft->setResizerPosition(newPosition - minimumWidth);
        } else {
            return true;
        }

        if (hitEdge)
            return true;

        resizeArea.setPosition(newPosition, 0);
        resizerPosition = newPosition / getWidth();
        return false;
    }
    // check all to right
    if (rightPos - (newPosition / getWidth()) < (minimumWidth / getWidth())) {
        if (splits[1]->resizerRight) {
            hitEdge = splits[1]->resizerRight->setResizerPosition(newPosition + minimumWidth, false);
        } else {
            return true;
        }

        if (hitEdge)
            return true;

        resizeArea.setPosition(newPosition, 0);
        resizerPosition = newPosition / getWidth();
        return false;
    }

    resizeArea.setPosition(newPosition, 0);
    resizerPosition = newPosition / getWidth();
    return false;
}

void SplitViewResizer::mouseDrag(MouseEvent const& e)
{
    if (rateReducer.tooFast())
        return;

    auto posX = e.position.x;

    if (splitMode == Split::SplitMode::Horizontal) {
        hitEdge = false;
        setResizerPosition(posX);
        getParentComponent()->resized();
    }

#if (ENABLE_SPLIT_RESIZER_DEBBUGING == 1)
    repaint();
#endif
}
