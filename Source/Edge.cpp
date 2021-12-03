#include "Edge.h"
#include "Canvas.h"
//#include "../../Source/Library.hpp"

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
    
    auto background_colour = ValueTreeObject::getProperty(Identifiers::edgeSignal) ? Colours::yellow : MainLook::highlight_colour;
    
    auto base_colour = background_colour.withMultipliedSaturation (hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);

    
    if (isDown() || isOver())
        base_colour = base_colour.contrasting (isDown() ? 0.2f : 0.05f);

    
    Path path;
    path.addEllipse(bounds);
    
    g.setColour (base_colour);
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
        bool is_input = ValueTreeObject::getProperty("Input");
        String ctx1 = is_input ? Edge::connectingEdge->ValueTreeObject::getProperty(Identifiers::edgeSignal) : ValueTreeObject::getProperty(Identifiers::edgeSignal);
        String ctx2 = !is_input ? Edge::connectingEdge->ValueTreeObject::getProperty(Identifiers::edgeSignal) : ValueTreeObject::getProperty(Identifiers::edgeSignal);
        
        
        bool connection_allowed = ctx1 == ctx2 && connectingEdge->getParentComponent() != getParentComponent() && Edge::connectingEdge->box->cnv == box->cnv;
        
        // Dont create if this is the same edge
        if(Edge::connectingEdge == this) {
            Edge::connectingEdge = nullptr;
        }
        else if(!connection_allowed) {
            Edge::connectingEdge = nullptr;
        }
        // Create new connection if allowed
        else if(connection_allowed) {
            ValueTree new_connection = ValueTree(Identifiers::connection);
            
            new_connection.setProperty(Identifiers::start_id, Edge::connectingEdge->ValueTreeObject::getProperty(Identifiers::edgeID), nullptr);
            new_connection.setProperty(Identifiers::end_id, ValueTreeObject::getProperty(Identifiers::edgeID), nullptr);
            Canvas* cnv = findParentComponentOfClass<Canvas>();
            cnv->appendChild(new_connection);
            Edge::connectingEdge = nullptr;
        }
    }
    // Else set this edge as start of a connection
    else {
        Edge::connectingEdge = this;
    }
}
