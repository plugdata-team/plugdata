#define PLUGDATA_TEST_TRANSLATION_UNIT 1
#include "Tests.h"
#include "ObjectFuzzTest.h"
#include "HelpfileFuzzTest.h"
#include "HelpfileErrorTest.h"
#include "SaveCloseRaceTest.h"
#include "AbstractionReloadRaceTest.h"
#include "SynchroniseDeletionStressTest.h"
#include "DanglingBindingTest.h"
#include "WeakReferenceLifetimeTest.h"
#include "ScalarTemplateTest.h"
#include "UIClickThroughTest.h"
#include "EditActionStressTest.h"
#include "TabSplitStressTest.h"
#include "PatchTextFuzzTest.h"
#include "DecompressTest.h"
#include "FilesystemInitTest.h"
#include "MarkupDisplayTest.h"
#include "StateRoundTripTest.h"
#include "DialogCoverageTest.h"
#include "ObjectInteractionTest.h"
#include "InspectorTest.h"
#include "TextEditorDialogTest.h"
#include "ExportProgressTest.h"
#include "SmallUITest.h"
#include "OnboardingTest.h"
#include "CommandInputTest.h"
#include "ConsoleHistoryTest.h"
#include "EditWorkflowTest.h"
#include "UndoRedoTest.h"
#include "LuaObjectTest.h"
#include "GuiPropertyTest.h"
#include "StoreDekenTest.h"
#include "DraggableNumberTest.h"
#include "CoverageSweepTest.h"
#include "AddObjectMenuTest.h"
#include "SaveOverwriteTest.h"
#include "SubpatchTabTest.h"
#include "PluginHostTest.h"
#include "BackupLoopTest.h"
#include "StackGuardTest.h"

// Settings come from environment variables, or from TestConfig.json in the app data directory,
// because an AUv3 extension never sees our environment. "<NAME>_<wrapper>" (e.g.
// PLUGDATA_TEST_FILTER_AUv3) takes precedence over "<NAME>" for that plugin format only.
static String getTestSetting(String const& name, String const& wrapperType)
{
    StringArray const keys { name + "_" + wrapperType, name };
    for (auto const& key : keys) {
        auto const value = SystemStats::getEnvironmentVariable(key, {});
        if (value.isNotEmpty())
            return value;
    }

    auto const config = JSON::parse(ProjectInfo::appDataDir.getChildFile("TestConfig.json"));
    for (auto const& key : keys) {
        if (config.hasProperty(key))
            return config[Identifier(key)].toString();
    }
    return {};
}

class PlugDataTestRunner final : public UnitTestRunner {
public:
    explicit PlugDataTestRunner(File logFile)
        : logFile(std::move(logFile))
    {
    }

    void logMessage(String const& message) override
    {
        UnitTestRunner::logMessage(message);
#if JUCE_WINDOWS
        // JUCE logs to OutputDebugString on Windows, which the CI harness can't read
        std::cout << message << std::endl;
#endif
        if (logFile != File())
            logFile.appendText(message + "\n", false, false, "\n");
    }

    void writeSummary(File const& file) const
    {
        int passes = 0, failures = 0;
        Array<var> failed;
        for (int i = 0; i < getNumResults(); i++) {
            auto const* result = getResult(i);
            passes += result->passes;
            failures += result->failures;
            if (result->failures > 0) {
                auto* entry = new DynamicObject();
                entry->setProperty("test", result->unitTestName);
                entry->setProperty("case", result->subcategoryName);
                entry->setProperty("messages", result->messages.joinIntoString("\n"));
                failed.add(var(entry));
            }
        }

        auto* summary = new DynamicObject();
        summary->setProperty("passes", passes);
        summary->setProperty("failures", failures);
        summary->setProperty("failed", failed);
        file.replaceWithText(JSON::toString(var(summary)));
    }

private:
    File logFile;
};

