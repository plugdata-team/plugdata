/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include "Components/PropertiesPanel.h"

struct SettingsDialogPanel : public Component {
    virtual PropertiesPanel* getPropertiesPanel() { return nullptr; }
};

#include "AudioSettingsPanel.h"
#include "MidiSettingsPanel.h"
#include "ThemeSettingsPanel.h"
#include "PathsSettingsPanel.h"
#include "AdvancedSettingsPanel.h"
#include "KeyMappingSettingsPanel.h"

class SettingsDialog final : public Component {

public:
    explicit SettingsDialog(PluginEditor* pluginEditor)
        : processor(dynamic_cast<PluginProcessor*>(pluginEditor->getAudioProcessor()))
        , editor(pluginEditor)
    {
        setVisible(false);

        currentPanel = std::clamp<int>(lastPanel, 0, toolbarButtons.size() - 1);

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i].setRadioGroupId(hash("settings_toolbar_button"));
            addAndMakeVisible(toolbarButtons[i]);
            toolbarButtons[i].onClick = [this, i]() mutable { showPanel(i); };
        }

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this] {
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

        if (ProjectInfo::isStandalone) {
            panels.add(new StandaloneAudioSettingsPanel());
        } else {
            panels.add(new DAWAudioSettingsPanel(processor));
        }

        panels.add(new MidiSettingsPanel(processor));
        panels.add(new ThemeSettingsPanel(processor));
        panels.add(new PathsSettingsPanel());
        panels.add(new KeyMappingSettingsPanel(editor->commandManager.getKeyMappings()));
        panels.add(new AdvancedSettingsPanel(editor));

        SmallArray<PropertiesPanel*> propertiesPanels;
        for (auto* i : panels) {
            addChildComponent(i);

            if (auto* panel = i->getPropertiesPanel()) {
                propertiesPanels.add(panel);
            }
        }
        searcher = std::make_unique<PropertiesSearchPanel>(propertiesPanels);
        addChildComponent(searcher.get());

        searchButton.toFront(false);
        toolbarButtons[currentPanel].setToggleState(true, dontSendNotification);
        panels[currentPanel]->setVisible(true);
        resized();
    }

    void resized() override
    {
        auto const b = getLocalBounds().withTrimmedTop(toolbarHeight);

        int toolbarPosition = 44;
        auto const spacing = (getWidth() - 96) / toolbarButtons.size();

        searchButton.setBounds(4, 1, toolbarHeight - 2, toolbarHeight - 2);
        searcher->setBounds(getLocalBounds());

        for (auto& button : toolbarButtons) {
            button.setBounds(toolbarPosition, 1, spacing, toolbarHeight - 2);
            toolbarPosition += spacing;
        }

        for (auto* panel : panels) {
            panel->setBounds(b);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto const titlebarBounds = getLocalBounds().removeFromTop(toolbarHeight).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(p);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(toolbarHeight, 0.0f, getWidth());
    }

    void showPanel(int const idx)
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

    static inline int lastPanel = 0;
    int currentPanel;
    OwnedArray<SettingsDialogPanel> panels;
    AudioDeviceManager* deviceManager = nullptr;

    StackArray<SettingsToolbarButton, 6> toolbarButtons = {
        SettingsToolbarButton(Icons::Audio, "Audio"),
        SettingsToolbarButton(Icons::MIDI, "MIDI"),
        SettingsToolbarButton(Icons::Pencil, "Themes"),
        SettingsToolbarButton(Icons::Search, "Paths"),
        SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
        SettingsToolbarButton(Icons::Wrench, "Advanced")
    };
};
