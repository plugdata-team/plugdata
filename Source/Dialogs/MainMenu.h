/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../PluginEditor.h"

class MainMenu : public PopupMenu {

public:
    MainMenu(PluginEditor* editor)
        : settingsTree(SettingsFile::getInstance()->getValueTree())
        , themeSelector(settingsTree)
        , zoomSelector(editor, settingsTree, editor->splitView.isRightTabbarActive())
    {
        addCustomItem(1, themeSelector, 70, 45, false);
        addCustomItem(2, zoomSelector, 70, 30, false);
        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::NewPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::NewPatch)]), nullptr, "New patch");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::OpenPatch), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::OpenPatch)]), nullptr, "Open patch");

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

        addCustomItem(getMenuItemID(MenuItem::History), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::History)]), std::unique_ptr<PopupMenu const>(recentlyOpened), "Recently opened");

        addSeparator();
        addCustomItem(getMenuItemID(MenuItem::Save), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Save)]), nullptr, "Save patch");
        addCustomItem(getMenuItemID(MenuItem::SaveAs), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::SaveAs)]), nullptr, "Save patch as");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::Close), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Close)]), nullptr, "Close patch");
        addCustomItem(getMenuItemID(MenuItem::CloseAll), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::CloseAll)]), nullptr, "Close all patches");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::CompiledMode), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::CompiledMode)]), nullptr, "Compiled mode");
        addCustomItem(getMenuItemID(MenuItem::Compile), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Compile)]), nullptr, "Compile...");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::EnablePalettes), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::EnablePalettes)]), nullptr, "Enable Palettes");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::PluginMode), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::PluginMode)]), nullptr, "Plugin Mode");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::AutoConnect), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::AutoConnect)]), nullptr, "Auto-connect objects");

        addSeparator();

        addCustomItem(getMenuItemID(MenuItem::FindExternals), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::FindExternals)]), nullptr, "Find externals...");
        
        addCustomItem(getMenuItemID(MenuItem::Settings), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::Settings)]), nullptr, "Settings...");
        addCustomItem(getMenuItemID(MenuItem::About), std::unique_ptr<IconMenuItem>(menuItems[getMenuItemIndex(MenuItem::About)]), nullptr, "About...");

        // Toggles hvcc compatibility mode
        bool palettesEnabled = settingsTree.hasProperty("show_palettes") ? static_cast<bool>(settingsTree.getProperty("show_palettes")) : false;
        bool hvccModeEnabled = settingsTree.hasProperty("hvcc_mode") ? static_cast<bool>(settingsTree.getProperty("hvcc_mode")) : false;
        bool autoconnectEnabled = settingsTree.hasProperty("autoconnect") ? static_cast<bool>(settingsTree.getProperty("autoconnect")) : false;
        bool hasCanvas = editor->getCurrentCanvas() != nullptr;

        zoomSelector.setEnabled(hasCanvas);
        menuItems[getMenuItemIndex(MenuItem::Save)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(MenuItem::SaveAs)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(MenuItem::Close)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(MenuItem::CloseAll)]->isActive = hasCanvas;
        menuItems[getMenuItemIndex(MenuItem::PluginMode)]->isActive = hasCanvas;

        menuItems[getMenuItemIndex(MenuItem::EnablePalettes)]->isTicked = palettesEnabled;
        menuItems[getMenuItemIndex(MenuItem::CompiledMode)]->isTicked = hvccModeEnabled;
        menuItems[getMenuItemIndex(MenuItem::AutoConnect)]->isTicked = autoconnectEnabled;
        menuItems[getMenuItemIndex(MenuItem::PluginMode)]->isTicked = false;
    }

    class ZoomSelector : public Component {
        TextButton zoomIn;
        TextButton zoomOut;
        TextButton zoomReset;

        Value zoomValue;

        PluginEditor* _editor;

        float const minZoom = 0.2f;
        float const maxZoom = 3.0f;

    public:
        ZoomSelector(PluginEditor* editor, ValueTree settingsTree, bool splitZoom)
            : _editor(editor)
        {
            auto cnv = _editor->getCurrentCanvas();
            auto buttonText = String("100.0%");
            if (cnv)
                buttonText = String(getValue<float>(cnv->zoomScale) * 100.0f, 1) + "%";

            zoomIn.setButtonText("+");
            zoomReset.setButtonText(buttonText);
            zoomOut.setButtonText("-");

            addAndMakeVisible(zoomIn);
            addAndMakeVisible(zoomReset);
            addAndMakeVisible(zoomOut);

            zoomIn.setConnectedEdges(Button::ConnectedOnLeft);
            zoomOut.setConnectedEdges(Button::ConnectedOnRight);
            zoomReset.setConnectedEdges(12);

            zoomIn.onClick = [this]() {
                applyZoom(ZoomIn);
            };
            zoomOut.onClick = [this]() {
                applyZoom(ZoomOut);
            };
            zoomReset.onClick = [this]() {
                applyZoom(Reset);
            };
        }

        enum ZoomType { ZoomIn,
            ZoomOut,
            Reset };

        void applyZoom(ZoomType zoomEventType)
        {
            auto cnv = _editor->getCurrentCanvas();

            if (!cnv)
                return;

            float scale = getValue<float>(cnv->zoomScale);

            // Apply limits
            switch (zoomEventType) {
            case ZoomIn:
                scale = std::clamp(scale + 0.1f, minZoom, maxZoom);
                break;
            case ZoomOut:
                scale = std::clamp(scale - 0.1f, minZoom, maxZoom);
                break;
            default:
                scale = 1.0f;
                break;
            }

            // Round in case we zoomed with scrolling
            scale = static_cast<float>(static_cast<int>(round(scale * 10.))) / 10.;

            // Get the current viewport position in canvas coordinates
            auto oldViewportPosition = cnv->getLocalPoint(cnv->viewport, cnv->viewport->getViewArea().withZeroOrigin().toFloat().getCentre());

            // Apply transform and make sure viewport bounds get updated
            cnv->setTransform(AffineTransform::scale(scale));
            cnv->viewport->resized();

            // After zooming, get the new viewport position in canvas coordinates
            auto newViewportPosition = cnv->getLocalPoint(cnv->viewport, cnv->viewport->getViewArea().withZeroOrigin().toFloat().getCentre());

            // Calculate offset to keep the center point of the viewport the same as before this zoom action
            auto offset = newViewportPosition - oldViewportPosition;

            // Set the new canvas position
            // TODO: there is an accumulated error when zooming in/out
            //       possibly we should save the canvas position as an additional Point<float> ?
            cnv->setTopLeftPosition((cnv->getPosition().toFloat() + offset).roundToInt());

            cnv->zoomScale = scale;

            zoomReset.setButtonText(String(scale * 100.0f, 1) + "%");
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
            idealHeight = 24;
        }

        void paint(Graphics& g) override
        {
            auto r = getLocalBounds().reduced(0, 1);

            auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isItemHighlighted() && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(2, 0), Corners::smallCornerRadius);

                colour = findColour(PlugDataColour::popupMenuActiveTextColourId);
            }

            g.setColour(colour);

            r.reduce(jmin(5, r.getWidth() / 20), 0);

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).withSizeKeepingCentre(maxFontHeight, maxFontHeight);

            if (menuItemIcon.isNotEmpty()) {
                Fonts::drawIcon(g, menuItemIcon, iconArea.translated(3.5f, 0.0f), colour, std::min(15.0f, maxFontHeight), true);
            } else if (hasTickBox) {

                g.setColour(colour);
                g.drawRoundedRectangle(iconArea.toFloat().translated(3.5f, 0.5f).reduced(1.0f), 4.0f, 1.0f);

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
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            Fonts::drawFittedText(g, menuItemText, r, colour, fontHeight);

            /*
            if (shortcutKeyText.isNotEmpty()) {
             Fonts::drawText(g, shortcutKeyText, r.translated(-2, 0), findColour(PopupMenu::textColourId), f2.getHeight() * 0.75f, Justification::centredRight);
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

    enum MenuItem {
        NewPatch = 1,
        OpenPatch,
        History,
        Save,
        SaveAs,
        Close,
        CloseAll,
        CompiledMode,
        Compile,
        PluginMode,
        AutoConnect,
        EnablePalettes,
        FindExternals,
        Settings,
        About
    };

    int getMenuItemID(MenuItem item)
    {
        if (item == MenuItem::History)
            return 100;

        return item;
    };

    int getMenuItemIndex(MenuItem item)
    {
        return item - 1;
    }

    std::vector<IconMenuItem*> menuItems = {
        new IconMenuItem(Icons::New, "New patch", false, false),
        new IconMenuItem(Icons::Open, "Open patch...", false, false),
        new IconMenuItem(Icons::History, "Recently opened", true, false),

        new IconMenuItem(Icons::SavePatch, "Save patch", false, false),
        new IconMenuItem(Icons::SaveAs, "Save patch as...", false, false),

        new IconMenuItem(Icons::ClosePatch, "Close patch", false, false),
        new IconMenuItem(Icons::CloseAllPatches, "Close all patches", false, false),

        new IconMenuItem("", "Compiled Mode", false, true),
        new IconMenuItem(Icons::DevTools, "Compile...", false, false),

        new IconMenuItem("", "Plugin Mode", false, true),

        new IconMenuItem("", "Auto-connect objects", false, true),
        new IconMenuItem("", "Enable palettes", false, true),

        new IconMenuItem(Icons::Externals, "Find Externals...", false, false),
        new IconMenuItem(Icons::Settings, "Settings...", false, false),
        new IconMenuItem(Icons::Info, "About...", false, false),
    };

    ValueTree settingsTree;
    ThemeSelector themeSelector;
    ZoomSelector zoomSelector;
};
