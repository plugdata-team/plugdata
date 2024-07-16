/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class DaisyExporter : public ExporterBase {
public:
    Value targetBoardValue = SynchronousValue(var(1));
    Value exportTypeValue = SynchronousValue(var(3));
    Value usbMidiValue = SynchronousValue(var(0));
    Value debugPrintValue = SynchronousValue(var(0));
    Value blocksizeValue = SynchronousValue(48);
    Value samplerateValue = SynchronousValue(var(4));
    Value patchSizeValue = SynchronousValue(var(1));
    Value appTypeValue = SynchronousValue(var(0));

    bool dontOpenFileChooser = false;

    File customBoardDefinition;
    File customLinker;

    TextButton flashButton = TextButton("Flash");
    PropertiesPanelProperty* usbMidiProperty;
    PropertiesPanelProperty* appTypeProperty;

    DaisyExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        Array<PropertiesPanelProperty*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "Pod", "Petal", "Patch", "Patch.Init()", "Field", "Versio", "Terrarium", "Simple", "Custom JSON..." }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary", "Flash" }));
        usbMidiProperty = new PropertiesPanel::BoolComponent("USB MIDI", usbMidiValue, { "No", "Yes" });
        properties.add(usbMidiProperty);
        properties.add(new PropertiesPanel::BoolComponent("Debug printing", debugPrintValue, { "No", "Yes" }));
        auto blocksizeProperty = new PropertiesPanel::EditableComponent<int>("Blocksize", blocksizeValue);
        blocksizeProperty->setRangeMin(1);
        blocksizeProperty->setRangeMax(256);
        blocksizeProperty->editableOnClick(false);
        properties.add(blocksizeProperty);
        properties.add(new PropertiesPanel::ComboComponent("Samplerate", samplerateValue, { "8000", "16000", "32000", "48000", "96000" }));
        properties.add(new PropertiesPanel::ComboComponent("Patch size", patchSizeValue, { "Small", "Big", "Huge", "Custom Linker..." }));
        appTypeProperty = new PropertiesPanel::ComboComponent("App type", appTypeValue, { "SRAM", "QSPI" });
        properties.add(appTypeProperty);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("Daisy", properties);

        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);

        auto backgroundColour = findColour(PlugDataColour::panelBackgroundColourId);
        flashButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        flashButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        flashButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        exportTypeValue.addListener(this);
        targetBoardValue.addListener(this);
        usbMidiValue.addListener(this);
        debugPrintValue.addListener(this);
        blocksizeValue.addListener(this);
        samplerateValue.addListener(this);
        patchSizeValue.addListener(this);
        appTypeValue.addListener(this);

        flashButton.onClick = [this]() {
            auto tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            Toolchain::deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        };
    }

    ValueTree getState() override
    {
        ValueTree stateTree("Daisy");
        stateTree.setProperty("inputPatchValue", getValue<String>(inputPatchValue), nullptr);
        stateTree.setProperty("projectNameValue", getValue<String>(projectNameValue), nullptr);
        stateTree.setProperty("projectCopyrightValue", getValue<String>(projectCopyrightValue), nullptr);
        stateTree.setProperty("customBoardDefinitionValue", customBoardDefinition.getFullPathName(), nullptr);
        stateTree.setProperty("targetBoardValue", getValue<int>(targetBoardValue), nullptr);
        stateTree.setProperty("exportTypeValue", getValue<int>(exportTypeValue), nullptr);
        stateTree.setProperty("usbMidiValue", getValue<int>(usbMidiValue), nullptr);
        stateTree.setProperty("debugPrintValue", getValue<int>(debugPrintValue), nullptr);
        stateTree.setProperty("blocksizeValue", getValue<int>(blocksizeValue), nullptr);
        stateTree.setProperty("samplerateValue", getValue<int>(samplerateValue), nullptr);
        stateTree.setProperty("patchSizeValue", getValue<int>(patchSizeValue), nullptr);
        stateTree.setProperty("appTypeValue", getValue<int>(appTypeValue), nullptr);
        stateTree.setProperty("customLinkerValue", customLinker.getFullPathName(), nullptr);
        return stateTree;
    }

    void setState(ValueTree& stateTree) override
    {
        ScopedValueSetter<bool> scopedValueSetter(dontOpenFileChooser, true);

        auto tree = stateTree.getChildWithName("Daisy");
        inputPatchValue = tree.getProperty("inputPatchValue");
        projectNameValue = tree.getProperty("projectNameValue");
        projectCopyrightValue = tree.getProperty("projectCopyrightValue");
        customBoardDefinition = File(tree.getProperty("customBoardDefinitionValue").toString());
        targetBoardValue = tree.getProperty("targetBoardValue");
        exportTypeValue = tree.getProperty("exportTypeValue");
        usbMidiValue = tree.getProperty("usbMidiValue");
        debugPrintValue = tree.getProperty("debugPrintValue");
        blocksizeValue = tree.getProperty("blocksizeValue");
        samplerateValue = tree.getProperty("samplerateValue");
        patchSizeValue = tree.getProperty("patchSizeValue");
        appTypeValue = tree.getProperty("appTypeValue");
        customLinker = File(tree.getProperty("customLinkerValue").toString());
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

        bool debugPrint = getValue<int>(debugPrintValue);
        usbMidiProperty->setEnabled(!debugPrint);

        // need to actually hide this property until needed
        int patchSize = getValue<int>(patchSizeValue);
        appTypeProperty->setEnabled(patchSize == 4);

        if (patchSize <= 3) {
            appTypeValue.setValue(patchSize - 1);
        }

        if (v.refersToSameSourceAs(targetBoardValue)) {
            int idx = getValue<int>(targetBoardValue);

            // Custom board option
            if (idx == 9 && !dontOpenFileChooser) {
                Dialogs::showOpenDialog([this](URL url) {
                    auto result = url.getLocalFile();
                    if (result.existsAsFile()) {
                        customBoardDefinition = result;
                    } else {
                        customBoardDefinition = File();
                    }
                },
                    true, false, "*.json", "DaisyCustomBoard", nullptr);
            }
        }

        if (v.refersToSameSourceAs(patchSizeValue)) {
            int idx = getValue<int>(patchSizeValue);

            // Custom linker option
            if (idx == 4 && !dontOpenFileChooser) {
                Dialogs::showOpenDialog([this](URL url) {
                    auto result = url.getLocalFile();
                    if (result.existsAsFile()) {
                        customLinker = result;
                    } else {
                        customLinker = File();
                    }
                },
                    true, false, "*.lds", "DaisyCustomLinker", nullptr);
            }
        }
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        auto target = getValue<int>(targetBoardValue) - 1;
        bool compile = getValue<int>(exportTypeValue) - 1;
        bool flash = getValue<int>(exportTypeValue) == 3;
        bool usbMidi = getValue<int>(usbMidiValue);
        bool print = getValue<int>(debugPrintValue);
        auto blocksize = getValue<int>(blocksizeValue);
        auto rate = getValue<int>(samplerateValue) - 1;
        auto size = getValue<int>(patchSizeValue);
        auto appType = getValue<int>(appTypeValue);

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        // set board definition
        auto boards = StringArray { "pod", "petal", "patch", "patch_init", "field", "versio", "terrarium", "simple", "custom" };
        auto const& board = boards[target];

        auto extra_boards = StringArray { "versio", "terrarium", "simple" };

        DynamicObject::Ptr metaJson(new DynamicObject());
        var metaDaisy(new DynamicObject());

        if (board == "custom") {
            metaDaisy.getDynamicObject()->setProperty("board_file", customBoardDefinition.getFullPathName());
        } else if (extra_boards.contains(board)) {
            metaDaisy.getDynamicObject()->setProperty("board_file", Toolchain::dir.getChildFile("etc").getChildFile(board + ".json").getFullPathName());
        } else {
            metaDaisy.getDynamicObject()->setProperty("board", board);
        }

        // enable debug printing option
        if (usbMidi && !print) {
            metaDaisy.getDynamicObject()->setProperty("usb_midi", "True");
        }

        // enable debug printing option
        if (print) {
            metaDaisy.getDynamicObject()->setProperty("debug_printing", "True");
        }

        // blocksize and samplerate
        if (rate != 3) {
            auto sampleRates = Array { 8000, 16000, 32000, 48000, 96000 };
            auto const& samplerate = sampleRates[rate];
            metaDaisy.getDynamicObject()->setProperty("samplerate", samplerate);
        }

        if (blocksize != 48) {
            metaDaisy.getDynamicObject()->setProperty("blocksize", blocksize);
        }

        // set linker script and if we want bootloader
        bool bootloader = false;

        if (size == 2) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_sram.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
            bootloader = true;
        } else if (size == 3) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_qspi.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
            bootloader = true;
        } else if (size == 4) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", customLinker.getFullPathName());
            if (appType == 1) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
            } else if (appType == 2) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
            }
            bootloader = true;
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

        exportingView->logToConsole("Compiling for " + board + "...\n");

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
                    // We should first detect whether our device already has the bootloader installed
                    // This (likely) is not the case when more than one altsetting is found by dfu-util
                    //
                    // # No Bootloader:
                    // Found DFU: [0483:df11] ver=0200, devnum=33, cfg=1, intf=0, path="1-4", alt=1, name="@Option Bytes   /0x5200201C/01*128 e", serial="200364500000"
                    // Found DFU: [0483:df11] ver=0200, devnum=33, cfg=1, intf=0, path="1-4", alt=0, name="@Internal Flash   /0x08000000/16*128Kg", serial="200364500000"

                    // # Bootloader:
                    // Found DFU: [0483:df11] ver=0200, devnum=46, cfg=1, intf=0, path="1-4", alt=0, name="@Flash /0x90000000/64*4Kg/0x90040000/60*64Kg/0x90400000/60*64Kg", serial="395B377C3330"
                    //
                    // So we check for `alt=1`

                    exportingView->logToConsole("Testing bootloader...\n");

                    String testBootloaderScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
                        + dfuUtil.getFullPathName() + " -l ";

                    Toolchain runTest;
                    auto output = runTest.startShellScriptWithOutput(testBootloaderScript);
                    bool bootloaderNotFound = output.contains("alt=1");

                    if (bootloaderNotFound) {
                        exportingView->logToConsole("Bootloader not found...\n");
                        exportingView->logToConsole("Flashing bootloader...\n");

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

                        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 900);

                    } else {
                        exportingView->logToConsole("Bootloader found...\n");
                    }
                }

                exportingView->logToConsole("Flashing...\n");

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
                sourceDir.getChildFile("build").getChildFile("HeavyDaisy_" + name + ".bin").moveFileTo(binLocation);
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
};
