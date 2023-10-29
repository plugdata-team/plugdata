/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "LookAndFeel.h"

#pragma once

class AdvancedSettingsPanel : public SettingsDialogPanel
    , public Value::Listener {

public:
    explicit AdvancedSettingsPanel(Component* editor)
        : editor(editor)
    {
        auto* settingsFile = SettingsFile::getInstance();

        Array<PropertiesPanelProperty*> otherProperties;

        if (ProjectInfo::isStandalone) {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            macTitlebarButtons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));
            reloadPatch.referTo(settingsFile->getPropertyAsValue("reload_last_state"));

            macTitlebarButtons.addListener(this);
            nativeTitlebar.addListener(this);

            Array<PropertiesPanelProperty*> windowProperties;

            windowProperties.add(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            windowProperties.add(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macTitlebarButtons, { "No", "Yes" }));

            propertiesPanel.addSection("Window", windowProperties);

            otherProperties.add(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));
        } else {

            if (!settingsTree.hasProperty("NativeDialog")) {
                settingsTree.setProperty("NativeDialog", true, nullptr);
            }

            nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));

            otherProperties.add(new PropertiesPanel::BoolComponent("Use system file dialogs", nativeDialogValue, StringArray { "No", "Yes" }));
        }

        showPalettesValue.referTo(settingsFile->getPropertyAsValue("show_palettes"));
        showPalettesValue.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Show palette bar", showPalettesValue, { "No", "Yes" }));

        showAllAudioDeviceValues.referTo(settingsFile->getPropertyAsValue("show_all_audio_device_rates"));
        showAllAudioDeviceValues.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Show all audio device rates", showAllAudioDeviceValues, { "No", "Yes" }));

        autoPatchingValue.referTo(settingsFile->getPropertyAsValue("autoconnect"));
        otherProperties.add(new PropertiesPanel::BoolComponent("Enable auto patching", autoPatchingValue, { "No", "Yes" }));

        scaleValue = settingsFile->getProperty<float>("global_scale");
        scaleValue.addListener(this);
        otherProperties.add(new PropertiesPanel::EditableComponent<float>("Global scale factor", scaleValue));

        defaultZoom = settingsFile->getProperty<float>("default_zoom");
        defaultZoom.addListener(this);
        otherProperties.add(new PropertiesPanel::EditableComponent<float>("Default zoom %", defaultZoom));

        centerResized = settingsFile->getPropertyAsValue("center_resized_canvas");
        centerResized.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Center canvas when resized", centerResized, { "No", "Yes" }));

        propertiesPanel.addSection("Other", otherProperties);

        addAndMakeVisible(propertiesPanel);
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &propertiesPanel;
    }

    void resized() override
    {
        propertiesPanel.setBounds(getLocalBounds());
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(nativeTitlebar)) {
            // Make sure titlebar buttons are greyed out because a dialog is still showing
            if (auto* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent())) {
                if (auto* closeButton = window->getCloseButton())
                    closeButton->setEnabled(false);
                if (auto* minimiseButton = window->getMinimiseButton())
                    minimiseButton->setEnabled(false);
                if (auto* maximiseButton = window->getMaximiseButton())
                    maximiseButton->setEnabled(false);
            }
        }
        if (v.refersToSameSourceAs(macTitlebarButtons)) {
            editor->resized();
        }
        if (v.refersToSameSourceAs(showPalettesValue)) {
            editor->resized();
        }
        if (v.refersToSameSourceAs(scaleValue)) {
            auto scale = std::clamp(getValue<float>(scaleValue), 0.5f, 2.5f);
            SettingsFile::getInstance()->setGlobalScale(scale);
            scaleValue = scale;
        }
        if (v.refersToSameSourceAs(defaultZoom)) {
            auto zoom = std::clamp(getValue<float>(defaultZoom), 20.0f, 300.0f);
            SettingsFile::getInstance()->setProperty("default_zoom", zoom);
            defaultZoom = zoom;
        }
    }
    Component* editor;

    ValueTree settingsTree;

    Value nativeTitlebar;
    Value macTitlebarButtons;
    Value reloadPatch;
    Value scaleValue;
    Value defaultZoom;
    Value centerResized;

    Value showPalettesValue;
    Value autoPatchingValue;
    Value showAllAudioDeviceValues;
    Value nativeDialogValue;

    PropertiesPanel propertiesPanel;
};
