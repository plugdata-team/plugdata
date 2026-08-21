// Regression test for a use-after-free in Canvas::performSynchronise().
//
// Crash signature (v0.9.2, macOS): SIGABRT "pointer being freed was not
// allocated" with this stack:
//
//     juce::Component::~Component()            <- free(childComponentList)
//     Canvas::performSynchronise()             <- "remove deleted objects" loop
//     PluginProcessor::reloadAbstractions()
//     pd::Patch::savePatch()::$_0              <- async reload after save
//
// Mechanism:
// 1. Saving an abstraction queues reloadAbstractions(), and canvas_reload()
//    deletes + recreates every instance of that abstraction. A tab showing the
//    *inside* of an old instance is left wrapping a dead t_glist, and every
//    Object on that canvas has a dead pd pointer.
// 2. reloadAbstractions() synchronises all canvases. On the dead canvas,
//    performSynchronise() destroys the dead Objects inline.
// 3. If one of them is a subpatch, ~SubpatchObject() calls
//    closeOpenedSubpatchers(), which finds the tab to close by comparing
//    pd::Patch::operator==. That compares getRawPointer(), which returns
//    nullptr for *every* dead patch, so nullptr == nullptr makes the dying
//    subpatch match the first canvas whose patch is dead: the canvas that is
//    currently mid-performSynchronise. closeTab() then deletes that Canvas
//    synchronously, destroying all of its pool-allocated Objects (including
//    the one whose destructor we are inside of), and the loop resumes on a
//    freed `this`, re-destroying already-destroyed Objects.
//
// The arrangement below makes step 3 deterministic: the abstraction instance
// sits inside [pd holder], and the holder view is never opened, so the dying
// instance object is not on any open canvas. (If it were, that canvas's
// earlier synchronise pass would destroy it first and close the dead
// abstraction view through the SafePointer-guarded path in
// reloadAbstractions(), hiding the bug. The instance view is opened by
// looking the instance glist up directly, because opening and closing a
// holder view would also close the instance view: ~SubpatchObject() of the
// holder's instance object closes views of its subpatch even when the pd
// object is still alive.)
//
// Run with AddressSanitizer: the PooledPtrArray poisoning makes ASAN report
// the use-after-free the moment the deleted Canvas/Objects are touched again.
// Without ASAN this aborts in libmalloc, like the production crash.

class AbstractionReloadRaceTest : public PlugDataUnitTest
{
public:
    AbstractionReloadRaceTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Abstraction Reload Race Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Canvas deleted from within its own synchronise after abstraction reload");

        auto const dir = File::getSpecialLocation(File::tempDirectory);

        // An abstraction containing a plain subpatch: when an instance dies, the
        // canvas showing its inside destroys a dead SubpatchObject, whose
        // destructor calls closeOpenedSubpatchers()
        abstractionFile = dir.getChildFile("reload_race_abs.pd");
        abstractionFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#N canvas 30 30 200 140 sub 0;\n"
            "#X obj 40 40 +~;\n"
            "#X restore 60 60 pd sub;\n");

        // The parent patch, instantiating the abstraction inside [pd holder]
        parentFile = dir.getChildFile("reload_race_parent.pd");
        parentFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#N canvas 30 30 200 140 holder 0;\n"
            "#X obj 40 40 reload_race_abs;\n"
            "#X restore 60 60 pd holder;\n");

        auto* pd = editor->pd;
        auto& tabbar = editor->getTabComponent();

        auto* parentCnv = tabbar.openPatch(pd->loadPatch(URL(parentFile)));
        if (!parentCnv || parentCnv->objects.empty()) {
            signalDone(false);
            return;
        }

        // Find the abstraction instance's glist inside [pd holder] directly, and
        // open a view of it, without ever opening a view of the holder itself
        t_glist* instanceGlist = nullptr;
        if (auto parentPatch = parentCnv->patch.getPointer()) {
            for (t_gobj* holder = parentPatch->gl_list; holder; holder = holder->g_next) {
                if (holder->g_pd != canvas_class)
                    continue;
                for (t_gobj* instance = reinterpret_cast<t_glist*>(holder)->gl_list; instance; instance = instance->g_next) {
                    if (instance->g_pd == canvas_class)
                        instanceGlist = reinterpret_cast<t_glist*>(instance);
                }
            }
        }
        if (!instanceGlist) {
            signalDone(false);
            return;
        }

        auto* instanceCnv = tabbar.openPatch(pd::Patch::Ptr(new pd::Patch(pd::WeakReference(instanceGlist, pd), pd, false)));
        if (!instanceCnv || instanceCnv->objects.empty()) {
            signalDone(false);
            return;
        }

        // Open the abstraction's own file and save it, like a user editing an
        // abstraction. savePatch() queues reloadAbstractions() asynchronously;
        // the saved root canvas itself is excluded from the reload, but the
        // instance inside [pd holder] is recreated, killing the instance view's
        // glist
        auto* abstractionCnv = tabbar.openPatch(pd->loadPatch(URL(abstractionFile)));
        if (!abstractionCnv) {
            signalDone(false);
            return;
        }
        abstractionCnv->refCountedPatch->savePatch();

        // The reload lambda queued by savePatch() is delivered before this one
        // (FIFO). Surviving its delivery is the actual test.
        MessageManager::callAsync([this] {
            abstractionFile.deleteFile();
            parentFile.deleteFile();
            signalDone(true);
        });
    }

    File abstractionFile, parentFile;
};
