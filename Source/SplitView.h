#pragma once

#include "SplitViewResizer.h"
#include "ResizableTabbedComponent.h"


class PluginEditor;
class Canvas;
class FadeAnimation;
class SplitView : public Component {
public:
    explicit SplitView(PluginEditor* parent);
    ~SplitView() override;

    TabComponent* getActiveTabbar();

    //void setSplitEnabled(bool splitEnabled);
    //bool isSplitEnabled() const;
    //bool isRightTabbarActive() const;

    void splitCanvasesAfterIndex(int idx, bool direction);
    void splitCanvasView(Canvas* cnv, bool splitviewFocus);

    void addSplit(ResizableTabbedComponent* toSplit);
    void addResizer(SplitViewResizer* resizer);

    void removeSplit(TabComponent* toRemove);

    bool canSplit();

    void setFocus(ResizableTabbedComponent* selectedTabComponent);
    //void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
    //bool hasFocus(Canvas* cnv);

    void closeEmptySplits();

    void paintOverChildren(Graphics& g) override;

    int getTabComponentSplitIndex(TabComponent* tabComponent);

    //void setSplitFocusIndex(int index);

    //bool isSplitView() { return splitView; };

    OwnedArray<ResizableTabbedComponent> splits;
    OwnedArray<SplitViewResizer> resizers;

    float splitViewWidth = 0.5f;
    void resized() override;
private:
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    bool splitView = false;
    int activeTabIndex = 0;
    bool splitFocusIndex = false;

    Rectangle<int> selectedSplit;
    ResizableTabbedComponent* activeTabComponent = nullptr;
    ResizableTabbedComponent* rootComponent;

    std::unique_ptr<FadeAnimation> fadeAnimation;
    std::unique_ptr<FadeAnimation> fadeAnimationLeft;
    std::unique_ptr<FadeAnimation> fadeAnimationRight;

    bool splitviewIndicator = false;

    PluginEditor* editor;

    std::unique_ptr<Component> splitViewResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitView)
};
