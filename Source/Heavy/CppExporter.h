/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class CppExporter final : public ExporterBase {
public:
    CppExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        globalState->setProperty("cpp", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("cpp").getDynamicObject();
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
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

        args.add("-v");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        if (shouldQuit)
            return true;

        auto const command = args.joinIntoString(" ");
        startShellScript(command);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto const outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        return getExitCode();
    }
};
