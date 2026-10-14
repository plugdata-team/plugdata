#pragma once
/*
 // Copyright (c) 2024 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class WASMExporter final : public ExporterBase {
public:
    Value emsdkPathValue;

    WASMExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::DirectoryPathComponent("EMSDK path", emsdkPathValue));

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        emsdkPathValue.addListener(this);

        panel.addSection("WASM", properties);
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("emsdk_path_value", getValue<String>(emsdkPathValue));
        globalState->setProperty("wasm", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("wasm").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        emsdkPathValue = state->getProperty("emsdk_path_value");
    }

    bool canPerformExport() const override
    {
        return ExporterBase::canPerformExport() && getValue<String>(emsdkPathValue).isNotEmpty();
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        return runExportScript("wasm", pdPatch, outdir, name, copyright, searchPaths,
            { "--emsdk", getValue<String>(emsdkPathValue).replaceCharacter('\\', '/') });
    }
};
