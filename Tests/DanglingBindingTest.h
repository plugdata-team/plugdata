// Regression test for a use-after-free on the "#A" binding.
//
// Crash signature (ASAN, found by HelpFileFuzzTest): heap-buffer-overflow /
// use-after-free READ in pd_findbyclass, called from textbuf_free (or
// garray_free, or Gem's CPPExtern destructor) while closing a patch.
//
// Mechanism:
// 1. Several things bind themselves to "#A" to receive saved state while
//    loading: abstraction canvases (canvas_popabstraction), [scalar define],
//    [text define], garrays, and [clone] (which bashes #A->s_thing directly).
// 2. Nothing ever unbinds them. Vanilla pd gets away with this because file
//    load, paste and undo wrap evaluation in a save/zero/restore of
//    #A->s_thing, discarding whatever binding the eval left behind.
// 3. Objects created *outside* those wrappers - anything made interactively
//    through pd::Interface::createObject, or by dynamic patching at runtime -
//    leave #A bound to themselves indefinitely. Deleting such an object (or
//    closing its patch) frees it while #A->s_thing still points at it.
// 4. The next pd_findbyclass(gensym("#A"), ...) - e.g. the "just in case"
//    cleanup loop in textbuf_free when any [text]/[qlist]/[textfile] is
//    freed - dereferences the stale pointer.
//
// The fix unbinds the dying object from #A in pd_free().
//
// This test creates and deletes each kind of #A-binding object through the
// normal object creation/deletion path (no artificial bindings), then
// creates and deletes a [textfile] to force the pd_findbyclass(#A) walk.
// Run with AddressSanitizer to catch the read of freed memory directly; even
// without ASAN, the test fails pre-fix because #A is still bound to a freed
// object after each deletion.

class DanglingBindingTest : public PlugDataUnitTest
{
public:
    DanglingBindingTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Dangling #A Binding Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Deleting objects bound to #A must not leave a dangling binding");

        auto const dir = File::getSpecialLocation(File::tempDirectory);

        // A trivial abstraction, so we can instantiate it (and [clone] it)
        // from a patch in the same directory
        abstractionFile = dir.getChildFile("dangling_a_abs.pd");
        abstractionFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 40 40 +~;\n");

        patchFile = dir.getChildFile("dangling_a_patch.pd");
        patchFile.replaceWithText("#N canvas 100 100 450 300 12;\n");

        auto* pd = editor->pd;
        auto& tabbar = editor->getTabComponent();
        auto* cnv = tabbar.openPatch(pd->loadPatch(URL(patchFile)));
        if (!cnv) {
            signalDone(false);
            return;
        }
        auto patch = cnv->refCountedPatch;

        bool allClean = true;

        // Each of these binds itself (or its canvas) to #A when created, and
        // pre-fix was freed without ever unbinding:
        //  - [scalar define]: binds its own canvas, free method is plain canvas_free
        //  - an abstraction instance: bound by canvas_popabstraction
        //  - [clone]: bashes #A->s_thing to itself at the end of clone_new
        for (auto* binder : { "scalar define", "dangling_a_abs", "clone dangling_a_abs 2" }) {
            bool const created = createAndDelete(patch, binder);
            expect(created, String("could not create and delete [") + binder + "]");

            // Pre-fix this read freed memory (caught by ASAN); post-fix the
            // binding was removed in pd_free and the loop finds nothing
            bool const walked = createAndDelete(patch, "textfile");
            expect(walked, "could not create and delete [textfile]");

            pd->lockAudioThread();
            pd->setThis();
            bool const clean = gensym("#A")->s_thing == nullptr;
            pd->unlockAudioThread();
            expect(clean, String("#A still bound to freed object after deleting [") + binder + "]");

            allClean = allClean && created && walked && clean;
        }

        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        abstractionFile.deleteFile();
        patchFile.deleteFile();
        signalDone(allClean);
    }

    bool createAndDelete(pd::Patch::Ptr const& patch, String const& name)
    {
        // Patch::createObject/removeObjects take the pd lock internally, the
        // same path the editor uses when the user types or deletes an object
        t_gobj* obj = patch->createObject(50, 50, name);
        if (!obj)
            return false;
        patch->removeObjects({ obj });
        return true;
    }

    File abstractionFile, patchFile;
};
