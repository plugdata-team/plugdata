#include "Connection.h"
#include "Edge.h"
#include "Box.h"
#include "Canvas.h"
//#include "../../Source/Library.hpp"

//==============================================================================
Connection::Connection(Canvas* parent, ValueTree tree) : ValueTreeObject(tree)
{
    cnv = parent;
    rebuildObjects();
   
    start = cnv->findEdgeByID(tree.getProperty("StartID"));
    end = cnv->findEdgeByID(tree.getProperty("EndID"));
    
    if(!start || !end) {
        start = nullptr;
        end = nullptr;
        parent->getState().removeChild(tree, nullptr);
        return;
    }

    if(start->ValueTreeObject::getState().getProperty(Identifiers::edge_in)) {
        inIdx = start->ValueTreeObject::getState().getProperty(Identifiers::edge_idx);
        outIdx = end->ValueTreeObject::getState().getProperty(Identifiers::edge_idx);
        inObj = start->box->pd_object;
        outObj = end->box->pd_object;
    }
    else {
        inIdx = end->ValueTreeObject::getState().getProperty(Identifiers::edge_idx);
        outIdx = start->ValueTreeObject::getState().getProperty(Identifiers::edge_idx);
        inObj = end->box->pd_object;
        outObj = start->box->pd_object;
    }
        
    
    if(!getState().getProperty(Identifiers::exists)) {
        bool can_connect = parent->patch.createConnection(outObj, outIdx, inObj, inIdx);

        if(!can_connect) {
            start = nullptr;
            end = nullptr;
            parent->getState().removeChild(tree, nullptr);
            return;
        }
    }
    
    start->addComponentListener(this);
    end->addComponentListener(this);
    
    setInterceptsMouseClicks(false, false);
    
    setSize (600, 400);
    
    componentMovedOrResized(*start, true, true);
    componentMovedOrResized(*end, true, true);
    
    resized();
    repaint();
}

Connection::~Connection()
{
    delete_listeners();
}

void Connection::delete_listeners() {
    if(start) {
        start->removeComponentListener(this);
    }
    if(end) {
        end->removeComponentListener(this);
    }
}
//==============================================================================
void Connection::paint (Graphics& g)
{
    g.setColour(Colours::grey);
    g.strokePath(path, PathStrokeType(3.0f));

    auto base_colour = Colours::white;

    if(isSelected) {
        
        base_colour = start->ValueTreeObject::getState().getProperty("Context") ? Colours::yellow : MainLook::highlight_colour;
    }
    
    g.setColour(base_colour);
    g.strokePath(path, PathStrokeType(1.5f));
}

void Connection::mouseDown(const MouseEvent& e)  {
    if(path.contains(e.getPosition().toFloat())) {
        isSelected = !isSelected;
    }
}


void Connection::componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) {
    int left = std::min(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) - 10;
    int top = std::min(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) - 10;
    int right = std::max(start->getCanvasBounds().getCentreX(), end->getCanvasBounds().getCentreX()) + 10;
    int bottom = std::max(start->getCanvasBounds().getCentreY(), end->getCanvasBounds().getCentreY()) + 10;
    
    setBounds(left, top, right - left, bottom - top);
    resized();
    repaint();
}

void Connection::resized()
{
    Point<float> pstart = start->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    Point<float> pend = end->getCanvasBounds().getCentre().toFloat() - getPosition().toFloat();
    path.clear();
    path.startNewSubPath(pstart.x, pstart.y);

    int curvetype = fabs(pstart.x - pend.x) < (fabs(pstart.y - pend.y) * 5.0f) ? 1 : 2;
    
    curvetype *= !(fabs(pstart.x - pend.x) < 2 || fabs(pstart.y - pend.y) < 2);
    
    if (curvetype == 1) // smooth vertical lines
        path.cubicTo(pstart.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, pend.y);
    else if (curvetype == 2) // smooth horizontal lines
        path.cubicTo(fabs(pstart.x - pend.x) * 0.5f, pstart.y, fabs(pstart.x - pend.x) * 0.5f, pend.y, pend.x, pend.y);
    else // Dont smooth when almost straight
        path.lineTo(pend.x, pend.y);
}


void Connection::componentBeingDeleted(Component& component) {
    delete_listeners();
    
    if(!deleted) {
        deleted = true;
        getState().getParent().removeChild(getState(), nullptr);
    }
}
