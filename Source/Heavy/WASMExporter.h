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
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        emsdkPathValue = state->getProperty("emsdk_path_value");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        String const emsdkPath = getValue<String>(emsdkPathValue);

        if (emsdkPath.isNotEmpty()) {
            exportButton.setEnabled(true);
        } else {
            exportButton.setEnabled(false);
        }
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Exporting);

        auto const heavyPath = pathToString(heavyExecutable);
        StringArray args = { heavyPath.quoted(), pdPatch.quoted(), "-o", outdir.quoted() };
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add(copyright.quoted());
        }

        auto emsdkPath = getValue<String>(emsdkPathValue);

        args.add("-v");
        args.add("-gjs");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        if (shouldQuit)
            return true;

        auto compileString = args.joinIntoString(" ");

#if JUCE_WINDOWS
        auto buildScript = "source " + emsdkPath.replaceCharacter('\\', '/') + "/emsdk_env.sh; " + compileString;
#else
        auto buildScript = "source " + emsdkPath + "/emsdk_env.sh; " + compileString;
#endif

        startShellScript(buildScript);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto const outputFile = File(outdir);
        outputFile.getChildFile("c").deleteRecursively();
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        bool const generationExitCode = getExitCode();
        return generationExitCode;
    }
};
