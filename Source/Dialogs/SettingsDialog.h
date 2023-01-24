/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

#include "AboutPanel.h"

#include "AudioSettingsPanel.h"
#include "ThemePanel.h"
#include "SearchPathPanel.h"
#include "AdvancedSettingsPanel.h"
#include "KeyMappingPanel.h"
#include "LibraryLoadPanel.h"
#include "Deken.h"

// Toolbar button for settings panel, with both icon and text
// We have too many specific items to have only icons at this point
class SettingsToolbarButton : public TextButton {

    String icon;
    String text;

public:
    SettingsToolbarButton(String iconToUse, String textToShow)
        : icon(iconToUse)
        , text(textToShow)
    {
    }

    void paint(Graphics& g) override
    {

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if (!lnf)
            return;

        auto b = getLocalBounds().reduced(2);

        auto colour = findColour(getToggleState() ? PlugDataColour::toolbarActiveColourId : PlugDataColour::toolbarTextColourId);

        auto iconBounds = b.removeFromTop(b.getHeight() * 0.65f).withTrimmedTop(5);
        auto textBounds = b.withTrimmedBottom(3);

        auto font = lnf->iconFont.withHeight(iconBounds.getHeight() / 1.9f);
        g.setFont(font);

        PlugDataLook::drawFittedText(g, icon, iconBounds, Justification::centred, colour);

        font = lnf->defaultFont.withHeight(textBounds.getHeight() / 1.25f);
        g.setFont(font);

        // Draw bottom text
        PlugDataLook::drawFittedText(g, text, textBounds, Justification::centred, colour);
    }
};

class SettingsDialog : public Component {

public:
    SettingsDialog(AudioProcessor* processor, Dialog* dialog, AudioDeviceManager* manager, ValueTree const& settingsTree)
        : audioProcessor(processor)
    {

        setVisible(false);

        toolbarButtons = { new SettingsToolbarButton(Icons::Audio, "Audio"),
            new SettingsToolbarButton(Icons::Pencil, "Themes"),
            new SettingsToolbarButton(Icons::Search, "Paths"),
            new SettingsToolbarButton(Icons::Library, "Libraries"),
            new SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
            new SettingsToolbarButton(Icons::Externals, "Externals")
#if PLUGDATA_STANDALONE
                ,
            new SettingsToolbarButton(Icons::Wrench, "Advanced")
#endif
        };

        currentPanel = std::clamp(lastPanel.load(), 0, toolbarButtons.size() - 1);

        auto* editor = dynamic_cast<ApplicationCommandManager*>(processor->getActiveEditor());

#if PLUGDATA_STANDALONE
        panels.add(new StandaloneAudioSettings(dynamic_cast<PluginProcessor*>(processor), *manager));
#else
        panels.add(new DAWAudioSettings(processor));
#endif

        panels.add(new ThemePanel(settingsTree));
        panels.add(new SearchPathComponent(settingsTree.getChildWithName("Paths")));
        panels.add(new LibraryLoadPanel(settingsTree.getChildWithName("Libraries")));
        panels.add(new KeyMappingComponent(*editor->getKeyMappings(), settingsTree));
        panels.add(new Deken());

#if PLUGDATA_STANDALONE
        panels.add(new AdvancedSettingsPanel(settingsTree));
#endif

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(0110);
            toolbarButtons[i]->setConnectedEdges(12);
            toolbarButtons[i]->setName("toolbar:settings");
            addAndMakeVisible(toolbarButtons[i]);

            addChildComponent(panels[i]);
            toolbarButtons[i]->onClick = [this, i]() mutable { showPanel(i); };
        }

        toolbarButtons[currentPanel]->setToggleState(true, sendNotification);

        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);
    }

    ~SettingsDialog() override
    {
        lastPanel = currentPanel;
        SettingsFile::getInstance()->saveSettings();
    }

    void resized() override
    {
        auto b = getLocalBounds().withTrimmedTop(toolbarHeight).withTrimmedBottom(6);

        int toolbarPosition = 2;
        for (auto& button : toolbarButtons) {
            button->setBounds(toolbarPosition, 1, 70, toolbarHeight - 2);
            toolbarPosition += 70;
        }

        for (auto* panel : panels) {
            panel->setBounds(b);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), PlugDataLook::windowCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, PlugDataLook::windowCornerRadius);
        g.fillRect(toolbarBounds.withTrimmedTop(15.0f));

