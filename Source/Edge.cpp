/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Edge.h"

#include "Canvas.h"
#include "Connection.h"

//==============================================================================
Edge::Edge(Box* parent, bool input) : box(parent)
{
  isInput = input;
  setSize(8, 8);

  parent->addAndMakeVisible(this);

  onClick = [this]()
  {
    if (box->cnv->pd->locked) return;
    createConnection();
  };

  setBufferedToImage(true);
}

bool Edge::hasConnection()
{
  // Check if it has any connections
  for (auto* connection : box->cnv->connections)
  {
    if (connection->start == this || connection->end == this)
    {
      return true;
    }
  }

  return false;
}

Rectangle<int> Edge::getCanvasBounds()
{
  // Get bounds relative to canvas, used for positioning connections
  return getBounds() + getParentComponent()->getPosition();
}

//==============================================================================
void Edge::paint(Graphics& g)
{
  auto bounds = getLocalBounds().toFloat();

  auto backgroundColour = isSignal ? Colours::yellow : MainLook::highlightColour;

  auto baseColour = backgroundColour.darker(isOver() ? 0.15f : 0.0f);

  if (isDown() || isOver()) baseColour = baseColour.contrasting(isDown() ? 0.2f : 0.05f);

  Path path;

  // Visual change if it has a connection
  if (hasConnection())
  {
    if (isInput)
    {
      path.addPieSegment(bounds, MathConstants<float>::pi + MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
    }
    else
    {
      path.addPieSegment(bounds, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
    }
  }
  else
  {
    path.addEllipse(bounds);
  }

  g.setColour(baseColour);
  g.fillPath(path);
  g.setColour(findColour(ComboBox::outlineColourId));
  g.strokePath(path, PathStrokeType(1.f));
}

void Edge::resized() {}

void Edge::mouseDrag(const MouseEvent& e)
{
  // Ignore when locked
  if (box->cnv->pd->locked) return;

  // For dragging to create new connections
  TextButton::mouseDrag(e);
  if (!box->cnv->connectingEdge && e.getLengthOfMousePress() > 300)
  {
    box->cnv->connectingEdge = this;
    auto* cnv = findParentComponentOfClass<Canvas>();
    cnv->connectingWithDrag = true;
  }
}

void Edge::mouseMove(const MouseEvent& e) {}

void Edge::createConnection()
{
  // Check if this is the start or end action of connecting
  if (box->cnv->connectingEdge)
  {
    // Check type for input and output
    bool sameDirection = isInput == box->cnv->connectingEdge->isInput;

    bool connectionAllowed = box->cnv->connectingEdge->getParentComponent() != getParentComponent() && !sameDirection;

    // Don't create if this is the same edge
    if (box->cnv->connectingEdge == this)
    {
      box->cnv->connectingEdge = nullptr;
    }
    // Create new connection if allowed
    else if (connectionAllowed)
    {
      auto* cnv = findParentComponentOfClass<Canvas>();
      cnv->connections.add(new Connection(cnv, box->cnv->connectingEdge, this));
      box->cnv->connectingEdge = nullptr;
    }
  }
  // Else set this edge as start of a connection
  else
  {
    box->cnv->connectingEdge = this;
  }
}
