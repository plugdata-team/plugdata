#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "PluginEditor.h"
#include "SplitView.h"
#include "Canvas.h"
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

SplitView::SplitView(PluginEditor* parent)
    : editor(parent)
    , fadeAnimation(new FadeAnimation(this))
    , fadeAnimationLeft(new FadeAnimation(this))
    , fadeAnimationRight(new FadeAnimation(this))
{
    rootComponent = new ResizableTabbedComponent(editor);
    splits.add(rootComponent);
    addAndMakeVisible(rootComponent);
    // this will cause the default welcome screen to be selected
    // which we don't want
    // either we check if the tabcomponent is welcome mode, or we check if it's nullptr down the line
    activeTabComponent = rootComponent;

    addMouseListener(this, true);
}

SplitView::~SplitView() = default;

bool SplitView::canSplit()
{
    return splits.size() < 3;
}

void SplitView::removeSplit(TabComponent* toRemove)
{
    ResizableTabbedComponent* toBeRemoved;
    for (auto* split : splits) {
        if (split->getTabComponent() == toRemove) {
            toBeRemoved = split;
            splits.removeObject(split, false);
            break;
        }
    }
    if (toBeRemoved) {
        if (toBeRemoved->resizerRight) {
            toBeRemoved->resizerRight->splits[1]->resizerLeft = toBeRemoved->resizerLeft;
            // We prioritize the deletion of the Right Resizer, and hence also need to 
            // update the pointer to the split
            if (toBeRemoved->resizerLeft) {
                toBeRemoved->resizerLeft->splits[0]->resizerRight->splits[1] = toBeRemoved->resizerRight->splits[1];
            }
            resizers.removeObject(toBeRemoved->resizerRight, true);
        }
        else if (toBeRemoved->resizerLeft) {
            toBeRemoved->resizerLeft->splits[0]->resizerRight = toBeRemoved->resizerRight;
            resizers.removeObject(toBeRemoved->resizerLeft, true);
        }
    }
    delete toBeRemoved;
}

void SplitView::addSplit(ResizableTabbedComponent* split)
{
    splits.add(split);
    addAndMakeVisible(split);
    setFocus(split);
}

void SplitView::addResizer(SplitViewResizer* resizer)
{
    resizers.add(resizer);
    addAndMakeVisible(resizer);
    resizer->setBounds(getLocalBounds());
}

int SplitView::getTabComponentSplitIndex(TabComponent* tabComponent)
{
    for (int i = 0; i < splits.size(); i++) {
        if (splits[i]->getTabComponent() == tabComponent) {
            return i;
        }
    }
}

//void SplitView::setSplitEnabled(bool splitEnabled)
//{
//    splitView = splitEnabled;
//    splitFocusIndex = splitEnabled;
//
//    resized();
//}

//bool SplitView::isSplitEnabled() const
//{
//    return splitView;
//}

void SplitView::resized()
{
    auto b = getLocalBounds();
    for (auto* split : splits) {
        split->setBoundsWithFactors(b);
    }
    for (auto* resizer : resizers) {
        resizer->setBounds(b);
    }
}

void SplitView::setFocus(ResizableTabbedComponent* selectedTabComponent)
{
    if (activeTabComponent != selectedTabComponent) {
        activeTabComponent = selectedTabComponent;
        repaint();
    }
}

//void SplitView::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
//{
//    if (auto tabComp = dynamic_cast<ResizableTabbedComponent*>(&component)) {
//        if (activeTabComponent) {
//            repaint();
//        }
//    }
//}

//bool SplitView::hasFocus(Canvas* cnv)
//{
//    if ((cnv->getTabbar() == getRightTabbar()) == splitFocusIndex)
//        return true;
//    else
//        return false;
//}

//bool SplitView::isRightTabbarActive() const
//{
//    return splitFocusIndex;
//}

void SplitView::closeEmptySplits()
{   /*
    if (!splits[1]->getTabComponent()->getNumTabs()) {
        // Disable splitview if all splitview tabs are closed
        setSplitEnabled(false);
    }
    if (splitView && !splits[0]->getTabComponent()->getNumTabs()) {

        // move all tabs over to the left side
        for (int i = splits[1]->getTabComponent()->getNumTabs() - 1; i >= 0; i--) {
            splitCanvasView(splits[1]->getTabComponent()->getCanvas(i), false);
        }

        setSplitEnabled(false);
    }
    if (splits[0]->getTabComponent()->getCurrentTabIndex() < 0 && splits[0]->getTabComponent()->getNumTabs()) {
        splits[0]->getTabComponent()->setCurrentTabIndex(0);
    }
    // Make sure to show the welcome screen if this was the last tab
    else if (splits[0]->getTabComponent()->getCurrentTabIndex() < 0) {
        splits[0]->getTabComponent()->currentTabChanged(-1, "");
    }

    if (splits[1]->getTabComponent()->getCurrentTabIndex() < 0 && splits[1]->getTabComponent()->getNumTabs()) {
        splits[1]->getTabComponent()->setCurrentTabIndex(0);
    }
    */
}

void SplitView::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f));
    auto screenBounds = activeTabComponent->getScreenBounds();
    auto b = getLocalArea(nullptr, screenBounds);
    g.drawRect(b, 2.5f);
    /*
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
        auto scale = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
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
    */
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

    //if (splitViewFocus) {
    //    setSplitEnabled(true);
    //} else {
    //    // Check if the right tabbar has any tabs left after performing split
    //    setSplitEnabled(getRightTabbar()->getNumTabs());
    //}
    //
    //splitFocusIndex = splitViewFocus;
    //editor->addTab(cnv);
    //fadeAnimation->fadeOut();
}

TabComponent* SplitView::getActiveTabbar()
{
    if (activeTabComponent)
        return activeTabComponent->getTabComponent();

    return nullptr;
}

void SplitView::mouseDrag(MouseEvent const& e)
{
    /*
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
    */
}

void SplitView::mouseUp(MouseEvent const& e)
{
    /*
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
    */
}
