/*
 // Copyright (c) 2021-2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Tabbar.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"
#include "TabBarButtonComponent.h"
#include "Utility/StackShadow.h"

class ButtonBar::GhostTab : public Component {
public:
    GhostTab(PlugDataLook& lnfRef)
        : lnf(lnfRef)
    {
    }

    void setTabButtonToGhost(TabBarButton* tabButton)
    {
        tab = tabButton;
        // this should never happen, if it does then the index for the tab is wrong ( getTab(idx) will return nullptr )
        // which can happen if the tabbar goes into overflow, because we don't know exactly when that will happen
        if (tab){
            setBounds(tab->getBounds());
            repaint();
        }
    }

    int getIndex()
    {
        if (tab)
            return tab->getIndex();

        return -1;
    }

    void resized() override
    {
        shadowPath.clear();
        shadowPath.addRoundedRectangle(getLocalBounds().reduced(8).toFloat(), Corners::defaultCornerRadius);
    }

    void paint(Graphics& g) override
    {
        if (tab) {
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.3f), 4);
            lnf.drawTabButton(*tab, g, true, true, true);
        }
    }

private:
    SafePointer<TabBarButton> tab;
    PlugDataLook& lnf;

    Path shadowPath;
};

ButtonBar::ButtonBar(TabComponent& tabComp, TabbedButtonBar::Orientation o)
    : TabbedButtonBar(o)
    , owner(tabComp)
{
    ghostTab = std::make_unique<GhostTab>(dynamic_cast<PlugDataLook&>(LookAndFeel::getDefaultLookAndFeel()));
    addChildComponent(ghostTab.get());
    ghostTab->setAlwaysOnTop(true);

    ghostTabAnimator.addChangeListener(this);

    setInterceptsMouseClicks(true, true);
}

bool ButtonBar::isInterestedInDragSource(SourceDetails const& dragSourceDetails)
{
    if (dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get()))
        return true;

    return false;
}

void ButtonBar::changeListenerCallback(ChangeBroadcaster* source)
{
    if (&ghostTabAnimator == source) {
        if (!ghostTabAnimator.isAnimating()) {
            ghostTab->setVisible(false);
            auto* tabButton = getTabButton(ghostTabIdx);
            auto ghostTabFinalPos = ghostTab->getBounds();

            // we need to reset the final position of the tab, as we have stored the ghosttabs entry position into it
            tabButton->setBounds(ghostTabFinalPos);
            tabButton->getProperties().set("dragged", var(false));
        }
    }
}

void ButtonBar::itemDropped(SourceDetails const& dragSourceDetails)
{
    auto animateTabToPosition = [this]() {
        auto* tabButton = getTabButton(ghostTabIdx);
        tabButton->getProperties().set("dragged", var(true));

        ghostTabAnimator.animateComponent(ghostTab.get(), ghostTab->getBounds().withPosition(Point<int>(ghostTab->getIndex() * (getWidth() / getNumVisibleTabs()), 0)), 1.0f, 200, false, 3.0f, 0.0f);
    };

    // this has a whole lot of code replication from ResizableTabbedComponent.cpp, good candidate for refactoring!
    if (!inOtherSplit) {
        animateTabToPosition();
    } else {
        auto sourceTabButton = static_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
        int sourceTabIndex = sourceTabButton->getIndex();
        auto sourceTabContent = sourceTabButton->getTabComponent();
        int sourceNumTabs = sourceTabContent->getNumVisibleTabs();

        auto ghostTabBounds = ghostTab->getBounds();

        inOtherSplit = false;
        // we remove the ghost tab, which is NOT a proper tab, (it only a tab, and doesn't have a viewport)
        owner.removeTab(ghostTabIdx);
        auto tabCanvas = sourceTabContent->getCanvas(sourceTabIndex);
        auto tabTitle = tabCanvas->patch.getTitle();
        // we then re-add the ghost tab, but this time we add it from the owner (tabComponent)
        // which allows us to inject the viewport
        owner.addTab(tabTitle, sourceTabContent->getCanvas(sourceTabIndex)->viewport.get(), ghostTabIdx);
        owner.setCurrentTabIndex(ghostTabIdx);

        // we need to give the ghost tab the new tab button, as the old one will be deleted before
        // the ghost tabs animation has finished
        // this is easier than keeping the old tab alive
        auto newTab = owner.tabs->getTabButton(ghostTabIdx);
        newTab->setBounds(ghostTabBounds);
        ghostTab->setTabButtonToGhost(newTab);

        sourceTabContent->removeTab(sourceTabIndex);
        auto sourceCurrentIndex = sourceTabIndex > (sourceTabContent->getNumVisibleTabs() - 1) ? sourceTabIndex - 1 : sourceTabIndex;
        sourceTabContent->setCurrentTabIndex(sourceCurrentIndex);

        if (sourceNumTabs < 2) {
            // we don't animate the ghostTab moving into position, as the geometry of the splits is changing
            ghostTab->setVisible(false);
            ghostTabAnimator.cancelAllAnimations(true);

            owner.editor->splitView.removeSplit(sourceTabContent);
            for (auto* split : owner.editor->splitView.splits) {
                split->setBoundsWithFactors(owner.editor->splitView.getLocalBounds());
            }
        } else {
            animateTabToPosition();
        }

        // set all current canvas viewports to visible, (if they already are this shouldn't do anything)
        for (auto* split : owner.editor->splitView.splits) {
            if (auto tabComponent = split->getTabComponent()) {
                if (auto* cnv = tabComponent->getCanvas(tabComponent->getCurrentTabIndex())) {
                    cnv->viewport->setVisible(true);
                    split->resized();
                    split->getTabComponent()->resized();
                }
            }
        }
    }
}

void ButtonBar::itemDragEnter(SourceDetails const& dragSourceDetails)
{
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        ghostTabAnimator.cancelAllAnimations(false);
        owner.setFocused();
        // if this tabbar is DnD on itself, we don't need to add a new tab
        // we move the existing tab
        if (tab->getTabComponent() == &owner) {
            tab->getProperties().set("dragged", var(true));
            inOtherSplit = false;
            ghostTabIdx = tab->getIndex();
            ghostTab->setTabButtonToGhost(tab);
        } else {
            // we calculate where the tab will go when its added,
            // so we need to add 1 to the number of existing tabs
            // to take the added tab into account

            // WARNING: because we are using the overflow (show extra items menu)
            // we need to find out how many tabs are visible, not how many there are all together
            auto targetTabPos = getWidth() / (getNumVisibleTabs() + 1);
            // FIXME: This is a hack. When tab is added to tabbar right edge, and it goes into overflow
            //        we don't know when that will happen.
            //        So we force the right most tab to think it's always two less when it gets added
            auto tabPos = jmin(dragSourceDetails.localPosition.getX() / targetTabPos, getNumVisibleTabs() - 2);
            tabPos = jmax(0, tabPos);

            inOtherSplit = true;
            auto unusedComponent = std::make_unique<Component>();
            owner.addTab(tab->getButtonText(), unusedComponent.get(), tabPos);

            auto* fakeTab = getTabButton(tabPos);
            tab->getProperties().set("dragged", var(true));
            ghostTab->setTabButtonToGhost(fakeTab);
            ghostTabIdx = tabPos;
        }
        ghostTab->setVisible(true);
    }
}

void ButtonBar::itemDragExit(SourceDetails const& dragSourceDetails)
{
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        ghostTab->setVisible(false);
        tab->getProperties().set("dragged", var(true));
        if (inOtherSplit) {
            inOtherSplit = false;
            owner.removeTab(ghostTabIdx);
        }
    }
}

int ButtonBar::getNumVisibleTabs()
{
    int numVisibleTabs = 0;
    for (int i = 0; i < getNumTabs(); i++) {
        if (getTabButton(i)->isVisible())
            numVisibleTabs++;
    }
    return numVisibleTabs;
}

void ButtonBar::itemDragMove(SourceDetails const& dragSourceDetails)
{
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        auto ghostTabCentreOffset = ghostTab->getWidth() / 2;
        auto targetTabPos = getWidth() / (getNumVisibleTabs());
        auto tabPos = ghostTab->getBounds().getCentreX() / targetTabPos;

        auto leftPos = dragSourceDetails.localPosition.getX() - ghostTabCentreOffset;
        auto rightPos = dragSourceDetails.localPosition.getX() + ghostTabCentreOffset;
        auto tabCentre = tab->getBounds().getCentreY();
        if (leftPos >= 0 && rightPos <= getWidth()) {
            ghostTab->setCentrePosition(dragSourceDetails.localPosition.getX(), tabCentre);
        } else if (leftPos < 0) {
            ghostTab->setCentrePosition(0 + ghostTabCentreOffset, tabCentre);
        } else {
            ghostTab->setCentrePosition(getWidth() - ghostTabCentreOffset, tabCentre);
        }
        if (tabPos != ghostTabIdx) {
            owner.moveTab(ghostTabIdx, tabPos);
            ghostTabIdx = tabPos;
        }
        tab->getProperties().set("dragged", var(true));
        getTabButton(tabPos)->getProperties().set("dragged", var(true));
    }
}

void ButtonBar::currentTabChanged(int newCurrentTabIndex, String const& newTabName)
{
    owner.changeCallback(newCurrentTabIndex, newTabName);
}

TabBarButton* ButtonBar::createTabButton(String const& tabName, int tabIndex)
{
    auto tabBarButton = new TabBarButtonComponent(&owner, tabName, *owner.tabs.get());
    return tabBarButton;
}

TabComponent::TabComponent(PluginEditor* parent)
    : editor(parent)
{
    tabs.reset(new ButtonBar(*this, TabbedButtonBar::Orientation::TabsAtTop));
    addAndMakeVisible(tabs.get());

    addAndMakeVisible(newButton);
    newButton.getProperties().set("Style", "LargeIcon");
    newButton.setButtonText(Icons::Add);
    newButton.setColour(TextButton::buttonColourId, Colours::transparentBlack);
    newButton.setTooltip("New patch");
    newButton.onClick = [this]() {
        newTab();
    };

    addAndMakeVisible(welcomePanel);

    welcomePanel.newButton.onClick = [this]() {
        newTab();
    };

    welcomePanel.openButton.onClick = [this]() {
        openProject();
    };

    welcomePanel.recentlyOpened.onPatchOpen = [this](File patchFile) {
        openProjectFile(patchFile);
    };

    setVisible(false);
    setTabBarDepth(0);
    tabs->addMouseListener(this, true);
}

TabComponent::~TabComponent()
{
    tabs->removeMouseListener(this);
    clearTabs();
    tabs.reset();
}

void TabComponent::setFocused()
{
    for (auto * split : editor->splitView.splits){
        if (split->getTabComponent() == this)
            editor->splitView.setFocus(split);
    }
}

int TabComponent::getCurrentTabIndex()
{
    return tabs->getCurrentTabIndex();
}

void TabComponent::setCurrentTabIndex(int idx)
{
    tabs->setCurrentTabIndex(idx);
    for (int i = 0; i < tabs->getNumTabs(); i++) {
        dynamic_cast<TabBarButtonComponent*>(tabs->getTabButton(i))->updateCloseButtonState();
    }
}

int TabComponent::getNumVisibleTabs()
{
    int numVisibleTabs = 0;
    for (int i = 0; i < getNumTabs(); i++) {
        if (tabs->getTabButton(i)->isVisible())
            numVisibleTabs++;
    }
    return numVisibleTabs;
}

void TabComponent::clearTabs()
{
    if (panelComponent != nullptr) {
        panelComponent->setVisible(false);
        removeChildComponent(panelComponent.get());
        panelComponent = nullptr;
    }

    tabs->clearTabs();

    contentComponents.clear();
}

PluginEditor* TabComponent::getEditor()
{
    return editor;
}

void TabComponent::newTab()
{
    editor->newProject();
}

void TabComponent::addTab(String const& tabName, Component* contentComponent, int insertIndex)
{
    contentComponents.insert(insertIndex, WeakReference<Component>(contentComponent));

    tabs->addTab(tabName, findColour(ResizableWindow::backgroundColourId), insertIndex);
    
    setTabBarDepth(30); // Make sure tabbar isn't invisible
    resized();
}

void TabComponent::removeTab(int idx)
{
    contentComponents.remove(idx);
    tabs->removeTab(idx);
}

void TabComponent::moveTab(int currentIndex, int newIndex)
{
    contentComponents.move(currentIndex, newIndex);
    tabs->moveTab(currentIndex, newIndex, true);
}

void TabComponent::openProject()
{
    editor->openProject();
}

void TabComponent::onTabMoved()
{
    editor->pd->savePatchTabPositions();
}

void TabComponent::onTabChange(int tabIndex)
{
    editor->updateCommandStatus();

    // Show welcome panel if there are no tabs open
    if (tabs->getNumTabs() == 0) {
        setTabBarDepth(0);
        tabs->setVisible(false);
        welcomePanel.show();
    } else {
        tabs->setVisible(true);
        // static_cast<TabBarButtonComponent*>(getTabbedButtonBar().getTabButton(newCurrentTabIndex))->tabTextChanged(newCurrentTabName);
        welcomePanel.hide();
        setTabBarDepth(30);
        // we need to update the dropzones, because no resize will be automatically triggered when there is a tab added from welcome screen
        if (auto* parentHolder = dynamic_cast<ResizableTabbedComponent*>(getParentComponent()))
            parentHolder->updateDropZones();
    }

    auto* cnv = getCurrentCanvas();
    if (!cnv || tabIndex == -1 || editor->pd->isPerformingGlobalSync)
        return;

    editor->sidebar->tabChanged();

    for (auto* split : editor->splitView.splits) {
        auto tabBar = split->getTabComponent();
        if (split->getTabComponent()->getCurrentCanvas())
            split->getTabComponent()->getCurrentCanvas()->tabChanged();
    }
}

void TabComponent::changeCallback(int newCurrentTabIndex, String const& newTabName)
{
    auto* newPanelComp = getTabContentComponent(getCurrentTabIndex());

    if (newPanelComp != panelComponent) {
        if (panelComponent != nullptr) {
            panelComponent->setVisible(false);
            removeChildComponent(panelComponent);
        }

        panelComponent = newPanelComp;

        if (panelComponent != nullptr) {
            // do these ops as two stages instead of addAndMakeVisible() so that the
            // component has always got a parent when it gets the visibilityChanged() callback
            addChildComponent(panelComponent);
            panelComponent->sendLookAndFeelChange();
            panelComponent->setVisible(true);
            panelComponent->toFront(true);
        }
    }
    currentTabChanged(newCurrentTabIndex, newTabName);
}

void TabComponent::openProjectFile(File& patchFile)
{
    editor->pd->loadPatch(patchFile);
}

void TabComponent::setTabBarDepth(int newDepth)
{
    if (tabDepth != newDepth) {
        tabDepth = newDepth;
        resized();
    }
}

void TabComponent::currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName)
{
    triggerAsyncUpdate();
}

void TabComponent::handleAsyncUpdate()
{
    onTabChange(tabs->getCurrentTabIndex());
}

void TabComponent::resized()
{
    auto content = getLocalBounds();

    welcomePanel.setBounds(content);
    newButton.setBounds(3, 0, tabDepth, tabDepth); // slighly offset to make it centred next to the tabs

    auto tabBounds = content.removeFromTop(tabDepth).withTrimmedLeft(tabDepth);
    tabs->setBounds(tabBounds);

    for (int c = 0; c < tabs->getNumTabs(); c++) {
        if (auto* comp = getTabContentComponent(c)) {
            if (auto* positioner = comp->getPositioner()) {
                positioner->applyNewBounds(content);
            } else {
                comp->setBounds(content);
            }
        }
    }
}

Component* TabComponent::getTabContentComponent(int tabIndex) const noexcept
{
    return contentComponents[tabIndex].get();
}

void TabComponent::paint(Graphics& g)
{
    g.fillAll(findColour(PlugDataColour::tabBackgroundColourId));
}

void TabComponent::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0, tabDepth, getWidth(), tabDepth);
    g.drawLine(0, 0, getWidth(), 0);
}

int TabComponent::getIndexOfCanvas(Canvas* cnv)
{
    if (!cnv->viewport || !cnv->editor)
        return -1;

    for (int i = 0; i < tabs->getNumTabs(); i++) {
        if (getTabContentComponent(i) == cnv->viewport.get()) {
            return i;
        }
    }

    return -1;
}

Canvas* TabComponent::getCanvas(int idx)
{
    auto* viewport = dynamic_cast<Viewport*>(getTabContentComponent(idx));

    if (!viewport)
        return nullptr;

    return dynamic_cast<Canvas*>(viewport->getViewedComponent());
}

Canvas* TabComponent::getCurrentCanvas()
{
    auto* viewport = dynamic_cast<Viewport*>(getCurrentContentComponent());

    if (!viewport)
        return nullptr;

    return dynamic_cast<Canvas*>(viewport->getViewedComponent());
}

void TabComponent::setTabText(int tabIndex, String const& newName)
{
    dynamic_cast<TabBarButtonComponent*>(tabs->getTabButton(tabIndex))->setTabText(newName);
}
