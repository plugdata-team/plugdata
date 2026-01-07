/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class PdExporter final : public ExporterBase {
public:
    Value exportTypeValue = Value(var(2));
    Value copyToPath = Value(var(0));

    PropertiesPanel::BoolComponent* copyToPathProperty;

    PdExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary" }));

        copyToPathProperty = new PropertiesPanel::BoolComponent("Copy to externals path", copyToPath, { "No", "Yes" });
        properties.add(copyToPathProperty);

        panel.addSection("Pd", properties);

        exportTypeValue.addListener(this);
    }

    ValueTree getState() override
    {
        ValueTree stateTree("PdExt");
        stateTree.setProperty("inputPatchValue", getValue<String>(inputPatchValue), nullptr);
        stateTree.setProperty("projectNameValue", getValue<String>(projectNameValue), nullptr);
        stateTree.setProperty("projectCopyrightValue", getValue<String>(projectCopyrightValue), nullptr);

        stateTree.setProperty("exportTypeValue", getValue<int>(exportTypeValue), nullptr);
        stateTree.setProperty("copyToPath", getValue<int>(copyToPath), nullptr);

        return stateTree;
    }

    void setState(ValueTree& stateTree) override
    {
        auto const tree = stateTree.getChildWithName("PdExt");
        inputPatchValue = tree.getProperty("inputPatchValue");
        projectNameValue = tree.getProperty("projectNameValue");
        projectCopyrightValue = tree.getProperty("projectCopyrightValue");
        exportTypeValue = tree.getProperty("exportTypeValue");
        copyToPath = tree.getProperty("copyToPath");
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(exportTypeValue)) {
            copyToPathProperty->setEnabled(exportTypeValue == 2);
            if (exportTypeValue == 1) {
                copyToPath = 0;
            }
        } else {
            ExporterBase::valueChanged(v);
        }
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Exporting);

#if JUCE_WINDOWS
        auto const heavyPath = heavyExecutable.getFullPathName().replaceCharacter('\\', '/');
#else
        auto const heavyPath = heavyExecutable.getFullPathName();
#endif
        StringArray args = { heavyPath.quoted(), pdPatch.quoted(), "-o", outdir.quoted() };

        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add(copyright.quoted());
        }

        args.add("-v");
        args.add("-gpdext");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        if (shouldQuit)
            return true;

        auto const command = args.joinIntoString(" ");
        exportingView->logToConsole("Command: " + command + "\n");
        Toolchain::startShellScript(command, this);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        bool const generationExitCode = getExitCode();
        // Check if we need to compile
        if (!generationExitCode && getValue<int>(exportTypeValue) == 2) {
            auto const workingDir = File::getCurrentWorkingDirectory();

            outputFile.setAsCurrentWorkingDirectory();

            auto const bin = Toolchain::dir.getChildFile("bin");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto makefile = outputFile.getChildFile("Makefile");

#if JUCE_MAC
            Toolchain::startShellScript("make -j4", this);
#elif JUCE_WINDOWS
            File pdDll;
            if (ProjectInfo::isStandalone) {
                pdDll = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory();
            } else {
                pdDll = File::getSpecialLocation(File::globalApplicationsDirectory).getChildFile("plugdata");
            }

            auto path = "export PATH=\"$PATH:" + Toolchain::dir.getChildFile("bin").getFullPathName().replaceCharacter('\\', '/') + "\"\n";
            auto cc = "CC=" + Toolchain::dir.getChildFile("bin").getChildFile("gcc.exe").getFullPathName().replaceCharacter('\\', '/') + " ";
            auto cxx = "CXX=" + Toolchain::dir.getChildFile("bin").getChildFile("g++.exe").getFullPathName().replaceCharacter('\\', '/') + " ";
            auto pdbindir = "PDBINDIR=\"" + pdDll.getFullPathName().replaceCharacter('\\', '/') + "\" ";
            auto shell = " SHELL=" + Toolchain::dir.getChildFile("bin").getChildFile("bash.exe").getFullPathName().replaceCharacter('\\', '/').quoted();

            Toolchain::startShellScript(path + cc + cxx + pdbindir + make.getFullPathName().replaceCharacter('\\', '/') + " -j4" + shell, this);

#else // Linux or BSD
            auto prepareEnvironmentScript = Toolchain::dir.getChildFile("scripts").getChildFile("anywhere-setup.sh").getFullPathName() + "\n";

            auto buildScript = prepareEnvironmentScript
                + make.getFullPathName()
                + " -j4";

            Toolchain::startShellScript(buildScript, this);
#endif

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            workingDir.setAsCurrentWorkingDirectory();

#if JUCE_MAC
            auto external = outputFile.getChildFile(name + "~.pd_darwin");
#elif JUCE_WINDOWS
            auto external = outputFile.getChildFile(name + "~.dll");
#else
            auto external = outputFile.getChildFile(name + "~.pd_linux");
#endif

            if (getValue<bool>(copyToPath)) {
                exportingView->logToConsole("Copying to Externals folder...\n");
                auto const copy_location = ProjectInfo::appDataDir.getChildFile("Externals").getChildFile(external.getFileName());
                external.copyFileTo(copy_location.getFullPathName());
                copy_location.setExecutePermission(1);
            }

            // Clean up
            outputFile.getChildFile("c").deleteRecursively();
            outputFile.getChildFile("pdext").deleteRecursively();
            outputFile.getChildFile("Makefile").deleteFile();
            outputFile.getChildFile("Makefile.pdlibbuilder").deleteFile();

            bool const compilationExitCode = getExitCode();

            return compilationExitCode;
        }

        return generationExitCode;
    }
};
