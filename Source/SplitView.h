#pragma once

#include "Tabbar.h"

class PluginEditor;
class Canvas;
class SplitViewResizer;
class SplitView : public Component
                , private Timer {
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

    bool splitviewIndicator = false;
    float indicatorAlpha;
    float alphaTarget;
    void timerCallback() override
    {
        float const stepSize = 0.03f;
        if (alphaTarget > indicatorAlpha) {
            indicatorAlpha += stepSize;
            if (indicatorAlpha >= alphaTarget) {
                indicatorAlpha = alphaTarget;
                stopTimer();
            }
        } else if (alphaTarget < indicatorAlpha) {
            indicatorAlpha -= stepSize;
            if (indicatorAlpha <= alphaTarget) {
                indicatorAlpha = alphaTarget;
                stopTimer();
            }
        }
        repaint();
    }
    float fadeIn()
    {
        alphaTarget = 0.3f;
        if (!isTimerRunning())
            startTimerHz(60);

        return indicatorAlpha;
    }

    float fadeOut()
    {
        alphaTarget = 0.0f;
        if (!isTimerRunning())
            startTimerHz(60);

        return indicatorAlpha;
    }

    PluginEditor* editor;

    std::unique_ptr<Component> splitViewResizer;
};
