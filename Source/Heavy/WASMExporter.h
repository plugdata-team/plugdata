/*
 // Copyright (c) 2024 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class WASMExporter : public ExporterBase {
public:

    Value emsdkPathValue;

    WASMExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::DirectoryPathComponent("EMSDK path", emsdkPathValue));

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        emsdkPathValue.addListener(this);

        panel.addSection("WASM", properties);
    }

    ValueTree getState() override
    {
        ValueTree stateTree("WASM");

        stateTree.setProperty("inputPatchValue", getValue<String>(inputPatchValue), nullptr);
        stateTree.setProperty("projectNameValue", getValue<String>(projectNameValue), nullptr);
        stateTree.setProperty("projectCopyrightValue", getValue<String>(projectCopyrightValue), nullptr);
        stateTree.setProperty("emsdkPathValue", getValue<String>(emsdkPathValue), nullptr);

        return stateTree;
    }

    void setState(ValueTree& stateTree) override
    {
        auto tree = stateTree.getChildWithName("WASM");
        inputPatchValue = tree.getProperty("inputPatchValue");
        projectNameValue = tree.getProperty("projectNameValue");
        projectCopyrightValue = tree.getProperty("projectCopyrightValue");
        emsdkPathValue = tree.getProperty("emsdkPathValue");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        String const emsdkPath = getValue<String>(emsdkPathValue);

        if (emsdkPath.isNotEmpty()) {
            exportButton.setEnabled(true);
        } else {
            exportButton.setEnabled(false);
        }
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Exporting);

#if JUCE_WINDOWS
        StringArray args = { heavyExecutable.getFullPathName().replaceCharacter('\\', '/'), pdPatch.replaceCharacter('\\', '/'), "-o" + outdir.replaceCharacter('\\', '/') };
#else
        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };
#endif

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        auto emsdkPath = getValue<String>(emsdkPathValue);

        args.add("-v");
        args.add("-gjs");

        String paths = "-p";
        for (auto& path : searchPaths) {
#if JUCE_WINDOWS
            paths += " " + path.replaceCharacter('\\', '/');
#else
            paths += " " + path;
#endif
        }

        args.add(paths);

        if (shouldQuit)
            return true;

#if JUCE_WINDOWS
        auto buildScript = "source " + emsdkPath.replaceCharacter('\\', '/') + "/emsdk_env.sh; " + args.joinIntoString(" ");
#else
        auto buildScript = "source " + emsdkPath + "/emsdk_env.sh; " + args.joinIntoString(" ");
#endif

        Toolchain::startShellScript(buildScript, this);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto outputFile = File(outdir);
        outputFile.getChildFile("c").deleteRecursively();
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        bool generationExitCode = getExitCode();
        return generationExitCode;
    }
};
