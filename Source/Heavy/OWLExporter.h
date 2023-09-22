/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class OWLExporter : public ExporterBase {
public:
    // Value targetBoardValue = Value(var(1));
    Value exportTypeValue = Value(var(3));

    File customBoardDefinition;

    TextButton flashButton = TextButton("Flash");
    PropertiesPanel::Property* usbMidiProperty;

    OWLExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        Array<PropertiesPanel::Property*> properties;
        // properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "OWL1", "OWL2", "OWL3" }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, { "Source code", "Binary", "Flash" }));


        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("OWL", properties);

        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);

        flashButton.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));

        exportTypeValue.addListener(this);
        // targetBoardValue.addListener(this);

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
    }

    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        // auto target = getValue<int>(targetBoardValue) - 1;
        bool compile = getValue<int>(exportTypeValue) - 1;
        bool flash = getValue<int>(exportTypeValue) == 3;

        StringArray args = { heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir };

        name = name.replaceCharacter('-', '_');
        args.add("-n owl");

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }

        args.add("-v");
        args.add("-gOWL");

        String paths = "-p";
        for (auto& path : searchPaths) {
            paths += " " + path;
        }

        args.add(paths);

        start(args.joinIntoString(" "));
        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        exportingView->logToConsole("Compiling...\n");

        if (shouldQuit)
            return true;

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        auto outputFile = File(outdir);
        auto sourceDir = outputFile.getChildFile("Source");

        bool heavyExitCode = getExitCode();

        if (compile) {

            auto bin = Toolchain::dir.getChildFile("bin");
            auto OWL = Toolchain::dir.getChildFile("lib").getChildFile("OwlProgram");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto compiler = bin.getChildFile("arm-none-eabi-gcc" + exeSuffix);

            OWL.copyDirectoryTo(outputFile.getChildFile("OwlProgram"));

            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();

            auto workingDir = File::getCurrentWorkingDirectory();

            auto OwlDir = outputFile.getChildFile("OwlProgram");
            OwlDir.setAsCurrentWorkingDirectory();
            OwlDir.getChildFile("Tools/FirmwareSender").setExecutePermission(1);

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
                + " -j4"
                + " TOOLROOT=" + gccPath + "/"
                + " BUILD=../"
                + " PATCHNAME=" + name
                + " PATCHCLASS=HeavyPatch"
                + " PATCHFILE=HeavyOWL_owl.hpp"
                + " PLATFORM=OWL2"
                + " load";

            Toolchain::startShellScript(buildScript, this);
#endif

            waitForProcessToFinish(-1);
            exportingView->flushConsole();

            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();

            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

            auto compileExitCode = getExitCode();
//             if (flash && !compileExitCode) {

//                 auto dfuUtil = bin.getChildFile("dfu-util" + exeSuffix);


//                 exportingView->logToConsole("Flashing...\n");

// #if JUCE_WINDOWS
//                 String flashScript = "export PATH=\"" + bin.getFullPathName().replaceCharacter('\\', '/') + ":$PATH\"\n"
//                     + "cd " + sourceDir.getFullPathName().replaceCharacter('\\', '/') + "\n"
//                     + make.getFullPathName().replaceCharacter('\\', '/') + " program-dfu"
//                     + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/')
//                     + " PROJECT_NAME=" + name;
// #else
//                 String flashScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
//                     + "cd " + sourceDir.getFullPathName() + "\n"
//                     + make.getFullPathName() + " program-dfu"
//                     + " GCC_PATH=" + gccPath
//                     + " PROJECT_NAME=" + name;
// #endif

//                 Toolchain::startShellScript(flashScript, this);

//                 waitForProcessToFinish(-1);
//                 exportingView->flushConsole();

//                 // Delay to get correct exit code
//                 Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

//                 auto flashExitCode = getExitCode();

//                 return heavyExitCode && flashExitCode;
//             } else {
//                 auto binLocation = outputFile.getChildFile(name + ".bin");
//                 sourceDir.getChildFile("build").getChildFile("HeavyOWL_" + name + ".bin").moveFileTo(binLocation);
//             }

            // outputFile.getChildFile("OWL").deleteRecursively();
            // outputFile.getChildFile("libOWL").deleteRecursively();

            return heavyExitCode && compileExitCode;
        } else {
            auto outputFile = File(outdir);

            auto libOWL = Toolchain::dir.getChildFile("lib").getChildFile("libOWL");
            libOWL.copyDirectoryTo(outputFile.getChildFile("libOWL"));

            // outputFile.getChildFile("ir").deleteRecursively();
            // outputFile.getChildFile("hv").deleteRecursively();
            // outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }
};
