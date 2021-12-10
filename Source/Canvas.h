/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#pragma once

#include <JuceHeader.h>
#include "Box.h"
#include "PluginProcessor.h"
#include "Pd/PdPatch.hpp"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/



struct Identifiers
{
    inline static Identifier hideHeaders = Identifier("HideHeaders");
    inline static Identifier connectionStyle = Identifier("ConnectionStyle");
    
};

class GraphArea;
class Edge;
class PlugDataPluginEditor;
class PlugDataPluginProcessor;
class Canvas  : public Component, public KeyListener, public MultiComponentDraggerListener
{
public:
    
    static inline constexpr int guiUpdateMs = 25;
    
    //==============================================================================
    Canvas(PlugDataPluginEditor& parent, bool isGraph = false, bool isGraphChild = false);
    
    ~Canvas();
    
    PlugDataPluginEditor& main;
    PlugDataAudioProcessor& pd;
    
    //==============================================================================
    void paintOverChildren (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    
    void createPatch();
    void loadPatch(pd::Patch& patch);
    
    void synchronise();

    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;
    
    void copySelection();
    void removeSelection();
    void pasteSelection();
    void duplicateSelection();
    
    void checkBounds();

    void undo();
    void redo();
    
    void dragCallback(int dx, int dy) override;
    
    void closeAllInstances();
      
    bool changed() {
        return hasChanged && boxes.size();
    }
    
    bool hasChanged = false;
    
    Array<Edge*> getAllEdges();
    
    Viewport* viewport = nullptr;
    
    bool connectingWithDrag = false;
    
    pd::Patch patch;
    
    // Our objects are bigger than pd's, so move everything apart by this factor
    static inline constexpr float zoomX = 3.0f;
    static inline constexpr float zoomY = 2.0f;
    
    OwnedArray<Box> boxes;
    OwnedArray<Connection> connections;
    
    bool isGraph = false;
    bool isGraphChild = false;
    
    String title = "Untitled Patcher";
    
    MultiComponentDragger<Box> dragger = MultiComponentDragger<Box>(this, &boxes);
    
    std::unique_ptr<GraphArea> graphArea;
    
private:
    
    Point<int> dragStartPosition;
    Point<int> lastMousePos;
    
    LassoComponent<Box*> lasso;
    PopupMenu popupMenu;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
