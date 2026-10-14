/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class PdExporter final : public ExporterBase {
public:
    Value copyToPath = Value(var(0));

    PropertiesPanel::BoolComponent* copyToPathProperty;

    PdExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        exportTypeValue = var(2);

        PropertiesArray properties;
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, getExportTypes()));

        copyToPathProperty = new PropertiesPanel::BoolComponent("Copy to externals path", copyToPath, { "No", "Yes" });
        properties.add(copyToPathProperty);

        panel.addSection("Pd", properties);

        exportTypeValue.addListener(this);
    }

    StringArray getExportTypes() const override
    {
        return { "Source code", "Binary" };
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("export_type_value", getValue<int>(exportTypeValue));
        state->setProperty("copy_to_path", getValue<int>(copyToPath));
        globalState->setProperty("pdext", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("pdext").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        exportTypeValue = state->getProperty("export_type_value");
        copyToPath = state->getProperty("copy_to_path");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        if (v.refersToSameSourceAs(exportTypeValue)) {
            copyToPathProperty->setEnabled(exportTypeValue == 2);
            if (exportTypeValue == 1) {
                copyToPath = 0;
            }
        }
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto const build = getExportType() == 2;
        StringArray options { "--mode", build ? "build" : "source" };
#if JUCE_WINDOWS
        if (build) {
            auto const pdBin = ProjectInfo::isStandalone
                ? File::getSpecialLocation(File::currentApplicationFile).getParentDirectory()
                : File::getSpecialLocation(File::globalApplicationsDirectory).getChildFile("plugdata");
            options.addArray({ "--pd-bin", pathToString(pdBin) });
        }
#endif
        if (runExportScript("pd", pdPatch, outdir, name, copyright, searchPaths, options))
            return true;

        if (build && getValue<bool>(copyToPath)) {
            if (shouldQuit)
                return true;

#if JUCE_MAC
            auto const extension = "~.pd_darwin";
#elif JUCE_WINDOWS
            auto const extension = "~.dll";
#else
            auto const extension = "~.pd_linux";
#endif
            auto const external = File(outdir).getChildFile(name + extension);
            auto const destination = ProjectInfo::appDataDir.getChildFile("Externals").getChildFile(external.getFileName());
            exportingView->reportStatus("Copying");
            exportingView->logToConsole("Copying to Externals folder...\n");
            if (!destination.getParentDirectory().createDirectory() || !external.copyFileTo(destination)) {
                exportingView->logToConsole("Failed to copy external to " + destination.getFullPathName() + "\n");
                return true;
            }
            destination.setExecutePermission(true);
        }
        return false;
    }
};
