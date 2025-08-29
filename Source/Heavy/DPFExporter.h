/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class DPFExporter final : public ExporterBase {
public:
    Value makerNameValue;
    Value projectLicenseValue;
    Value midiinEnableValue = Value(var(0));
    Value midioutEnableValue = Value(var(0));

    Value lv2EnableValue = Value(var(1));
    Value vst2EnableValue = Value(var(1));
    Value vst3EnableValue = Value(var(1));
    Value clapEnableValue = Value(var(1));
    Value jackEnableValue = Value(var(0));

    Value exportTypeValue = Value(var(1));
    Value pluginTypeValue = Value(var(1));

    Value disableSIMD = Value(var(0));

    PropertiesPanelProperty* midiinProperty;
    PropertiesPanelProperty* midioutProperty;

    DPFExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::EditableComponent<String>("Maker Name (optional)", makerNameValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Project License (optional)", projectLicenseValue));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Binary", "Binary + GUI", "Source code", "Source + GUI code" }));
        properties.add(new PropertiesPanel::ComboComponent("Plugin type", pluginTypeValue, { "Effect", "Instrument", "Custom" }));

        midiinProperty = new PropertiesPanel::BoolComponent("Midi Input", midiinEnableValue, { "No", "yes" });
        properties.add(midiinProperty);
        midioutProperty = new PropertiesPanel::BoolComponent("Midi Output", midioutEnableValue, { "No", "yes" });
        properties.add(midioutProperty);

        PropertiesArray pluginFormats;

        pluginFormats.add(new PropertiesPanel::BoolComponent("LV2", lv2EnableValue, { "No", "Yes" }));
        lv2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST2", vst2EnableValue, { "No", "Yes" }));
        vst2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST3", vst3EnableValue, { "No", "Yes" }));
        vst3EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("CLAP", clapEnableValue, { "No", "Yes" }));
        clapEnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("JACK", jackEnableValue, { "No", "Yes" }));
        jackEnableValue.addListener(this);

        PropertiesArray pro_properties;

        pro_properties.add(new PropertiesPanel::BoolComponent("Disable SIMD", disableSIMD, { "No", "Yes" }));

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }
        for (auto* property : pluginFormats) {
            property->setPreferredHeight(28);
        }

        pluginTypeValue.addListener(this);
        midiinEnableValue.addListener(this);
        midioutEnableValue.addListener(this);

        panel.addSection("DPF", properties);
        panel.addSection("Plugin formats", pluginFormats);
        panel.addSection("Advanced", pro_properties);
    }

    ValueTree getState() override
    {
        ValueTree stateTree("DPF");

        stateTree.setProperty("inputPatchValue", getValue<String>(inputPatchValue), nullptr);
        stateTree.setProperty("projectNameValue", getValue<String>(projectNameValue), nullptr);
        stateTree.setProperty("projectCopyrightValue", getValue<String>(projectCopyrightValue), nullptr);
        stateTree.setProperty("makerNameValue", getValue<String>(makerNameValue), nullptr);
        stateTree.setProperty("projectLicenseValue", getValue<String>(projectLicenseValue), nullptr);
        stateTree.setProperty("midiinEnableValue", getValue<int>(midioutEnableValue), nullptr);
        stateTree.setProperty("lv2EnableValue", getValue<int>(lv2EnableValue), nullptr);
        stateTree.setProperty("vst2EnableValue", getValue<int>(vst2EnableValue), nullptr);
        stateTree.setProperty("vst3EnableValue", getValue<int>(vst3EnableValue), nullptr);
        stateTree.setProperty("clapEnableValue", getValue<int>(clapEnableValue), nullptr);
        stateTree.setProperty("jackEnableValue", getValue<int>(jackEnableValue), nullptr);
        stateTree.setProperty("exportTypeValue", getValue<int>(exportTypeValue), nullptr);
        stateTree.setProperty("pluginTypeValue", getValue<int>(pluginTypeValue), nullptr);
        stateTree.setProperty("disableSIMD", getValue<int>(disableSIMD), nullptr);

        return stateTree;
    }

    void setState(ValueTree& stateTree) override
    {
        auto const tree = stateTree.getChildWithName("DPF");
        inputPatchValue = tree.getProperty("inputPatchValue");
        projectNameValue = tree.getProperty("projectNameValue");
        projectCopyrightValue = tree.getProperty("projectCopyrightValue");
        makerNameValue = tree.getProperty("makerNameValue");
        projectLicenseValue = tree.getProperty("projectLicenseValue");
        midiinEnableValue = tree.getProperty("midiinEnableValue");
        midioutEnableValue = tree.getProperty("midioutEnableValue");
        lv2EnableValue = tree.getProperty("lv2EnableValue");
        vst2EnableValue = tree.getProperty("vst2EnableValue");
        vst3EnableValue = tree.getProperty("vst3EnableValue");
        clapEnableValue = tree.getProperty("clapEnableValue");
        jackEnableValue = tree.getProperty("jackEnableValue");
        exportTypeValue = tree.getProperty("exportTypeValue");
        pluginTypeValue = tree.getProperty("pluginTypeValue");
        disableSIMD = tree.getProperty("disableSIMD");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        int const pluginType = getValue<int>(pluginTypeValue);
        midiinProperty->setEnabled(pluginType == 3);
        midioutProperty->setEnabled(pluginType == 3);

        if (pluginType == 1) {
            midiinEnableValue.setValue(0);
            midioutEnableValue.setValue(0);
        } else if (pluginType == 2) {
            midiinEnableValue.setValue(1);
            midioutEnableValue.setValue(0);
        }
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Exporting);

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        auto makerName = getValue<String>(makerNameValue);
        auto projectLicense = getValue<String>(projectLicenseValue);

        int exportType = getValue<int>(exportTypeValue);
        int midiin = getValue<int>(midiinEnableValue);
        int midiout = getValue<int>(midioutEnableValue);

        bool lv2 = getValue<int>(lv2EnableValue);
        bool vst2 = getValue<int>(vst2EnableValue);
        bool vst3 = getValue<int>(vst3EnableValue);
        bool clap = getValue<int>(clapEnableValue);
        bool jack = getValue<int>(jackEnableValue);

        bool nosimd = getValue<int>(disableSIMD);

        StringArray formats;

        if (lv2) {
            formats.add("lv2_sep");
        }
        if (vst2) {
            formats.add("vst2");
        }
        if (vst3) {
            formats.add("vst3");
        }
        if (clap) {
            formats.add("clap");
        }
        if (jack) {
            formats.add("jack");
        }

        DynamicObject::Ptr metaJson(new DynamicObject());

        var metaDPF(new DynamicObject());
        metaDPF.getDynamicObject()->setProperty("project", true);
        metaDPF.getDynamicObject()->setProperty("description", "Rename Me");
        if (makerName.isNotEmpty()) {
            metaDPF.getDynamicObject()->setProperty("maker", makerName);
        } else {
            metaDPF.getDynamicObject()->setProperty("maker", "plugdata");
        }
        if (projectLicense.isNotEmpty()) {
            metaDPF.getDynamicObject()->setProperty("license", projectLicense);
        } else {
            metaDPF.getDynamicObject()->setProperty("license", "ISC");
        }
        metaDPF.getDynamicObject()->setProperty("midi_input", midiin);
        metaDPF.getDynamicObject()->setProperty("midi_output", midiout);
        metaDPF.getDynamicObject()->setProperty("plugin_formats", formats);

        if (exportType == 2 || exportType == 4) {
            metaDPF.getDynamicObject()->setProperty("enable_ui", true);
        }

        metaJson->setProperty("dpf", metaDPF);
        metaJson->setProperty("nosimd", nosimd);

        args.add("-m" + createMetaJson(metaJson));

        args.add("-v");
        args.add("-gdpf");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        if (shouldQuit)
            return true;

        auto compileString = args.joinIntoString(" ");
        exportingView->logToConsole("Command: " + compileString + "\n");
        start(compileString);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        outputFile.getChildFile("c").deleteRecursively();

        auto DPF = Toolchain::dir.getChildFile("lib").getChildFile("dpf");
        DPF.copyDirectoryTo(outputFile.getChildFile("dpf"));

        if (exportType == 2 || exportType == 4) {
            auto DPFGui = Toolchain::dir.getChildFile("lib").getChildFile("dpf-widgets");
            DPFGui.copyDirectoryTo(outputFile.getChildFile("dpf-widgets"));
        }

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        bool generationExitCode = getExitCode();
        // Check if we need to compile
        if (!generationExitCode && (exportType == 1 || exportType == 2)) {
            auto workingDir = File::getCurrentWorkingDirectory();

            outputFile.setAsCurrentWorkingDirectory();

            auto bin = Toolchain::dir.getChildFile("bin");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto makefile = outputFile.getChildFile("Makefile");

#if JUCE_MAC
            Toolchain::startShellScript("make -j4 -f " + makefile.getFullPathName(), this);
#elif JUCE_WINDOWS
            auto path = "export PATH=\"$PATH:" + Toolchain::dir.getChildFile("bin").getFullPathName().replaceCharacter('\\', '/') + "\"\n";
            auto cc = "CC=" + Toolchain::dir.getChildFile("bin").getChildFile("gcc.exe").getFullPathName().replaceCharacter('\\', '/') + " ";
            auto cxx = "CXX=" + Toolchain::dir.getChildFile("bin").getChildFile("g++.exe").getFullPathName().replaceCharacter('\\', '/') + " ";

            Toolchain::startShellScript(path + cc + cxx + make.getFullPathName().replaceCharacter('\\', '/') + " -j4 -f " + makefile.getFullPathName().replaceCharacter('\\', '/'), this);

#else // Linux or BSD
            auto prepareEnvironmentScript = Toolchain::dir.getChildFile("scripts").getChildFile("anywhere-setup.sh").getFullPathName() + "\n";

            auto buildScript = prepareEnvironmentScript
                + make.getFullPathName()
                + " -j4 -f " + makefile.getFullPathName();

            // For some reason we need to do this again
            outputFile.getChildFile("dpf").getChildFile("utils").getChildFile("generate-ttl.sh").setExecutePermission(true);
            Toolchain::dir.getChildFile("scripts").getChildFile("anywhere-setup.sh").getChildFile("generate-ttl.sh").setExecutePermission(true);

            Toolchain::startShellScript(buildScript, this);
#endif

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            workingDir.setAsCurrentWorkingDirectory();

            // Copy output
            if (lv2)
                outputFile.getChildFile("bin").getChildFile(name + ".lv2").copyDirectoryTo(outputFile.getChildFile(name + ".lv2"));
            if (vst3)
                outputFile.getChildFile("bin").getChildFile(name + ".vst3").copyDirectoryTo(outputFile.getChildFile(name + ".vst3"));
            if (vst2)
#if JUCE_WINDOWS
                outputFile.getChildFile("bin").getChildFile(name + "-vst.dll").moveFileTo(outputFile.getChildFile(name + "-vst.dll"));
#elif JUCE_LINUX
                outputFile.getChildFile("bin").getChildFile(name + "-vst.so").moveFileTo(outputFile.getChildFile(name + "-vst.so"));
#elif JUCE_MAC
                outputFile.getChildFile("bin").getChildFile(name + ".vst").copyDirectoryTo(outputFile.getChildFile(name + ".vst"));
#endif
            if (clap)
                outputFile.getChildFile("bin").getChildFile(name + ".clap").moveFileTo(outputFile.getChildFile(name + ".clap"));
            if (jack) {
#if JUCE_MAC
                if (exportType == 2) {
                    outputFile.getChildFile("bin").getChildFile(name + ".app").moveFileTo(outputFile.getChildFile(name + ".app"));
                } else {
                    outputFile.getChildFile("bin").getChildFile(name).moveFileTo(outputFile.getChildFile(name));
                }
#elif JUCE_WINDOWS
                outputFile.getChildFile("bin").getChildFile(name + ".exe").moveFileTo(outputFile.getChildFile(name + ".exe"));
#else
                outputFile.getChildFile("bin").getChildFile(name).moveFileTo(outputFile.getChildFile(name));
#endif
            }

            bool compilationExitCode = getExitCode();

            // Clean up if successful
            if (!compilationExitCode) {
                outputFile.getChildFile("dpf").deleteRecursively();
                outputFile.getChildFile("dpf-widgets").deleteRecursively();
                outputFile.getChildFile("build").deleteRecursively();
                outputFile.getChildFile("plugin").deleteRecursively();
                outputFile.getChildFile("bin").deleteRecursively();
                outputFile.getChildFile("README.md").deleteFile();
                outputFile.getChildFile("Makefile").deleteFile();
            }

            return compilationExitCode;
        }

        return generationExitCode;
    }
};
