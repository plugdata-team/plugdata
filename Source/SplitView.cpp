/*
 // Copyright (c) 2021-2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "PluginEditor.h"
#include "SplitView.h"
#include "Canvas.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"

class SplitViewFocusOutline : public Component
    , public ComponentListener {
public:
    SplitViewFocusOutline()
    {
        setInterceptsMouseClicks(false, false);
    }
    
    ~SplitViewFocusOutline()
    {
        if(tabbedComponent) tabbedComponent->removeComponentListener(this);
    }

    void setActive(ResizableTabbedComponent* tabComponent)
    {
        setVisible(true);

        if (tabbedComponent != tabComponent) {
            if (tabbedComponent)
                tabbedComponent->removeComponentListener(this);

            tabComponent->addComponentListener(this);
            setBounds(tabComponent->getBounds());
            tabbedComponent = tabComponent;
        }
    }

    void componentMovedOrResized(Component& component, bool moved, bool resized) override
    {
        if (&component == tabbedComponent) {
            setBounds(component.getBounds());
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f));
        g.drawRect(getLocalBounds(), 2.5f);
    }

private:
    SafePointer<ResizableTabbedComponent> tabbedComponent = nullptr;
};

SplitView::SplitView(PluginEditor* parent)
    : editor(parent)
{
    rootComponent = new ResizableTabbedComponent(editor);
    splits.add(rootComponent);
    addAndMakeVisible(rootComponent);
    // this will cause the default welcome screen to be selected
    // which we don't want
    // either we check if the tabcomponent is welcome mode, or we check if it's nullptr down the line
    activeTabComponent = rootComponent;

    focusOutline = std::make_unique<SplitViewFocusOutline>();
    addChildComponent(focusOutline.get());
    focusOutline->setAlwaysOnTop(true);

    addMouseListener(this, true);
}

SplitView::~SplitView() = default;

bool SplitView::canSplit()
{
    return splits.size() < 2;
}

void SplitView::removeSplit(TabComponent* toRemove)
{
    ResizableTabbedComponent* toBeRemoved = nullptr;
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
        } else if (toBeRemoved->resizerLeft) {
            toBeRemoved->resizerLeft->splits[0]->resizerRight = toBeRemoved->resizerRight;
            setFocus(toBeRemoved->resizerLeft->splits[0]);
            resizers.removeObject(toBeRemoved->resizerLeft, true);
        }
    }
    delete toBeRemoved;

    if (splits.size() == 1)
        focusOutline->setVisible(false);
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

    // This might happen when running multiple window
    return 0;
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
        focusOutline->setActive(activeTabComponent);
    }
}

void SplitView::closeEmptySplits()
{
    // if we have one split, allow welcome screen to show
    if (splits.size() == 1)
        return;

    auto removedSplit = false;

    // search over all splits, and see if they have tab components with tabs, if not, delete
    for (int i = splits.size() - 1; i >= 0; i--) {
        auto* split = splits[i];
        if (auto* tabComponent = split->getTabComponent()) {
            if (tabComponent->getNumTabs() == 0 && splits.size() > 1) {
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

ResizableTabbedComponent* SplitView::getSplitAtScreenPosition(Point<int> position)
{
    for (auto* split : splits) {
        if (split->getScreenBounds().contains(position)) {
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
