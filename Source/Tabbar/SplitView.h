/*
 // Copyright (c) 2021-2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SplitViewResizer.h"
#include "ResizableTabbedComponent.h"

class PluginEditor;
class Canvas;
class SplitViewFocusOutline;
class SplitView : public Component {
public:
    explicit SplitView(PluginEditor* parent);
    ~SplitView() override;

    TabComponent* getActiveTabbar();

    void createNewSplit(Canvas* cnv);
    void addSplit(ResizableTabbedComponent* toSplit);
    void addResizer(SplitViewResizer* resizer);

    void removeSplit(TabComponent* toRemove);

    bool canSplit();

    void setFocus(ResizableTabbedComponent* selectedTabComponent);

    void closeEmptySplits();

    int getTabComponentSplitIndex(TabComponent* tabComponent);

    ResizableTabbedComponent* getSplitAtScreenPosition(Point<int> position);

    OwnedArray<ResizableTabbedComponent> splits;
    OwnedArray<SplitViewResizer> resizers;

    float splitViewWidth = 0.5f;
    void resized() override;

private:
    Rectangle<int> selectedSplit;
    SafePointer<ResizableTabbedComponent> activeTabComponent = nullptr;
    ResizableTabbedComponent* rootComponent;

    std::unique_ptr<SplitViewFocusOutline> focusOutline;

    PluginEditor* editor;

    std::unique_ptr<Component> splitViewResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitView)
};
