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
    bool isRightTabbarActive();

    void splitCanvasesAfterIndex(int idx, bool direction);
    void splitCanvasView(Canvas* cnv, bool splitviewFocus);

    void setFocus(Canvas* cnv);
    bool hasFocus(Canvas* cnv);

    void closeEmptySplits();

    void paintOverChildren(Graphics& g) override;

private:
    void resized() override;

    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    float splitViewWidth = 0.5f;
    bool splitView = false;
    int activeTabIndex = 0;
    bool splitFocusIndex = 0;
    TabComponent splits[2];

    bool drawSplitviewIndicator = false;
    bool splitviewAtMouseup = false;

    PluginEditor* editor;

    std::unique_ptr<Component> splitViewResizer;
};
