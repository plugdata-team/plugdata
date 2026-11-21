// When the host doesn't call processBlock, PluginProcessor::runBackupLoop keeps Pd's scheduler running
// while the editor is open. These tests host the plugdata VST3 (see PluginHostTest.h) and starve it of audio.

#ifdef PLUGDATA_TEST_VST3

namespace BackupLoopPatches {

// [param in] reaches [param out] (both range 0-100) through a [pipe], which needs Pd's clock to run
inline String const delayedEcho = "#N canvas 0 50 450 300 12;\n"
                                  "#X obj 20 20 loadbang;\n"
                                  "#X obj 20 50 t b b;\n"
                                  "#X msg 20 80 create 0 0 100;\n"
                                  "#X msg 180 80 create 0 0 100;\n"
                                  "#X obj 20 120 param in;\n"
                                  "#X obj 180 120 param out;\n"
                                  "#X obj 20 160 pipe 50;\n"
                                  "#X connect 0 0 1 0;\n"
                                  "#X connect 1 0 2 0;\n"
                                  "#X connect 1 1 3 0;\n"
                                  "#X connect 2 0 4 0;\n"
                                  "#X connect 3 0 5 0;\n"
                                  "#X connect 4 0 6 0;\n"
                                  "#X connect 6 0 5 0;\n";

// Setting [param start] to 1 reports the wall-clock time that 2000 ms of Pd time took in [param elapsed] (range 0-10000)
inline String const clock = "#N canvas 0 50 450 300 12;\n"
                            "#X obj 20 20 loadbang;\n"
                            "#X obj 20 50 t b b;\n"
                            "#X msg 20 80 create 0 0 1;\n"
                            "#X msg 180 80 create 0 0 10000;\n"
                            "#X obj 20 120 param start;\n"
                            "#X obj 180 120 param elapsed;\n"
                            "#X obj 20 160 sel 1;\n"
                            "#X obj 20 190 t b b;\n"
                            "#X obj 20 220 del 2000;\n"
                            "#X obj 20 250 realtime;\n"
                            "#X connect 0 0 1 0;\n"
                            "#X connect 1 0 2 0;\n"
                            "#X connect 1 1 3 0;\n"
                            "#X connect 2 0 4 0;\n"
                            "#X connect 3 0 5 0;\n"
                            "#X connect 4 0 6 0;\n"
                            "#X connect 6 0 7 0;\n"
                            "#X connect 7 0 8 0;\n"
                            "#X connect 7 1 9 0;\n"
                            "#X connect 8 0 9 1;\n"
                            "#X connect 9 0 5 0;\n";

// A note to the DAW every 10 ms
inline String const notes = "#N canvas 0 50 450 300 12;\n"
                            "#X obj 20 20 loadbang;\n"
                            "#X obj 20 50 metro 10;\n"
                            "#X msg 20 80 60 100;\n"
                            "#X obj 20 110 noteout 1;\n"
                            "#X connect 0 0 1 0;\n"
                            "#X connect 1 0 2 0;\n"
                            "#X connect 2 0 3 0;\n";

// Setting [param play] (range 0-1) to 1 or 0 sends note 60 on or off to the DAW
inline String const gate = "#N canvas 0 50 450 300 12;\n"
                           "#X obj 20 20 loadbang;\n"
                           "#X msg 20 50 create 0 0 1;\n"
                           "#X obj 20 80 param play;\n"
                           "#X obj 20 110 sel 1 0;\n"
                           "#X msg 20 140 60 100;\n"
                           "#X msg 100 140 60 0;\n"
                           "#X obj 20 170 noteout 1;\n"
                           "#X connect 0 0 1 0;\n"
                           "#X connect 1 0 2 0;\n"
                           "#X connect 2 0 3 0;\n"
                           "#X connect 3 0 4 0;\n"
                           "#X connect 3 1 5 0;\n"
                           "#X connect 4 0 6 0;\n"
                           "#X connect 5 0 6 0;\n";

} // namespace BackupLoopPatches

class VST3BackupLoopTest final : public UnitTest {
public:
    VST3BackupLoopTest()
        : UnitTest("VST3 Backup Loop Test")
    {
    }

