#pragma once

// Covers the configurable console scrollback: Instance must trim its message
// store to the "console_history_size" setting instead of a hard-coded 800.
class ConsoleHistoryTest : public PlugDataUnitTest
{
public:
    ConsoleHistoryTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Console History Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Console history size");

        auto* pd = editor->pd;
        auto* settings = SettingsFile::getInstance();
        expect(settings->hasProperty("console_history_size"), "console history size setting must exist");

        auto const originalSize = settings->getProperty<int>("console_history_size");

        settings->setProperty("console_history_size", 5);
        for (int i = 0; i < 20; i++)
            pd->logMessage("console history test " + String(i));
        pd->flushConsoleMessages();

        expectEquals(static_cast<int>(pd->getConsoleMessages().size()), 5,
            "console messages must be trimmed to the configured history size");

        settings->setProperty("console_history_size", originalSize);
        pd->getConsoleMessages().clear();

        signalDone(true);
    }
};
