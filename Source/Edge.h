/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class Connection;
class Box;
class Edge  : public TextButton
{
    
public:
    
    Box* box;
    
    Edge(Box* parent, bool isInput);
    ~Edge() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    
    void createConnection();
    
    
    bool hasConnection();
    
    Rectangle<int> getCanvasBounds();
    
    static inline SafePointer<Edge> connectingEdge = nullptr;
    
    int edgeIdx;
    bool isInput;
    bool isSignal;
    
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Edge)
};
