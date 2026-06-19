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

void runTests(PluginEditor* editor)
{
    // Need to execute tests on a separate thread, since our tests will block until the message thread has processed every test case
    std::thread testRunnerThread([editor] {
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
        ObjectFuzzTest objectFuzzer(editor);
        HelpFileFuzzTest helpfileFuzzer(editor);
        HelpFileErrorTest helpfileErrorTest(editor);

        UnitTestRunner runner;
        runner.runTests({ &saveCloseRaceTest,
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
                          &uiClickThroughTest,
                          &editActionStressTest,
                          &tabSplitStressTest,
                          /*&helpfileFuzzer,
                          &objectFuzzer,
                          &helpfileErrorTest */}, 23);

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
