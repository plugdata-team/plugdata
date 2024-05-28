/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"

#include "Utility/GlobalMouseListener.h"
#include "Components/BouncingViewport.h"
#include "Components/Buttons.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "Components/ZoomableDragAndDropContainer.h"

class PluginEditor;

class ButtonBar : public TabbedButtonBar
    , public DragAndDropTarget
    , public ChangeListener {
public:
    ButtonBar(TabComponent& tabComp, TabbedButtonBar::Orientation o);

    bool isInterestedInDragSource(SourceDetails const& dragSourceDetails) override;
    void itemDropped(SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(SourceDetails const& dragSourceDetails) override;
    void itemDragExit(SourceDetails const& dragSourceDetails) override;
    void itemDragMove(SourceDetails const& dragSourceDetails) override;

    void currentTabChanged(int newCurrentTabIndex, String const& newTabName) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    TabBarButton* createTabButton(String const& tabName, int tabIndex) override;

    int getNumVisibleTabs();

    ComponentAnimator ghostTabAnimator;

private:
    TabComponent& owner;

    class GhostTab;
    std::unique_ptr<GhostTab> ghostTab;
    int ghostTabIdx = -1;
    bool inOtherSplit = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonBar)
};

class TabComponent : public Component
    , public AsyncUpdater {

    MainToolbarButton newButton = MainToolbarButton(Icons::Add);
    
    PluginEditor* editor;

public:
    TabComponent(PluginEditor* editor);
    ~TabComponent() override;

    void onTabChange(int tabIndex);
    void newTab();
    void addTab(String const& tabName, Component* contentComponent, int insertIndex);
    void moveTab(int oldIndex, int newIndex);
    void clearTabs();
    void setTabBarDepth(int newDepth);
    Component* getTabContentComponent(int tabIndex) const noexcept;
    Component* getCurrentContentComponent() const noexcept { return panelComponent.get(); }
    int getCurrentTabIndex();
    void setCurrentTabIndex(int idx);
    int getNumTabs() const noexcept { return tabs->getNumTabs(); }
    int getNumVisibleTabs();
    void removeTab(int idx);
    int getTabBarDepth() const noexcept { return tabDepth; }
    void changeCallback(int newCurrentTabIndex, String const& newTabName);

    void currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName);
    void handleAsyncUpdate() override;
    void resized() override;

    void paint(Graphics& g) override;

    void paintOverChildren(Graphics& g) override;

    int getIndexOfCanvas(Canvas* cnv);

    void setTabText(int tabIndex, String const& newName);
    String getTabText(int tabIndex);

    Canvas* getCanvas(int idx);

    Canvas* getCurrentCanvas();

    void setFocused();

    PluginEditor* getEditor();

    Image tabSnapshot;
    ScaledImage tabSnapshotScaled;

private:

    int tabDepth = 30;

    friend ButtonBar;

    Array<WeakReference<Component>> contentComponents;
    std::unique_ptr<TabbedButtonBar> tabs;
    WeakReference<Component> panelComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabComponent)
};
