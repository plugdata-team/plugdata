// Stress test for Pd deleting objects while plugdata is synchronising a canvas.
//
// Crash signature (ASAN, Linux, found by HelpFileFuzzTest on 4.data.structures/16.FFT-plot.pd):
// SEGV on a null read in Object::setType(), called from Canvas::performSynchronise().
//
// Mechanism:
// 1. In that patch, [bang~] sends "clear" to a subpatch and [append]s new scalars to it on every
//    DSP block, so the audio thread keeps deleting and recreating objects.
// 2. performSynchronise() collects the glist's objects, then builds an Object for each new one
//    without holding the Pd lock (holding it while constructing GUIs would stall audio).
// 3. When Pd deleted an object in between, ObjectBase::createGui() saw the dead WeakReference and
//    returned nullptr, which setType() then dereferenced. If the object died even earlier,
//    setType() mistook the dead reference for "no object passed" and created an empty object box
//    in Pd in its place.
//
// With dynamic patching, plugdata can't know that an object is about to be deleted: any code that
// touches a pd object has to unpack its WeakReference first, and handle the object having died.
// This test makes that happen as often as possible. A mutator thread plays the role of the audio
// thread, patching dynamically under the Pd lock (clear, create GUIs, scalars, graphs and
// subpatches, connect, toggle graph-on-parent, add arrays to graphs). Meanwhile the message thread
// keeps synchronising two views of the same subpatch (its own tab, and its graph-on-parent in the
// parent canvas) and clicking the GUIs inside them.
//
// The mutator doesn't need an audio device, so this also races on headless CI machines.
// Run with AddressSanitizer: the test checks that the canvas ends up consistent with Pd, the
// sanitizer catches use-after-frees along the way.

class SynchroniseDeletionStressTest : public PlugDataUnitTest {
public:
    explicit SynchroniseDeletionStressTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Synchronise Deletion Stress Test")
    {
    }

    ~SynchroniseDeletionStressTest() override
    {
        stopMutator();
    }

private:
    static constexpr int numTicks = 400;
    static constexpr double maxDurationMs = 10000.0; // Cap for slow (sanitizer) builds
    static constexpr int maxObjects = 40;            // Clear the target once it holds this many objects

    // [pd stress-target] is the canvas being mutated, shown as graph-on-parent in the root canvas.
    // The template and the [append] chain live outside of it, so "clear" doesn't delete them.
    static constexpr char const* patchText =
        "#N canvas 0 50 900 600 12;\n"
        "#N canvas 0 50 450 300 stress-template 0;\n"
        "#X obj 20 20 struct stress-template float x float y float w;\n"
        "#X obj 20 60 filledpolygon 900 0 1 0 0 20 0 20 20 0 20;\n"
        "#X obj 20 100 drawnumber w 0 25 0 w=;\n"
        "#X restore 20 420 pd stress-template;\n"
        "#N canvas 0 50 450 300 stress-gen 0;\n"
        "#X obj 20 20 r stress-traverse;\n"
        "#X msg 20 60 traverse pd-stress-target \\, bang;\n"
        "#X obj 20 100 pointer;\n"
        "#X obj 200 20 r stress-append;\n"
        "#X obj 200 140 append stress-template x y;\n"
        "#X connect 0 0 1 0;\n"
        "#X connect 1 0 2 0;\n"
        "#X connect 2 0 4 2;\n"
        "#X connect 3 0 4 0;\n"
        "#X restore 20 460 pd stress-gen;\n"
        "#N canvas 0 50 600 400 stress-target 0;\n"
        "#X coords 0 0 400 300 400 300 1 0 0;\n"
        "#X restore 20 20 pd stress-target;\n";

    struct ObjectSpec {
        char const* selector;
        char const* args;
        bool patchable; // Has at least one control inlet and outlet
    };

