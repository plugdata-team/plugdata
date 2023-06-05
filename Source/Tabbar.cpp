#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Tabbar.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"

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

void TabComponent::setSplitFocusIndex(int idx)
{
    auto index = idx;
    if (index = -1)
        index = editor->splitView.getTabComponentSplitIndex(this);

    editor->splitView.setSplitFocusIndex(index);
}

void TabComponent::newTab()
{
    setSplitFocusIndex();
    editor->newProject();
}

void TabComponent::openProject()
{
    setSplitFocusIndex();
    editor->openProject();
}

void TabComponent::onTabMoved()
{
    editor->pd->savePatchTabPositions();
}

void TabComponent::onFocusGrab()
{
    if (auto* cnv = getCurrentCanvas()) {
        editor->splitView.setFocus(cnv);
    }
}

void TabComponent::onTabChange(int tabIndex)
{
    auto splitIndex = editor->splitView.getTabComponentSplitIndex(this);

    setSplitFocusIndex(tabIndex >= 0 ? splitIndex : 0);
    editor->updateCommandStatus();

    auto* cnv = getCurrentCanvas();

    if (!cnv || tabIndex == -1 || editor->pd->isPerformingGlobalSync)
        return;

    editor->sidebar->tabChanged();
    cnv->tabChanged();

    if (auto* splitCnv = editor->splitView.splits[1 - splitIndex]->getCurrentCanvas()) {
        splitCnv->tabChanged();
    }
}

void TabComponent::openProjectFile(File& patchFile)
{
    setSplitFocusIndex();
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
        welcomePanel.hide();
        setTabBarDepth(30);
    }

        triggerAsyncUpdate();
}

void TabComponent::rightClick(int tabIndex, String const& tabName)
{
    auto splitIndex = editor->splitView.getTabComponentSplitIndex(this);

    PopupMenu tabMenu;

#if JUCE_MAC
            String revealTip = "Reveal in Finder";
#elif JUCE_WINDOWS
            String revealTip = "Reveal in Explorer";
#else
            String revealTip = "Reveal in file browser";
#endif

    auto* cnv = getCanvas(tabIndex);
    if (!cnv)
        return;

    bool canReveal = cnv->patch.getCurrentFile().existsAsFile();

    tabMenu.addItem(revealTip, canReveal, false, [cnv]() {
        cnv->patch.getCurrentFile().revealToUser();
    });

    bool canSplit = true;
    if (splitIndex == 0 && !editor->splitView.isSplitView())
        canSplit = editor->splitView.getLeftTabbar()->getNumTabs() > 1;

    tabMenu.addItem(splitIndex == 0 ? "Split Right" : "Split Left", canSplit, false, [this, cnv, splitIndex]() {
        editor->splitView.splitCanvasView(cnv, splitIndex == 0);
        editor->splitView.closeEmptySplits();
        });
        // Show the popup menu at the mouse position
        tabMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withParentComponent(editor->pd->getActiveEditor()));
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
    g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
    g.fillRect(getLocalBounds().removeFromTop(30));
}

void TabComponent::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());

    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 0, 0, getBottom());
}

void TabComponent::popupMenuClickOnTab(int tabIndex, String const& tabName)
{
    rightClick(tabIndex, tabName);
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

    return reinterpret_cast<Canvas*>(viewport->getViewedComponent());
}

Canvas* TabComponent::getCurrentCanvas()
{
    auto* viewport = dynamic_cast<Viewport*>(getCurrentContentComponent());

    if (!viewport)
        return nullptr;

    return reinterpret_cast<Canvas*>(viewport->getViewedComponent());
}

void TabComponent::mouseDown(MouseEvent const& e)
{
    tabWidth = tabs->getWidth() / std::max(1, getNumTabs());
    clickedTabIndex = getCurrentTabIndex();
    onFocusGrab();
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

            if (tabSnapshot.isNull() && (getParentWidth() != getWidth() || getNumTabs() > 1)) {
                // Create ghost tab & hide dragged tab
                auto* tabButton = tabs->getTabButton(clickedTabIndex);
                currentTabBounds = tabButton->getBounds().translated(getTabBarDepth(), 0);

                auto scale = Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
                tabSnapshot = Image(Image::PixelFormat::ARGB, tabButton->getWidth() * scale, tabButton->getHeight() * scale, true);

                auto g = Graphics(tabSnapshot);
                g.addTransform(AffineTransform::scale(scale));
                getLookAndFeel().drawTabButton(*tabButton, g, false, false);

                tabSnapshotBounds = currentTabBounds;
                tabs->getTabButton(clickedTabIndex)->setVisible(false);
            }
            // Keep ghost tab within view
            auto newPosition = Point<int>(std::clamp(currentTabBounds.getX() + getX() + e.getDistanceFromDragStartX(), 0, getParentWidth() - tabWidth), std::clamp(currentTabBounds.getY() + e.getDistanceFromDragStartY(), 0, getHeight() - tabs->getHeight()));
            tabSnapshotBounds.setPosition(newPosition);
            getParentComponent()->repaint();
        }
    }

void TabComponent::mouseUp(MouseEvent const& e)
{
    tabSnapshot = Image();
    if (clickedTabIndex >= 0)
        tabs->getTabButton(clickedTabIndex)->setVisible(true);
    getParentComponent()->repaint(tabSnapshotBounds);
}