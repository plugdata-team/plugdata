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
    bool isSplitEnabled();
    int getCurrentSplitIndex();
    
    void splitCanvasesAfterIndex(int idx, bool direction);
    void splitCanvasView(Canvas* cnv, bool splitviewFocus);
    
    void setFocus(Canvas* cnv);
    bool hasFocus(Canvas* cnv);

    void closeEmptySplits();

private:
    void resized() override;

    float splitViewWidth = 0.5f;
    bool splitView = false;
    int activeTabIndex = 0;
    bool splitFocusIndex = 0;
    TabComponent splits[2];

    PluginEditor* editor;

    std::unique_ptr<Component> splitViewResizer;
};
