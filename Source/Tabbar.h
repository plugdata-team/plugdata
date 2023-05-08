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
    
    class RecentlyOpenedListBox : public Component, public ListBoxModel
    {
    public:
        RecentlyOpenedListBox()
        {
            listBox.setModel(this);
            listBox.setClickingTogglesRowSelection(true);
            update();
            
            listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
            
            addAndMakeVisible(listBox);
        }
        
        void update()
        {
            items.clear();
            
            auto settingsTree = SettingsFile::getInstance()->getValueTree();
            auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
            if (recentlyOpenedTree.isValid()) {
                for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                    auto path = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                    items.add({path.getFileName(), path});
                }
            }
            
            listBox.updateContent();
        }
           
        std::function<void(File)> onPatchOpen = [](File){};
        
    private:
        int getNumRows() override
        {
            return items.size();
        }
        
        void listBoxItemClicked(int row, const MouseEvent& e) override
        {
            if(e.getNumberOfClicks() >= 2)
            {
                onPatchOpen(items[row].second);
            }
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(1, 36, getWidth() - 2, getHeight() - 37, Corners::defaultCornerRadius, 1.0f);
            
            Fonts::drawStyledText(g, "Recently Opened", 0, 0, getWidth(), 30, findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);

        }
        
        void resized() override
        {
            listBox.setBounds(getLocalBounds().withTrimmedTop(35));
        }
        
        void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Corners::defaultCornerRadius);
            }

            auto colour = rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);
            
            Fonts::drawText(g, items[rowNumber].first, 12, 0, width - 9, height, colour, 15);
        }
   
        ListBox listBox;
        Array<std::pair<String, File>> items;
    };

public:
    WelcomePanel() : newButton(Icons::New, "New patch", "Create a new empty patch")
                   , openButton(Icons::Open, "Open patch...", "Open a saved patch")
                    
            
    {
        addAndMakeVisible(newButton);
        addAndMakeVisible(openButton);
        addAndMakeVisible(recentlyOpened);
    }

    void resized() override
    {
        newButton.setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(0, -70));
        openButton.setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(0, -10));
        
        if(getHeight() > 400) {
            recentlyOpened.setBounds(getLocalBounds().withSizeKeepingCentre(275, 160).translated(0, 110));
            recentlyOpened.setVisible(true);
        }
        else {
            recentlyOpened.setVisible(false);
        }
    }
            
    void show()
    {
        recentlyOpened.update();
        setVisible(true);
    }
            
    void hide()
    {
        setVisible(false);
    }
            
    void paint(Graphics& g) override
    {
        auto styles = Font(32).getAvailableStyles();

        g.fillAll(findColour(PlugDataColour::panelBackgroundColourId));

        Fonts::drawStyledText(g, "No Patch Open", 0, getHeight() / 2 - 195, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        Fonts::drawStyledText(g, "Open a file to begin patching", 0, getHeight() / 2 - 160, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Thin, 23, Justification::centred);
    }

    WelcomeButton newButton;
    WelcomeButton openButton;
            
    RecentlyOpenedListBox recentlyOpened;
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
    std::function<void(File&)> openProjectFile = [](File&) {};
    std::function<void(int tabIndex, String const& tabName)> rightClick = [](int tabIndex, String const& tabName) {};

    TabComponent()
        : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
        addAndMakeVisible(newButton);
        newButton.getProperties().set("Style", "LargeIcon");
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
        tabs.get()->addMouseListener(this, true);
    }

    void currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName) override
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

    void handleAsyncUpdate() override
    {
        onTabChange(getCurrentTabIndex());
    }

    void resized() override
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

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(30));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());

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
                auto* tabButton = tabs->getTabButton(clickedTabIndex);
                currentTabBounds = tabButton->getBounds().translated(getTabBarDepth(), 0);
                
                tabSnapshot = Image(Image::PixelFormat::ARGB, tabButton->getWidth(), tabButton->getHeight(), true);
                
                auto g = Graphics(tabSnapshot);
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
