/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class DaisyExporter final : public ExporterBase {
public:
    Value targetBoardValue = SynchronousValue(var(1));
    Value usbMidiValue = SynchronousValue(var(0));
    Value debugPrintValue = SynchronousValue(var(0));
    Value blocksizeValue = SynchronousValue(48);
    Value samplerateValue = SynchronousValue(var(4));
    Value patchSizeValue = SynchronousValue(var(1));
    Value appTypeValue = SynchronousValue(var(0));

    File customBoardDefinition;
    File customLinker;

    PropertiesPanelProperty* usbMidiProperty;
    PropertiesPanelProperty* appTypeProperty;

    DaisyExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        exportTypeValue = var(3);

        PropertiesArray properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "Pod", "Petal", "Patch", "Patch.Init()", "Field", "Versio", "Terrarium", "Hothouse", "Simple", "Custom JSON..." }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, getExportTypes()));
        usbMidiProperty = new PropertiesPanel::BoolComponent("USB MIDI", usbMidiValue, { "No", "Yes" });
        properties.add(usbMidiProperty);
        properties.add(new PropertiesPanel::BoolComponent("Debug printing", debugPrintValue, { "No", "Yes" }));
        auto const blocksizeProperty = new PropertiesPanel::EditableComponent<int>("Blocksize", blocksizeValue, true, 1, 256);
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

        exportTypeValue.addListener(this);
        targetBoardValue.addListener(this);
        usbMidiValue.addListener(this);
        debugPrintValue.addListener(this);
        blocksizeValue.addListener(this);
        samplerateValue.addListener(this);
        patchSizeValue.addListener(this);
        appTypeValue.addListener(this);
    }

    StringArray getExportTypes() const override
    {
        return { "Source code", "Binary", "Flash", "Flash Bootloader" };
    }

    ExportAction getExportAction() const override
    {
        switch (getExportType()) {
        case 3:
            return Flash;
        case 4:
            return FlashBootloader;
        default:
            return Export;
        }
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("export_type_value", getValue<int>(exportTypeValue));
        state->setProperty("target_board_value", getValue<int>(targetBoardValue));
        state->setProperty("custom_board_definition_value", customBoardDefinition.getFullPathName());
        state->setProperty("usb_midi_value", getValue<int>(usbMidiValue));
        state->setProperty("debug_print_value", getValue<int>(debugPrintValue));
        state->setProperty("blocksize_value", getValue<int>(blocksizeValue));
        state->setProperty("samplerate_value", getValue<int>(samplerateValue));
        state->setProperty("patch_size_value", getValue<int>(patchSizeValue));
        state->setProperty("app_type_value", getValue<int>(appTypeValue));
        state->setProperty("custom_linker_value", customLinker.getFullPathName());
        globalState->setProperty("daisy", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("daisy").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        exportTypeValue = state->getProperty("export_type_value");
        targetBoardValue = state->getProperty("target_board_value");
        customBoardDefinition = File(state->getProperty("custom_board_definition_value"));
        usbMidiValue = state->getProperty("usb_midi_value");
        debugPrintValue = state->getProperty("debug_print_value");
        blocksizeValue = state->getProperty("blocksize_value");
        samplerateValue = state->getProperty("samplerate_value");
        patchSizeValue = state->getProperty("patch_size_value");
        appTypeValue = state->getProperty("app_type_value");
        customLinker = File(state->getProperty("custom_linker_value"));
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

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
            if (idx == 10 && !blockDialog) {
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
            if (idx == 6 && !blockDialog) {
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

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        if (getExportAction() == FlashBootloader) {
            exportingView->reportStatus("Flashing bootloader");
            return runToolchainScript("flash-daisy-bootloader.sh", { });
        }

        auto target = getValue<int>(targetBoardValue) - 1;
        bool usbMidi = getValue<int>(usbMidiValue);
        bool print = getValue<int>(debugPrintValue);
        auto blocksize = getValue<int>(blocksizeValue);
        auto rate = getValue<int>(samplerateValue) - 1;
        auto size = getValue<int>(patchSizeValue);
        auto appType = getValue<int>(appTypeValue);

        // set board definition
        auto boards = StringArray { "pod", "petal", "patch", "patch_init", "field", "versio", "terrarium", "hothouse", "simple", "custom" };
        auto const& board = boards[target];

        auto extra_boards = StringArray { "versio", "terrarium", "hothouse", "simple" };

        DynamicObject::Ptr metaJson(new DynamicObject());
        var metaDaisy(new DynamicObject());

        if (board == "custom") {
            metaDaisy.getDynamicObject()->setProperty("board_file", pathToString(customBoardDefinition));
        } else if (extra_boards.contains(board)) {
            metaDaisy.getDynamicObject()->setProperty("board_file", pathToString(toolchainDir.getChildFile("etc").getChildFile(board + ".json")));
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
        if (size == 2) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_sram.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
        } else if (size == 3) {
            metaDaisy.getDynamicObject()->setProperty(
                "linker_script",
                pathToString(toolchainDir.getChildFile("etc").getChildFile("linkers").getChildFile("sram_linker_sdram.lds")));
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
        } else if (size == 4) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", "../../libdaisy/core/STM32H750IB_qspi.lds");
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
        } else if (size == 5) {
            metaDaisy.getDynamicObject()->setProperty(
                "linker_script",
                pathToString(toolchainDir.getChildFile("etc").getChildFile("linkers").getChildFile("qspi_linker_sdram.lds")));
            metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
        } else if (size == 6) {
            metaDaisy.getDynamicObject()->setProperty("linker_script", pathToString(customLinker));
            if (appType == 2) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_SRAM");
            } else if (appType == 3) {
                metaDaisy.getDynamicObject()->setProperty("bootloader", "BOOT_QSPI");
            }
        }

        metaJson->setProperty("daisy", metaDaisy);
        auto metaJsonFile = createMetaJson(metaJson);
        auto const exportType = getExportType();
        return runExportScript("daisy", pdPatch, outdir, name, copyright, searchPaths,
            { "--mode", exportType == 1 ? "source" : exportType == 3 ? "flash" : "build",
                "--meta", pathToString(metaJsonFile) });
    }
};
