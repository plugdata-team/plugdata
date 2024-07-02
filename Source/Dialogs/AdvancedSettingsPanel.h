/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "LookAndFeel.h"
#include "Utility/Autosave.h"
#pragma once

class AdvancedSettingsPanel : public SettingsDialogPanel
    , public Value::Listener {

public:
    explicit AdvancedSettingsPanel(Component* editor)
        : editor(editor)
    {
        auto* settingsFile = SettingsFile::getInstance();
        auto settingsTree = settingsFile->getValueTree();

        Array<PropertiesPanelProperty*> interfaceProperties;
        Array<PropertiesPanelProperty*> otherProperties;
        Array<PropertiesPanelProperty*> autosaveProperties;

        if (ProjectInfo::isStandalone) {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            macTitlebarButtons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));

            macTitlebarButtons.addListener(this);
            nativeTitlebar.addListener(this);

            Array<PropertiesPanelProperty*> windowProperties;

            windowProperties.add(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            windowProperties.add(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macTitlebarButtons, { "No", "Yes" }));

            propertiesPanel.addSection("Window", windowProperties);
        } else {

            if (!settingsTree.hasProperty("NativeDialog")) {
                settingsTree.setProperty("NativeDialog", true, nullptr);
            }

            nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));

            otherProperties.add(new PropertiesPanel::BoolComponent("Use system file dialogs", nativeDialogValue, StringArray { "No", "Yes" }));
        }

        showPalettesValue.referTo(settingsFile->getPropertyAsValue("show_palettes"));
        showPalettesValue.addListener(this);
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Show palette bar", showPalettesValue, { "No", "Yes" }));

        showAllAudioDeviceValues.referTo(settingsFile->getPropertyAsValue("show_all_audio_device_rates"));
        showAllAudioDeviceValues.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Show all audio device rates", showAllAudioDeviceValues, { "No", "Yes" }));

        autoPatchingValue.referTo(settingsFile->getPropertyAsValue("autoconnect"));
        otherProperties.add(new PropertiesPanel::BoolComponent("Enable auto patching", autoPatchingValue, { "No", "Yes" }));

        autosaveInterval.referTo(settingsFile->getPropertyAsValue("autosave_interval"));
        autosaveProperties.add(new PropertiesPanel::EditableComponent<int>("Autosave interval (seconds)", autosaveInterval, 15, 900));

        autosaveEnabled.referTo(settingsFile->getPropertyAsValue("autosave_enabled"));
        autosaveProperties.add(new PropertiesPanel::BoolComponent("Enable autosave", autosaveEnabled, { "No", "Yes" }));

        autosaveProperties.add(new PropertiesPanel::ActionComponent([this, editor]() {
            autosaveHistoryDialog = std::make_unique<AutosaveHistoryComponent>(dynamic_cast<PluginEditor*>(editor));
            auto* parent = getParentComponent();
            parent->addAndMakeVisible(autosaveHistoryDialog.get());
            autosaveHistoryDialog->setBounds(parent->getLocalBounds());
        },
            Icons::Save, "Show autosave history"));

        struct ScaleComponent : public PropertiesPanelProperty {
            ScaleComponent(String const& propertyName, Value& value)
                : PropertiesPanelProperty(propertyName)
                , scaleValue(value)
            {
                StringArray comboItems = { "50%", "62.5%", "75%", "87.5%", "100%", "112.5%", "125%", "137.5%", "150%", "162.5%", "175%", "187.5%", "200%" };
                Array<float> scaleValues = { 0.5f, 0.625f, 0.75f, 0.875f, 1.0f, 1.125f, 1.25f, 1.375f, 1.5f, 1.625f, 1.75f, 1.875f, 2.0f };

                comboBox.addItemList(comboItems, 1);

                // Find number closest to current scale factor
                auto closest = std::min_element(scaleValues.begin(), scaleValues.end(),
                    [target = getValue<float>(value)](float a, float b) {
                        return std::abs(a - target) < std::abs(b - target);
                    });
                auto currentIndex = std::distance(scaleValues.begin(), closest);

                comboBox.setSelectedItemIndex(currentIndex);
                comboBox.onChange = [this, scaleValues]() {
                    scaleValue = scaleValues[comboBox.getSelectedItemIndex()];
                };

                comboBox.getProperties().set("Style", "Inspector");

                addAndMakeVisible(comboBox);
            }

            void resized() override
            {
                comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
            }

            PropertiesPanelProperty* createCopy() override
            {
                return new ScaleComponent(getName(), scaleValue);
            }

            Value& scaleValue;
            ComboBox comboBox;
        };

        scaleValue = settingsFile->getProperty<float>("global_scale");
        scaleValue.addListener(this);
        interfaceProperties.add(new ScaleComponent("Global scale factor", scaleValue));

        defaultZoom = settingsFile->getProperty<float>("default_zoom");
        defaultZoom.addListener(this);
        interfaceProperties.add(new PropertiesPanel::EditableComponent<float>("Default zoom %", defaultZoom));

        centreResized = settingsFile->getPropertyAsValue("centre_resized_canvas");
        centreResized.addListener(this);
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Centre canvas when resized", centreResized, { "No", "Yes" }));

        centreSidepanelButtons = settingsFile->getPropertyAsValue("centre_sidepanel_buttons");
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Centre canvas sidepanel selectors", centreSidepanelButtons, { "No", "Yes" }));

        patchDownwardsOnly = settingsFile->getPropertyAsValue("patch_downwards_only");
        otherProperties.add(new PropertiesPanel::BoolComponent("Patch downwards only", patchDownwardsOnly, { "No", "Yes" }));

        propertiesPanel.addSection("Interface", interfaceProperties);
        propertiesPanel.addSection("Autosave", autosaveProperties);
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
            SettingsFile::getInstance()->setGlobalScale(getValue<float>(scaleValue));
        }
        if (v.refersToSameSourceAs(defaultZoom)) {
            auto zoom = std::clamp(getValue<float>(defaultZoom), 20.0f, 300.0f);
            SettingsFile::getInstance()->setProperty("default_zoom", zoom);
            defaultZoom = zoom;
        }
    }
    Component* editor;

    Value nativeTitlebar;
    Value macTitlebarButtons;
    Value scaleValue;
    Value defaultZoom;
    Value centreResized;
    Value centreSidepanelButtons;

    Value showPalettesValue;
    Value autoPatchingValue;
    Value showAllAudioDeviceValues;
    Value nativeDialogValue;
    Value autosaveInterval;
    Value autosaveEnabled;

    Value patchDownwardsOnly;

    PropertiesPanel propertiesPanel;

    std::unique_ptr<AutosaveHistoryComponent> autosaveHistoryDialog;
};
