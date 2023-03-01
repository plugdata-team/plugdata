/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class MainMenu : public PopupMenu {

public:
    MainMenu(PluginEditor* editor)
        : settingsTree(SettingsFile::getInstance()->getValueTree())
        , themeSelector(settingsTree)
        , zoomSelector(settingsTree, editor->splitView.isRightTabbarActive())
    {
        addCustomItem(1, themeSelector, 70, 45, false);
        addCustomItem(2, zoomSelector, 70, 30, false);
        addSeparator();

        addCustomItem(getMenuItemID(menuItem::newPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::newPatch)]), nullptr, "New patch");

        addSeparator();

        addCustomItem(getMenuItemID(menuItem::openPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::openPatch)]), nullptr, "Open patch");

        auto recentlyOpened = new PopupMenu();

        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
        if (recentlyOpenedTree.isValid()) {
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                auto path = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                recentlyOpened->addItem(path.getFileName(), [this, path, editor]() mutable {
                    editor->pd->loadPatch(path);
                    SettingsFile::getInstance()->addToRecentlyOpened(path);
                });
            }

            menuItems[2]->isActive = recentlyOpenedTree.getNumChildren() > 0;
        }

        addCustomItem(getMenuItemID(menuItem::history), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::history)]), std::unique_ptr<PopupMenu const>(recentlyOpened), "Recently opened");

        addSeparator();
        addCustomItem(getMenuItemID(menuItem::save), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::save)]), nullptr, "Save patch");
        addCustomItem(getMenuItemID(menuItem::saveAs), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::saveAs)]), nullptr, "Save patch as");

        addSeparator();

        addCustomItem(getMenuItemID(menuItem::close), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::close)]), nullptr, "Close patch");
        addCustomItem(getMenuItemID(menuItem::closeAll), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::closeAll)]), nullptr, "Close all patches");

        addSeparator();

        addCustomItem(getMenuItemID(menuItem::compiledMode), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::compiledMode)]), nullptr, "Compiled mode");
        addCustomItem(getMenuItemID(menuItem::compile), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::compile)]), nullptr, "Compile...");

        addSeparator();

        addCustomItem(getMenuItemID(menuItem::autoConnect), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::autoConnect)]), nullptr, "Auto-connect objects");

        addSeparator();

        addCustomItem(getMenuItemID(menuItem::settings), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::settings)]), nullptr, "Settings...");
        addCustomItem(getMenuItemID(menuItem::about), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(menuItem::about)]), nullptr, "About...");

        // Toggles hvcc compatibility mode
        bool hvccModeEnabled = settingsTree.hasProperty("hvcc_mode") ? static_cast<bool>(settingsTree.getProperty("hvcc_mode")) : false;
        bool autoconnectEnabled = settingsTree.hasProperty("autoconnect") ? static_cast<bool>(settingsTree.getProperty("autoconnect")) : false;
        bool hasCanvas = editor->getCurrentCanvas() != nullptr;
        ;

        menuItems[getMenuItemIndex(menuItem::save)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(menuItem::saveAs)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(menuItem::close)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(menuItem::closeAll)]->isActive = hasCanvas;

        menuItems[getMenuItemIndex(menuItem::compiledMode)]->isTicked = hvccModeEnabled;
        menuItems[getMenuItemIndex(menuItem::autoConnect)]->isTicked = autoconnectEnabled;
    }

    class ZoomSelector : public Component {
        TextButton zoomIn;
        TextButton zoomOut;
        TextButton zoomReset;

        Value zoomValue;

    public:
        ZoomSelector(ValueTree settingsTree, bool splitZoom)
        {
            zoomValue = settingsTree.getPropertyAsValue(splitZoom ? "split_zoom" : "zoom", nullptr);

            zoomIn.setButtonText("+");
            zoomReset.setButtonText(String(static_cast<float>(zoomValue.getValue()) * 100, 1) + "%");
            zoomOut.setButtonText("-");

            addAndMakeVisible(zoomIn);
            addAndMakeVisible(zoomReset);
            addAndMakeVisible(zoomOut);

            zoomIn.setConnectedEdges(Button::ConnectedOnLeft);
            zoomOut.setConnectedEdges(Button::ConnectedOnRight);
            zoomReset.setConnectedEdges(12);

            zoomIn.onClick = [this]() {
                applyZoom(true);
            };
            zoomOut.onClick = [this]() {
                applyZoom(false);
            };
            zoomReset.onClick = [this]() {
                resetZoom();
            };
        }

        void applyZoom(bool zoomIn)
        {
            float value = static_cast<float>(zoomValue.getValue());

            // Apply limits
            value = std::clamp(zoomIn ? value + 0.1f : value - 0.1f, 0.5f, 2.0f);

            // Round in case we zoomed with scrolling
            value = static_cast<float>(static_cast<int>(round(value * 10.))) / 10.;

            zoomValue = value;

            zoomReset.setButtonText(String(value * 100.0f, 1) + "%");
        }

        void resetZoom()
        {
            zoomValue = 1.0f;
            zoomReset.setButtonText("100.0%");
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(8, 4);
            int buttonWidth = (getWidth() - 8) / 3;

            zoomOut.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
            zoomReset.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
            zoomIn.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
        }
    };

    class IconMenuItem : public PopupMenu::CustomComponent {

        String menuItemIcon;
        String menuItemText;

        bool hasSubMenu;
        bool hasTickBox;
        bool isMouseOver = false;

    public:
        bool isTicked = false;
        bool isActive = true;

        IconMenuItem(String icon, String text, bool hasChildren, bool tickBox)
            : menuItemIcon(icon)
            , menuItemText(text)
            , hasSubMenu(hasChildren)
            , hasTickBox(tickBox)
        {
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 70;
            idealHeight = 22;
        }

        void paint(Graphics& g) override
        {
            auto r = getLocalBounds().reduced(0, 1);

            auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isItemHighlighted() && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(2, 0), 4.0f);

                colour = findColour(PlugDataColour::popupMenuActiveTextColourId);
            }

            g.setColour(colour);

            r.reduce(jmin(5, r.getWidth() / 20), 0);

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).withSizeKeepingCentre(maxFontHeight, maxFontHeight);

            if (menuItemIcon.isNotEmpty()) {
                PlugDataLook::drawIcon(g, menuItemIcon, iconArea, colour, std::min(15.0f, maxFontHeight), false);
            } else if (hasTickBox) {

                g.setColour(colour);
                g.drawRoundedRectangle(iconArea.toFloat().translated(0, 0.5f), 4.0f, 1.0f);

                if (isTicked) {
                    g.setColour(colour);
                    auto tick = getLookAndFeel().getTickShape(1.0f);
                    g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.toFloat().translated(0, 0.5f).reduced(2.5f, 3.5f), false));
                }

                /*
                auto tick = lnf.getTickShape(1.0f);
                g.fillPath(tick, tick.getTransformToScaleToFit(, true)); */
            }

            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));

            int fontHeight = std::min(17.0f, maxFontHeight);
            if (hasSubMenu) {
                auto arrowH = 0.6f * Font(fontHeight).getAscent();

                auto x = static_cast<float>(r.removeFromRight((int)arrowH).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            PlugDataLook::drawFittedText(g, menuItemText, r, colour, fontHeight);

            /*
            if (shortcutKeyText.isNotEmpty()) {
             PlugDataLook::drawText(g, shortcutKeyText, r.translated(-2, 0), findColour(PopupMenu::textColourId), f2.getHeight() * 0.75f, Justification::centredRight);
            } */
        }
    };

    class ThemeSelector : public Component {

        Value theme;
        ValueTree settingsTree;

    public:
        ThemeSelector(ValueTree tree)
            : settingsTree(tree)
        {
            theme.referTo(settingsTree.getPropertyAsValue("theme", nullptr));
        }

        void paint(Graphics& g)
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

            g.setColour(PlugDataLook::getThemeColour(firstThemeTree, PlugDataColour::objectOutlineColourId));
            g.drawEllipse(firstBounds.toFloat(), 1.0f);

            g.setColour(PlugDataLook::getThemeColour(secondThemeTree, PlugDataColour::objectOutlineColourId));
            g.drawEllipse(secondBounds.toFloat(), 1.0f);

            auto tick = getLookAndFeel().getTickShape(0.6f);
            auto tickBounds = Rectangle<int>();

            if (theme.toString() == firstThemeTree.getProperty("theme").toString()) {
                g.setColour(PlugDataLook::getThemeColour(firstThemeTree, PlugDataColour::canvasTextColourId));
                tickBounds = firstBounds;
            } else {
                g.setColour(PlugDataLook::getThemeColour(secondThemeTree, PlugDataColour::canvasTextColourId));
                tickBounds = secondBounds;
            }

            g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(9, 9).toFloat(), false));
        }

        void mouseUp(MouseEvent const& e)
        {
            auto secondBounds = getLocalBounds();
            auto firstBounds = secondBounds.removeFromLeft(getWidth() / 2.0f);

            firstBounds = firstBounds.withSizeKeepingCentre(30, 30);
            secondBounds = secondBounds.withSizeKeepingCentre(30, 30);

            if (firstBounds.contains(e.x, e.y)) {
                theme = PlugDataLook::selectedThemes[0];
                repaint();
            } else if (secondBounds.contains(e.x, e.y)) {
                theme = PlugDataLook::selectedThemes[1];
                repaint();
            }
        }
    };

    enum menuItem {
        newPatch = 1,
        openPatch,
        history,
        save,
        saveAs,
        close,
        closeAll,
        compiledMode,
        compile,
        autoConnect,
        settings,
        about
    };

    int getMenuItemID(menuItem item)
    {
        if (item == menuItem::history)
            return 100;

        return item;
    };

    int getMenuItemIndex(menuItem item)
    {
        return item - 1;
    }

    std::vector<IconMenuItem*> menuItems = {
        new IconMenuItem(Icons::New, "New patch", false, false),
        new IconMenuItem(Icons::Open, "Open patch...", false, false),
        new IconMenuItem(Icons::History, "Recently opened", true, false),

        new IconMenuItem(Icons::Save, "Save patch", false, false),
        new IconMenuItem(Icons::SaveAs, "Save patch as...", false, false),

        new IconMenuItem(Icons::Clear, "Close patch", false, false),
        new IconMenuItem(Icons::Clear, "Close all patches", false, false),

        new IconMenuItem("", "Compiled Mode", false, true),
        new IconMenuItem("", "Compile...", false, false),

        new IconMenuItem("", "Auto-connect objects", false, true),

        new IconMenuItem(Icons::Settings, "Settings...", false, false),
        new IconMenuItem(Icons::Info, "About...", false, false),
    };

    ValueTree settingsTree;
    ThemeSelector themeSelector;
    ZoomSelector zoomSelector;
};
