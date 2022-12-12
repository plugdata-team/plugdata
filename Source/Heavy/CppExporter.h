/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


class CppExporter : public ExporterBase
{
public:
    CppExporter(PluginEditor* editor, ExportingProgressView* exportingView) : ExporterBase(editor, exportingView) {
    }
    
    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingProgressView::Busy);
        
        StringArray args = {heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir};
        
        args.add("-n" + name);
        
        if(copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }
        
        args.add("-v");
        
        for(auto& path : searchPaths) {
            args.add("-p" + path);
        }
        
        if(shouldQuit) return 1;
        
        start(args.joinIntoString(" "));
        waitForProcessToFinish(-1);
        
        if(shouldQuit) return 1;
        
        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        
        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
        
        return getExitCode();
    }
};
