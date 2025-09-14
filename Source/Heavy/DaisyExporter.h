/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class DaisyExporter final : public ExporterBase {
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
    TextButton flashBootloaderButton = TextButton("Bootloader");
    PropertiesPanelProperty* usbMidiProperty;
    PropertiesPanelProperty* appTypeProperty;

    DaisyExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "Pod", "Petal", "Patch", "Patch.Init()", "Field", "Versio", "Terrarium", "Hothouse", "Simple", "Custom JSON..." }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary", "Flash", "Flash Bootloader" }));
        usbMidiProperty = new PropertiesPanel::BoolComponent("USB MIDI", usbMidiValue, { "No", "Yes" });
        properties.add(usbMidiProperty);
        properties.add(new PropertiesPanel::BoolComponent("Debug printing", debugPrintValue, { "No", "Yes" }));
        auto const blocksizeProperty = new PropertiesPanel::EditableComponent<int>("Blocksize", blocksizeValue);
        blocksizeProperty->setRangeMin(1);
        blocksizeProperty->setRangeMax(256);
        blocksizeProperty->editableOnClick(false);
        properties.add(blocksizeProperty);
        properties.add(new PropertiesPanel::ComboComponent("Samplerate", samplerateValue, { "8000", "16000", "32000", "48000", "96000" }));
        properties.add(new PropertiesPanel::ComboComponent("Patch size", patchSizeValue, { "Small", "Big", "Big + SDRAM", "Huge", "Huge + SDRAM", "Custom Linker..." }));
        appTypeProperty = new PropertiesPanel::ComboComponent("App type", appTypeValue, { "NONE", "SRAM", "QSPI" });
        properties.add(appTypeProperty);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("Daisy", properties);

        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);
        addAndMakeVisible(flashBootloaderButton);

        auto const backgroundColour = findColour(PlugDataColour::panelBackgroundColourId);
        flashButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        flashButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        flashButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        flashBootloaderButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        flashBootloaderButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        flashBootloaderButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        exportTypeValue.addListener(this);
        targetBoardValue.addListener(this);
        usbMidiValue.addListener(this);
        debugPrintValue.addListener(this);
        blocksizeValue.addListener(this);
        samplerateValue.addListener(this);
        patchSizeValue.addListener(this);
        appTypeValue.addListener(this);

        flashButton.onClick = [this] {
            auto const tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            Toolchain::deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        };

        flashBootloaderButton.onClick = [this, exportingView] {
            addJob([this, exportingView]() mutable {
                exportingView->monitorProcessOutput(this);
                exportingView->showState(ExportingProgressView::Flashing);

                auto const bin = Toolchain::dir.getChildFile("bin");
                auto const make = bin.getChildFile("make" + exeSuffix);
                auto const& gccPath = bin.getFullPathName();
                auto const sourceDir = Toolchain::dir.getChildFile("lib").getChildFile("libdaisy").getChildFile("core");

                int const result = flashBootloader(bin, sourceDir, make, gccPath);

                exportingView->showState(result ? ExportingProgressView::BootloaderFlashFailure : ExportingProgressView::BootloaderFlashSuccess);
                exportingView->stopMonitoring();

                MessageManager::callAsync([this] {
                    repaint();
                });
            });
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

        auto const tree = stateTree.getChildWithName("Daisy");
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
        flashBootloaderButton.setBounds(exportButton.getBounds());
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        flashButton.setEnabled(validPatchSelected);

        bool const flash = getValue<int>(exportTypeValue) == 3;
        exportButton.setVisible(!flash);
        flashButton.setVisible(flash);

        bool const flashBootloader = getValue<int>(exportTypeValue) == 4;
        exportButton.setVisible(!flashBootloader);
        flashBootloaderButton.setVisible(flashBootloader);

        bool const debugPrint = getValue<int>(debugPrintValue);
        usbMidiProperty->setEnabled(!debugPrint);

        // need to actually hide this property until needed
        int const patchSize = getValue<int>(patchSizeValue);
        appTypeProperty->setEnabled(patchSize == 6);

        if (patchSize == 1) {
            appTypeValue.setValue(1);
        } else if (patchSize == 2 || patchSize == 3) {
            appTypeValue.setValue(2);
        } else if (patchSize == 4 || patchSize == 5) {
            appTypeValue.setValue(3);
        }

        if (v.refersToSameSourceAs(targetBoardValue)) {
            int const idx = getValue<int>(targetBoardValue);

            // Custom board option
            if (idx == 10 && !dontOpenFileChooser) {
                Dialogs::showOpenDialog([this](URL const& url) {
                    auto const result = url.getLocalFile();
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
            int const idx = getValue<int>(patchSizeValue);

            // Custom linker option
            if (idx == 6 && !dontOpenFileChooser) {
                Dialogs::showOpenDialog([this](URL const& url) {
                    auto const result = url.getLocalFile();
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

    int flashBootloader(auto bin, auto sourceDir, auto make, auto gccPath)
    {
        exportingView->logToConsole("Flashing bootloader...\n");

#if JUCE_WINDOWS
        String bootloaderScript = "export PATH=\"" + bin.getFullPathName().replaceCharacter('\\', '/') + ":$PATH\"\n"
            + "cd " + sourceDir.getFullPathName().replaceCharacter('\\', '/') + "\n"
            + make.getFullPathName().replaceCharacter('\\', '/') + " program-boot"
            + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/');
#else
        String bootloaderScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
            + "cd " + sourceDir.getFullPathName() + "\n"
            + make.getFullPathName() + " program-boot"
            + " GCC_PATH=" + gccPath;
#endif

        Toolchain::startShellScript(bootloaderScript, this);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 900);

        return getExitCode();
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


        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o", outdir.quoted()};

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add(copyright.quoted());
        }

        // set board definition
        auto boards = StringArray { "pod", "petal", "patch", "patch_init", "field", "versio", "terrarium", "hothouse", "simple", "custom" };
        auto const& board = boards[target];

        auto extra_boards = StringArray { "versio", "terrarium", "hothouse", "simple" };

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

        if (size > 1) {
            bootloader = true;
        }

        if (size == 2) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_sram.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
        } else if (size == 3) {
            metaDaisy.getDynamicObject()->setProperty(
                "linker_script",
                Toolchain::dir.getChildFile("etc").getChildFile("linkers").getChildFile("sram_linker_sdram.lds").getFullPathName());
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
        } else if (size == 4) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_qspi.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
        } else if (size == 5) {
            metaDaisy.getDynamicObject()->setProperty(
                "linker_script",
                Toolchain::dir.getChildFile("etc").getChildFile("linkers").getChildFile("qspi_linker_sdram.lds").getFullPathName());
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
        } else if (size == 6) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", customLinker.getFullPathName());
            if (appType == 2) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
            } else if (appType == 3) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
            }
        }

        metaJson->setProperty("daisy", metaDaisy);
        auto metaJsonFile = createMetaJson(metaJson);
        args.add("-m" + metaJsonFile.getFullPathName().quoted());

        args.add("-v");
        args.add("-gdaisy");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        auto const command = args.joinIntoString(" ");
        exportingView->logToConsole("Command: " + command + "\n");
        Toolchain::startShellScript(command, this);
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
        metaJsonFile.copyFileTo(outputFile.getChildFile("meta.json"));

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
                + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/').quoted()
                + " SHELL=" + Toolchain::dir.getChildFile("bin").getChildFile("bash.exe").getFullPathName().replaceCharacter('\\', '/').quoted()
                + " GCC_PATH="
                + gccPath.replaceCharacter('\\', '/')
                + " PROJECT_NAME=" + name;

            Toolchain::startShellScript(buildScript, this);
#else
            String buildScript = make.getFullPathName()
                + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName().quoted()
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
                int bootloaderExitCode = 0;

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
                    if (output.contains("alt=1")) {
                        exportingView->logToConsole("Bootloader not found...\n");
                        bootloaderExitCode = flashBootloader(bin, sourceDir, make, gccPath);
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

                return heavyExitCode && flashExitCode && bootloaderExitCode;
            }
            auto binLocation = outputFile.getChildFile(name + ".bin");
            sourceDir.getChildFile("build").getChildFile("HeavyDaisy_" + name + ".bin").moveFileTo(binLocation);

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
