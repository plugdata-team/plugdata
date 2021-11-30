#pragma once

#include <JuceHeader.h>
#include "Utility/gin_valuetreeobject.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class Box;
class Edge  : public TextButton, public ValueTreeObject
{
    
public:
    
    Box* box;
    
    Edge(ValueTree tree, Box* parent);
    ~Edge() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    
    void createConnection();
    
    Rectangle<int> getCanvasBounds();
    
    static inline SafePointer<Edge> connectingEdge = nullptr;
    
private:



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Edge)
};
