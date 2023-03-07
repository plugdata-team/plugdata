/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

#include "../Standalone/PlugDataWindow.h"

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
        auto b = getLocalBounds().reduced(2);

        g.setColour(findColour(getToggleState() ? PlugDataColour::toolbarActiveColourId : PlugDataColour::toolbarTextColourId));

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

        auto* processor = dynamic_cast<PluginProcessor*>(editor->getAudioProcessor());
#if PLUGDATA_STANDALONE
        auto& deviceManager = editor->findParentComponentOfClass<PlugDataWindow>()->getDeviceManager();
        panels.add(new StandaloneAudioSettings(processor, deviceManager));
#else
        panels.add(new DAWAudioSettings(processor));
#endif

        panels.add(new ThemePanel(processor));
        panels.add(new SearchPathComponent());
        panels.add(new LibraryLoadPanel());
        panels.add(new KeyMappingComponent(*editor->getKeyMappings()));
        panels.add(new Deken());

#if PLUGDATA_STANDALONE
        panels.add(new AdvancedSettingsPanel());
#endif

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(0110);
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
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, Corners::windowCornerRadius);
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
            g.fillRoundedRectangle(statusbarBounds, Corners::windowCornerRadius);
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

    AudioProcessor* processor;
    ComponentBoundsConstrainer constrainer;

    static constexpr int toolbarHeight = 55;

    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<TextButton> toolbarButtons;
};
