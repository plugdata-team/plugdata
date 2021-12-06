#include "Edge.h"
#include "Canvas.h"
#include "Connection.h"

//==============================================================================
Edge::Edge(ValueTree tree, Box* parent) : ValueTreeObject(tree), box(parent)
{
    setSize (8, 8);
    
    onClick = [this](){
        createConnection();
    };
}

Edge::~Edge()
{
}

Array<Component::SafePointer<Connection>> Edge::getConnections() {
    Array<Component::SafePointer<Connection>> connections;
    for(auto* connection : box->cnv->findChildrenOfClass<Connection>()) {
        if(connection->start == this || connection->end == this) {
            connections.add(connection);
        }
    }
    
    return connections;
}

bool Edge::hasConnection() {
    for(auto* connection : box->cnv->findChildrenOfClass<Connection>()) {
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
    
    auto backgroundColour = getProperty(Identifiers::edgeSignal) ? Colours::yellow : MainLook::highlightColour;
    
    auto baseColour = backgroundColour.withMultipliedSaturation (hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);
    
    if (isDown() || isOver())
        baseColour = baseColour.contrasting (isDown() ? 0.2f : 0.05f);

    Path path;
    
    if(hasConnection()) {
        //path.addArc(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, MathConstants<float>::pi);
        if((bool)getProperty(Identifiers::edgeIsInput)) {
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
        bool isInput = getProperty("Input");
        String ctx1 = isInput ? Edge::connectingEdge->getProperty(Identifiers::edgeSignal) : getProperty(Identifiers::edgeSignal);
        String ctx2 = !isInput ? Edge::connectingEdge->getProperty(Identifiers::edgeSignal) : getProperty(Identifiers::edgeSignal);
        
        
        bool connectionAllowed = connectingEdge->getParentComponent() != getParentComponent() && Edge::connectingEdge->box->cnv == box->cnv;
        
        // Dont create if this is the same edge
        if(Edge::connectingEdge == this) {
            Edge::connectingEdge = nullptr;
        }
        // Create new connection if allowed
        else if(connectionAllowed) {
            ValueTree newConnection = ValueTree(Identifiers::connection);
            
            newConnection.setProperty(Identifiers::startID, Edge::connectingEdge->getProperty(Identifiers::edgeID), nullptr);
            newConnection.setProperty(Identifiers::endID, getProperty(Identifiers::edgeID), nullptr);
            Canvas* cnv = findParentComponentOfClass<Canvas>();
            cnv->appendChild(newConnection);
            Edge::connectingEdge = nullptr;
        }
    }
    // Else set this edge as start of a connection
    else {
        Edge::connectingEdge = this;
    }
}