    // A mix of plain objects and the GUIs with the most involved construction
    static constexpr ObjectSpec objectSpecs[] = {
        { "obj", "bng", true },
        { "obj", "tgl", true },
        { "obj", "nbx", true },
        { "obj", "hsl", true },
        { "obj", "vsl", true },
        { "obj", "hradio", true },
        { "obj", "vu", false },
        { "obj", "cnv 15 80 40", false },
        { "obj", "knob", true },
        { "obj", "keyboard", false },
        { "obj", "function", false },
        { "obj", "scope~", false },
        { "obj", "+ 1", true },
        { "obj", "t b b", true },
        { "obj", "metro 100", true },
        { "obj", "osc~ 440", false },
        { "obj", "text define", false },
        { "msg", "stress bang", true },
        { "floatatom", "5 0 0 0 - - - 0", true },
        { "symbolatom", "10 0 0 0 - - - 0", false },
        { "listbox", "20 0 0 0 - - - 0", false },
        { "text", "stress comment", false },
    };

    // Clicking these only sends messages to Pd: anything else could open a tab or dialog
    inline static StringArray const clickableTypes = { "bng", "tgl", "nbx", "hsl", "vsl", "hradio", "knob", "keyboard", "msg", "message", "floatatom", "gatom", "function" };

    struct MutatorState {
        int numObjects = 0;
        int nextId = 0;
        StringArray liveSubpatches; // Subpatches and graphs created since the last clear
        StringArray liveGraphs;
    };

    void perform() override
    {
        beginTest("Objects deleted by Pd while the canvas synchronises");

        auto& tabbar = editor->getTabComponent();
        auto* root = tabbar.openPatch(String(patchText));
        if (!root) {
            expect(false, "couldn't open the stress patch");
            signalDone(false);
            return;
        }

        // Open the target's own view through the patch of its graph-on-parent object, like a user would
        editor->pd->lockAudioThread();
        auto* targetGlist = reinterpret_cast<t_glist*>(pd_findbyclass(gensym("pd-stress-target"), canvas_class));
        editor->pd->unlockAudioThread();

        pd::Patch::Ptr targetPatch;
        for (auto* object : root->objects) {
            if (!object->gui)
                continue;
            if (auto subpatch = object->gui->getPatch()) {
                if (subpatch->getPointer().get() == targetGlist)
                    targetPatch = subpatch;
            }
        }

        auto* targetView = targetPatch ? tabbar.openPatch(targetPatch) : nullptr;
        if (!targetGlist || !targetView) {
            expect(false, "couldn't open a view of [pd stress-target]");
            closeViews(root, targetView);
            signalDone(false);
            return;
        }

        rootView = root;
        this->targetView = targetView;
        rootView->locked = true;
        this->targetView->locked = true;

        // With an audio device, the audio thread races too
        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();

        foundObjectWithoutGui = false;
        stopMutating = false;
        mutatorRounds = 0;
        mutator = std::thread([this, seed = rng.nextInt64()] { runMutator(seed); });

        startTime = Time::getMillisecondCounterHiRes();
        tick(numTicks);
    }

    void tick(int ticksLeft)
    {
        if (!rootView || !targetView) {
            stopMutator();
            expect(false, "a view of the stress patch got closed");
            closeViews(rootView, targetView);
            signalDone(false);
            return;
        }

        for (auto* view : { rootView.getComponent(), targetView.getComponent() }) {
            // Synchronise directly, on top of the async synchronise that every dynamic patching message triggers
            view->performSynchronise();

            // Clicks can trigger synchronisation, so collect the objects first
            HeapArray<Component::SafePointer<Object>> objects;
            for (auto* object : view->objects)
                objects.add(object);

            for (auto& object : objects) {
                if (!object)
                    continue;

                // performSynchronise() removes Objects whose pd object died before they got a gui
                if (!object->gui) {
                    foundObjectWithoutGui = foundObjectWithoutGui || !object->isInitialEditorShown();
                    continue;
                }

                // These read from pd through the gui's WeakReference, which can be dead by now
                object->gui->getText();
                object->gui->updateProperties();

                if (rng.nextInt(4) == 0 && clickableTypes.contains(object->gui->getType().toString()))
                    TestHelpers::simulateClick(object->gui.get());
            }
        }

        if (ticksLeft <= 1 || Time::getMillisecondCounterHiRes() - startTime > maxDurationMs) {
            finish();
            return;
        }

        Timer::callAfterDelay(1, [this, ticksLeft] {
            tick(ticksLeft - 1);
        });
    }

