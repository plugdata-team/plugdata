#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Tabbar.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"
#include "TabBarButtonComponent.h"

class ButtonBar::GhostTab : public Component {
public:
    GhostTab() {}

    void setTabButtonToGhost(TabBarButton* tabButton)
    {
        tab = tabButton;
        setBounds(tab->getBounds());
        repaint();
    }

    void paint(Graphics& g) override
    {
        LookAndFeel::getDefaultLookAndFeel().drawTabButton(*tab, g, true, true);
    }

private:
    TabBarButton* tab;
};

ButtonBar::ButtonBar(TabComponent& tabComp, TabbedButtonBar::Orientation o)
    : TabbedButtonBar(o)
    , owner(tabComp)
{
    ghostTab = std::make_unique<GhostTab>();
    addChildComponent(ghostTab.get());
    ghostTab->setAlwaysOnTop(true);

    setInterceptsMouseClicks(true, true);
}

bool ButtonBar::isInterestedInDragSource(SourceDetails const& dragSourceDetails)
{
    if (dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get()))
        return true;

    return false;
}

void ButtonBar::itemDropped(SourceDetails const& dragSourceDetails)
{
    ghostTab->setVisible(false);
    // this has a whole lot of code replication from ResizableTabbedComponent.cpp, good candidate for refactoring!
    if (inOtherSplit) {
        auto sourceTabButton = static_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
        int sourceTabIndex = sourceTabButton->getIndex();
        auto sourceTabContent = sourceTabButton->getTabComponent();
        int sourceNumTabs = sourceTabContent->getNumTabs();

        inOtherSplit = false;
        // we remove the ghost tab, which is NOT a proper tab, (it only a tab, and doesn't have a viewport)
        removeTab(ghostTabIdx, true);
        auto tabCanvas = sourceTabContent->getCanvas(sourceTabIndex);
        auto tabTitle = tabCanvas->patch.getTitle();
        // we then re-add the ghost tab, but this time we add it from the owner (tabComponent) 
        // which allows us to inject the viewport
        owner.addTab(tabTitle, sourceTabContent->getCanvas(sourceTabIndex)->viewport.get(), ghostTabIdx);
        owner.setCurrentTabIndex(ghostTabIdx);

        sourceTabContent->removeTab(sourceTabIndex);
        auto sourceCurrentIndex = sourceTabIndex > (sourceTabContent->getNumTabs() - 1) ? sourceTabIndex - 1 : sourceTabIndex;
        sourceTabContent->setCurrentTabIndex(sourceCurrentIndex);

        if (sourceNumTabs < 2) {
            owner.editor->splitView.removeSplit(sourceTabContent);
            for (auto* split : owner.editor->splitView.splits) {
                split->setBoundsWithFactors(owner.editor->splitView.getLocalBounds());
            }
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
        // if this tabbar is DnD on itself, we don't need to add a new tab
        // we move the existing tab
        if (tab->getTabComponent() == &owner) {
            tab->setVisible(false);
            inOtherSplit = false;
            ghostTabIdx = tab->getIndex();
            ghostTab->setTabButtonToGhost(tab);
        } else {
            // we calculate where the tab will go when its added,
            // so we need to add 1 to the number of existing tabs
            // to take the added tab into account
            auto targetTabPos = getWidth() / (getNumTabs() + 1);
            auto tabPos = dragSourceDetails.localPosition.getX() / targetTabPos;
            inOtherSplit = true;
            addTab(tab->getButtonText(), Colours::transparentBlack, tabPos);
            auto* fakeTab = getTabButton(tabPos);
            fakeTab->setVisible(false);
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
        tab->setVisible(false);
        if (inOtherSplit) {
            inOtherSplit = false;
            removeTab(ghostTabIdx, true);
        }
    }
}

void ButtonBar::itemDragMove(SourceDetails const& dragSourceDetails)
{
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        auto ghostTabCentreOffset = ghostTab->getWidth() / 2;
        auto targetTabPos = getWidth() / getNumTabs();
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
        tab->setVisible(false);
        getTabButton(tabPos)->setVisible(false);
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

int TabComponent::getCurrentTabIndex()
{
    return tabs->getCurrentTabIndex();
}

void TabComponent::setCurrentTabIndex(int idx)
{
    tabs->setCurrentTabIndex(idx);
}

void TabComponent::clearTabs()
{
    if (panelComponent != nullptr)
    {
        panelComponent->setVisible (false);
        removeChildComponent (panelComponent.get());
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
    contentComponents.insert (insertIndex, WeakReference<Component> (contentComponent));

    tabs->addTab (tabName, findColour(ResizableWindow::backgroundColourId), insertIndex);
    resized();
}

void TabComponent::removeTab(int idx)
{
    contentComponents.remove(idx);
    tabs->removeTab(idx);
}

void TabComponent::moveTab(int currentIndex, int newIndex)
{
    contentComponents.move (currentIndex, newIndex);
    tabs->moveTab (currentIndex, newIndex, true);
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
    auto* newPanelComp = getTabContentComponent (getCurrentTabIndex());

    if (newPanelComp != panelComponent)
    {
        if (panelComponent != nullptr)
        {
            panelComponent->setVisible (false);
            removeChildComponent (panelComponent);
        }

        panelComponent = newPanelComp;

        if (panelComponent != nullptr)
        {
            // do these ops as two stages instead of addAndMakeVisible() so that the
            // component has always got a parent when it gets the visibilityChanged() callback
            addChildComponent (panelComponent);
            panelComponent->sendLookAndFeelChange();
            panelComponent->setVisible (true);
            panelComponent->toFront (true);
        }

        repaint();
    }

    resized();
    currentTabChanged (newCurrentTabIndex, newTabName);
}

void TabComponent::openProjectFile(File& patchFile)
{
    editor->pd->loadPatch(patchFile);
}

void TabComponent::setTabBarDepth (int newDepth)
{
    if (tabDepth != newDepth)
    {
        tabDepth = newDepth;
        resized();
    }
}

void TabComponent::currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName)
{
    if (tabs->getNumTabs() == 0) {
        setTabBarDepth(0);
        tabs->setVisible(false);
        welcomePanel.show();
    } else {
        tabs->setVisible(true);
        //static_cast<TabBarButtonComponent*>(getTabbedButtonBar().getTabButton(newCurrentTabIndex))->tabTextChanged(newCurrentTabName);
        welcomePanel.hide();
        setTabBarDepth(30);
        // we need to update the dropzones, because no resize will be automatically triggered when there is a tab added from welcome screen
        if (auto* parentHolder = dynamic_cast<ResizableTabbedComponent*>(getParentComponent()))
            parentHolder->updateDropZones();
    }

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

Component* TabComponent::getTabContentComponent (int tabIndex) const noexcept
{
    return contentComponents[tabIndex].get();
}

void TabComponent::paint(Graphics& g)
{
    g.fillAll(findColour(PlugDataColour::tabBackgroundColourId));
    //g.fillRect(getLocalBounds().removeFromTop(30));
}

void TabComponent::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0, tabDepth, getWidth(), tabDepth);

    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 0, 0, getBottom());
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


