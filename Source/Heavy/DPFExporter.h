/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class DPFExporter : public ExporterBase {
public:
    Value midiinEnableValue = Value(var(1));
    Value midioutEnableValue = Value(var(1));

    Value lv2EnableValue = Value(var(1));
    Value vst2EnableValue = Value(var(1));
    Value vst3EnableValue = Value(var(1));
    Value clapEnableValue = Value(var(1));
    Value jackEnableValue = Value(var(1));

    Value exportTypeValue = Value(var(2));

    DPFExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        Array<PropertiesPanel::Property*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary" }));
        properties.add(new PropertiesPanel::BoolComponent("Midi Input", midiinEnableValue, { "No", "yes" }));
        properties.add(new PropertiesPanel::BoolComponent("Midi Output", midioutEnableValue, { "No", "yes" }));

        Array<PropertiesPanel::Property*> pluginFormats;
        
        pluginFormats.add(new PropertiesPanel::BoolComponent("LV2", lv2EnableValue, { "No", "Yes" }));
        lv2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST2", vst2EnableValue, { "No", "Yes" }));
        vst2EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("VST3", vst3EnableValue, { "No", "Yes" }));
        vst3EnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("CLAP", clapEnableValue, { "No", "Yes" }));
        clapEnableValue.addListener(this);
        pluginFormats.add(new PropertiesPanel::BoolComponent("JACK", jackEnableValue, { "No", "Yes" }));

        
        for(auto* property : properties)
        {
            property->setPreferredHeight(28);
        }
        for(auto* property : pluginFormats)
        {
            property->setPreferredHeight(28);
        }
        
        midiinEnableValue.addListener(this);
        midioutEnableValue.addListener(this);

        jackEnableValue.addListener(this);
        
        panel.addSection("DPF", properties);
        panel.addSection("Plugin formats", pluginFormats);
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Busy);

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        int midiin = getValue<int>(midiinEnableValue);
        int midiout = getValue<int>(midioutEnableValue);

        bool lv2 = getValue<int>(lv2EnableValue);
        bool vst2 = getValue<int>(vst2EnableValue);
        bool vst3 = getValue<int>(vst3EnableValue);
        bool clap = getValue<int>(clapEnableValue);
        bool jack = getValue<int>(jackEnableValue);

        StringArray formats;

        if (lv2) {
            formats.add("lv2_dsp");
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
        metaDPF.getDynamicObject()->setProperty("project", "true");
        metaDPF.getDynamicObject()->setProperty("description", "Rename Me");
        metaDPF.getDynamicObject()->setProperty("maker", "Wasted Audio");
        metaDPF.getDynamicObject()->setProperty("license", "ISC");
        metaDPF.getDynamicObject()->setProperty("midi_input", midiin);
        metaDPF.getDynamicObject()->setProperty("midi_output", midiout);
        metaDPF.getDynamicObject()->setProperty("plugin_formats", formats);

        metaJson->setProperty("dpf", metaDPF);

        args.add("-m" + createMetaJson(metaJson));

        args.add("-v");
        args.add("-gdpf");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        if (shouldQuit)
            return 1;

        start(args.joinIntoString(" "));

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return 1;

        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        outputFile.getChildFile("c").deleteRecursively();

        auto DPF = Toolchain::dir.getChildFile("lib").getChildFile("dpf");
        DPF.copyDirectoryTo(outputFile.getChildFile("dpf"));

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        bool generationExitCode = getExitCode();
        // Check if we need to compile
        if (!generationExitCode && getValue<int>(exportTypeValue) == 2) {
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
            auto bash = String("#!/bin/bash\n");
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
#if JUCE_WINDOWS
            if (vst2)
                outputFile.getChildFile("bin").getChildFile(name + "-vst.dll").moveFileTo(outputFile.getChildFile(name + "-vst.dll"));
#elif JUCE_LINUX
            if (vst2)
                outputFile.getChildFile("bin").getChildFile(name + "-vst.so").moveFileTo(outputFile.getChildFile(name + "-vst.so"));
#elif JUCE_MAC
            if (vst2)
                outputFile.getChildFile("bin").getChildFile(name + ".vst").copyDirectoryTo(outputFile.getChildFile(name + ".vst"));
#endif
            if (clap)
                outputFile.getChildFile("bin").getChildFile(name + ".clap").moveFileTo(outputFile.getChildFile(name + ".clap"));
            if (jack)
                outputFile.getChildFile("bin").getChildFile(name).moveFileTo(outputFile.getChildFile(name));

            bool compilationExitCode = getExitCode();

            // Clean up if successful
            if (!compilationExitCode) {
                outputFile.getChildFile("dpf").deleteRecursively();
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
