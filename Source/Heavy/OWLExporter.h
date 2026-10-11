#pragma once
/*
 // Copyright (c) 2024 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class OWLExporter final : public ExporterBase {
public:
    Value targetBoardValue = Value(var(2));
    Value storeSlotValue = SynchronousValue(var(1));

    PropertiesPanelProperty* storeSlotProperty;

    OWLExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        exportTypeValue = var(3);

        Array<PropertiesPanelProperty*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "OWL1", "OWL2", "OWL3" }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, getExportTypes()));
        storeSlotProperty = new PropertiesPanel::ComboComponent(
            "Store slot", storeSlotValue, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F" });
        properties.add(storeSlotProperty);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("OWL", properties);

        targetBoardValue.addListener(this);
        exportTypeValue.addListener(this);
        storeSlotValue.addListener(this);
    }

    StringArray getExportTypes() const override
    {
        return { "Source code", "Binary", "Load", "Store" };
    }

    ExportAction getExportAction() const override
    {
        int const exportType = getExportType();
        return exportType == 3 || exportType == 4 ? Flash : Export;
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("export_type_value", getValue<int>(exportTypeValue));
        state->setProperty("target_board_value", getValue<int>(targetBoardValue));
        state->setProperty("store_slot_value", getValue<int>(storeSlotValue));
        globalState->setProperty("owl", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("owl").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        exportTypeValue = state->getProperty("export_type_value");
        targetBoardValue = state->getProperty("target_board_value");
        storeSlotValue = state->getProperty("store_slot_value");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        storeSlotProperty->setEnabled(getValue<int>(exportTypeValue) == 4);
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto const target = getValue<int>(targetBoardValue);
        bool const compile = getExportType() - 1;
        bool const load = getExportType() == 3;
        bool const store = getExportType() == 4;
        int const slot = getValue<int>(storeSlotValue);

        auto const heavyPath = pathToString(heavyExecutable);
        StringArray args = { heavyPath.quoted(), pdPatch.quoted(), "-o", outdir.quoted() };

        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add(copyright.quoted());
        }

        args.add("-v");
        args.add("-gOWL");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        auto const command = args.joinIntoString(" ");
        startShellScript(command);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        exportingView->reportStatus("Compiling");
        exportingView->logToConsole("Compiling...\n");

        if (shouldQuit)
            return true;

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        auto const outputFile = File(outdir);
        auto sourceDir = outputFile.getChildFile("Source");

        bool const heavyExitCode = getExitCode();

        if (compile) {
            auto const workingDir = File::getCurrentWorkingDirectory();

            auto const bin = toolchainDir.getChildFile("bin");
            auto const OWL = toolchainDir.getChildFile("lib").getChildFile("OwlProgram");
            auto make = bin.getChildFile("make" + exeSuffix);

            OWL.copyDirectoryTo(outputFile.getChildFile("OwlProgram"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();

            // Run from within OwlProgram directory
            auto const OwlDir = outputFile.getChildFile("OwlProgram");
            OwlDir.setAsCurrentWorkingDirectory();
            OwlDir.getChildFile("Tools/FirmwareSender" + exeSuffix).setExecutePermission(1);

            auto const& gccPath = bin.getFullPathName();

            String buildScript;

            buildScript += pathToString(make)
                + " -j4"
#if JUCE_WINDOWS
                + " SHELL=" + pathToString(toolchainDir.getChildFile("bin").getChildFile("bash.exe")).quoted()
#endif
                + " TOOLROOT=" + pathToString(gccPath) + "/"
                + " BUILD=../"
                + " PATCHNAME=" + name
                + " PATCHCLASS=HeavyPatch"
                + " PATCHFILE=HeavyOWL_" + name + ".hpp";

            buildScript += " PLATFORM=OWL" + String(target);

            if (load) {
                // load into flash memory
                buildScript += " load";
            } else if (store) {
                // store into specific slot
                buildScript += " store";
                buildScript += " SLOT=" + String(slot);
            } else {
                // only build a binary
                buildScript += " patch";
            }

            startShellScript(buildScript);

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            auto const compileExitCode = getExitCode();

            // cleanup
            outputFile.getChildFile("OwlProgram").deleteRecursively();
            outputFile.getChildFile("web").deleteRecursively();
            outputFile.getChildFile("Test").deleteRecursively();
            outputFile.getChildFile("Source").deleteRecursively();
            outputFile.getChildFile("patch.elf").deleteFile();

            for (auto const& extension : StringArray("h", "cpp", "o", "d")) {
                for (auto& file : outputFile.findChildFiles(2, false, "*." + extension)) {
                    file.deleteFile();
                }
            }

            // rename binary
            OSUtils::moveFileTo(outputFile.getChildFile("patch.bin"), outputFile.getChildFile(name + ".bin"));

            if (!compileExitCode) {
                exportingView->logToConsole("Compilation finished");
            }

            return heavyExitCode && compileExitCode;
        } else {
            auto const outputFile = File(outdir);

            auto const OWL = toolchainDir.getChildFile("lib").getChildFile("OwlProgram");
            OWL.copyDirectoryTo(outputFile.getChildFile("OwlProgram"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }
};
