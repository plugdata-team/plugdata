/*
 // Copyright (c) 2026 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class FMODExporter final : public ExporterBase {
public:
    FMODExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        globalState->setProperty("fmod", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("fmod").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        return runExportScript("fmod", pdPatch, outdir, name, copyright, searchPaths);
    }
};