#ifdef PLUGDATA_STANDALONE
        bool drawStatusbar = currentPanel > 0;
#else
        bool drawStatusbar = true;
#endif

        if (drawStatusbar) {
            auto statusbarBounds = getLocalBounds().reduced(1).removeFromBottom(32).toFloat();
            g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

            g.fillRect(statusbarBounds.withHeight(20));
            g.fillRoundedRectangle(statusbarBounds, PlugDataLook::windowCornerRadius);
        }

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, toolbarHeight, getWidth(), toolbarHeight);

        if (currentPanel > 0) {
            g.drawLine(0.0f, getHeight() - 33, getWidth(), getHeight() - 33);
        }
    }

    void showPanel(int idx)
    {
        panels[currentPanel]->setVisible(false);
        panels[idx]->setVisible(true);
        currentPanel = idx;
        repaint();
    }

    AudioProcessor* audioProcessor;
    ComponentBoundsConstrainer constrainer;

    static constexpr int toolbarHeight = 55;

    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<TextButton> toolbarButtons;
};

class SettingsPopup : public PopupMenu {

public:
    SettingsPopup(AudioProcessor* processor, ValueTree tree)
        : settingsTree(tree)
        , themeSelector(tree)
        , zoomSelector(tree)
    {
        auto* editor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());

        addCustomItem(1, themeSelector, 70, 45, false);
        addCustomItem(2, zoomSelector, 70, 30, false);
        addSeparator();

        addCustomItem(1, std::unique_ptr<IconMenuItem>(menuItems[0]), nullptr, "New patch");

        addSeparator();

        addCustomItem(2, std::unique_ptr<IconMenuItem>(menuItems[1]), nullptr, "Open patch");

        auto recentlyOpened = new PopupMenu();

        auto recentlyOpenedTree = tree.getChildWithName("RecentlyOpened");
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
                
        addCustomItem(100, std::unique_ptr<IconMenuItem>(menuItems[2]), std::unique_ptr<const PopupMenu>(recentlyOpened), "Recently opened");

        addSeparator();
        addCustomItem(3, std::unique_ptr<IconMenuItem>(menuItems[3]), nullptr, "Save patch");
        addCustomItem(4, std::unique_ptr<IconMenuItem>(menuItems[4]), nullptr, "Save patch as");

        addSeparator();

        addCustomItem(5, std::unique_ptr<IconMenuItem>(menuItems[5]), nullptr, "Compiled mode");
        addCustomItem(6, std::unique_ptr<IconMenuItem>(menuItems[6]), nullptr, "Compile...");
                
        addSeparator();
        
        addCustomItem(7, std::unique_ptr<IconMenuItem>(menuItems[7]), nullptr, "Auto-connect objects");

        addSeparator();
        
        addCustomItem(8, std::unique_ptr<IconMenuItem>(menuItems[8]), nullptr, "Settings...");
        addCustomItem(9, std::unique_ptr<IconMenuItem>(menuItems[9]), nullptr, "About...");
        
        
        // Toggles hvcc compatibility mode
        bool hvccModeEnabled = settingsTree.hasProperty("HvccMode") ? static_cast<bool>(settingsTree.getProperty("HvccMode")) : false;
        bool autoconnectEnabled = settingsTree.hasProperty("AutoConnect") ? static_cast<bool>(settingsTree.getProperty("AutoConnect")) : false;
        bool hasCanvas = editor->getCurrentCanvas() != nullptr;;
        
        menuItems[3]->isActive = hasCanvas;
        menuItems[4]->isActive = hasCanvas;
        
