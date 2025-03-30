#include "Tests.h"
#include "ObjectFuzzTest.h"
#include "HelpfileFuzzTest.h"
#include "HelpfileErrorTest.h"

void runTests(PluginEditor* editor)
{
    // Make window large for tests that depend on clicking
   // editor->getTopLevelComponent()->getPeer()->setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea, false);

    // Need to execute tests on a separate thread, since our tests will block until the message thread has processed every test case
    std::thread testRunnerThread([editor] {
        ObjectFuzzTest objectFuzzer(editor);
        HelpFileFuzzTest helpfileFuzzer(editor);
        HelpFileErrorTest helpfileErrorTest(editor);
        
        UnitTestRunner runner;
        runner.runTests({&helpfileFuzzer, &objectFuzzer, &helpfileErrorTest}, 23);
    });
    testRunnerThread.detach();
}