    void finish()
    {
        stopMutator();
        logMessage("Mutator rounds: " + String(mutatorRounds.load()));
        expect(mutatorRounds > 0, "the mutator never ran");

        // Let queued synchronisations and gui swaps (graph-on-parent toggles) run, then compare with Pd
        Timer::callAfterDelay(100, [this] {
            bool passed = true;
            if (!rootView || !targetView) {
                expect(false, "a view of the stress patch got closed");
                passed = false;
            } else {
                targetView->performSynchronise();

                expect(!foundObjectWithoutGui, "performSynchronise() left an Object without a gui");
                passed = passed && !foundObjectWithoutGui;

                auto const numPdObjects = static_cast<int>(targetView->patch.getObjects().size());
                auto const numObjects = static_cast<int>(targetView->objects.size());
                expectEquals(numObjects, numPdObjects, "the canvas doesn't match the pd patch");
                passed = passed && numObjects == numPdObjects;

                // A dead WeakReference passed to Object::setType() used to create an empty object box in Pd
                auto const numEmptyBoxes = countEmptyObjectBoxes(*targetView);
                expectEquals(numEmptyBoxes, 0, "plugdata created stray objects in the pd patch");
                passed = passed && numEmptyBoxes == 0;
            }

            closeViews(rootView, targetView);
            signalDone(passed);
        });
    }

    void runMutator(int64 const seed)
    {
        Random random(seed);
        MutatorState state;

        while (!stopMutating) {
            editor->pd->lockAudioThread();
            mutate(random, state);
            editor->pd->unlockAudioThread();
            ++mutatorRounds;

            // Leave gaps for the message thread to take the lock, like the audio thread does between blocks
            std::this_thread::sleep_for(std::chrono::microseconds(random.nextInt(1000)));
        }
    }

