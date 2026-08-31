// Regression test for a use-after-free in Patch::savePatch()
// (crashed in v0.8.3, fixed on master by 0bb4a1114).
//
// v0.8.3 queued this from savePatch():
//
//     MessageManager::callAsync([this, patch = ptr.getRaw<t_glist>()]() {
//         sys_lock();
//         instance->reloadAbstractions(currentFile, patch);
//         sys_unlock();
//     });
//
// If the pd::Patch was destroyed before the message queue delivered that
// lambda (save-then-close, or a reload recreating the patch), the lambda ran
// on a dangling `this` and a freed t_glist. Because reloadAbstractions()
// takes its File argument by value, the call site copies this->currentFile:
// JUCE's String copy bumps a refcount stored at (text - 16). With the freed
// Patch memory reused/zeroed, text == nullptr, producing the signature crash
// at KERN_INVALID_ADDRESS 0xfffffffffffffff0 inside
// pd::Patch::savePatch()::$_0 on the JUCE Message Thread.
//
// This test forces that exact interleaving. On v0.8.3 it segfaults; with the
// weak-reference captures on master the queued lambdas must no-op safely.
// Run with AddressSanitizer to catch the UAF even if freed memory is benign.

class SaveCloseRaceTest : public PlugDataUnitTest
{
public:
    SaveCloseRaceTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Save/Close Race Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Patch destroyed between savePatch() and async reload delivery");

        // A minimal valid patch on disk, so the patch has a currentFile to save to
        patchFile = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_save_race_test.pd");
        patchFile.replaceWithText("#N canvas 100 100 450 300 12;\n#X obj 50 50 metro 500;\n");

        auto& tabbar = editor->getTabComponent();
        tabbar.openPatch(URL(patchFile));

        auto* cnv = tabbar.getCurrentCanvas();
        pd::Patch::Ptr patch = cnv->refCountedPatch;

        // 1) Queue the async abstraction-reload that savePatch() schedules
        //    internally. Twice, so a second lambda is still pending while the
        //    first one runs reloadAbstractions().
        patch->savePatch();
        patch->savePatch();

        // 2) Destroy the patch *before* the message queue delivers those
        //    lambdas. closeTab() releases the Canvas' reference and removes
        //    the patch from pd->patches; dropping our reference makes the
        //    refcount hit zero: ~Patch() -> libpd_closefile() also frees the
        //    underlying t_glist.
        tabbar.closeTab(cnv);
        patch = nullptr;

        // 3) The two save lambdas are still queued and will be delivered
        //    after this callback returns, before the lambda below (FIFO).
        //    Surviving their delivery is the actual test.
        MessageManager::callAsync([this] {
            patchFile.deleteFile();
            signalDone(true);
        });
    }

    File patchFile;
};
