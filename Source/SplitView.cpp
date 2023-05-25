#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "SplitView.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"

class FadeAnimation : private Timer {
public:
    explicit FadeAnimation(SplitView* splitView)
        : splitView(splitView)
    {
    }

    float fadeIn()
    {
        targetAlpha = 0.3f;
        if (!isTimerRunning() && currentAlpha < targetAlpha)
            startTimerHz(60);

        return currentAlpha;
    }

    float fadeOut()
    {
        targetAlpha = 0.0f;
        if (!isTimerRunning() && currentAlpha > targetAlpha)
            startTimerHz(60);

        return currentAlpha;
    }

private:
    void timerCallback() override
    {
        float const stepSize = 0.025f;
        if (targetAlpha > currentAlpha) {
            currentAlpha += stepSize;
            if (currentAlpha >= targetAlpha) {
                currentAlpha = targetAlpha;
                stopTimer();
            }
        } else if (targetAlpha < currentAlpha) {
            currentAlpha -= stepSize;
            if (currentAlpha <= targetAlpha) {
                currentAlpha = targetAlpha;
                stopTimer();
            }
        }
        if (splitView != nullptr)
            splitView->repaint();
    }

private:
    SplitView* splitView;
    float currentAlpha = 0.0f;
    float targetAlpha = 0.0f;
};

class SplitViewResizer : public Component {
public:
    static inline constexpr int width = 6;
    std::function<void(int)> onMove = [](int) {};

    SplitViewResizer()
    {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        setAlwaysOnTop(true);
    }

private:
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(getX() + width / 2, getY(), getX() + width / 2, getBottom());
    }

    void mouseDown(MouseEvent const& e) override
    {
        dragStartWidth = getX();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        int newX = std::clamp<int>(dragStartWidth + e.getDistanceFromDragStartX(), getParentComponent()->getWidth() * 0.25f, getParentComponent()->getWidth() * 0.75f);
        setTopLeftPosition(newX, 0);
        onMove(newX + width / 2);
    }

    int dragStartWidth = 0;
    bool draggingSplitview = false;
};

SplitView::SplitView(PluginEditor* parent)
    : editor(parent)
    , fadeAnimation(new FadeAnimation(this))
    , fadeAnimationLeft(new FadeAnimation(this))
    , fadeAnimationRight(new FadeAnimation(this))
{
    auto* resizer = new SplitViewResizer();
    resizer->onMove = [this](int x) {
        splitViewWidth = static_cast<float>(x) / getWidth();
        resized();
    };
    addChildComponent(resizer);

    splitViewResizer.reset(resizer);

    int i = 0;
    for (auto& tabbar : splits) {

        tabbar.newTab = [this, i]() {
            splitFocusIndex = i;
            editor->newProject();
        };

        tabbar.openProject = [this, i]() {
            splitFocusIndex = i;
            editor->openProject();
        };

        tabbar.openProjectFile = [this, i](File patchFile) {
            splitFocusIndex = i;
            editor->pd->loadPatch(patchFile);
        };

        tabbar.onTabChange = [this, i, &tabbar](int idx) {
            splitFocusIndex = i;

            editor->updateCommandStatus();

            auto* cnv = tabbar.getCurrentCanvas();

            if (!cnv || idx == -1 || editor->pd->isPerformingGlobalSync)
                return;

            editor->sidebar->tabChanged();
            cnv->tabChanged();

            if (auto* splitCnv = splits[1 - i].getCurrentCanvas()) {
                splitCnv->tabChanged();
            }
        };

        tabbar.onFocusGrab = [this, &tabbar]() {
            if (auto* cnv = tabbar.getCurrentCanvas()) {
                setFocus(cnv);
            }
        };

        tabbar.onTabMoved = [this]() {
            editor->pd->savePatchTabPositions();
        };

        tabbar.rightClick = [this, &tabbar, i](int tabIndex, String const& tabName) {
            PopupMenu tabMenu;

#if JUCE_MAC
            String revealTip = "Reveal in Finder";
#elif JUCE_WINDOWS
            String revealTip = "Reveal in Explorer";
#else
            String revealTip = "Reveal in file browser";
#endif

            auto* cnv = tabbar.getCanvas(tabIndex);
            if (!cnv)
                return;

            bool canReveal = cnv->patch.getCurrentFile().existsAsFile();

            tabMenu.addItem(revealTip, canReveal, false, [cnv]() {
                cnv->patch.getCurrentFile().revealToUser();
            });

            bool canSplit = true;
            if (i == 0 && !splitView)
                canSplit = getLeftTabbar()->getNumTabs() > 1;

            tabMenu.addItem(i == 0 ? "Split Right" : "Split Left", canSplit, false, [this, cnv, i]() {
                splitCanvasView(cnv, i == 0);
                closeEmptySplits();
            });
            // Show the popup menu at the mouse position
            tabMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withParentComponent(editor->pd->getActiveEditor()));
        };

        tabbar.setOutline(0);
        addAndMakeVisible(tabbar);
        i++;
    }
    addMouseListener(this, true);
}

