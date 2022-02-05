/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "MultiComponentDragger.h"

#include "BoxEditor.h"
#include "Edge.h"
#include "GUIObjects.h"

#include <JuceHeader.h>
#include <m_pd.h>

class Canvas;
class Box : public Component, public ChangeListener {

public:
    //==============================================================================
    explicit Box(Canvas* parent, const String& name = "", Point<int> position = { 100, 100 });

    Box(pd::Object* object, Canvas* parent, const String& name = "", Point<int> position = { 100, 100 });

    ~Box() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

    void updatePorts();
    
    std::unique_ptr<pd::Object> pdObject = nullptr;

    int numInputs = 0;
    int numOutputs = 0;
    bool locked = false;
    
    ClickLabel textLabel;

    Canvas* cnv;

    std::unique_ptr<GUIComponent> graphics = nullptr;

    OwnedArray<Edge> edges;

    ComponentBoundsConstrainer restrainer;
    ResizableBorderComponent resizer;

    void setType(const String& newType, bool exists = false);


    
private:
    void initialise();

    bool hitTest(int x, int y) override;
    
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;


    Colour outline = MainLook::highlightColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
};
