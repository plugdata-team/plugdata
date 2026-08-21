#include "Components/PropertiesPanel.h"
#include "Utility/OSUtils.h"
#include "Dialogs/Dialogs.h"
#include "Heavy/ExportingProgressView.h"
#include "Heavy/ToolchainInstaller.h"
#include "Heavy/ExporterBase.h"
#include "Heavy/CppExporter.h"

// End-to-end test of the Heavy (hvcc) C/C++ source export.
//
// Two parts:
//
//  1. Fast, dependency-free coverage of ExportingProgressView: every export
//     state is shown and painted, and the console is fed ANSI-coloured /
//     word-wrapped output like the real compiler emits.
//
//  2. A real export pipeline run:
//       - ensure the Heavy toolchain is installed: if it's already present we
//         use it, otherwise we drive the real ToolchainInstaller to download
//         and unpack it (this is the ~hundreds-of-MB download, so it only
//         happens on a machine/CI without a cached toolchain; a network
//         failure soft-passes rather than failing the suite).
//       - run CppExporter on a minimal Heavy-compatible patch
//         ([osc~]->[*~]->[dac~]) into a temp directory
//       - wait for the export thread to reach Success/Failure
//       - verify the toolchain actually produced C/C++ source in the temp dir
//
// The generated output and the temp patch are cleaned up; the downloaded
// toolchain is left in place (re-downloading it every run would be absurd).

class ExportProgressTest : public PlugDataUnitTest
{
public:
    ExportProgressTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Heavy Export Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Export progress view states and console log");

        view = std::make_unique<ExportingProgressView>();
        editor->addAndMakeVisible(view.get());
        view->setBounds(0, 0, 600, 400);

        view->showState(ExportingProgressView::Exporting);
        view->logToConsole("Starting export...\n");
        view->logToConsole("\x1b[31merror:\x1b[0m red\n\x1b[1;32mok:\x1b[0m bold green\n");
        view->logToConsole(String::repeatedString("a long word-wrapped console line ", 40) + "\n");
        for (auto const state : { ExportingProgressView::Flashing, ExportingProgressView::Success, ExportingProgressView::Failure }) {
            view->showState(state);
            view->createComponentSnapshot(view->getLocalBounds());
        }
        view->showState(ExportingProgressView::NotExporting);

        // Now the real pipeline
        ensureToolchainThenExport();
    }

    void ensureToolchainThenExport()
    {
        if (ExporterBase::heavyExecutable.existsAsFile()) {
            logMessage("Heavy toolchain already installed at " + ExporterBase::heavyExecutable.getFullPathName() + " - skipping download");
            runExport();
            return;
        }

        beginTest("Download and install the Heavy toolchain");

        // Drive the production installer. It downloads the platform toolchain
        // archive and unpacks it via Decompress::extractTarXz.
        installerDialogOwner.reset(new Dialog(&installerDialogOwner, editor, 600, 400, false));
        installer = std::make_unique<ToolchainInstaller>(editor, installerDialogOwner.get());
        installer->toolchainInstalledCallback = [this] {
            expect(ExporterBase::heavyExecutable.existsAsFile(), "installer must produce the Heavy executable");
            runExport();
        };

        installer->installButton.onClick();

        // Poll for completion (success sets the callback; failure sets an error
        // message). Generous cap; soft-pass on network failure/timeout.
        pollToolchainInstall(0);
    }

    void pollToolchainInstall(int const attempt)
    {
        // ~5 minutes max (3s * 100); the callback short-circuits on success
        constexpr int maxAttempts = 100;

        if (ExporterBase::heavyExecutable.existsAsFile())
            return; // the installed callback handles the rest

        if (installer && installer->errorMessage.isNotEmpty()) {
            logMessage("Toolchain download unavailable (" + installer->errorMessage + ") - soft-passing the export test");
            finishSoft();
            return;
        }

        if (attempt >= maxAttempts) {
            logMessage("Toolchain download timed out - soft-passing the export test");
            finishSoft();
            return;
        }

        Timer::callAfterDelay(3000, [this, attempt] { pollToolchainInstall(attempt + 1); });
    }

    void runExport()
    {
        beginTest("Export a basic patch to C/C++ source");

        // A minimal Heavy-compatible patch: sine -> gain -> output
        outputDir = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_heavy_export_test");
        outputDir.deleteRecursively();
        outputDir.createDirectory();

        patchFile = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_heavy_test_patch.pd");
        patchFile.replaceWithText(
            "#N canvas 0 0 450 300 12;\n"
            "#X obj 80 80 osc~ 440;\n"
            "#X obj 80 140 *~ 0.2;\n"
            "#X obj 80 200 dac~;\n"
            "#X connect 0 0 1 0;\n"
            "#X connect 1 0 2 0;\n"
            "#X connect 1 0 2 1;\n");

        exporter = std::make_unique<CppExporter>(editor, view.get());
        exporter->patchFile = patchFile;
        exporter->realPatchFile = patchFile;
        exporter->projectNameValue = "heavytest";

        view->showState(ExportingProgressView::Exporting);
        exporter->startExport(outputDir);

        // startExport runs the compiler on a pool thread; poll the view state
        pollExport(0);
    }

    void pollExport(int const attempt)
    {
        constexpr int maxAttempts = 80; // ~80s; compiling a tiny patch is quick

        auto const state = view->state.load();
        bool const done = state == ExportingProgressView::Success || state == ExportingProgressView::Failure;

        if (done) {
            expect(state == ExportingProgressView::Success, "the Heavy export must succeed");

            // The toolchain writes generated C/C++ source under the output dir
            // (hvcc puts the C generator output in a subfolder)
            auto const sources = outputDir.findChildFiles(File::findFiles, true, "*.c;*.cpp;*.h;*.hpp");
            expect(sources.size() > 0, "the export must produce C/C++ source files, found " + String(sources.size()));
            if (!sources.isEmpty())
                logMessage("Heavy export produced " + String(sources.size()) + " source files, e.g. " + sources[0].getFileName());

            finishHard(state == ExportingProgressView::Success && !sources.isEmpty());
            return;
        }

        if (attempt >= maxAttempts) {
            expect(false, "the Heavy export did not finish in time");
            finishHard(false);
            return;
        }

        Timer::callAfterDelay(1000, [this, attempt] { pollExport(attempt + 1); });
    }

    // Tear down everything that may still have async callbacks queued, then
    // signal. Done on a delay so any in-flight ExportingProgressView callbacks
    // drain against a live object first.
    void teardown(std::function<void()> const& then)
    {
        if (exporter) {
            exporter->shouldQuit = true;
            exporter.reset(); // ThreadPool dtor joins the export job
        }
        installer.reset();
        installerDialogOwner.reset();

        Timer::callAfterDelay(100, [this, then] {
            if (view) {
                editor->removeChildComponent(view.get());
                view.reset();
            }
            patchFile.deleteFile();
            outputDir.deleteRecursively();
            then();
        });
    }

    void finishHard(bool const success) { teardown([this, success] { signalDone(success); }); }
    void finishSoft() { teardown([this] { signalDone(true); }); }

    std::unique_ptr<ExportingProgressView> view;
    std::unique_ptr<CppExporter> exporter;
    std::unique_ptr<ToolchainInstaller> installer;
    std::unique_ptr<Dialog> installerDialogOwner;
    File patchFile, outputDir;
};