void runTests(PluginEditor* editor)
{
    auto const wrapperType = String(AudioProcessor::getWrapperTypeDescription(editor->pd->wrapperType));

    // Set by host tests that need the plugin's editor open without the suite running in it
    if (getTestSetting("PLUGDATA_TEST_SKIP_SUITE", wrapperType) == "1")
        return;

    // An AUv3 host can create several editors in one extension process: run the suite once
    static std::atomic<bool> alreadyStarted = false;
    if (alreadyStarted.exchange(true))
        return;

    auto const filterSetting = getTestSetting("PLUGDATA_TEST_FILTER", wrapperType);
    auto const seedSetting = getTestSetting("PLUGDATA_TEST_SEED", wrapperType);

    File logFile, summaryFile;
    // Neither iOS nor a plugin in a host has a stdout we can rely on: leave the results where a harness
    // (or the standalone's VST3 Suite Test) can collect them
#if !JUCE_IOS
    if (!ProjectInfo::isStandalone)
#endif
    {
        auto const resultsDir = ProjectInfo::appDataDir.getChildFile("TestResults");
        resultsDir.createDirectory();
        logFile = resultsDir.getChildFile(wrapperType + ".log");
        summaryFile = resultsDir.getChildFile(wrapperType + ".json");
        logFile.deleteFile();
        summaryFile.deleteFile();
    }

    // Need to execute tests on a separate thread, since our tests will block until the message thread has processed every test case
    std::thread testRunnerThread([editor, filterSetting, seedSetting, logFile, summaryFile] {
        // Keep App Nap from throttling the message thread's timers while the window is in the background
        ScopedLowPowerModeDisabler const lowPowerModeDisabler;

        SaveCloseRaceTest saveCloseRaceTest(editor);
        AbstractionReloadRaceTest abstractionReloadRaceTest(editor);
        SynchroniseDeletionStressTest synchroniseDeletionStressTest(editor);
        DanglingBindingTest danglingBindingTest(editor);
        StackGuardTest stackGuardTest(editor);
        WeakReferenceLifetimeTest weakReferenceLifetimeTest(editor);
        ScalarTemplateTest scalarTemplateTest(editor);
        UIClickThroughTest uiClickThroughTest(editor);
        EditActionStressTest editActionStressTest(editor);
        TabSplitStressTest tabSplitStressTest(editor);
        PatchTextFuzzTest patchTextFuzzTest(editor);
        DecompressTest decompressTest(editor);
        FilesystemInitTest filesystemInitTest(editor);
        MarkupDisplayTest markupDisplayTest(editor);
        StateRoundTripTest stateRoundTripTest(editor);
        DialogCoverageTest dialogCoverageTest(editor);
        ObjectInteractionTest objectInteractionTest(editor);
        InspectorTest inspectorTest(editor);
        GuiPropertyTest guiPropertyTest(editor);
        TextEditorDialogTest textEditorDialogTest(editor);
        ExportProgressTest exportProgressTest(editor);
        SmallUITest smallUITest(editor);
        OnboardingTest onboardingTest(editor);
        CommandInputTest commandInputTest(editor);
        ConsoleHistoryTest consoleHistoryTest(editor);
        EditWorkflowTest editWorkflowTest(editor);
        UndoRedoTest undoRedoTest(editor);
        LuaObjectTest luaObjectTest(editor);
        StoreDekenTest storeDekenTest(editor);
        DraggableNumberTest draggableNumberTest(editor);
        CoverageSweepTest coverageSweepTest(editor);
        AddObjectMenuTest addObjectMenuTest(editor);
        SaveOverwriteTest saveOverwriteTest(editor);
        SubpatchTabTest subpatchTabTest(editor);
        ObjectFuzzTest objectFuzzer(editor);
        HelpFileFuzzTest helpfileFuzzer(editor);
        HelpFileErrorTest helpfileErrorTest(editor);

        PlugDataTestRunner runner(logFile);

        Array<UnitTest*> tests = { &saveOverwriteTest,
            &subpatchTabTest,
            &saveCloseRaceTest,
            &abstractionReloadRaceTest,
            &synchroniseDeletionStressTest,
            &danglingBindingTest,
            &stackGuardTest,
            &scalarTemplateTest,
            &decompressTest,
            &filesystemInitTest,
            &markupDisplayTest,
            &stateRoundTripTest,
            &dialogCoverageTest,
            &objectInteractionTest,
            &inspectorTest,
            &guiPropertyTest,
            &textEditorDialogTest,
            &exportProgressTest,
            &smallUITest,
            &onboardingTest,
            &commandInputTest,
            &consoleHistoryTest,
            &editWorkflowTest,
            &undoRedoTest,
            &luaObjectTest,
            &storeDekenTest,
            &draggableNumberTest,
            &addObjectMenuTest,
            &uiClickThroughTest,
            &editActionStressTest,
            &tabSplitStressTest,
            &helpfileFuzzer,
            &objectFuzzer };

        // Only plugin builds use PDINSTANCE. A second processor in the standalone
        // would overwrite its one global Pd instance's callbacks.
        if (!ProjectInfo::isStandalone)
            tests.add(&weakReferenceLifetimeTest);

#ifdef PLUGDATA_TEST_VST3
        VST3ParameterTest vst3ParameterTest;
        VST3StateTest vst3StateTest;
        VST3MultiInstanceTest vst3MultiInstanceTest;
        VST3BackupLoopTest vst3BackupLoopTest;
        VST3SuiteTest vst3SuiteTest;
        // Only the standalone hosts the VST3, unless PLUGDATA_TEST_SKIP_VST3=1. The suite runs last: if the plugin crashes, it takes this process with it
        if (ProjectInfo::isStandalone && getTestSetting("PLUGDATA_TEST_SKIP_VST3", "Standalone") != "1") {
            UnitTest* hostTests[] = { &vst3ParameterTest, &vst3StateTest, &vst3MultiInstanceTest, &vst3BackupLoopTest };
            tests.insertArray(tests.indexOf(&helpfileFuzzer), hostTests, static_cast<int>(std::size(hostTests)));
            tests.add(&vst3SuiteTest);
        }
#endif

        // PLUGDATA_TEST_FILTER: comma-separated, case-insensitive substrings of test names to run.
        // The filter can also select tests that don't run by default.
        auto const filter = StringArray::fromTokens(filterSetting, ",", "");
        if (!filter.isEmpty()) {
            tests.addArray(Array<UnitTest*> { &patchTextFuzzTest, &coverageSweepTest, &helpfileErrorTest });
            tests.removeIf([&filter](UnitTest* test) {
                for (auto const& token : filter) {
                    if (token.trim().isNotEmpty() && test->getName().containsIgnoreCase(token.trim()))
                        return false;
                }
                return true;
            });
        }

        // PLUGDATA_TEST_SEED: run the suite with a different random seed (0 picks a random one)
        runner.runTests(tests, seedSetting.isNotEmpty() ? seedSetting.getLargeIntValue() : 42);

        if (summaryFile != File())
            runner.writeSummary(summaryFile);

        // The suite has finished (whether tests passed or failed). Quit the app
        // so the process exits instead of idling forever - the CI harness waits
        // for the process to terminate and parses stdout for pass/fail.
        MessageManager::callAsync([] {
            if (auto* app = JUCEApplicationBase::getInstance())
                app->quit();
        });
    });
    testRunnerThread.detach();
}
