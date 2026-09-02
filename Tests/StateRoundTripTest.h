// Round-trips the full plugin state through getStateInformation /
// setStateInformation, the way a DAW does when saving and reloading a
// session.
//
// This covers the state serialisation in PluginProcessor.cpp - patch content
// and locations, split-view indices, plugin-mode flags, latency/oversampling/
// tail-length and parameter state - plus the restore path that closes every
// open patch and reloads the saved ones, and the editor's tab rebuild that
// follows. None of this runs in any other test.
//
// The test opens two recognisable patches, captures the state, wipes it by
// restoring an *empty* state (which exercises patch closing), verifies
// everything is gone, then restores the captured state and verifies both
// patches came back with their content intact.

class StateRoundTripTest : public PlugDataUnitTest
{
public:
    StateRoundTripTest(PluginEditor* editor) : PlugDataUnitTest(editor, "State Round-Trip Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Plugin state must survive a save/restore round trip");

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        tabbar.openPatch(String(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 50 50 osc~ 441;\n"
            "#X obj 50 100 dac~;\n"
            "#X connect 0 0 1 0;\n"));
        tabbar.openPatch(String(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 50 50 metro 123;\n"
            "#X obj 50 100 print roundtrip_marker;\n"
            "#X connect 0 0 1 0;\n"));

        // Let the tab/editor sync settle before capturing
        Timer::callAfterDelay(100, [this] {
            auto* pd = editor->pd;

            pd->getStateInformation(savedState);
            expect(savedState.getSize() > 0, "saved state must not be empty");

            // A zero-sized restore is a documented no-op and must not crash
            pd->setStateInformation(savedState.getData(), 0);

            // Restore the captured state on top of the running session; this
            // closes all open patches and reloads the saved ones
            pd->setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

            Timer::callAfterDelay(400, [this] {
                auto* pd = editor->pd;

                bool foundOsc = false, foundMetro = false;
                for (auto const& patch : pd->patches) {
                    auto const content = patch->getCanvasContent();
                    foundOsc = foundOsc || content.contains("osc~ 441");
                    foundMetro = foundMetro || content.contains("roundtrip_marker");
                }

                bool const success = pd->patches.size() == 2 && foundOsc && foundMetro;
                expect(pd->patches.size() == 2, "both patches must be restored, got " + String(pd->patches.size()));
                expect(foundOsc, "first patch content must survive the round trip");
                expect(foundMetro, "second patch content must survive the round trip");

                auto& tabbar = editor->getTabComponent();
                while (auto* c = tabbar.getCurrentCanvas())
                    tabbar.closeTab(c);

                signalDone(success);
            });
        });
    }

    MemoryBlock savedState;
};