    // Called with the Pd lock held
    void mutate(Random& random, MutatorState& state) const
    {
        auto send = [this](String const& receiver, char const* selector, SmallArray<pd::Atom> const& args = {}) {
            editor->pd->sendMessage(receiver.toRawUTF8(), selector, args);
        };
        auto randomX = [&random] { return static_cast<float>(random.nextInt(360)); };
        auto randomY = [&random] { return static_cast<float>(random.nextInt(260)); };

        bool const clear = state.numObjects > maxObjects || random.nextInt(4) != 0;
        if (clear) {
            send("pd-stress-target", "clear");
            state.numObjects = 0;
            state.liveSubpatches.clear();
            state.liveGraphs.clear();
        } else {
            // Make plugdata swap the gui of an existing object: these swaps run asynchronously,
            // so the next clear can delete the object before they run
            if (!state.liveSubpatches.isEmpty() && random.nextBool()) {
                auto const& name = state.liveSubpatches[random.nextInt(state.liveSubpatches.size())];
                send("pd-" + name, "coords", { 0.0f, 0.0f, 100.0f, 60.0f, 100.0f, 60.0f, static_cast<float>(random.nextInt(2)), 0.0f, 0.0f });
            }
            if (!state.liveGraphs.isEmpty() && random.nextBool()) {
                auto const& name = state.liveGraphs[random.nextInt(state.liveGraphs.size())];
                auto const array = "stress-array-" + String(state.nextId++);
                send("pd-" + name, "array", { gensym(array.toRawUTF8()), 16.0f, gensym("float"), 0.0f });
            }
        }

        // Scalars, appended the same way the FFT-plot help patch does it. A fresh traverse
        // starts [append] at the head of the glist, so the scalars go before everything else.
        send("stress-traverse", "bang");
        int const numScalars = random.nextInt(4);
        for (int i = 0; i < numScalars; i++)
            send("stress-append", "list", { randomX(), randomY() });

        SmallArray<int> patchable;
        int index = numScalars; // Only meaningful after a clear
        int const numNewObjects = 1 + random.nextInt(8);
        for (int i = 0; i < numNewObjects; i++, index++) {
            int const kind = random.nextInt(static_cast<int>(std::size(objectSpecs)) + 2);
            float const x = randomX(), y = randomY();

            if (kind == std::size(objectSpecs)) {
                // A graph with an array, the same messages a patch file sends to load one
                auto const graph = "stress-graph-" + String(state.nextId++);
                auto const array = "stress-array-" + String(state.nextId++);
                send("pd-stress-target", "graph", { gensym(graph.toRawUTF8()), 0.0f, 1.0f, 16.0f, -1.0f, x, y, x + 100.0f, y + 60.0f });
                send("pd-" + graph, "array", { gensym(array.toRawUTF8()), 16.0f, gensym("float"), 0.0f });
                send("pd-" + graph, "pop", { 0.0f });
                state.liveGraphs.add(graph);
            } else if (kind == std::size(objectSpecs) + 1) {
                // A subpatch with something inside, so it has contents to show as graph-on-parent
                auto const subpatch = "stress-sub-" + String(state.nextId++);
                send("pd-stress-target", "obj", { x, y, gensym("pd"), gensym(subpatch.toRawUTF8()) });
                send("pd-" + subpatch, "obj", { 10.0f, 10.0f, gensym("tgl") });
                state.liveSubpatches.add(subpatch);
            } else {
                auto const& spec = objectSpecs[kind];
                SmallArray<pd::Atom> args = { x, y };
                for (auto const& atom : pd::Atom::atomsFromString(spec.args))
                    args.add(atom);
                send("pd-stress-target", spec.selector, args);
                if (clear && spec.patchable)
                    patchable.add(index);
            }
        }
        state.numObjects += numScalars + numNewObjects;

        // Connect from lower to higher indices only: a cycle would overflow the stack when clicked
        if (patchable.size() >= 2) {
            int const numConnections = random.nextInt(static_cast<int>(patchable.size()));
            for (int i = 0; i < numConnections; i++) {
                int const from = random.nextInt(static_cast<int>(patchable.size()) - 1);
                int const to = from + 1 + random.nextInt(static_cast<int>(patchable.size()) - from - 1);
                send("pd-stress-target", "connect", { static_cast<float>(patchable[from]), 0.0f, static_cast<float>(patchable[to]), 0.0f });
            }
        }
    }

    static int countEmptyObjectBoxes(Canvas& view)
    {
        int count = 0;
        if (auto glist = view.patch.getPointer()) {
            for (t_gobj* y = glist->gl_list; y; y = y->g_next) {
                if (auto const* text = pd_checkobject(&y->g_pd); text && text->te_type == T_OBJECT && binbuf_getnatom(text->te_binbuf) == 0)
                    count++;
            }
        }
        return count;
    }

    void stopMutator()
    {
        stopMutating = true;
        if (mutator.joinable())
            mutator.join();
    }

    void closeViews(Canvas* root, Canvas* target) const
    {
        auto& tabbar = editor->getTabComponent();
        if (target)
            tabbar.closeTab(target);
        if (root)
            tabbar.closeTab(root);
    }

    Component::SafePointer<Canvas> rootView, targetView;
    std::thread mutator;
    std::atomic<bool> stopMutating = false;
    std::atomic<int> mutatorRounds = 0;
    bool foundObjectWithoutGui = false;
    double startTime = 0.0;
};
