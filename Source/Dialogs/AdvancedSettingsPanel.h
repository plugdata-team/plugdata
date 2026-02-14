/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include "LookAndFeel.h"
#include "Utility/Autosave.h"
#pragma once

class AdvancedSettingsPanel final : public SettingsDialogPanel
    , public Value::Listener {

public:
    explicit AdvancedSettingsPanel(Component* editor)
        : editor(editor)
    {
        auto* settingsFile = SettingsFile::getInstance();
        auto settingsTree = settingsFile->getValueTree();

        PropertiesArray interfaceProperties;
        PropertiesArray otherProperties;
        PropertiesArray autosaveProperties;

#if !JUCE_IOS
        if (ProjectInfo::isStandalone) {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            nativeTitlebar.addListener(this);

            PropertiesArray windowProperties;
            windowProperties.add(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            propertiesPanel.addSection("Window", windowProperties);
        } else {
            if (!settingsTree.hasProperty("NativeDialog")) {
                settingsTree.setProperty("NativeDialog", true, nullptr);
            }

            nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));
            otherProperties.add(new PropertiesPanel::BoolComponent("Use system file dialogs", nativeDialogValue, StringArray { "No", "Yes" }));
        }

        if (ProjectInfo::isStandalone) {
            openPatchesInWindow.referTo(settingsFile->getPropertyAsValue("open_patches_in_window"));
            openPatchesInWindow.addListener(this);
            interfaceProperties.add(new PropertiesPanel::BoolComponent("Open patches in new window", openPatchesInWindow, { "No", "Yes" }));
        }
#endif

        showPalettesValue.referTo(settingsFile->getPropertyAsValue("show_palettes"));
        showPalettesValue.addListener(this);
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Show palette bar", showPalettesValue, { "No", "Yes" }));

        commandClickSwitchesModeValue.referTo(settingsFile->getPropertyAsValue("cmd_click_switches_mode"));
        commandClickSwitchesModeValue.addListener(this);
#if JUCE_MAC
        String cmdName = "Command";
#else
        String cmdName = "Ctrl";
#endif
        interfaceProperties.add(new PropertiesPanel::BoolComponent(cmdName + " + click on canvas switches mode", commandClickSwitchesModeValue, { "No", "Yes" }));

        showAllAudioDeviceValues.referTo(settingsFile->getPropertyAsValue("show_all_audio_device_rates"));
        showAllAudioDeviceValues.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Show all audio device rates", showAllAudioDeviceValues, { "No", "Yes" }));

        autoPatchingValue.referTo(settingsFile->getPropertyAsValue("autoconnect"));
        otherProperties.add(new PropertiesPanel::BoolComponent("Enable auto patching", autoPatchingValue, { "No", "Yes" }));

        autosaveInterval.referTo(settingsFile->getPropertyAsValue("autosave_interval"));
        autosaveProperties.add(new PropertiesPanel::EditableComponent<int>("Auto-save interval (minutes)", autosaveInterval, true, 1, 60));

        autosaveEnabled.referTo(settingsFile->getPropertyAsValue("autosave_enabled"));
        autosaveProperties.add(new PropertiesPanel::BoolComponent("Enable autosave", autosaveEnabled, { "No", "Yes" }));

        autosaveProperties.add(new PropertiesPanel::ActionComponent([this, editor] {
            autosaveHistoryDialog = std::make_unique<AutosaveHistoryComponent>(dynamic_cast<PluginEditor*>(editor));
            auto* parent = getParentComponent();
            parent->addAndMakeVisible(autosaveHistoryDialog.get());
            autosaveHistoryDialog->setBounds(parent->getLocalBounds());
        },
            Icons::Save, "Show autosave history"));

        class ScaleComponent : public PropertiesPanelProperty {
        public:
            ScaleComponent(String const& propertyName, Value& value)
                : PropertiesPanelProperty(propertyName)
                , scaleValue(value)
            {
                StringArray const comboItems = { "50%", "62.5%", "75%", "87.5%", "100%", "112.5%", "125%", "137.5%", "150%", "162.5%", "175%", "187.5%", "200%" };
                SmallArray<float, 13> scaleValues = { 0.5f, 0.625f, 0.75f, 0.875f, 1.0f, 1.125f, 1.25f, 1.375f, 1.5f, 1.625f, 1.75f, 1.875f, 2.0f };

                comboBox.addItemList(comboItems, 1);

                // Find number closest to current scale factor
                auto const closest = std::ranges::min_element(scaleValues,
                    [target = getValue<float>(value)](float const a, float const b) {
                        return std::abs(a - target) < std::abs(b - target);
                    });
                auto const currentIndex = std::distance(scaleValues.begin(), closest);

                comboBox.setSelectedItemIndex(currentIndex);
                comboBox.onChange = [this, scaleValues] {
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
        interfaceProperties.add(new PropertiesPanel::EditableComponent<float>("Default zoom %", defaultZoom, true, 25, 300));

        centreResized = settingsFile->getPropertyAsValue("centre_resized_canvas");
        centreResized.addListener(this);
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Centre canvas when resized", centreResized, { "No", "Yes" }));

        centreSidepanelButtons = settingsFile->getPropertyAsValue("centre_sidepanel_buttons");
        interfaceProperties.add(new PropertiesPanel::BoolComponent("Sidepanel controls position", centreSidepanelButtons, { "Top", "Centre" }));

        showMinimap = settingsFile->getPropertyAsValue("show_minimap");
        interfaceProperties.add(new PropertiesPanel::ComboComponent("Show minimap", showMinimap, { "Never", "When outside of patch", "Always" }));

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
            if (auto const* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent())) {
                if (auto* closeButton = window->getCloseButton())
                    closeButton->setEnabled(false);
                if (auto* minimiseButton = window->getMinimiseButton())
                    minimiseButton->setEnabled(false);
                if (auto* maximiseButton = window->getMaximiseButton())
                    maximiseButton->setEnabled(false);
            }
        }
        if (v.refersToSameSourceAs(showPalettesValue)) {
            editor->resized();
        }
        if (v.refersToSameSourceAs(scaleValue)) {
            SettingsFile::getInstance()->setGlobalScale(getValue<float>(scaleValue));
        }
        if (v.refersToSameSourceAs(defaultZoom)) {
            auto const zoom = std::clamp(getValue<float>(defaultZoom), 20.0f, 300.0f);
            SettingsFile::getInstance()->setProperty("default_zoom", zoom);
            defaultZoom = zoom;
        }
    }
    Component* editor;

    Value nativeTitlebar;
    Value scaleValue;
    Value defaultZoom;
    Value centreResized;
    Value centreSidepanelButtons;
    Value showMinimap;

    Value openPatchesInWindow;
    Value showPalettesValue;
    Value autoPatchingValue;
    Value showAllAudioDeviceValues;
    Value nativeDialogValue;
    Value autosaveInterval;
    Value autosaveEnabled;
    Value commandClickSwitchesModeValue;

    Value patchDownwardsOnly;

    PropertiesPanel propertiesPanel;

    std::unique_ptr<AutosaveHistoryComponent> autosaveHistoryDialog;
};
