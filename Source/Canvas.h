#pragma once

#include <JuceHeader.h>
#include "Box.h"

#include "Utility/gin_valuetreeobject.h"
#include "PlugData.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

struct Identifiers
{
    // Object identifiers
    inline static Identifier canvas = Identifier("Canvas");
    inline static Identifier box = Identifier("Box");
    inline static Identifier edge = Identifier("Edge");
    inline static Identifier connection = Identifier("Connection");
    
    // Other identifiers
    inline static Identifier exists = Identifier("Exists");
    inline static Identifier is_graph = Identifier("Graph");
    
    // Box
    inline static Identifier box_x = Identifier("X");
    inline static Identifier box_y = Identifier("Y");
    inline static Identifier box_name = Identifier("Name");
    
    // Edge
    inline static Identifier edge_id = Identifier("ID");
    inline static Identifier edge_idx = Identifier("Index");
    inline static Identifier edge_in = Identifier("Input");
    inline static Identifier edge_ctx = Identifier("Context");

    
    // Connection
    inline static Identifier start_id = Identifier("StartID");
    inline static Identifier end_id = Identifier("EndID");
    
};

class Edge;
class MainComponent;
class Canvas  : public Component, public ValueTreeObject, public KeyListener
{
public:
    //==============================================================================
    Canvas(ValueTree tree, MainComponent* parent);
    
    ~Canvas();
    
    MainComponent* main;
    
    ValueTreeObject* factory (const juce::Identifier&, const juce::ValueTree&) override;

    //==============================================================================
    void paintOverChildren (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    
    void loadPatch(String patch);
    void loadPatch(pd::Patch& patch);
    
    void synchronise();

    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;
    
    void copySelection();
    void removeSelection();
    void pasteSelection();
    void duplicateSelection();
    
    void undo();
    void redo();
    
    void valueTreeChanged() override {
        hasChanged = true;
    }
    
    bool changed() {
        return hasChanged && findChildrenOfClass<Box>().size();
    }
    
    bool hasChanged = false;
    
    Edge* findEdgeByID(String ID);
    
    Array<Edge*> getAllEdges();
    
    Viewport viewport;
    
    bool connectingWithDrag = false;
    
    bool isMainPatch = true;

    pd::Patch patch;
    
private:


    
    MultiComponentDragger<Box> dragger = MultiComponentDragger<Box>(this);

    LassoComponent<Box*> lasso;
    
    Point<int> dragStartPosition;
   
    PopupMenu popupMenu;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
