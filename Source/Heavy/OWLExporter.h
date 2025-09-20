/*
 // Copyright (c) 2024 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class OWLExporter : public ExporterBase {
public:
    Value targetBoardValue = Value(var(2));
    Value exportTypeValue = SynchronousValue(var(3));
    Value storeSlotValue = SynchronousValue(var(1));

    TextButton flashButton = TextButton("Flash");

    PropertiesPanelProperty* storeSlotProperty;

    OWLExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        Array<PropertiesPanelProperty*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "OWL1", "OWL2", "OWL3" }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary", "Load", "Store" }));
        storeSlotProperty = new PropertiesPanel::ComboComponent(
            "Store slot", storeSlotValue, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F" });
        properties.add(storeSlotProperty);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("OWL", properties);

        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);

        auto const backgroundColour = findColour(PlugDataColour::panelBackgroundColourId);
        flashButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        flashButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        flashButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        targetBoardValue.addListener(this);
        exportTypeValue.addListener(this);
        storeSlotValue.addListener(this);

        flashButton.onClick = [this] {
            auto const tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            Toolchain::deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        };
    }

    ValueTree getState() override
    {
        ValueTree stateTree("OWL");
        stateTree.setProperty("targetBoardValue", getValue<int>(targetBoardValue), nullptr);
        stateTree.setProperty("exportTypeValue", getValue<int>(exportTypeValue), nullptr);
        stateTree.setProperty("storeSlotValue", getValue<int>(storeSlotValue), nullptr);
        return stateTree;
    }

    void setState(ValueTree& stateTree) override
    {
        auto tree = stateTree.getChildWithName("OWL");
        targetBoardValue = tree.getProperty("targetBoardValue");
        exportTypeValue = tree.getProperty("exportTypeValue");
        storeSlotValue = tree.getProperty("storeSlotValue");
    }

    void resized() override
    {
        ExporterBase::resized();
        flashButton.setBounds(exportButton.getBounds());
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        flashButton.setEnabled(validPatchSelected);

        int const exportType = getValue<int>(exportTypeValue);
        bool flash = exportType == 3 || exportType == 4;
        exportButton.setVisible(!flash);
        flashButton.setVisible(flash);

        storeSlotProperty->setEnabled(exportType == 4);
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto target = getValue<int>(targetBoardValue);
        bool compile = getValue<int>(exportTypeValue) - 1;
        bool load = getValue<int>(exportTypeValue) == 3;
        bool store = getValue<int>(exportTypeValue) == 4;
        int slot = getValue<int>(storeSlotValue);

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
        args.add("-gOWL");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        auto const command = args.joinIntoString(" ");
        exportingView->logToConsole("Command: " + command + "\n");
        Toolchain::startShellScript(command, this);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        exportingView->logToConsole("Compiling...\n");

        if (shouldQuit)
            return true;

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        auto outputFile = File(outdir);
        auto sourceDir = outputFile.getChildFile("Source");

        bool heavyExitCode = getExitCode();

        if (compile) {
            auto workingDir = File::getCurrentWorkingDirectory();

            auto bin = Toolchain::dir.getChildFile("bin");
            auto OWL = Toolchain::dir.getChildFile("lib").getChildFile("OwlProgram");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto compiler = bin.getChildFile("arm-none-eabi-gcc" + exeSuffix);

            OWL.copyDirectoryTo(outputFile.getChildFile("OwlProgram"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();

            // Run from within OwlProgram directory
            auto OwlDir = outputFile.getChildFile("OwlProgram");
            OwlDir.setAsCurrentWorkingDirectory();
            OwlDir.getChildFile("Tools/FirmwareSender" + exeSuffix).setExecutePermission(1);

            auto const& gccPath = bin.getFullPathName();

            String buildScript;

#if JUCE_WINDOWS
            buildScript += make.getFullPathName().replaceCharacter('\\', '/')
                + " -j4"
                + " TOOLROOT=" + gccPath.replaceCharacter('\\', '/') + "/"
                + " BUILD=../"
                + " PATCHNAME=" + name
                + " PATCHCLASS=HeavyPatch"
                + " PATCHFILE=HeavyOWL_" + name + ".hpp"
                + " SHELL=" + Toolchain::dir.getChildFile("bin").getChildFile("bash.exe").getFullPathName().replaceCharacter('\\', '/').quoted();
#else
            buildScript += make.getFullPathName()
                + " -j4"
                + " TOOLROOT=" + gccPath + "/"
                + " BUILD=../"
                + " PATCHNAME=" + name
                + " PATCHCLASS=HeavyPatch"
                + " PATCHFILE=HeavyOWL_" + name + ".hpp";
#endif

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

            Toolchain::startShellScript(buildScript, this);

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            auto compileExitCode = getExitCode();

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
            outputFile.getChildFile("patch.bin").moveFileTo(outputFile.getChildFile(name + ".bin"));

            return heavyExitCode && compileExitCode;
        } else {
            auto outputFile = File(outdir);

            auto OWL = Toolchain::dir.getChildFile("lib").getChildFile("OwlProgram");
            OWL.copyDirectoryTo(outputFile.getChildFile("OwlProgram"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }
};
