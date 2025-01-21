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

        auto emsdkPath = getValue<String>(emsdkPathValue);

        args.add("-v");
        args.add("-gjs");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        if (shouldQuit)
            return true;

        auto buildScript = "source " + emsdkPath + "/emsdk_env.sh; " + args.joinIntoString(" ");

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
