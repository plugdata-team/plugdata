#pragma once

#include "Tabbar.h"

class PluginEditor;
class Canvas;
class SplitViewResizer;
class SplitView : public Component {
    
public:
    SplitView(PluginEditor* parent);

    TabComponent* getActiveTabbar();
    TabComponent* getLeftTabbar();
    TabComponent* getRightTabbar();
    
    void setSplitEnabled(bool splitEnabled);
    
    void splitCanvasView(Canvas* cnv, bool splitviewFocus);
    
    void setFocus(Canvas* cnv);
    
    void closeEmptySplits();

private:
    
    void resized() override;
    
    int splitViewWidth = 0;
    bool splitView = false;
    int activeTabIndex = 0;
    bool splitFocusIndex = 0;
    TabComponent splits[2];
    
    PluginEditor* editor;
    
    std::unique_ptr<Component> splitViewResizer;
};
