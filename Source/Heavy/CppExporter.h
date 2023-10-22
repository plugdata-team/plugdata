/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CppExporter : public ExporterBase {
public:
    CppExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
    }
    
    ValueTree getState() override
    {
        ValueTree stateTree("CPP");
        stateTree.setProperty("inputPatchValue", getValue<String>(inputPatchValue), nullptr);
        stateTree.setProperty("projectNameValue", getValue<String>(projectNameValue), nullptr);
        stateTree.setProperty("projectCopyrightValue", getValue<String>(projectCopyrightValue), nullptr);
        return stateTree;
    }
    
    void setState(ValueTree& stateTree) override
    {
        auto tree = stateTree.getChildWithName("CPP");
        inputPatchValue = tree.getProperty("inputPatchValue");
        projectNameValue = tree.getProperty("projectNameValue");
        projectCopyrightValue = tree.getProperty("projectCopyrightValue");
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Busy);

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        name = name.replaceCharacter('-', '_');
        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        args.add("-v");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        if (shouldQuit)
            return true;

        start(args.joinIntoString(" "));

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        return getExitCode();
    }
};
