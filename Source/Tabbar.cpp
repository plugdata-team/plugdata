#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Tabbar.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"
#include "TabBarButtonComponent.h"

TabComponent::TabComponent(PluginEditor* parent)
    : TabbedComponent(TabbedButtonBar::TabsAtTop)
    , editor(parent)
{
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
    setOutline(0);
}

TabComponent::~TabComponent()
{
    tabs->removeMouseListener(this);
}

// We override this method to create our own tabbarbuttons
// so that our tabbarbuttons have drag and drop ability
TabBarButton* TabComponent::createTabButton (const String& tabName, int tabIndex)
{
    auto tabBarButton = new TabBarButtonComponent (this, tabName, *tabs);
    tabBarButton->addMouseListener(this, true);
    return tabBarButton;
}

PluginEditor* TabComponent::getEditor()
{
    return editor;
}

void TabComponent::newTab()
{
    editor->newProject();
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

void TabComponent::openProjectFile(File& patchFile)
{
    editor->pd->loadPatch(patchFile);
}

void TabComponent::currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName)
{
    if (getNumTabs() == 0) {
        setTabBarDepth(0);
        getTabbedButtonBar().setVisible(false);
        welcomePanel.show();
    } else {
        getTabbedButtonBar().setVisible(true);
        //static_cast<TabBarButtonComponent*>(getTabbedButtonBar().getTabButton(newCurrentTabIndex))->tabTextChanged(newCurrentTabName);
        welcomePanel.hide();
        setTabBarDepth(30);
    }

        triggerAsyncUpdate();
}

void TabComponent::handleAsyncUpdate()
{
    onTabChange(getCurrentTabIndex());
}

void TabComponent::resized()
{
    int depth = getTabBarDepth();
    auto content = getLocalBounds();

    welcomePanel.setBounds(content);
    newButton.setBounds(3, 0, depth, depth); // slighly offset to make it centred next to the tabs

    auto tabBounds = content.removeFromTop(depth).withTrimmedLeft(depth);
    tabs->setBounds(tabBounds);

    for (int c = 0; c < getNumTabs(); c++) {
        if (auto* comp = getTabContentComponent(c)) {
            if (auto* positioner = comp->getPositioner()) {
                positioner->applyNewBounds(content);
            } else {
                comp->setBounds(content);
            }
        }
    }
}

void TabComponent::paint(Graphics& g)
{
    g.fillAll(findColour(PlugDataColour::tabBackgroundColourId));
    //g.fillRect(getLocalBounds().removeFromTop(30));
}

void TabComponent::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());

    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 0, 0, getBottom());
}

int TabComponent::getIndexOfCanvas(Canvas* cnv)
{
    if (!cnv->viewport || !cnv->editor)
        return -1;

    for (int i = 0; i < getNumTabs(); i++) {
        if (getTabContentComponent(i) == cnv->viewport) {
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

void TabComponent::mouseDown(MouseEvent const& e)
{
    tabWidth = tabs->getWidth() / std::max(1, getNumTabs());
    clickedTabIndex = getCurrentTabIndex();
    setCurrentTabIndex(clickedTabIndex);
}

void TabComponent::setTabText(int tabIndex, const String& newName)
{
    dynamic_cast<TabBarButtonComponent*>(getTabbedButtonBar().getTabButton(tabIndex))->setTabText(newName);
}

void TabComponent::mouseMove(MouseEvent const& e)
{
    //tabs->getTabButton(getCurrentTabIndex())->repaint();
}

void TabComponent::mouseDrag(MouseEvent const& e)
{
    // Don't respond to clicks on close button
    if (dynamic_cast<TextButton*>(e.originalComponent))
        return;
    // Drag tabs to move their index
    int const dragPosition = e.getEventRelativeTo(tabs.get()).x;
    int const newTabIndex = (dragPosition < clickedTabIndex * tabWidth) ? clickedTabIndex - 1
        : (dragPosition >= (clickedTabIndex + 1) * tabWidth)            ? clickedTabIndex + 1
                                                                        : clickedTabIndex;
    int const dragDistance = std::abs(e.getDistanceFromDragStartX());

        if (dragDistance > 5) {
            if ((tabs->contains(e.getEventRelativeTo(tabs.get()).getPosition()) || e.getDistanceFromDragStartY() < 0) && newTabIndex != clickedTabIndex && newTabIndex >= 0 && newTabIndex < getNumTabs()) {
                moveTab(clickedTabIndex, newTabIndex, true);
                clickedTabIndex = newTabIndex;
                onTabMoved();
                tabs->getTabButton(clickedTabIndex)->setVisible(false);
            }
            // Keep ghost tab within view
            auto newPosition = Point<int>(std::clamp(currentTabBounds.getX() + getX() + e.getDistanceFromDragStartX(), 0, getParentWidth() - tabWidth), std::clamp(currentTabBounds.getY() + e.getDistanceFromDragStartY(), 0, getHeight() - tabs->getHeight()));
            tabSnapshotBounds.setPosition(newPosition);
            getParentComponent()->repaint();
        }
    }


