/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class DaisyExporter : public ExporterBase {
public:
    Value targetBoardValue = Value(var(1));
    Value exportTypeValue = Value(var(3));
    Value romOptimisationType = Value(var(2));
    Value ramOptimisationType = Value(var(2));

    File customBoardDefinition;

    TextButton flashButton = TextButton("Flash");
    PropertiesPanel::Property* ramOptimisation;
    PropertiesPanel::Property* romOptimisation;

    DaisyExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        Array<PropertiesPanel::Property*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "Seed", "Pod", "Petal", "Patch", "Patch Init", "Field", "Simple", "Custom JSON..." }));

        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary", "Flash" }));

        romOptimisation = new PropertiesPanel::ComboComponent("ROM Optimisation", romOptimisationType, { "Optimise for size", "Optimise for speed" });
        ramOptimisation = new PropertiesPanel::ComboComponent("RAM Optimisation", ramOptimisationType, { "Optimise for size", "Optimise for speed" });

        properties.add(romOptimisation);
        properties.add(ramOptimisation);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("Daisy", properties);

        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);

        exportTypeValue.addListener(this);
        targetBoardValue.addListener(this);

        flashButton.onClick = [this]() {
            auto tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            Toolchain::deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        };
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

        bool flash = getValue<int>(exportTypeValue) == 3;
        exportButton.setVisible(!flash);
        flashButton.setVisible(flash);

        ramOptimisation->setVisible(flash);
        romOptimisation->setVisible(flash);

        if (v.refersToSameSourceAs(targetBoardValue)) {
            int idx = getValue<int>(targetBoardValue);

            // Custom board option
            if (idx == 8) {
                // Open file browser
                openChooser = std::make_unique<FileChooser>("Choose file to open", File::getSpecialLocation(File::userHomeDirectory), "*.json", true);

                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser) {
                    auto result = fileChooser.getResult();
                    if (result.existsAsFile()) {
                        customBoardDefinition = result;
                    } else {
                        customBoardDefinition = File();
                    }
                });
            } else {
                customBoardDefinition = File();
            }
        }
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        auto target = getValue<int>(targetBoardValue) - 1;
        bool compile = getValue<int>(exportTypeValue) - 1;
        bool flash = getValue<int>(exportTypeValue) == 3;

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        auto boards = StringArray { "seed", "pod", "petal", "patch", "patch_init", "field", "simple", "custom" };
        auto const& board = boards[target];

        DynamicObject::Ptr metaJson(new DynamicObject());
        var metaDaisy(new DynamicObject());

        if (board == "custom") {
            metaDaisy.getDynamicObject()->setProperty("board_file", customBoardDefinition.getFullPathName());
        } else if (board == "simple") {
            metaDaisy.getDynamicObject()->setProperty("board_file", Toolchain::dir.getChildFile("etc").getChildFile("simple.json").getFullPathName());
        } else {
            metaDaisy.getDynamicObject()->setProperty("board", board);
        }
        metaJson->setProperty("daisy", metaDaisy);
        args.add("-m" + createMetaJson(metaJson));

        args.add("-v");
        args.add("-gdaisy");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        start(args.joinIntoString(" "));
        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        exportingView->logToConsole("Compiling...");

        if (shouldQuit)
            return true;

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        auto outputFile = File(outdir);
        auto sourceDir = outputFile.getChildFile("daisy").getChildFile("source");

        bool heavyExitCode = getExitCode();

        if (compile) {

            auto bin = Toolchain::dir.getChildFile("bin");
            auto libDaisy = Toolchain::dir.getChildFile("lib").getChildFile("libdaisy");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto compiler = bin.getChildFile("arm-none-eabi-gcc" + exeSuffix);

            libDaisy.copyDirectoryTo(outputFile.getChildFile("libdaisy"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();

            auto workingDir = File::getCurrentWorkingDirectory();

            sourceDir.setAsCurrentWorkingDirectory();

            sourceDir.getChildFile("build").createDirectory();
            Toolchain::dir.getChildFile("lib").getChildFile("heavy-static.a").copyFileTo(sourceDir.getChildFile("build").getChildFile("heavy-static.a"));
            Toolchain::dir.getChildFile("etc").getChildFile("daisy_makefile").copyFileTo(sourceDir.getChildFile("Makefile"));

            bool bootloader = setMakefileVariables(sourceDir.getChildFile("Makefile"));

            auto const& gccPath = bin.getFullPathName();

#if JUCE_WINDOWS
            auto buildScript = make.getFullPathName().replaceCharacter('\\', '/')
                + " -j4 -f "
                + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/')
                + " GCC_PATH="
                + gccPath.replaceCharacter('\\', '/')
                + " PROJECT_NAME=" + name;

            Toolchain::startShellScript(buildScript, this);
#else
            String buildScript = make.getFullPathName()
                + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName()
                + " GCC_PATH=" + gccPath
                + " PROJECT_NAME=" + name;

            Toolchain::startShellScript(buildScript, this);
#endif

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            auto compileExitCode = getExitCode();
            if (flash && !compileExitCode) {

                auto dfuUtil = bin.getChildFile("dfu-util" + exeSuffix);

                if (bootloader) {
                    exportingView->logToConsole("Flashing bootloader...");

#if JUCE_WINDOWS
                    String bootloaderScript = "export PATH=\"" + bin.getFullPathName().replaceCharacter('\\', '/') + ":$PATH\"\n"
                        + "cd " + sourceDir.getFullPathName().replaceCharacter('\\', '/') + "\n"
                        + make.getFullPathName().replaceCharacter('\\', '/') + " program-boot"
                        + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/')
                        + " PROJECT_NAME=" + name;
#else
                    String bootloaderScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
                        + "cd " + sourceDir.getFullPathName() + "\n"
                        + make.getFullPathName() + " program-boot"
                        + " GCC_PATH=" + gccPath
                        + " PROJECT_NAME=" + name;
#endif

                    Toolchain::startShellScript(bootloaderScript, this);

                    waitForProcessToFinish(-1);
                    exportingView->flushConsole();

                    Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 600);

                    // We need to enable DFU mode again after flashing the bootloader
                    // This will show DFU mode dialog synchonously
                    // exportingView->waitForUserInput("Please put your Daisy in DFU mode again");
                }

                exportingView->logToConsole("Flashing...");

#if JUCE_WINDOWS
                String flashScript = "export PATH=\"" + bin.getFullPathName().replaceCharacter('\\', '/') + ":$PATH\"\n"
                    + "cd " + sourceDir.getFullPathName().replaceCharacter('\\', '/') + "\n"
                    + make.getFullPathName().replaceCharacter('\\', '/') + " program-dfu"
                    + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/')
                    + " PROJECT_NAME=" + name;
#else
                String flashScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
                    + "cd " + sourceDir.getFullPathName() + "\n"
                    + make.getFullPathName() + " program-dfu"
                    + " GCC_PATH=" + gccPath
                    + " PROJECT_NAME=" + name;
#endif

                Toolchain::startShellScript(flashScript, this);

                waitForProcessToFinish(-1);
                exportingView->flushConsole();

                // Delay to get correct exit code
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

                auto flashExitCode = getExitCode();

                return heavyExitCode && flashExitCode;
            } else {
                auto binLocation = outputFile.getChildFile(name + ".bin");
                sourceDir.getChildFile("build").getChildFile("Heavy_" + name + ".bin").moveFileTo(binLocation);
            }

            outputFile.getChildFile("daisy").deleteRecursively();
            outputFile.getChildFile("libdaisy").deleteRecursively();

            return heavyExitCode && compileExitCode;
        } else {
            auto outputFile = File(outdir);

            auto libDaisy = Toolchain::dir.getChildFile("lib").getChildFile("libdaisy");
            libDaisy.copyDirectoryTo(outputFile.getChildFile("libdaisy"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }

    bool setMakefileVariables(File const& makefile) const
    {

        int ramType = getValue<int>(ramOptimisationType);
        int romType = getValue<int>(romOptimisationType);

        auto linkerDir = Toolchain::dir.getChildFile("etc").getChildFile("linkers");
        File linkerFile;

        bool bootloader = false;

        // Optimisation: 1 is size, 2 is speed
        if (romType == 1) {
            if (ramType == 1) {
                linkerFile = linkerDir.getChildFile("sram_linker_sdram.lds");
            } else if (ramType == 2) {
                linkerFile = linkerDir.getChildFile("sram_linker.lds");
            }

            bootloader = true;
        } else if (romType == 2 && ramType == 1) {
            linkerFile = linkerDir.getChildFile("default_linker_sdram.lds");
        }
        // 2-2 is skipped because it's the default

        // Modify makefile
        auto makefileText = makefile.loadFileAsString();
        if (linkerFile.existsAsFile())
            makefileText = makefileText.replace("# LINKER", "LDSCRIPT = " + linkerFile.getFullPathName());
        if (bootloader)
            makefileText = makefileText.replace("# BOOTLOADER", "APP_TYPE = BOOT_SRAM");

        makefile.replaceWithText(makefileText, false, false, "\n");

        return bootloader;
    }
};
