/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

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
        : icon(iconToUse)
        , text(textToShow)
    {
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(2);

        if (isMouseOver() || getToggleState()) {
            auto background = findColour(PlugDataColour::toolbarHoverColourId);
            if (getToggleState())
                background = background.darker(0.05f);

            g.setColour(background);
            g.fillRoundedRectangle(b.toFloat().reduced(4.0f, 2.0f), Corners::defaultCornerRadius);
        }

        g.setColour(findColour(PlugDataColour::toolbarTextColourId));

        auto iconBounds = b.removeFromTop(b.getHeight() * 0.65f).withTrimmedTop(5);
        auto textBounds = b.withTrimmedBottom(3);

        auto font = Fonts::getIconFont().withHeight(iconBounds.getHeight() / 1.9f);
        g.setFont(font);

        g.drawFittedText(icon, iconBounds, Justification::centred, 1);

        font = Fonts::getCurrentFont().withHeight(textBounds.getHeight() / 1.25f);
        g.setFont(font);

        // Draw bottom text
        g.drawFittedText(text, textBounds, Justification::centred, 1);
    }
};

class SettingsDialog : public Component {

public:
    SettingsDialog(PluginEditor* editor, Dialog* dialog)
        : processor(dynamic_cast<PluginProcessor*>(editor->getAudioProcessor()))
    {
        setVisible(false);
        
        if(ProjectInfo::isStandalone)
        {
            toolbarButtons = {
                new SettingsToolbarButton(Icons::Audio, "Audio"),
                new SettingsToolbarButton(Icons::MIDI, "MIDI"),
                new SettingsToolbarButton(Icons::Pencil, "Themes"),
                new SettingsToolbarButton(Icons::Search, "Paths"),
                new SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
                new SettingsToolbarButton(Icons::Wrench, "Advanced") };
        }
        else {
            toolbarButtons = {
                new SettingsToolbarButton(Icons::Audio, "Audio"),
                new SettingsToolbarButton(Icons::Pencil, "Themes"),
                new SettingsToolbarButton(Icons::Search, "Paths"),
                new SettingsToolbarButton(Icons::Keyboard, "Shortcuts"),
                new SettingsToolbarButton(Icons::Wrench, "Advanced") };
        }


        currentPanel = std::clamp(lastPanel.load(), 0, toolbarButtons.size() - 1);

        auto* processor = dynamic_cast<PluginProcessor*>(editor->getAudioProcessor());

        if (auto* deviceManager = ProjectInfo::getDeviceManager()) {
            panels.add(new StandaloneAudioSettings(processor, *deviceManager));
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
            toolbarButtons[i]->setRadioGroupId(0111);
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

        auto spacing = ((getWidth() - 120) / toolbarButtons.size());

        int toolbarPosition = 40;

        for (auto& button : toolbarButtons) {
            button->setBounds(toolbarPosition, 1, 70, toolbarHeight - 2);
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

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, toolbarHeight, getWidth(), toolbarHeight);
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

    static constexpr int toolbarHeight = 55;

    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<TextButton> toolbarButtons;
};
