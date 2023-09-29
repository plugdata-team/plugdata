/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#include <utility>

#include "Utility/PropertiesPanel.h"

#include "AboutPanel.h"

#include "AudioSettingsPanel.h"
#include "MIDISettingsPanel.h"
#include "ThemePanel.h"
#include "PathsAndLibrariesPanel.h"
#include "AdvancedSettingsPanel.h"
#include "KeyMappingPanel.h"

// Toolbar button for settings panel, with both icon and text
// We have too many specific items to have only icons at this point
class SettingsToolbarButton : public TextButton {

    String icon;
    String text;

public:
    SettingsToolbarButton(String iconToUse, String textToShow)
        : icon(std::move(iconToUse))
        , text(std::move(textToShow))
    {
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(2.0f, 4.0f);

        if (isMouseOver() || getToggleState()) {
            auto background = findColour(PlugDataColour::toolbarHoverColourId);
            if (getToggleState())
                background = background.darker(0.025f);

            g.setColour(background);
            PlugDataLook::fillSmoothedRectangle(g, b.toFloat(), Corners::defaultCornerRadius);
        }

        auto textColour = findColour(PlugDataColour::toolbarTextColourId);
        auto boldFont = Fonts::getBoldFont().withHeight(13.5f);
        auto iconFont = Fonts::getIconFont().withHeight(13.5f);

        auto textWidth = boldFont.getStringWidth(text);
        auto iconWidth = iconFont.getStringWidth(icon);

        AttributedString attrStr;
        attrStr.setJustification(Justification::centred);
        attrStr.append(icon, iconFont, textColour);
        attrStr.append("  " + text, boldFont, textColour);
        attrStr.draw(g, b.toFloat());
    }
};

class SettingsDialog : public Component {

public:
    explicit SettingsDialog(PluginEditor* editor)
        : processor(dynamic_cast<PluginProcessor*>(editor->getAudioProcessor()))
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

        auto* processor = dynamic_cast<PluginProcessor*>(editor->getAudioProcessor());

        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            panels.add(new StandaloneAudioSettings(*deviceManager));
            panels.add(new StandaloneMIDISettings(processor, *deviceManager));
        } else {
            panels.add(new DAWAudioSettings(processor));
        }

        panels.add(new ThemePanel(processor));
        panels.add(new PathsAndLibrariesPanel());
        panels.add(new KeyMappingComponent(*editor->getKeyMappings()));
        panels.add(new AdvancedSettingsPanel(editor));

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(hash("settings_toolbar_button"));
            toolbarButtons[i]->setConnectedEdges(12);
            toolbarButtons[i]->getProperties().set("Style", "LargeIcon");
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
        auto b = getLocalBounds().withTrimmedTop(toolbarHeight);

        int toolbarPosition = 24;
        auto spacing = (getWidth() - 72) / toolbarButtons.size();

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

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, Corners::windowCornerRadius);
        g.fillRect(toolbarBounds.withTrimmedTop(15.0f));

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

    AudioProcessor* processor;
    ComponentBoundsConstrainer constrainer;

    static constexpr int toolbarHeight = 42;

    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<SettingsToolbarButton> toolbarButtons;
};
