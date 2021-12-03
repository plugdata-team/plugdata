#pragma once


#include "Utility/MultiComponentDragger.h"
#include "Utility/ValueTreeObject.h"

//#include "../../Source/Engine.hpp"

#include "GUIObjects.h"

#include <m_pd.h>
#include <JuceHeader.h>



//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */


class Box;
struct ClickLabel : Label
{
    
    Box* box;
    
    bool isDown = false;
    
    ClickLabel(Box* parent, MultiComponentDragger<Box>& multi_dragger) : box(parent), dragger(multi_dragger) {};
    
    void mouseDown(const MouseEvent & e) override;
    void mouseUp(const MouseEvent & e) override;
    void mouseDrag(const MouseEvent & e) override;
    
    MultiComponentDragger<Box>& dragger;
};

class Canvas;
class Box  : public Component, public ValueTreeObject
{
    
public:
    //==============================================================================
    Box(Canvas* parent, ValueTree tree, MultiComponentDragger<Box>& multiDragger);

    ~Box() override;
    
    Canvas* cnv;
    
    std::unique_ptr<GUIComponent> graphics = nullptr;
    
    ValueTreeObject* factory (const juce::Identifier&, const juce::ValueTree&) override;
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void moved() override;
    
    void updatePosition();
    
    void setType(String new_type);
    
    void setLimits(std::tuple<int, int, int, int> limits);
    
    void updatePorts();
    
    void remove(bool clear_pd = true);
    
    t_pd* pd_object = nullptr;
    
    void mouseMove(const MouseEvent& e) override;
    
    void valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    
    int numInputs = 0;
    int numOutputs = 0;
    
    bool is_pdinfo = false;
    
    ComponentBoundsConstrainer restrainer;
    std::tuple<int, int, int, int> defaultLimits = {40, 32, 100, 32};
    
    ClickLabel textLabel;
    
    std::unique_ptr<ResizableBorderComponent> resizer = nullptr;
    
private:

    MultiComponentDragger<Box>& dragger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};
