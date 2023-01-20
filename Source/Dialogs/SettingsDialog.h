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
struct SettingsToolbarButton : public TextButton {

    String icon;
    String text;

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

        g.setColour(findColour(getToggleState() ? PlugDataColour::toolbarActiveColourId : PlugDataColour::toolbarTextColourId));

        auto iconBounds = b.removeFromTop(b.getHeight() * 0.65f).withTrimmedTop(5);
        auto textBounds = b.withTrimmedBottom(3);

        auto font = lnf->iconFont.withHeight(iconBounds.getHeight() / 1.9f);
        g.setFont(font);

        g.drawFittedText(icon, iconBounds, Justification::centred, 1);

        font = lnf->defaultFont.withHeight(textBounds.getHeight() / 1.25f);
        g.setFont(font);

        // Draw bottom text
        g.drawFittedText(text, textBounds, Justification::centred, 1);
    }
};

struct SettingsDialog : public Component {

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
        dynamic_cast<PluginProcessor*>(audioProcessor)->saveSettings();
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

struct SettingsPopup : public PopupMenu {

    SettingsPopup(AudioProcessor* processor, ValueTree tree)
        : settingsTree(tree)
        , themeSelector(tree)
        , zoomSelector(tree)
    {
        auto* editor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());

        addCustomItem(1, themeSelector, 70, 45, false);
        addCustomItem(2, zoomSelector, 70, 30, false);
        addSeparator();

        StringArray iconText = { Icons::New, Icons::Open, Icons::Save, Icons::SaveAs, Icons::Settings, Icons::Info, Icons::History };

        Array<Image> icons;

        for (int i = 0; i < iconText.size(); i++) {
            icons.add(Image(Image::ARGB, 32, 32, true));
            auto& icon = icons.getReference(i);
            Graphics g(icon);
            g.setColour(editor->findColour(PlugDataColour::popupMenuTextColourId));
            g.setFont(editor->pd->lnf->iconFont.withHeight(28));
            g.drawText(iconText[i], 0, 0, 32, 32, Justification::right);
        }

        addItem(1, "New patch", true, false, icons[0]);

        addSeparator();

        addItem(2, "Open patch...", true, false, icons[1]);

        PopupMenu recentlyOpened;

        auto recentlyOpenedTree = tree.getChildWithName("RecentlyOpened");
        if (recentlyOpenedTree.isValid()) {
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                auto path = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                recentlyOpened.addItem(path.getFileName(), [this, path, editor]() mutable {
                    editor->pd->loadPatch(path);
                    editor->addToRecentlyOpened(path);
                });
            }
        }

        addSubMenu("Recently opened", recentlyOpened, true, icons[6]);

        addSeparator();

        addItem(3, "Save patch", editor->getCurrentCanvas() != nullptr, false, icons[2]);
        addItem(4, "Save patch as...", editor->getCurrentCanvas() != nullptr, false, icons[3]);

        addSeparator();

        // Toggles hvcc compatibility mode
        bool hvccModeEnabled = settingsTree.hasProperty("HvccMode") ? static_cast<bool>(settingsTree.getProperty("HvccMode")) : false;
        addItem("Compiled mode", true, hvccModeEnabled, [this]() mutable {
            bool ticked = settingsTree.hasProperty("HvccMode") ? static_cast<bool>(settingsTree.getProperty("HvccMode")) : false;
            settingsTree.setProperty("HvccMode", !ticked, nullptr);
        });

        addItem("Compile...", [this, editor]() mutable {
            Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
        });

        addSeparator();

        bool autoconnectEnabled = settingsTree.hasProperty("AutoConnect") ? static_cast<bool>(settingsTree.getProperty("AutoConnect")) : false;

        addItem("Auto-connect objects", true, autoconnectEnabled, [this]() mutable {
            bool ticked = settingsTree.hasProperty("AutoConnect") ? static_cast<bool>(settingsTree.getProperty("AutoConnect")) : false;
            settingsTree.setProperty("AutoConnect", !ticked, nullptr);
        });

        addSeparator();
        addItem(5, "Settings...", true, false, icons[4]);
        addItem(6, "About...", true, false, icons[5]);
    }

    static void showSettingsPopup(AudioProcessor* processor, AudioDeviceManager* manager, Component* centre, ValueTree settingsTree)
    {
        auto* popup = new SettingsPopup(processor, settingsTree);
        auto* editor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());

        popup->showMenuAsync(PopupMenu::Options().withMinimumWidth(170).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
            [editor, processor, popup, manager, centre, settingsTree](int result) {
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
                if (result == 5) {

                    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                    auto* settingsDialog = new SettingsDialog(processor, dialog, manager, settingsTree);
                    dialog->setViewedComponent(settingsDialog);
                    editor->openedDialog.reset(dialog);
                }
                if (result == 6) {
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

    struct ZoomSelector : public Component {
        TextButton zoomIn;
        TextButton zoomOut;
        TextButton zoomReset;

        Value zoomValue;

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

    struct ThemeSelector : public Component {
        ThemeSelector(ValueTree tree) : settingsTree(tree)
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

        Value theme;
        ValueTree settingsTree;
    };

    ThemeSelector themeSelector;
    ZoomSelector zoomSelector;

    ValueTree settingsTree;
};
