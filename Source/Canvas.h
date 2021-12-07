#pragma once

#include <JuceHeader.h>
#include "Box.h"

#include "Utility/ValueTreeObject.h"
#include "PluginProcessor.h"
#include "Pd/PdPatch.hpp"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class TreeSorter
{
public:
    
    Array<ValueTree>* indexTree;
    
    TreeSorter(Array<ValueTree>* index) : indexTree(index) {
        
    }
    
    int compareElements (const ValueTree& first, const ValueTree& second)
    {
        auto firstIdx = indexTree->indexOf(first);
        auto secondIdx = indexTree->indexOf(second);
        return (firstIdx < secondIdx) ? -1 : ((secondIdx < firstIdx) ? 1 : 0);
    }
};

struct Identifiers
{
    // Object identifiers
    inline static Identifier canvas = Identifier("Canvas");
    inline static Identifier box = Identifier("Box");
    inline static Identifier edge = Identifier("Edge");
    inline static Identifier connection = Identifier("Connection");
    
    // Other identifiers
    inline static Identifier exists = Identifier("Exists");
    inline static Identifier isGraph = Identifier("Graph");
    
    // Box
    inline static Identifier boxX = Identifier("X");
    inline static Identifier boxY = Identifier("Y");
    inline static Identifier boxName = Identifier("Name");
    
    // Edge
    inline static Identifier edgeID = Identifier("ID");
    inline static Identifier edgeIdx = Identifier("Index");
    inline static Identifier edgeIsInput = Identifier("Input");
    inline static Identifier edgeSignal = Identifier("Signal");

    
    // Connection
    inline static Identifier startID = Identifier("StartID");
    inline static Identifier endID = Identifier("EndID");
    
    
    inline static Identifier hideHeaders = Identifier("HideHeaders");
    inline static Identifier connectionStyle = Identifier("ConnectionStyle");
    
};

class Edge;
class PlugDataPluginEditor;
class Canvas  : public Component, public ValueTreeObject, public KeyListener, public MultiComponentDraggerListener
{
public:
    
    static inline constexpr int guiUpdateMs = 500;
    
    //==============================================================================
    Canvas(ValueTree tree, PlugDataPluginEditor* parent);
    
    ~Canvas();
    
    PlugDataPluginEditor* main;
    
    ValueTreeObject* factory (const juce::Identifier&, const juce::ValueTree&) override;

    //==============================================================================
    void paintOverChildren (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    
    void createPatch();
    void loadPatch(pd::Patch& patch);
    
    void synchroniseAll();
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
    
    void dragCallback(int dx, int dy) override;
    
    void closeAllInstances();
      
    bool changed() {
        return hasChanged && findChildrenOfClass<Box>().size();
    }
    
    bool hasChanged = false;
    
    Edge* findEdgeByID(String ID);
    
    Array<Edge*> getAllEdges();
    
    Viewport* viewport = nullptr;
    
    bool connectingWithDrag = false;
    
    pd::Patch patch;
    
    // Our objects are bigger than pd's, so move everything apart by this factor
    static inline constexpr float zoomX = 3.0f;
    static inline constexpr float zoomY = 2.0f;
    
private:
    
    
    Point<int> lastMousePos;

    MultiComponentDragger<Box> dragger = MultiComponentDragger<Box>(this);

    LassoComponent<Box*> lasso;
    
    Point<int> dragStartPosition;
   
    PopupMenu popupMenu;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
