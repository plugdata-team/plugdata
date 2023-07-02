/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>

#include <utility>

#include <utility>

#include "Utility/GlobalMouseListener.h"
#include "Utility/BouncingViewport.h"
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
            : iconText(std::move(std::move(std::move(icon))))
            , topText(std::move(mainText))
            , bottomText(std::move(subText))
        {
            setInterceptsMouseClicks(true, false);
            setAlwaysOnTop(true);
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::panelTextColourId);
            if (isMouseOver()) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                PlugDataLook::fillSmoothedRectangle(g, Rectangle<float>(1, 1, getWidth() - 2, getHeight() - 2), Corners::largeCornerRadius);
                colour = findColour(PlugDataColour::panelActiveTextColourId);
            }

            Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
            Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
            Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Thin, 14);
        }

        void mouseUp(MouseEvent const& e) override
        {
            onClick();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            repaint();
        }
    };

    class RecentlyOpenedListBox : public Component
        , public ListBoxModel {
    public:
        RecentlyOpenedListBox()
        {
            listBox.setModel(this);
            listBox.setClickingTogglesRowSelection(true);
            update();

            listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

            addAndMakeVisible(listBox);
            
            bouncer = std::make_unique<BouncingViewportAttachment>(listBox.getViewport());
        }

        void update()
        {
            items.clear();

            auto settingsTree = SettingsFile::getInstance()->getValueTree();
            auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
            if (recentlyOpenedTree.isValid()) {
                for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                    auto path = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                    items.add({ path.getFileName(), path });
                }
            }

            listBox.updateContent();
        }

        std::function<void(File)> onPatchOpen = [](File) {};

    private:
        int getNumRows() override
        {
            return items.size();
        }

        void listBoxItemClicked(int row, MouseEvent const& e) override
        {
            if (e.getNumberOfClicks() >= 2) {
                onPatchOpen(items[row].second);
            }
        }

        void paint(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::outlineColourId));
            PlugDataLook::drawSmoothedRectangle(g, PathStrokeType(1.0f), Rectangle<float>(1, 36, getWidth() - 2, getHeight() - 37), Corners::defaultCornerRadius);

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
                PlugDataLook::fillSmoothedRectangle(g, { 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Corners::defaultCornerRadius);
            }

            auto colour = rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);

            Fonts::drawText(g, items[rowNumber].first, 12, 0, width - 9, height, colour, 15);
        }

        std::unique_ptr<BouncingViewportAttachment> bouncer;
        ListBox listBox;
        Array<std::pair<String, File>> items;
    };

public:
    WelcomePanel()
        : newButton(Icons::New, "New patch", "Create a new empty patch")
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

        if (getHeight() > 400) {
            recentlyOpened.setBounds(getLocalBounds().withSizeKeepingCentre(275, 160).translated(0, 110));
            recentlyOpened.setVisible(true);
        } else {
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

class PluginEditor;
class TabComponent : public TabbedComponent
    , public AsyncUpdater {

    TextButton newButton;
    WelcomePanel welcomePanel;
    PluginEditor* editor;

public:
    TabComponent(PluginEditor* editor);

    void onTabMoved();
    void onFocusGrab();
    void onTabChange(int tabIndex);
    void newTab();
    void openProject();
    void openProjectFile(File& patchFile);
    void rightClick(int tabIndex, String const& tabName);

    void setSplitFocusIndex(int idx = -1);

    void currentTabChanged(int newCurrentTabIndex, String const& newCurrentTabName) override;
    void handleAsyncUpdate() override;
    void resized() override;

    void paint(Graphics& g) override;

    void paintOverChildren(Graphics& g) override;

    void popupMenuClickOnTab(int tabIndex, String const& tabName) override;

    int getIndexOfCanvas(Canvas* cnv);

    Canvas* getCanvas(int idx);

    Canvas* getCurrentCanvas();

    void mouseDown(MouseEvent const& e) override;

    void mouseDrag(MouseEvent const& e) override;

    void mouseUp(MouseEvent const& e) override;

    Image tabSnapshot;
    Rectangle<int> tabSnapshotBounds;
    Rectangle<int> currentTabBounds;

private:
    int clickedTabIndex;
    int tabWidth;
};