    void runTest() override
    {
        // Must outlive every plugin below, or a late editor could still start the suite inside the plugin
        ScopedSuiteSkip const skipSuite;

        beginTest("Messages run before the host prepares the plugin");
        String error;
        auto plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->loadPatch(BackupLoopPatches::delayedEcho);
        plugin->showEditor();
        auto* in = plugin->waitForParameter("in");
        auto* out = plugin->waitForParameter("out");
        expect(in && out, "The host doesn't see the parameters the patch created");
        if (!in || !out)
            return;

        in->setValueNotifyingHost(0.42f);
        expect(HostedPlugin::waitForValue(out, 0.42f), "The patch didn't run without audio");

        beginTest("Messages run after the host stops processing");
        plugin->startAudio();
        in->setValueNotifyingHost(0.17f);
        expect(HostedPlugin::waitForValue(out, 0.17f), "The patch didn't respond while processing");
        plugin->pauseAudio();
        in->setValueNotifyingHost(0.66f);
        expect(HostedPlugin::waitForValue(out, 0.66f), "The patch stopped when the host stopped calling processBlock");

        beginTest("Messages run after the host releases the plugin");
        plugin->startAudio();
        plugin->stopAudio();
        in->setValueNotifyingHost(0.3f);
        expect(HostedPlugin::waitForValue(out, 0.3f), "The patch stopped when the host released the plugin");

        // Saves CPU in sessions with many plugdata instances that the host doesn't process
        beginTest("Without an editor, the patch waits for audio");
        plugin->closeEditor();
        in->setValueNotifyingHost(0.8f);
        Thread::sleep(500);
        expectWithinAbsoluteError(out->getValue(), 0.3f, 0.001f, "The patch ran without an editor or audio");
        plugin->showEditor();
        expect(HostedPlugin::waitForValue(out, 0.8f), "The patch didn't resume when the editor opened");

        beginTest("Pd's clock keeps time without audio");
        plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->prepare();
        plugin->loadPatch(BackupLoopPatches::clock);
        plugin->showEditor();
        expectKeepsTime(*plugin, "without audio");

        beginTest("Pd's clock keeps time without audio while oversampling");
        plugin->loadPatch(BackupLoopPatches::clock, 1);
        expectKeepsTime(*plugin, "without audio, 2x oversampled");
        // The plugin also saves oversampling in its settings, which later instances start with
        plugin->loadPatch(BackupLoopPatches::clock);

        beginTest("Audio takes over from the backup loop, and hands back to it");
        plugin->resumeAudio();
        expectKeepsTime(*plugin, "with audio");
        plugin->pauseAudio();
        expectKeepsTime(*plugin, "after audio stopped");

        beginTest("MIDI for the DAW doesn't pile up without audio");
        plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->prepare();
        plugin->loadPatch(BackupLoopPatches::notes);
        plugin->showEditor();
        Thread::sleep(1000);

        auto const firstBlock = plugin->processBlock();
        expectLessOrEqual(firstBlock.getNumEvents(), 4, "The first block after a silent second carried the notes from that second");

        int numEvents = 0;
        for (int i = 0; i < 40; i++)
            numEvents += plugin->processBlock().getNumEvents();
        expectGreaterOrEqual(numEvents, 15, "40 blocks (213 ms) should carry about 21 notes");

        beginTest("Notes the DAW received are released when it resumes");
        plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->prepare();
        plugin->loadPatch(BackupLoopPatches::gate);
        plugin->showEditor();
        auto* play = plugin->waitForParameter("play");
        expect(play != nullptr, "The host doesn't see the parameter the patch created");
        if (!play)
            return;

        play->setValueNotifyingHost(1.0f);
        expect(HostedPlugin::waitUntil([&plugin] { return countNotes(plugin->processBlock(), true) > 0; }, 2000), "The note-on didn't reach the DAW");

        // The host stops calling processBlock. The patch plays the note again and releases it: only the release may reach the DAW
        Thread::sleep(300);
        play->setValueNotifyingHost(0.0f);
        Thread::sleep(200);
        play->setValueNotifyingHost(1.0f);
        Thread::sleep(200);
        play->setValueNotifyingHost(0.0f);
        Thread::sleep(200);

        auto const resumed = plugin->processBlock();
        expectEquals(countNotes(resumed, false), 1, "The first block after the pause must release the note once");
        expectEquals(countNotes(resumed, true), 0, "A note-on from while the DAW wasn't processing arrived late");
        expectEquals(countNotes(plugin->processBlock(), false), 0, "The held note-off was sent again");
    }

private:
    // Note 60 on channel 1, the gate patch's note
    static int countNotes(MidiBuffer const& midi, bool const noteOn)
    {
        int count = 0;
        for (auto const metadata : midi) {
            auto const message = metadata.getMessage();
            if (message.isNoteOnOrOff() && message.getChannel() == 1 && message.getNoteNumber() == 60 && message.isNoteOn() == noteOn)
                count++;
        }
        return count;
    }

    // Opening the VST3's editor starts the test suite inside it (see runTests), unless this is set
    struct ScopedSuiteSkip {
        ScopedSuiteSkip() { set(true); }
        ~ScopedSuiteSkip() { set(false); }

        static void set(bool const skip)
        {
            MessageManager::callSync([skip] {
#if JUCE_WINDOWS
                _putenv_s("PLUGDATA_TEST_SKIP_SUITE_VST3", skip ? "1" : "");
#else
                if (skip)
                    setenv("PLUGDATA_TEST_SKIP_SUITE_VST3", "1", 1);
                else
                    unsetenv("PLUGDATA_TEST_SKIP_SUITE_VST3");
#endif
            });
        }
    };

    // The clock patch measures the wall-clock time of 2000 ms of Pd time
    void expectKeepsTime(HostedPlugin& plugin, String const& context)
    {
        auto* start = plugin.waitForParameter("start");
        auto* elapsed = plugin.waitForParameter("elapsed");
        if (!start || !elapsed) {
            expect(false, "The host doesn't see the parameters the patch created");
            return;
        }

        start->setValueNotifyingHost(0.0f);
        elapsed->setValueNotifyingHost(0.0f);
        Thread::sleep(200);
        start->setValueNotifyingHost(1.0f);

        if (!HostedPlugin::waitUntil([elapsed] { return elapsed->getValue() > 0.0f; }, 10000)) {
            expect(false, "Pd's clock didn't reach 2000 ms " + context);
            return;
        }

        auto const wallClockMs = roundToInt(elapsed->getValue() * 10000.0f);
        logMessage("2000 ms of Pd time took " + String(wallClockMs) + " ms " + context);
        expect(wallClockMs > 1800 && wallClockMs < 2300, "2000 ms of Pd time took " + String(wallClockMs) + " ms " + context);
    }
};

#endif
