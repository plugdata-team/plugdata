#include "Edge.h"
#include "Canvas.h"
#include "Connection.h"

//==============================================================================
Edge::Edge(Box* parent, bool input) : box(parent)
{
    isInput = input;
    setSize (8, 8);
    
    parent->addAndMakeVisible(this);
    
    onClick = [this](){
        createConnection();
    };
}

Edge::~Edge()
{
}

bool Edge::hasConnection() {
    for(auto* connection : box->cnv->connections) {
        if(connection->start == this || connection->end == this) {
            return true;
        }
    }
    
    return false;
    
}

Rectangle<int> Edge::getCanvasBounds()
{
    auto bounds = getBounds();
    if(auto* box = dynamic_cast<Box*>(getParentComponent())) {
        bounds += box->getPosition();
    }
    
    return bounds;
}

//==============================================================================
void Edge::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    auto backgroundColour = isSignal ? Colours::yellow : MainLook::highlightColour;
    
    auto baseColour = backgroundColour.withMultipliedSaturation (hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);
    
    if (isDown() || isOver())
        baseColour = baseColour.contrasting (isDown() ? 0.2f : 0.05f);

    Path path;
    
    if(hasConnection()) {
        //path.addArc(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, MathConstants<float>::pi);
        if(isInput) {
            path.addPieSegment(bounds, MathConstants<float>::pi + MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
        }
        else {
            path.addPieSegment(bounds, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.3f);
        }
        
    }
    else {
        path.addEllipse(bounds);
    }
    
    
    g.setColour (baseColour);
    g.fillPath (path);
    g.setColour (findColour (ComboBox::outlineColourId));
    g.strokePath (path, PathStrokeType (1.f));
}

void Edge::resized()
{
}


void Edge::mouseDrag(const MouseEvent& e) {
    // For dragging to create new connections
    
    TextButton::mouseDrag(e);
    if(!Edge::connectingEdge && e.getLengthOfMousePress() > 300) {
        Edge::connectingEdge = this;
        auto* cnv = findParentComponentOfClass<Canvas>();
        cnv->connectingWithDrag = true;
    }
}

void Edge::mouseMove(const MouseEvent& e) {
    findParentComponentOfClass<Canvas>()->repaint();
}

void Edge::createConnection()
{
    // Check if this is the start or end action of connecting
    if(Edge::connectingEdge) {
        
        // Check type for input and output
        bool startSignal = isInput ? connectingEdge->isSignal : isSignal;
        bool endSignal =  !isInput ? connectingEdge->isSignal : isSignal;
        
        bool connectionAllowed = connectingEdge->getParentComponent() != getParentComponent() && Edge::connectingEdge->box->cnv == box->cnv;
        
        // Dont create if this is the same edge
        if(Edge::connectingEdge == this) {
            Edge::connectingEdge = nullptr;
        }
        // Create new connection if allowed
        else if(connectionAllowed) {
            Canvas* cnv = findParentComponentOfClass<Canvas>();
            cnv->connections.add(new Connection(cnv, connectingEdge, this));
            Edge::connectingEdge = nullptr;
        }
    }
    // Else set this edge as start of a connection
    else {
        Edge::connectingEdge = this;
    }
}
