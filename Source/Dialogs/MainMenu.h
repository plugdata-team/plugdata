/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <utility>

#include "PluginEditor.h"
#include "Utility/Autosave.h"

class MainMenu : public PopupMenu {

public:
    explicit MainMenu(PluginEditor* editor)
        : settingsTree(SettingsFile::getInstance()->getValueTree())
        , themeSelector(settingsTree)
    {
        addCustomItem(1, themeSelector, 70, 45, false);
        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::NewPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::NewPatch)]), nullptr, "New patch");

        addCustomItem(getMenuItemID(MenuItem::OpenPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::OpenPatch)]), nullptr, "Open patch");

        auto recentlyOpened = new PopupMenu();

        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
        if (recentlyOpenedTree.isValid()) {
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                auto path = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                recentlyOpened->addItem(path.getFileName(), [path, editor]() mutable {
                    if(path.existsAsFile()) {
                        editor->autosave->checkForMoreRecentAutosave(path, editor, [editor, path]() {
                            editor->getTabComponent().openPatch(URL(path));
                            SettingsFile::getInstance()->addToRecentlyOpened(path);
                        });
                    }
                    else {
                        editor->pd->logError("Patch not found");
                    }
                });
            }

            auto isActive = menuItems[2]->isActive = recentlyOpenedTree.getNumChildren() > 0;
            if (isActive) {
                recentlyOpened->addSeparator();
                recentlyOpened->addItem("Clear recently opened", [recentlyOpenedTree]() mutable {
                    recentlyOpenedTree.removeAllChildren(nullptr);
                    SettingsFile::getInstance()->reloadSettings();
                });
            }
        }

        addCustomItem(getMenuItemID(MenuItem::History), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::History)]), std::unique_ptr<PopupMenu const>(recentlyOpened), "Recently opened");

        addSeparator();
        addCustomItem(getMenuItemID(MenuItem::Save), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Save)]), nullptr, "Save patch");
        addCustomItem(getMenuItemID(MenuItem::SaveAs), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::SaveAs)]), nullptr, "Save patch as");

        auto plugdataState = new PopupMenu();
        plugdataState->addItem("Import workspace", [editor]() mutable {
            static auto openChooser = std::make_unique<FileChooser>("Choose file to open", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pdproj", SettingsFile::getInstance()->wantsNativeDialog());

            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [editor](FileChooser const& f) {
                MemoryBlock block;
                f.getResult().loadFileAsData(block);
                editor->processor.setStateInformation(block.getData(), block.getSize());
            });
        });
        plugdataState->addItem("Export workspace", [editor]() mutable {
            static auto saveChooser = std::make_unique<FileChooser>("Choose save location", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pdproj", SettingsFile::getInstance()->wantsNativeDialog());

            saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles, [editor](FileChooser const& f) {
                auto file = f.getResult();
                if (file.getParentDirectory().exists()) {
                    MemoryBlock destData;
                    editor->processor.getStateInformation(destData);
                    file.replaceWithData(destData.getData(), destData.getSize());
                }
            });
        });

        addCustomItem(getMenuItemID(MenuItem::State), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::State)]), std::unique_ptr<PopupMenu const>(plugdataState), "Workspace");

        addSeparator();

#if !JUCE_IOS
        addCustomItem(getMenuItemID(MenuItem::CompiledMode), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::CompiledMode)]), nullptr, "Compiled mode");
        addCustomItem(getMenuItemID(MenuItem::Compile), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Compile)]), nullptr, "Compile...");