SplitView::~SplitView() = default;

void SplitView::setSplitEnabled(bool splitEnabled)
{
    splitView = splitEnabled;
    splitFocusIndex = splitEnabled;

    splitViewResizer->setVisible(splitEnabled);
    resized();
}

bool SplitView::isSplitEnabled() const
{
    return splitView;
}

void SplitView::resized()
{
    auto b = getLocalBounds();
    auto splitWidth = splitView ? splitViewWidth * getWidth() : getWidth();

    getRightTabbar()->setBounds(b.removeFromRight(getWidth() - splitWidth));
    getLeftTabbar()->setBounds(b);

    int splitResizerWidth = SplitViewResizer::width;
    int halfSplitWidth = splitResizerWidth / 2;
    splitViewResizer->setBounds(splitWidth - halfSplitWidth, 0, splitResizerWidth, getHeight());
}

void SplitView::setFocus(Canvas* cnv)
{
    splitFocusIndex = cnv->getTabbar() == getRightTabbar();
    repaint();
}

bool SplitView::hasFocus(Canvas* cnv)
{
    if ((cnv->getTabbar() == getRightTabbar()) == splitFocusIndex)
        return true;
    else
        return false;
}

bool SplitView::isRightTabbarActive() const
{
    return splitFocusIndex;
}

void SplitView::closeEmptySplits()
{
    if (!splits[1].getNumTabs()) {
        // Disable splitview if all splitview tabs are closed
        setSplitEnabled(false);
    }
    if (splitView && !splits[0].getNumTabs()) {

        // move all tabs over to the left side
        for (int i = splits[1].getNumTabs() - 1; i >= 0; i--) {
            splitCanvasView(splits[1].getCanvas(i), false);
        }

        setSplitEnabled(false);
    } else if (!splits[0].getNumTabs()) {
    }

    if (splits[0].getCurrentTabIndex() < 0 && splits[0].getNumTabs()) {
        splits[0].setCurrentTabIndex(0);
    }
    // Make sure to show the welcome screen if this was the last tab
    else if (splits[0].getCurrentTabIndex() < 0) {
        splits[0].currentTabChanged(-1, "");
    }

    if (splits[1].getCurrentTabIndex() < 0 && splits[1].getNumTabs()) {
        splits[1].setCurrentTabIndex(0);
    }
}