        menuItems[5]->isTicked = hvccModeEnabled;
        menuItems[7]->isTicked = autoconnectEnabled;

    }
    
    static void showSettingsPopup(AudioProcessor* processor, AudioDeviceManager* manager, Component* centre, ValueTree settingsTree)
    {
        auto* popup = new SettingsPopup(processor, settingsTree);
        auto* editor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());

        popup->showMenuAsync(PopupMenu::Options().withMinimumWidth(220).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
            [editor, processor, popup, manager, centre, settingsTree](int result) mutable {
                if (result == 1) {
                    editor->newProject();
                }
                if (result == 2) {
                    editor->openProject();
                }
                if (result == 3 && editor->getCurrentCanvas()) {
                    editor->saveProject();
                }
                if (result == 4 && editor->getCurrentCanvas()) {
                    editor->saveProjectAs();
                }
                if(result == 5) {
                    bool ticked = settingsTree.hasProperty("HvccMode") ? static_cast<bool>(settingsTree.getProperty("HvccMode")) : false;
                    settingsTree.setProperty("HvccMode", !ticked, nullptr);
                }
                if(result == 6) {
                    Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
                }
                if(result == 7) {
                    bool ticked = settingsTree.hasProperty("AutoConnect") ? static_cast<bool>(settingsTree.getProperty("AutoConnect")) : false;
                    settingsTree.setProperty("AutoConnect", !ticked, nullptr);
                }
                if (result == 8) {

                    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                    auto* settingsDialog = new SettingsDialog(processor, dialog, manager, settingsTree);
                    dialog->setViewedComponent(settingsDialog);
                    editor->openedDialog.reset(dialog);
                }
                if (result == 9) {
                    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                    auto* aboutPanel = new AboutPanel();
                    dialog->setViewedComponent(aboutPanel);
                    editor->openedDialog.reset(dialog);
                }

                MessageManager::callAsync([popup]() {
                    delete popup;
                });
            });
    }

    class ZoomSelector : public Component {
        TextButton zoomIn;
        TextButton zoomOut;
        TextButton zoomReset;

        Value zoomValue;

    public:
        ZoomSelector(ValueTree settingsTree)
        {
            zoomValue = settingsTree.getPropertyAsValue("Zoom", nullptr);

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
        bool isMouseOver = false;

        
    public:
        bool isTicked = false;
        bool isActive = true;
        
        IconMenuItem(String icon, String text, bool hasChildren)
        : menuItemIcon(icon)
        , menuItemText(text)
        , hasSubMenu(hasChildren)
        {
        }

        void getIdealSize (int &idealWidth, int &idealHeight) override
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

            auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight));
            
            if(menuItemIcon.isNotEmpty()) {
                g.setFont(lnf.iconFont.withHeight(std::min(15.0f, maxFontHeight)));
                
                PlugDataLook::drawFittedText(g, menuItemIcon, iconArea, Justification::centredLeft, colour);
            }
            else if (isTicked) {
                auto tick = lnf.getTickShape(1.0f);
                g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
            }
            
            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));

            auto font = Font(std::min(17.0f, maxFontHeight));
            
            g.setFont(font);
            
            if (hasSubMenu) {
                auto arrowH = 0.6f * font.getAscent();

                auto x = static_cast<float>(r.removeFromRight((int)arrowH).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            PlugDataLook::drawFittedText(g, menuItemText, r, Justification::centredLeft, colour);

            /*
            if (shortcutKeyText.isNotEmpty()) {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

             PlugDataLook::drawText(g, shortcutKeyText, r.translated(-2, 0), Justification::centredRight, findColour(PopupMenu::textColourId));
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
            theme.referTo(settingsTree.getPropertyAsValue("Theme", nullptr));
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

    std::vector<IconMenuItem*> menuItems = {
        new IconMenuItem(Icons::New, "New patch", false),
        new IconMenuItem(Icons::Open, "Open patch...", false),
        new IconMenuItem(Icons::History, "Recently opened", true),
        
        new IconMenuItem(Icons::Save, "Save patch", false),
        new IconMenuItem(Icons::SaveAs, "Save patch as...", false),
        
        new IconMenuItem("", "Compiled Mode", false),
        new IconMenuItem("", "Compile...", false),
        
        new IconMenuItem("", "Auto-connect objects", false),
        
        new IconMenuItem(Icons::Settings, "Settings...", false),
        new IconMenuItem(Icons::Info, "About...", false),
    };
    
    ThemeSelector themeSelector;
    ZoomSelector zoomSelector;

    ValueTree settingsTree;
};
