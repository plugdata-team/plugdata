/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#include <utility>

#include "Components/PropertiesPanel.h"

#include "AboutPanel.h"

struct SettingsDialogPanel : public Component {
    virtual PropertiesPanel* getPropertiesPanel() { return nullptr; }
};

#include "AudioSettingsPanel.h"
#include "MIDISettingsPanel.h"
#include "ThemePanel.h"
#include "PathsAndLibrariesPanel.h"
#include "AdvancedSettingsPanel.h"
#include "KeyMappingPanel.h"

class SettingsDialog : public Component {

public:
    explicit SettingsDialog(PluginEditor* pluginEditor)
        : processor(dynamic_cast<PluginProcessor*>(pluginEditor->getAudioProcessor()))
        , editor(pluginEditor)
    {
        setVisible(false);

        if (ProjectInfo::isStandalone) {
            toolbarButtons = {
                new SettingsToolbarButton(Icons::Audio, "Audio"),
                new SettingsToolbarButton(Icons::MIDI, "MIDI"),
                new SettingsToolbarButton(Icons::Pencil, "Themes"),
                new SettingsToolbarButton(Icons::Search, "Paths"),
                new SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
                new SettingsToolbarButton(Icons::Wrench, "Advanced")
            };
        } else {
            toolbarButtons = {
                new SettingsToolbarButton(Icons::Audio, "Audio"),
                new SettingsToolbarButton(Icons::Pencil, "Themes"),
                new SettingsToolbarButton(Icons::Search, "Paths"),
                new SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
                new SettingsToolbarButton(Icons::Wrench, "Advanced")
            };
        }

        currentPanel = std::clamp(lastPanel.load(), 0, toolbarButtons.size() - 1);

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i]->setRadioGroupId(hash("settings_toolbar_button"));
            addAndMakeVisible(toolbarButtons[i]);
            toolbarButtons[i]->onClick = [this, i]() mutable { showPanel(i); };
        }

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this]() {
            if (searchButton.getToggleState()) {
                searcher->startSearching();
            } else {
                reloadPanels();
                searcher->stopSearching();
            }
        };
        addAndMakeVisible(searchButton);

        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);
        reloadPanels();
    }

    ~SettingsDialog() override
    {
        lastPanel = currentPanel;
        SettingsFile::getInstance()->saveSettings();
    }

    void reloadPanels()
    {
        panels.clear();

        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            panels.add(new StandaloneAudioSettings(*deviceManager));
            panels.add(new StandaloneMIDISettings(processor, *deviceManager));
        } else {
            panels.add(new DAWAudioSettings(processor));
        }

        panels.add(new ThemePanel(processor));
        panels.add(new PathsAndLibrariesPanel());
        panels.add(new KeyMappingComponent(editor->commandManager.getKeyMappings()));
        panels.add(new AdvancedSettingsPanel(editor));

        Array<PropertiesPanel*> propertiesPanels;
        for (auto* i : panels) {
            addChildComponent(i);

            if (auto* panel = i->getPropertiesPanel()) {
                propertiesPanels.add(panel);
            }
        }
        searcher = std::make_unique<PropertiesSearchPanel>(propertiesPanels);
        addChildComponent(searcher.get());

        searchButton.toFront(false);
        toolbarButtons[currentPanel]->setToggleState(true, dontSendNotification);
        panels[currentPanel]->setVisible(true);
        resized();
    }

    void resized() override
    {
        auto b = getLocalBounds().withTrimmedTop(toolbarHeight);

        int toolbarPosition = 44;
        auto spacing = (getWidth() - 96) / toolbarButtons.size();

        searchButton.setBounds(4, 1, toolbarHeight - 2, toolbarHeight - 2);
        searcher->setBounds(getLocalBounds());

        for (auto& button : toolbarButtons) {
            button->setBounds(toolbarPosition, 1, spacing, toolbarHeight - 2);
            toolbarPosition += spacing;
        }

        for (auto* panel : panels) {
            panel->setBounds(b);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto titlebarBounds = getLocalBounds().removeFromTop(toolbarHeight).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(toolbarHeight, 0.0f, getWidth());
    }

    void showPanel(int idx)
    {
        panels[currentPanel]->setVisible(false);
        panels[idx]->setVisible(true);
        currentPanel = idx;
        repaint();
    }

    PluginProcessor* processor;
    PluginEditor* editor;
    ComponentBoundsConstrainer constrainer;

    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);
    std::unique_ptr<PropertiesSearchPanel> searcher;

    static constexpr int toolbarHeight = 40;

    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<SettingsDialogPanel> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<SettingsToolbarButton> toolbarButtons;
};