void SplitView::paintOverChildren(Graphics& g)
{
    auto* tabbar = getActiveTabbar();
    Colour indicatorColour = findColour(PlugDataColour::objectSelectedOutlineColourId);

    if (splitView) {
        Colour leftTabbarOutline;
        Colour rightTabbarOutline;
        if (tabbar == getLeftTabbar()) {
            leftTabbarOutline = indicatorColour.withAlpha(fadeAnimationLeft->fadeIn());
            rightTabbarOutline = indicatorColour.withAlpha(fadeAnimationRight->fadeOut());
        } else {
            rightTabbarOutline = indicatorColour.withAlpha(fadeAnimationRight->fadeIn());
            leftTabbarOutline = indicatorColour.withAlpha(fadeAnimationLeft->fadeOut());
        }
        g.setColour(leftTabbarOutline);
        g.drawRect(getLeftTabbar()->getBounds().withTrimmedRight(-1), 2.0f);
        g.setColour(rightTabbarOutline);
        g.drawRect(getRightTabbar()->getBounds().withTrimmedLeft(-1).withTrimmedRight(1), 2.0f);
    }
    if (tabbar->tabSnapshot.isValid()) {
        g.setColour(indicatorColour);
        g.drawImage(tabbar->tabSnapshot, tabbar->tabSnapshotBounds.toFloat());
        if (splitviewIndicator) {
            g.setOpacity(fadeAnimation->fadeIn());
            if (!splitView) {
                g.fillRect(tabbar->getBounds().withTrimmedLeft(getWidth() / 2).withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            } else if (tabbar == getLeftTabbar()) {
                g.fillRect(getRightTabbar()->getBounds().withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            } else {
                g.fillRect(getLeftTabbar()->getBounds().withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            }
        } else {
            g.setOpacity(fadeAnimation->fadeOut());
            if (!splitView) {
                g.fillRect(tabbar->getBounds().withTrimmedLeft(getWidth() / 2).withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            } else if (tabbar == getLeftTabbar()) {
                g.fillRect(getRightTabbar()->getBounds().withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            } else {
                g.fillRect(getLeftTabbar()->getBounds().withTrimmedTop(tabbar->currentTabBounds.getHeight()));
            }
        }
    }
}

void SplitView::splitCanvasesAfterIndex(int idx, bool direction)
{
    Array<Canvas*> splitCanvases;

    // Two loops to make sure we don't edit the order during the first loop
    for (int i = idx; i < editor->canvases.size() && i >= 0; i++) {
        splitCanvases.add(editor->canvases[i]);
    }
    for (auto* cnv : splitCanvases) {
        splitCanvasView(cnv, direction);
    }
}
void SplitView::splitCanvasView(Canvas* cnv, bool splitViewFocus)
{
    auto* editor = cnv->editor;

    auto* currentTabbar = cnv->getTabbar();
    auto const tabIdx = cnv->getTabIndex();

    if (currentTabbar->getCurrentTabIndex() == tabIdx)
        currentTabbar->setCurrentTabIndex(tabIdx > 0 ? tabIdx - 1 : tabIdx);

    currentTabbar->removeTab(tabIdx);

    cnv->recreateViewport();

    if (splitViewFocus) {
        setSplitEnabled(true);
    } else {
        // Check if the right tabbar has any tabs left after performing split
        setSplitEnabled(getRightTabbar()->getNumTabs());
    }

    splitFocusIndex = splitViewFocus;
    editor->addTab(cnv);
    fadeAnimation->fadeOut();
}

TabComponent* SplitView::getActiveTabbar()
{
    return &splits[splitFocusIndex];
}

TabComponent* SplitView::getLeftTabbar()
{
    return &splits[0];
}

TabComponent* SplitView::getRightTabbar()
{
    return &splits[1];
}

void SplitView::mouseDrag(MouseEvent const& e)
{
    auto* activeTabbar = getActiveTabbar();

    // Check if the active tabbar has a valid tab snapshot and if the tab snapshot is below the current tab
    if (activeTabbar->tabSnapshot.isValid() && activeTabbar->tabSnapshotBounds.getY() > activeTabbar->getY() + activeTabbar->currentTabBounds.getHeight()) {
        if (!splitView) {
            // Check if the tab snapshot is on the right hand half of the viewport (activeTabbar)
            if (e.getEventRelativeTo(activeTabbar).getPosition().getX() > activeTabbar->getWidth() * 0.5f) {
                splitviewIndicator = true;

            } else {
                splitviewIndicator = false;
            }
        } else {
            auto* leftTabbar = getLeftTabbar();
            auto* rightTabbar = getRightTabbar();
            auto leftTabbarContainsPointer = leftTabbar->contains(e.getEventRelativeTo(leftTabbar).getPosition());

            if (activeTabbar == leftTabbar && !leftTabbarContainsPointer) {
                splitviewIndicator = true;
            } else if (activeTabbar == rightTabbar && leftTabbarContainsPointer) {
                splitviewIndicator = true;
            } else {
                splitviewIndicator = false;
            }
        }
    } else {
        splitviewIndicator = false;
    }
}

void SplitView::mouseUp(MouseEvent const& e)
{
    if (splitviewIndicator) {
        auto* tabbar = getActiveTabbar();
        if (tabbar == getLeftTabbar()) {
            splitCanvasView(tabbar->getCurrentCanvas(), true);
        } else {
            splitCanvasView(tabbar->getCurrentCanvas(), false);
        }
        splitviewIndicator = false;
        closeEmptySplits();
    }
}
