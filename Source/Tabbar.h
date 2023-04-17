/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/GlobalMouseListener.h"
#include "Constants.h"
#include "Canvas.h"

class WelcomePanel : public Component {

    class WelcomeButton : public Component {
        String iconText;
        String topText;
        String bottomText;

    public:
        std::function<void(void)> onClick = []() {};

        WelcomeButton(String icon, String mainText, String subText)
            : iconText(icon)
            , topText(mainText)
            , bottomText(subText)
        {
            setInterceptsMouseClicks(true, false);
            setAlwaysOnTop(true);
        }

        void paint(Graphics& g)
        {
            auto colour = findColour(PlugDataColour::panelTextColourId);
            if (isMouseOver()) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                g.fillRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, 6.0f);
                colour = findColour(PlugDataColour::panelActiveTextColourId);
            }

            Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
            Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
            Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Thin, 14);
        }

        void mouseUp(MouseEvent const& e)
        {
            onClick();
        }

        void mouseEnter(MouseEvent const& e)
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e)
        {
            repaint();
        }
    };

public:
    WelcomePanel()
    {
        newButton = std::make_unique<WelcomeButton>(Icons::New, "New patch", "Create a new empty patch");
        openButton = std::make_unique<WelcomeButton>(Icons::Open, "Open patch...", "Open a saved patch");

        addAndMakeVisible(newButton.get());
        addAndMakeVisible(openButton.get());
    }

    void resized() override
    {
        newButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, -30));
        openButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, 30));
    }

    void paint(Graphics& g) override
    {
        auto styles = Font(32).getAvailableStyles();

        g.fillAll(findColour(PlugDataColour::panelBackgroundColourId));

        Fonts::drawStyledText(g, "No Patch Open", 0, getHeight() / 2 - 150, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        Fonts::drawStyledText(g, "Open a file to begin patching", 0, getHeight() / 2 - 120, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Thin, 23, Justification::centred);
    }

    std::unique_ptr<WelcomeButton> newButton;
    std::unique_ptr<WelcomeButton> openButton;
};

class Canvas;
class TabComponent : public TabbedComponent
    , public AsyncUpdater {

    TextButton newButton = TextButton(Icons::Add);
    WelcomePanel welcomePanel;

public:
    std::function<void()> onTabMoved = []() {};
    std::function<void()> onFocusGrab = []() {};
    std::function<void(int)> onTabChange = [](int) {};
    std::function<void()> newTab = []() {};
    std::function<void()> openProject = []() {};
    std::function<void(int tabIndex, String const& tabName)> rightClick = [](int tabIndex, String const& tabName) {};

    TabComponent()
        : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
        addAndMakeVisible(newButton);
        newButton.getProperties().set("FontScale", 0.4f);
        newButton.getProperties().set("Style", "Icon");
        newButton.setTooltip("New patch");
        newButton.onClick = [this]() {
            newTab();
        };

        addAndMakeVisible(welcomePanel);

        welcomePanel.newButton->onClick = [this]() {
            newTab();
        };

        welcomePanel.openButton->onClick = [this]() {
            openProject();
        };

        setVisible(false);
        setTabBarDepth(0);
        tabs.get()->addMouseListener(this, true);
    }

    void currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName) override
    {
        if (getNumTabs() == 0) {
            setTabBarDepth(0);
            getTabbedButtonBar().setVisible(false);
            welcomePanel.setVisible(true);
        } else {
            getTabbedButtonBar().setVisible(true);
            welcomePanel.setVisible(false);
            setTabBarDepth(26);
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override
    {
        onTabChange(getCurrentTabIndex());
    }

    void resized() override
    {
        int depth = getTabBarDepth();
        auto content = getLocalBounds();

        welcomePanel.setBounds(content);
        newButton.setBounds(0, 0, depth, depth);

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

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(26));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());

        g.drawLine(Line<float>(getTabBarDepth() - 0.5f, 0, getTabBarDepth() - 0.5f, getTabBarDepth()), 1.0f);

        g.drawLine(0, 0, getWidth(), 0);
        g.drawLine(0, 0, 0, getBottom());
    }

    void popupMenuClickOnTab(int tabIndex, String const& tabName) override
    {
        rightClick(tabIndex, tabName);
    }

    int getIndexOfCanvas(Canvas* cnv)
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

    Canvas* getCanvas(int idx)
    {
        auto* viewport = dynamic_cast<Viewport*>(getTabContentComponent(idx));

        if (!viewport)
            return nullptr;

        return reinterpret_cast<Canvas*>(viewport->getViewedComponent());
    }

    Canvas* getCurrentCanvas()
    {
        auto* viewport = dynamic_cast<Viewport*>(getCurrentContentComponent());

        if (!viewport)
            return nullptr;

        return reinterpret_cast<Canvas*>(viewport->getViewedComponent());
    }

    void mouseDown(MouseEvent const& e) override
    {
        tabWidth = tabs->getWidth() / std::max(1, getNumTabs());
        clickedTabIndex = getCurrentTabIndex();
        onFocusGrab();
    }

    void mouseDrag(MouseEvent const& e) override
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
                currentTabBounds = tabs->getTabButton(clickedTabIndex)->getBounds().translated(getTabBarDepth(), 0);
                tabSnapshot = createComponentSnapshot(currentTabBounds, true, 2.0f);
                
                // Make sure the tab has a full outline
                Graphics g(tabSnapshot);
                g.setColour(findColour(PlugDataColour::outlineColourId));
                g.drawRect(tabSnapshot.getBounds());
                
                tabSnapshotBounds = currentTabBounds;
                tabs->getTabButton(clickedTabIndex)->setVisible(false);
            }
            // Keep ghost tab within view
            auto newPosition = Point<int>(std::clamp(currentTabBounds.getX() + getX() + e.getDistanceFromDragStartX(), 0, getParentWidth() - tabWidth), std::clamp(currentTabBounds.getY() + e.getDistanceFromDragStartY(), 0, getHeight() - tabs->getHeight()));
            tabSnapshotBounds.setPosition(newPosition);
            getParentComponent()->repaint();
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        tabSnapshot = Image();
        if (clickedTabIndex >= 0)
            tabs->getTabButton(clickedTabIndex)->setVisible(true);
        getParentComponent()->repaint(tabSnapshotBounds);
    }

    Image tabSnapshot;
    Rectangle<int> tabSnapshotBounds;
    Rectangle<int> currentTabBounds;

private:
    int clickedTabIndex;
    int tabWidth;
};
