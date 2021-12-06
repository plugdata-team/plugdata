#include "Connection.h"
#include "Edge.h"
#include "Box.h"
#include "Canvas.h"

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
        parent->removeChild(tree);
        return;
    }

    if(start->getProperty(Identifiers::edgeIsInput)) {
        inIdx = start->getProperty(Identifiers::edgeIdx);
        outIdx = end->getProperty(Identifiers::edgeIdx);
        inObj = start->box->pdObject.get();
        outObj = end->box->pdObject.get();
    }
    else {
        inIdx = end->getProperty(Identifiers::edgeIdx);
        outIdx = start->getProperty(Identifiers::edgeIdx);
        inObj = end->box->pdObject.get();
        outObj = start->box->pdObject.get();
    }
        
    
    if(!getProperty(Identifiers::exists)) {
        bool canConnect = parent->patch.createConnection(outObj, outIdx, inObj, inIdx);

        if(!canConnect) {
            start = nullptr;
            end = nullptr;
            parent->removeChild(tree);
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
    deleteListeners();
}

void Connection::deleteListeners() {
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
    g.strokePath(path, PathStrokeType(3.5f, PathStrokeType::mitered, PathStrokeType::rounded));

    auto baseColour = Colours::white;

    if(isSelected) {
        baseColour = start->getProperty(Identifiers::edgeSignal) ? Colours::yellow : MainLook::highlightColour;
    }
    
    
    g.setColour(baseColour.withAlpha(0.8f));
    g.strokePath(path, PathStrokeType(1.5f, PathStrokeType::mitered, PathStrokeType::rounded));
    
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

    bool curvedConnection = true;
    if(auto* main = findParentComponentOfClass<PlugDataPluginEditor>()) {
        curvedConnection = !main->getProperty(Identifiers::connectionStyle);
    }

    int curvetype = fabs(pstart.x - pend.x) < (fabs(pstart.y - pend.y) * 5.0f) ? 1 : 2;
    
    curvetype *= !(fabs(pstart.x - pend.x) < 2 || fabs(pstart.y - pend.y) < 2);
    
    if (curvetype == 1 && curvedConnection) // smooth vertical lines
        path.cubicTo(pstart.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, fabs(pstart.y - pend.y) * 0.5f, pend.x, pend.y);
    else if (curvetype == 2 && curvedConnection) // smooth horizontal lines
        path.cubicTo(fabs(pstart.x - pend.x) * 0.5f, pstart.y, fabs(pstart.x - pend.x) * 0.5f, pend.y, pend.x, pend.y);
    else // Dont smooth when almost straight
        path.lineTo(pend.x, pend.y);
    
    repaint();
}


void Connection::componentBeingDeleted(Component& component) {
    deleteListeners();
    
    if(!deleted) {
        deleted = true;
        getParent().removeChild(getObjectState(), nullptr);
    }
}
