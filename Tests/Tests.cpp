#define PLUGDATA_TEST_TRANSLATION_UNIT 1
#include "Tests.h"
#include "ObjectFuzzTest.h"
#include "HelpfileFuzzTest.h"
#include "HelpfileErrorTest.h"
#include "SaveCloseRaceTest.h"
#include "AbstractionReloadRaceTest.h"
#include "DanglingBindingTest.h"
#include "UIClickThroughTest.h"
#include "EditActionStressTest.h"
#include "TabSplitStressTest.h"
#include "PatchTextFuzzTest.h"
#include "DecompressTest.h"
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
#include "EditWorkflowTest.h"
#include "LuaObjectTest.h"
#include "StoreDekenTest.h"
#include "DraggableNumberTest.h"
#include "CoverageSweepTest.h"
#include "AddObjectMenuTest.h"

void runTests(PluginEditor* editor)
{
    // Need to execute tests on a separate thread, since our tests will block until the message thread has processed every test case
    std::thread testRunnerThread([editor] {
        // Keep App Nap from throttling the message thread's timers while the window is in the background
        ScopedLowPowerModeDisabler const lowPowerModeDisabler;

        SaveCloseRaceTest saveCloseRaceTest(editor);
        AbstractionReloadRaceTest abstractionReloadRaceTest(editor);
        DanglingBindingTest danglingBindingTest(editor);
        UIClickThroughTest uiClickThroughTest(editor);
        EditActionStressTest editActionStressTest(editor);
        TabSplitStressTest tabSplitStressTest(editor);
        PatchTextFuzzTest patchTextFuzzTest(editor);
        DecompressTest decompressTest(editor);
        MarkupDisplayTest markupDisplayTest(editor);
        StateRoundTripTest stateRoundTripTest(editor);
        DialogCoverageTest dialogCoverageTest(editor);
        ObjectInteractionTest objectInteractionTest(editor);
        InspectorTest inspectorTest(editor);
        TextEditorDialogTest textEditorDialogTest(editor);
        ExportProgressTest exportProgressTest(editor);
        SmallUITest smallUITest(editor);
        OnboardingTest onboardingTest(editor);
        CommandInputTest commandInputTest(editor);
        EditWorkflowTest editWorkflowTest(editor);
        LuaObjectTest luaObjectTest(editor);
        StoreDekenTest storeDekenTest(editor);
        DraggableNumberTest draggableNumberTest(editor);
        CoverageSweepTest coverageSweepTest(editor);
        AddObjectMenuTest addObjectMenuTest(editor);
        ObjectFuzzTest objectFuzzer(editor);
        HelpFileFuzzTest helpfileFuzzer(editor);
        HelpFileErrorTest helpfileErrorTest(editor);

        UnitTestRunner runner;

        Array<UnitTest*> tests = { &saveCloseRaceTest,
            &abstractionReloadRaceTest,
            &danglingBindingTest,
            &decompressTest,
            &markupDisplayTest,
            &stateRoundTripTest,
            &dialogCoverageTest,
            &objectInteractionTest,
            &inspectorTest,
            &textEditorDialogTest,
            &exportProgressTest,
            &smallUITest,
            &onboardingTest,
            &commandInputTest,
            &editWorkflowTest,
            &luaObjectTest,
            &storeDekenTest,
            &draggableNumberTest,
            &addObjectMenuTest,
            &uiClickThroughTest,
            &editActionStressTest,
            &tabSplitStressTest,
            &helpfileFuzzer,
            &objectFuzzer,
            &helpfileErrorTest };

        // PLUGDATA_TEST_FILTER: comma-separated, case-insensitive substrings of test names to run.
        // The filter can also select tests that don't run by default.
        auto const filter = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_TEST_FILTER", {}), ",", "");
        if (!filter.isEmpty()) {
            tests.addArray(Array<UnitTest*> { &patchTextFuzzTest, &coverageSweepTest });
            tests.removeIf([&filter](UnitTest* test) {
                for (auto const& token : filter) {
                    if (token.trim().isNotEmpty() && test->getName().containsIgnoreCase(token.trim()))
                        return false;
                }
                return true;
            });
        }

        // PLUGDATA_TEST_SEED: run the suite with a different random seed (0 picks a random one)
        auto const seedString = SystemStats::getEnvironmentVariable("PLUGDATA_TEST_SEED", {});
        runner.runTests(tests, seedString.isNotEmpty() ? seedString.getLargeIntValue() : 42);

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