#endif

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::FindExternals), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::FindExternals)]), nullptr, "Find externals...");

        // addCustomItem(getMenuItemID(MenuItem::Discover), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Discover)]), nullptr, "Discover...");

        addCustomItem(getMenuItemID(MenuItem::Settings), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Settings)]), nullptr, "Settings...");
        addCustomItem(getMenuItemID(MenuItem::About), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::About)]), nullptr, "About...");

        // Toggles hvcc compatibility mode
        bool hvccModeEnabled = settingsTree.hasProperty("hvcc_mode") && static_cast<bool>(settingsTree.getProperty("hvcc_mode"));
        bool hasCanvas = editor->getCurrentCanvas() != nullptr;

        menuItems[getMenuItemIndex(MenuItem::Save)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(MenuItem::SaveAs)]->isActive = hasCanvas;

        menuItems[getMenuItemIndex(MenuItem::CompiledMode)]->isTicked = hvccModeEnabled;
    }

    class IconMenuItem : public PopupMenu::CustomComponent {

        String menuItemIcon;
        String menuItemText;

        bool hasSubMenu;
        bool hasTickBox;

    public:
        bool isTicked = false;
        bool isActive = true;

        IconMenuItem(String icon, String text, bool hasChildren, bool tickBox)
            : menuItemIcon(std::move(icon))
            , menuItemText(std::move(text))
            , hasSubMenu(hasChildren)
            , hasTickBox(tickBox)
        {
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 70;
            idealHeight = 24;
        }

        void paint(Graphics& g) override
        {
            auto r = getLocalBounds();

            auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isItemHighlighted() && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));

                g.fillRoundedRectangle(r.toFloat().reduced(0, 1), Corners::defaultCornerRadius);
            }

            g.setColour(colour);

            r.reduce(jmin(5, r.getWidth() / 20), 0);

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).withSizeKeepingCentre(maxFontHeight, maxFontHeight);

            if (menuItemIcon.isNotEmpty()) {
                Fonts::drawIcon(g, menuItemIcon, iconArea.translated(3.0f, 0.0f), colour, std::min(15.0f, maxFontHeight), true);
            } else if (hasTickBox) {

                g.setColour(colour);
                g.drawRoundedRectangle(iconArea.toFloat().translated(3.0f, 0.5f).reduced(1.0f), 4.0f, 1.0f);

                if (isTicked) {
                    g.setColour(colour);
                    auto tick = getLookAndFeel().getTickShape(1.0f);
                    g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.toFloat().translated(3.5f, 0.5f).reduced(2.5f, 3.5f), false));
                }
            }

            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));

            int fontHeight = std::min(17.0f, maxFontHeight);
            if (hasSubMenu) {
                auto arrowH = 0.6f * Font(fontHeight).getAscent();

                auto x = static_cast<float>(r.removeFromRight((int)arrowH + 2).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.5f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(1.5f));
            }

            r.removeFromRight(3);
            Fonts::drawFittedText(g, menuItemText, r, colour, fontHeight);
        }
    };

    class ThemeSelector : public Component
        , public AsyncUpdater {

        Value theme;
        ValueTree settingsTree;

    public:
        explicit ThemeSelector(ValueTree tree)
            : settingsTree(std::move(tree))
        {
            theme.referTo(settingsTree.getPropertyAsValue("theme", nullptr));
        }

        void paint(Graphics& g) override
        {
            auto secondBounds = getLocalBounds();
            auto firstBounds = secondBounds.removeFromLeft(getWidth() / 2.0f);

            firstBounds = firstBounds.withSizeKeepingCentre(30, 30);
            secondBounds = secondBounds.withSizeKeepingCentre(30, 30);

            auto themesTree = settingsTree.getChildWithName("ColourThemes");
            auto firstThemeTree = themesTree.getChildWithProperty("theme", PlugDataLook::selectedThemes[0]);
            auto secondThemeTree = themesTree.getChildWithProperty("theme", PlugDataLook::selectedThemes[1]);

            g.setColour(PlugDataLook::getThemeColour(firstThemeTree, PlugDataColour::canvasBackgroundColourId));
            g.fillEllipse(firstBounds.toFloat());

            g.setColour(PlugDataLook::getThemeColour(secondThemeTree, PlugDataColour::canvasBackgroundColourId));
            g.fillEllipse(secondBounds.toFloat());

            g.setColour(PlugDataLook::getThemeColour(firstThemeTree, PlugDataColour::outlineColourId));
            g.drawEllipse(firstBounds.toFloat(), 1.0f);

            g.setColour(PlugDataLook::getThemeColour(secondThemeTree, PlugDataColour::outlineColourId));
            g.drawEllipse(secondBounds.toFloat(), 1.0f);

            auto tick = getLookAndFeel().getTickShape(0.6f);
            auto tickBounds = Rectangle<int>();

            if (theme.toString() == firstThemeTree.getProperty("theme").toString()) {
                auto textColour = PlugDataLook::getThemeColour(firstThemeTree, PlugDataColour::canvasBackgroundColourId).contrasting(0.8f);
                g.setColour(textColour);
                tickBounds = firstBounds;
            } else {
                auto textColour = PlugDataLook::getThemeColour(secondThemeTree, PlugDataColour::canvasBackgroundColourId).contrasting(0.8f);
                g.setColour(textColour);
                tickBounds = secondBounds;
            }

            g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(9, 9).toFloat(), false));
        }

        void mouseUp(MouseEvent const& e) override
        {
            auto secondBounds = getLocalBounds();
            auto firstBounds = secondBounds.removeFromLeft(getWidth() / 2.0f);

            firstBounds = firstBounds.withSizeKeepingCentre(30, 30);
            secondBounds = secondBounds.withSizeKeepingCentre(30, 30);

            if (firstBounds.contains(e.x, e.y)) {
                theme = PlugDataLook::selectedThemes[0];
                triggerAsyncUpdate();
            } else if (secondBounds.contains(e.x, e.y)) {
                theme = PlugDataLook::selectedThemes[1];
                triggerAsyncUpdate();
            }
        }

        void handleAsyncUpdate() override
        {
            // Make sure the actual popup menu updates its theme
            getTopLevelComponent()->sendLookAndFeelChange();
        }
    };

    enum MenuItem {
        NewPatch = 1,
        OpenPatch,
        History,
        Save,
        SaveAs,
        State,
        CompiledMode,
        Compile,
        FindExternals,
        Settings,
        About
    };

    static int getMenuItemID(MenuItem item)
    {
        if (item == MenuItem::History)
            return 100;

        return item;
    }

    static int getMenuItemIndex(MenuItem item)
    {
        return item - 1;
    }

    std::vector<IconMenuItem*> menuItems = {
        new IconMenuItem(Icons::New, "New patch", false, false),
        new IconMenuItem(Icons::Open, "Open patch...", false, false),
        new IconMenuItem(Icons::History, "Recently opened", true, false),

        new IconMenuItem(Icons::SavePatch, "Save patch", false, false),
        new IconMenuItem(Icons::SaveAs, "Save patch as...", false, false),

        new IconMenuItem(Icons::ExportState, "Workspace", true, false),

        new IconMenuItem("", "Compiled mode", false, true),
        new IconMenuItem(Icons::DevTools, "Compile...", false, false),

        new IconMenuItem(Icons::Externals, "Find externals...", false, false),
        // new IconMenuItem(Icons::Compass, "Discover...", false, false),
        new IconMenuItem(Icons::Settings, "Settings...", false, false),
        new IconMenuItem(Icons::Info, "About...", false, false),
    };

    ValueTree settingsTree;
    ThemeSelector themeSelector;
};
