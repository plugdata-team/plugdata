#pragma once

#include "Utility/ZoomableDragAndDropContainer.h"
#include "Utility/StackDropShadower.h"
#include "PluginProcessor.h"

class PluginMode;
class TabComponent final : public Component
    , public DragAndDropTarget
    , public AsyncUpdater {
    class TabBarButtonComponent;

public:
    explicit TabComponent(PluginEditor* editor);

    ~TabComponent() override;

    Canvas* newPatch();

    Canvas* openPatch(const URL& path);
    Canvas* openPatch(String const& patchContent);
    Canvas* openPatch(pd::Patch::Ptr existingPatch, bool warnIfAlreadyOpen = false);
    void openPatch();

    void openInPluginMode(pd::Patch::Ptr patch);

    void renderArea(NVGcontext* nvg, Rectangle<int> bounds);

    void nextTab();
    void previousTab();

    void askToCloseTab(Canvas* cnv);
    void closeTab(Canvas* cnv);
    void showTab(Canvas* cnv, int splitIndex = 0);
    void setActiveSplit(Canvas* cnv);

    void updateNow();

    void closeAllTabs(
        bool quitAfterComplete = false, Canvas* patchToExclude = nullptr, std::function<void()> afterComplete = [] { });
    Canvas* createNewWindow(Canvas* cnv);
    void createNewWindowFromTab(Component* tab);

    Canvas* getCurrentCanvas();
    Canvas* getCanvasAtScreenPosition(Point<int> screenPosition);

    SmallArray<Canvas*> getCanvases();

    using VisibleCanvasArray = SmallArray<Canvas*, 2>;
    VisibleCanvasArray getVisibleCanvases();

private:
    void clearCanvases();
    void handleAsyncUpdate() override;

    void sendTabUpdateToVisibleCanvases();

    void resized() override;

    void moveToLeftSplit(TabComponent::TabBarButtonComponent* tab);
    void moveToRightSplit(TabComponent::TabBarButtonComponent* tab);

    void saveTabPositions();
    void closeEmptySplits();

    bool isInterestedInDragSource(SourceDetails const& dragSourceDetails) override;
    void itemDropped(SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(SourceDetails const& dragSourceDetails) override;
    void itemDragExit(SourceDetails const& dragSourceDetails) override;
    void itemDragMove(SourceDetails const& dragSourceDetails) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;

    void addLastShownTab(Canvas* tab, int split);
    Canvas* getLastShownTab(Canvas* current, int split);

    void showHiddenTabsMenu(int splitIndex);

    StackArray<MainToolbarButton, 2> newTabButtons = { MainToolbarButton(Icons::Add), MainToolbarButton(Icons::Add) };
    StackArray<MainToolbarButton, 2> tabOverflowButtons = { MainToolbarButton(Icons::ThinDown), MainToolbarButton(Icons::ThinDown) };

    StackArray<OwnedArray<TabBarButtonComponent>, 2> tabbars;
    StackArray<SafePointer<Canvas>, 2> splits = { nullptr, nullptr };

    StackArray<SmallArray<SafePointer<Canvas>, 12>, 2> lastShownTabs;

    StackArray<pd::Patch*, 2> lastSplitPatches { nullptr, nullptr };
    t_glist* lastActiveCanvas = nullptr;

    bool draggingOverTabbar = false;
    bool draggingSplitResizer = false;
    Rectangle<int> splitDropBounds;

    float splitProportion = 2;
    int splitSize = 0;
    int activeSplitIndex = 0;

    OwnedArray<Canvas> canvases;

    PluginEditor* editor;
    PluginProcessor* pd;

    struct TabVisibilityMessageUpdater final : public AsyncUpdater {
        explicit TabVisibilityMessageUpdater(TabComponent* parent)
            : parent(parent)
        {
        }

        void handleAsyncUpdate() override;
        TabComponent* parent;
    };

    TabVisibilityMessageUpdater tabVisibilityMessageUpdater = TabVisibilityMessageUpdater(this);
};
