/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class DPFExporter final : public ExporterBase {
public:
    Value makerNameValue;
    Value projectLicenseValue;
    Value midiinEnableValue = Value(var(0));
    Value midioutEnableValue = Value(var(0));

    Value lv2EnableValue = Value(var(1));
    Value vst2EnableValue = Value(var(1));
    Value vst3EnableValue = Value(var(1));
    Value clapEnableValue = Value(var(1));
    Value jackEnableValue = Value(var(0));

    Value guiTypeValue = Value(var(1));
    Value useThemeValue = Value(var(1));
    Value pluginTypeValue = Value(var(1));

    Value disableSIMD = Value(var(0));

    PropertiesPanelProperty* useThemeProperty;
    PropertiesPanelProperty* midiinProperty;
    PropertiesPanelProperty* midioutProperty;

    DPFExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::EditableComponent<String>("Maker Name (optional)", makerNameValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Project License (optional)", projectLicenseValue));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, getExportTypes()));
        properties.add(new PropertiesPanel::ComboComponent("Gui type", guiTypeValue, { "None", "Generic (ImGui)", "Custom (NanoVG)" }));
        useThemeProperty = new PropertiesPanel::BoolComponent("Use theme", useThemeValue, { "No", "Yes" });
        properties.add(useThemeProperty);
        properties.add(new PropertiesPanel::ComboComponent("Plugin type", pluginTypeValue, { "Effect", "Instrument", "Custom" }));

        midiinProperty = new PropertiesPanel::BoolComponent("Midi Input", midiinEnableValue, { "No", "yes" });
        properties.add(midiinProperty);
        midioutProperty = new PropertiesPanel::BoolComponent("Midi Output", midioutEnableValue, { "No", "yes" });
        properties.add(midioutProperty);

        PropertiesArray pluginFormats;

        pluginFormats.add(new PropertiesPanel::BoolComponent("LV2", lv2EnableValue, { "No", "Yes" }));
        lv2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST2", vst2EnableValue, { "No", "Yes" }));
        vst2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST3", vst3EnableValue, { "No", "Yes" }));
        vst3EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("CLAP", clapEnableValue, { "No", "Yes" }));
        clapEnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("JACK", jackEnableValue, { "No", "Yes" }));
        jackEnableValue.addListener(this);

        PropertiesArray pro_properties;

        pro_properties.add(new PropertiesPanel::BoolComponent("Disable SIMD", disableSIMD, { "No", "Yes" }));

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }
        for (auto* property : pluginFormats) {
            property->setPreferredHeight(28);
        }

        guiTypeValue.addListener(this);
        pluginTypeValue.addListener(this);
        midiinEnableValue.addListener(this);
        midioutEnableValue.addListener(this);

        panel.addSection("DPF", properties);
        panel.addSection("Plugin formats", pluginFormats);
        panel.addSection("Advanced", pro_properties);
    }

    StringArray getExportTypes() const override
    {
        return { "Binary", "Source code" };
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("maker_name_value", getValue<String>(makerNameValue));
        state->setProperty("project_license_value", getValue<String>(projectLicenseValue));
        state->setProperty("midiin_enable_value", getValue<int>(midiinEnableValue));
        state->setProperty("midiout_enable_value", getValue<int>(midioutEnableValue));
        state->setProperty("lv2_enable_value", getValue<int>(lv2EnableValue));
        state->setProperty("vst2_enable_value", getValue<int>(vst2EnableValue));
        state->setProperty("vst3_enable_value", getValue<int>(vst3EnableValue));
        state->setProperty("clap_enable_value", getValue<int>(clapEnableValue));
        state->setProperty("jack_enable_value", getValue<int>(jackEnableValue));
        state->setProperty("export_type_value", getValue<int>(exportTypeValue));
        state->setProperty("gui_type_value", getValue<int>(guiTypeValue));
        state->setProperty("use_theme_value", getValue<int>(useThemeValue));
        state->setProperty("plugin_type_value", getValue<int>(pluginTypeValue));
        state->setProperty("disable_simd", getValue<int>(disableSIMD));
        globalState->setProperty("dpf", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("dpf").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        makerNameValue = state->getProperty("maker_name_value");
        projectLicenseValue = state->getProperty("project_license_value");
        midiinEnableValue = state->getProperty("midiin_enable_value");
        midioutEnableValue = state->getProperty("midiout_enable_value");
        lv2EnableValue = state->getProperty("lv2_enable_value");
        vst2EnableValue = state->getProperty("vst2_enable_value");
        vst3EnableValue = state->getProperty("vst3_enable_value");
        clapEnableValue = state->getProperty("clap_enable_value");
        jackEnableValue = state->getProperty("jack_enable_value");
        exportTypeValue = state->getProperty("export_type_value");
        guiTypeValue = state->getProperty("gui_type_value");
        if (!state->hasProperty("gui_type_value")) {
            // Older settings combined both: Binary, Binary + GUI, Source code, Source + GUI code
            int const oldExportType = getValue<int>(exportTypeValue);
            exportTypeValue = oldExportType <= 2 ? 1 : 2;
            guiTypeValue = oldExportType % 2 == 0 ? 2 : 1;
        }
        useThemeValue = state->getProperty("use_theme_value");
        pluginTypeValue = state->getProperty("plugin_type_value");
        disableSIMD = state->getProperty("disable_simd");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        int const guiType = getValue<int>(guiTypeValue);
        useThemeProperty->setEnabled(guiType == 3);

        int const pluginType = getValue<int>(pluginTypeValue);
        midiinProperty->setEnabled(pluginType == 3);
        midioutProperty->setEnabled(pluginType == 3);

        if (pluginType == 1) {
            midiinEnableValue.setValue(0);
            midioutEnableValue.setValue(0);
        } else if (pluginType == 2) {
            midiinEnableValue.setValue(1);
            midioutEnableValue.setValue(0);
        }
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto const makerName = getValue<String>(makerNameValue);
        auto const projectLicense = getValue<String>(projectLicenseValue);

        auto const exportType = getExportType();
        auto const guiType = getValue<int>(guiTypeValue);
        auto const useTheme = getValue<int>(useThemeValue);
        auto const midiin = getValue<int>(midiinEnableValue);
        auto const midiout = getValue<int>(midioutEnableValue);

        bool const lv2 = getValue<int>(lv2EnableValue);
        bool const vst2 = getValue<int>(vst2EnableValue);
        bool const vst3 = getValue<int>(vst3EnableValue);
        bool const clap = getValue<int>(clapEnableValue);
        bool const jack = getValue<int>(jackEnableValue);

        bool const nosimd = getValue<int>(disableSIMD);

        StringArray formats;

        if (lv2) {
            formats.add("lv2_sep");
        }
        if (vst2) {
            formats.add("vst2");
        }
        if (vst3) {
            formats.add("vst3");
        }
        if (clap) {
            formats.add("clap");
        }
        if (jack) {
            formats.add("jack");
        }

        DynamicObject::Ptr const metaJson(new DynamicObject());

        var const metaDPF(new DynamicObject());
        metaDPF.getDynamicObject()->setProperty("project", true);
        metaDPF.getDynamicObject()->setProperty("description", "Rename Me");
        if (makerName.isNotEmpty()) {
            metaDPF.getDynamicObject()->setProperty("maker", makerName);
        } else {
            metaDPF.getDynamicObject()->setProperty("maker", "plugdata");
        }
        if (projectLicense.isNotEmpty()) {
            metaDPF.getDynamicObject()->setProperty("license", projectLicense);
        } else {
            metaDPF.getDynamicObject()->setProperty("license", "ISC");
        }
        metaDPF.getDynamicObject()->setProperty("midi_input", midiin);
        metaDPF.getDynamicObject()->setProperty("midi_output", midiout);
        metaDPF.getDynamicObject()->setProperty("plugin_formats", formats);

        if (guiType == 2) {
            metaDPF.getDynamicObject()->setProperty("enable_ui", 1);
        } else if (guiType == 3) {
            metaDPF.getDynamicObject()->setProperty("enable_ui", 2);

            var const metaTheme(new DynamicObject());
            metaTheme.getDynamicObject()->setProperty("obj_corner_radius", getObjectCornerRadius(*this));
            metaTheme.getDynamicObject()->setProperty("cnv_color", "#" + getThemeColours(*this).canvasBackgroundColour.toDisplayString(false));
            metaTheme.getDynamicObject()->setProperty("cnv_txt_color", "#" + getThemeColours(*this).canvasTextColour.toDisplayString(false));

            if (useTheme) {
                metaDPF.getDynamicObject()->setProperty("ui_theme_set", 1);

                metaTheme.getDynamicObject()->setProperty("io_color", "#" + getThemeColours(*this).guiObjectInternalOutlineColour.toDisplayString(false));
                metaTheme.getDynamicObject()->setProperty("bg_color", "#" + getThemeColours(*this).guiObjectBackgroundColour.toDisplayString(false));
                metaTheme.getDynamicObject()->setProperty("sel_color", "#" + getThemeColours(*this).objectSelectedOutlineColour.toDisplayString(false));
                metaTheme.getDynamicObject()->setProperty("com_txt_color", "#" + getThemeColours(*this).commentTextColour.toDisplayString(false));
                metaTheme.getDynamicObject()->setProperty("out_color", "#" + getThemeColours(*this).outlineColour.toDisplayString(false));;
            }

            metaDPF.getDynamicObject()->setProperty("ui_theme", "default");

            var const metaThemes(new DynamicObject());
            metaThemes.getDynamicObject()->setProperty("default", metaTheme);
            metaDPF.getDynamicObject()->setProperty("ui_themes", metaThemes);
        }

        metaJson->setProperty("dpf", metaDPF);
        metaJson->setProperty("nosimd", nosimd);

        auto const metaJsonFile = createMetaJson(metaJson);
        StringArray options { "--mode", exportType == 1 ? "build" : "source", "--meta", pathToString(metaJsonFile) };
        if (guiType == 3)
            options.add("--gui"); // NanoVG UIs are generated from the patch's GUI objects
        return runExportScript("dpf", pdPatch, outdir, name, copyright, searchPaths, options);
    }
};
