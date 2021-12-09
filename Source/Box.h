#pragma once

#include "Utility/MultiComponentDragger.h"

#include "GUIObjects.h"
#include "BoxEditor.h"
#include "Edge.h"

#include <m_pd.h>
#include <JuceHeader.h>


//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */


class Canvas;
class Box  : public Component
{
    
public:
    //==============================================================================
    Box(Canvas* parent, String name = "");
    
    Box(pd::Object* object, Canvas* parent, String name = "");

    ~Box() override;
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void moved() override;
    
    void updatePosition();
    
    void setLimits(std::tuple<int, int, int, int> limits);
    
    void updatePorts();
    
    std::unique_ptr<pd::Object> pdObject = nullptr;
    
    int numInputs = 0;
    int numOutputs = 0;
    
    ClickLabel textLabel;
    
    std::unique_ptr<ResizableBorderComponent> resizer = nullptr;
    
    Canvas* cnv;
    
    std::unique_ptr<GUIComponent> graphics = nullptr;
    
    MultiComponentDragger<Box>& dragger;
    
    OwnedArray<Edge> edges;
    
    void setType(String newType, bool exists = false);
    
private:
    
    void initialise();
    
    void mouseMove(const MouseEvent& e) override;
    
    ComponentBoundsConstrainer restrainer;
    std::tuple<int, int, int, int> defaultLimits = {40, 32, 100, 32};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};
