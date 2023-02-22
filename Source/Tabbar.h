/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

// Special viewport that shows scrollbars on top of content instead of next to it
struct InsetViewport : public Viewport {
    struct ViewportPositioner : public Component::Positioner {
        ViewportPositioner(Viewport& comp)
            : Component::Positioner(comp)
            , inset(comp.getScrollBarThickness())
        {
        }

        void applyNewBounds(Rectangle<int> const& newBounds) override
        {
            auto& component = getComponent();
            if (newBounds != component.getBounds()) {
                component.setBounds(newBounds.withTrimmedRight(-inset).withTrimmedBottom(-inset));
            }
        }

        int inset;
    };

    InsetViewport()
    {
        setPositioner(new ViewportPositioner(*this));
    }

    void adjustScrollbarBounds()
    {
        auto& vbar = getVerticalScrollBar();
        auto& hbar = getHorizontalScrollBar();
        auto thickness = getScrollBarThickness();

        auto contentArea = getLocalBounds().withTrimmedRight(thickness).withTrimmedBottom(thickness);

        vbar.setBounds(contentArea.removeFromRight(thickness));
        hbar.setBounds(contentArea.removeFromBottom(thickness));
    }

    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    void resized() override
    {
        Viewport::resized();
        adjustScrollbarBounds();
    }
};

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

            PlugDataLook::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
            PlugDataLook::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
            PlugDataLook::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Thin, 14);
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

        PlugDataLook::drawStyledText(g, "No Patch Open", 0, getHeight() / 2 - 150, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        PlugDataLook::drawStyledText(g, "Open a file to begin patching", 0, getHeight() / 2 - 120, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Thin, 23, Justification::centred);
    }

    std::unique_ptr<WelcomeButton> newButton;
    std::unique_ptr<WelcomeButton> openButton;
};

class TabComponent : public TabbedComponent {

    TextButton newButton = TextButton(Icons::Add);
    WelcomePanel welcomePanel;

public:
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
            onTabChange(newCurrentTabIndex);
        }
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

    virtual void popupMenuClickOnTab(int tabIndex, String const& tabName) override
    {
        rightClick(tabIndex, tabName);
    }

    void mouseDown(MouseEvent const& e) override
    {
        currentTabIndex = getCurrentTabIndex();
        tabWidth = tabs->getWidth() / getNumTabs();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        // Drag tabs to move their index
        int const dragPosition = e.getEventRelativeTo(tabs.get()).x;
        int const newTabIndex = (dragPosition < currentTabIndex * tabWidth) ? currentTabIndex - 1
            : (dragPosition >= (currentTabIndex + 1) * tabWidth)            ? currentTabIndex + 1
                                                                            : currentTabIndex;

        if (newTabIndex != currentTabIndex && newTabIndex >= 0 && newTabIndex < getNumTabs()) {
            moveTab(currentTabIndex, newTabIndex, true);
            currentTabIndex = newTabIndex;
        }
    }

private:
    int currentTabIndex;
    int tabWidth;
};
