/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include "Tabbar.h"
#include "Utility/SplitModeEnum.h"

class TabComponent;
class PluginEditor;
class SplitViewResizer;
class ResizableTabbedComponent : public Component
    , public DragAndDropTarget {
public:
    ResizableTabbedComponent(PluginEditor* editor, TabComponent* splitTabComponent = nullptr);

    ~ResizableTabbedComponent();

    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;

    void resized() override;
    void paintOverChildren(Graphics& g) override;

    void itemDragMove(SourceDetails const& dragSourceDetails) override;
    bool isInterestedInDragSource(SourceDetails const& dragSourceDetails) override;
    void itemDropped(SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(SourceDetails const& dragSourceDetails) override;
    void itemDragExit(SourceDetails const& dragSourceDetails) override;

    void updateSubViewSizes();

    TabComponent* getTabComponent();

    void setBoundsWithFactors(Rectangle<int> mainBounds);

    SplitViewResizer* resizerLeft = nullptr;
    SplitViewResizer* resizerRight = nullptr;
    SplitViewResizer* resizerTop = nullptr;
    SplitViewResizer* resizerBottom = nullptr;

    enum DropZones { Left = 1,
        Top = 2,
        Right = 4,
        Bottom = 8,
        Centre = 16,
        TabBar = 32,
        None = 64 };

    void updateDropZones();

    void moveToSplit(int splitIdx, Canvas* canvas);
    void createNewSplit(DropZones activeZone, Canvas* canvas);

private:
    Split::SplitMode splitMode = Split::SplitMode::None;

    float dragFactor = 0.5f;

    int findZoneFromSource(SourceDetails const& dragSourceDetails);

    void moveTabToNewSplit(SourceDetails const& source);

    String getZoneName(int zone);

    int splitWidth = 0;

    int activeZone = -1;
    bool isDragAndDropOver = false;

    std::unique_ptr<TabComponent> tabComponent;

    PluginEditor* editor;

    std::tuple<DropZones, Path> dropZones[6];

    Rectangle<int> oldObjectBounds;
    Rectangle<int> oldBounds;

    float const topFactorDefault = 0.0f;
    float const bottomFactorDefault = 1.0f;
    float const leftFactorDefault = 0.0f;
    float const rightFactorDefault = 1.0f;

    float topFactor = 0.0f;
    float bottomFactor = 1.0f;
    float leftFactor = 0.0f;
    float rightFactor = 1.0f;

    Rectangle<int> splitBoundsTop, splitBoundsBottom, splitBoundsRight, splitBoundsLeft, splitBoundsFull;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizableTabbedComponent)
};
