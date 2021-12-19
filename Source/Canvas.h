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


extern juce::JUCEApplicationBase* juce_CreateApplication();

struct Identifiers
{
    inline static Identifier hideHeaders = Identifier("HideHeaders");
    inline static Identifier connectionStyle = Identifier("ConnectionStyle");
    
};

struct GraphArea;
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
    PlugDataAudioProcessor* pd;
    
    //==============================================================================
    void paintOverChildren (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    
    void createPatch();
    void loadPatch(pd::Patch patch);
    
    void synchronise();

    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;
    
    void copySelection();
    void removeSelection();
    void pasteSelection();
    void duplicateSelection();
    
    void paint(Graphics&) override;
    bool hitTest(int x, int y) override;

    void checkBounds();

    void undo();
    void redo();
    
    void dragCallback(int dx, int dy) override;
    
    void closeAllInstances();
      
    bool changed() {
        return hasChanged && boxes.size();
    }
    
    bool hasChanged = false;
    
    Viewport* viewport = nullptr;
    
    bool connectingWithDrag = false;
    
    pd::Patch patch;
    std::unique_ptr<PlugDataAudioProcessor> aux_instance;
    
    OwnedArray<Box> boxes;
    OwnedArray<Connection> connections;
    
    bool isGraph = false;
    bool isGraphChild = false;
    
    String title = "Untitled Patcher";
    
    MultiComponentDragger<Box> dragger = MultiComponentDragger<Box>(this, &boxes);
    
    std::unique_ptr<GraphArea> graphArea;
    
    // Layer to help us draw negative object positions
    Component transformLayer;
    
private:
    
    Point<int> dragStartPosition;
    Point<int> lastMousePos;
    
    
    LassoComponent<Box*> lasso;
    PopupMenu popupMenu;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};


// Graph bounds component
struct GraphArea : public Component, public ComponentDragger
{
    
    ResizableBorderComponent resizer;
    Canvas* canvas;
    
    Rectangle<int> startRect;
    
    GraphArea(Canvas* parent) : resizer(this, nullptr), canvas(parent) {

        addAndMakeVisible(resizer);
    }
    
    void paint(Graphics& g) override {
        g.setColour(MainLook::highlightColour);
        g.drawRect(getLocalBounds());
        
        g.setColour(MainLook::highlightColour.darker(0.8f));
        g.drawRect(getLocalBounds().reduced(6));
    }
    
    bool hitTest(int x, int y) override {
        return !getLocalBounds().reduced(8).contains(Point<int>{x, y});
    }
    
    void mouseMove(const MouseEvent& e) override {
        setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
    }
    
    void mouseDown(const MouseEvent& e) override {
        startDraggingComponent(this, e);
    }
    
    void mouseDrag(const MouseEvent& e) override {
        dragComponent(this, e, nullptr);
    }
    
    void mouseUp(const MouseEvent& e) override {
        t_canvas* cnv = canvas->patch.getPointer();
        // Lock first?
        if(cnv) {
            cnv->gl_pixwidth = getWidth();
            cnv->gl_pixheight = getHeight();
            
            cnv->gl_xmargin = (getX() - 4) / pd::Patch::zoom;
            cnv->gl_ymargin = (getY() - 4) / pd::Patch::zoom;
        
        }
        
        repaint();
    }
    
    void resized() override {
        
        t_canvas* cnv = canvas->patch.getPointer();
        // Lock first?
        if(cnv) {
            cnv->gl_pixwidth = getWidth();
            cnv->gl_pixheight = getHeight();
            cnv->gl_xmargin = (getX() - 4) / pd::Patch::zoom;
            cnv->gl_ymargin = (getY() - 4) / pd::Patch::zoom;
        }

        resizer.setBounds(getLocalBounds());
        repaint();
    }
};
