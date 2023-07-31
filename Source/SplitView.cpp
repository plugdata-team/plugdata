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
    return splits.size() < 2;
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
            setFocus(toBeRemoved->resizerRight->splits[1]);
            // We prioritize the deletion of the Right Resizer, and hence also need to 
            // update the pointer to the split
            if (toBeRemoved->resizerLeft) {
                toBeRemoved->resizerLeft->splits[0]->resizerRight->splits[1] = toBeRemoved->resizerRight->splits[1];
            }
            resizers.removeObject(toBeRemoved->resizerRight, true);
        }
        else if (toBeRemoved->resizerLeft) {
            toBeRemoved->resizerLeft->splits[0]->resizerRight = toBeRemoved->resizerRight;
            setFocus(toBeRemoved->resizerLeft->splits[0]);
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

void SplitView::createNewSplit(Canvas* canvas)
{
    activeTabComponent->createNewSplit(ResizableTabbedComponent::Right, canvas);
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

void SplitView::closeEmptySplits()
{   
    // if we have one split, allow welcome screen to show
    if (splits.size() == 1)
        return;

    auto removedSplit = false;

    // search over all splits, and see if they have tab components with tabs, if not, delete
    for(int i = splits.size() - 1; i >= 0; i--)
    {
        auto* split = splits[i];
        if (auto* tabComponent = split->getTabComponent()) {
            if (tabComponent->getNumTabs() == 0) {
                removeSplit(tabComponent);
                removedSplit = true;
            }
        }
    }

    // reset the other splits bounds factors
    if (removedSplit) {
        for (auto* split : splits) {
            split->setBoundsWithFactors(getLocalBounds());
        }
    }
}

void SplitView::paintOverChildren(Graphics& g)
{
    if(splits.size() > 1) {
        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f));
        auto screenBounds = activeTabComponent->getScreenBounds();
        auto b = getLocalArea(nullptr, screenBounds);
        g.drawRect(b, 2.5f);
    }
}

ResizableTabbedComponent* SplitView::getSplitAtScreenPosition(Point<int> position)
{
    for(auto* split : splits)
    {
        if(split->getScreenBounds().contains(position))
        {
            return split;
        }
    }
    
    return nullptr;
}

TabComponent* SplitView::getActiveTabbar()
{
    if (activeTabComponent)
        return activeTabComponent->getTabComponent();

    return nullptr;
}

