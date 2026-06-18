#include "Connection.h"
#include "Pd/Interface.h"

// Randomized stress test for the canvas editing machinery.
//
// Performs a long random sequence of the same edit actions a user performs
// interactively - create, select, connect, duplicate, copy/paste, triggerize,
// encapsulate, tidy, move, delete, undo, redo - with DSP running, so the
// signal graph is recompiled underneath the edits.
//
// This deliberately targets the areas that look most fragile:
//
//  - Canvas::performSynchronise(): every action ends in a synchronise that
//    diffs the pd glist against the Object/Connection components. Objects
//    deleted+recreated on the pd side (undo/redo, encapsulate, paste) must be
//    matched back up without touching freed components.
//  - encapsulateSelection()/triggerizeSelection(): these build raw patch text
//    (Canvas.cpp), splice it into the glist via Interface::paste, and rely on
//    object indices computed *before* the mutation. Selections shaped unlike
//    the ones they were written for (self-connected groups, GUI objects,
//    signal/message mixes) are good at breaking the index bookkeeping.
//  - undo/redo across composite actions: encapsulate wraps a remove + paste in
//    an undo sequence; undoing it must restore connections to objects that
//    were recreated with new pointers.
//  - pd's DSP graph: connections between signal objects are made and removed
//    while DSP is on, racing canvas_update_dsp against the audio thread.
//
// The action sequence is deterministic for a fixed UnitTestRunner seed.
// Run with AddressSanitizer/UBSan: the test itself only checks survival; the
// sanitizers turn latent use-after-frees into hard failures.

class EditActionStressTest : public PlugDataUnitTest
{
public:
    EditActionStressTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Edit Action Stress Test")
    {
    }

private:
    static constexpr int numSteps = 500;

    void perform() override
    {
        beginTest("Random edit/undo/redo sequences must not crash");

        auto* cnv = editor->getTabComponent().newPatch();

        // Run with DSP on (but muted), so signal graph recompilation is
        // stressed by every connect/disconnect/delete
        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();

        performStep(cnv, numSteps);
    }

    void performStep(Component::SafePointer<Canvas> cnv, int stepsLeft)
    {
        if (!cnv) {
            signalDone(false);
            return;
        }
        if (stepsLeft == 0) {
            auto& tabbar = editor->getTabComponent();
            while (auto* c = tabbar.getCurrentCanvas())
                tabbar.closeTab(c);
            signalDone(true);
            return;
        }

        performRandomAction(cnv.getComponent());

        Timer::callAfterDelay(1, [this, cnv, stepsLeft] {
            performStep(cnv, stepsLeft - 1);
        });
    }

    SmallArray<Object*> getObjects(Canvas* cnv)
    {
        SmallArray<Object*> result;
        for (auto* obj : cnv->objects)
            result.add(obj);
        return result;
    }

    Object* getRandomObject(Canvas* cnv)
    {
        auto objects = getObjects(cnv);
        return objects.empty() ? nullptr : objects[rng.nextInt(static_cast<int>(objects.size()))];
    }

    void selectRandomObjects(Canvas* cnv, int maxCount)
    {
        cnv->deselectAll();
        auto const count = rng.nextInt(maxCount) + 1;
        for (int i = 0; i < count; i++) {
            if (auto* obj = getRandomObject(cnv))
                cnv->setSelected(obj, true);
        }
    }

    void performRandomAction(Canvas* cnv)
    {
        // A mix of message, signal and GUI objects, so selections cross the
        // signal/message and text/GUI boundaries
        static constexpr char const* objectNames[] = {
            "osc~ 440", "+~", "*~ 0.5", "dac~", "adc~", "sig~ 1", "lop~ 800",
            "metro 100", "del 50", "f", "+ 1", "t b a f", "sel 1 2 3",
            "list append 1 2", "print", "r stress_recv", "s stress_recv",
            "bng", "tgl", "hsl", "nbx", "loadbang", "pd subpatch"
        };

        switch (rng.nextInt(15)) {
        case 0:
        case 1: { // Create a random object (weighted up, so the patch keeps growing)
            auto const name = objectNames[rng.nextInt(static_cast<int>(std::size(objectNames)))];
            cnv->patch.createObject(rng.nextInt(600), rng.nextInt(400), name);
            cnv->synchronise();
            break;
        }
        case 2: { // Delete a random selection through the UI path (generates undo entries)
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 3);
                cnv->removeSelection();
            }
            break;
        }
        case 3: { // Connect two random iolets
            auto* src = getRandomObject(cnv);
            auto* sink = getRandomObject(cnv);
            if (src && sink && src != sink && src->numOutputs > 0 && sink->numInputs > 0) {
                auto* srcPtr = pd::Interface::checkObject(src->getPointer());
                auto* sinkPtr = pd::Interface::checkObject(sink->getPointer());
                if (srcPtr && sinkPtr) {
                    int const nout = rng.nextInt(src->numOutputs);
                    int const nin = rng.nextInt(sink->numInputs);
                    if (cnv->patch.canConnect(srcPtr, nout, sinkPtr, nin)) {
                        cnv->patch.createConnection(srcPtr, nout, sinkPtr, nin);
                        cnv->synchronise();
                    }
                }
            }
            break;
        }
        case 4: { // Delete a random connection
            if (cnv->connections.size()) {
                cnv->deselectAll();
                auto it = cnv->connections.begin();
                std::advance(it, rng.nextInt(static_cast<int>(cnv->connections.size())));
                cnv->setSelected(*it, true);
                cnv->removeSelectedConnections();
            }
            break;
        }
        case 5: { // Duplicate
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 4);
                cnv->duplicateSelection();
            }
            break;
        }
        case 6: { // Copy + paste
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 4);
                cnv->copySelection();
                cnv->pasteSelection();
            }
            break;
        }
        case 7: { // Triggerize
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 3);
                cnv->triggerizeSelection();
                cnv->hideSuggestions();
                cnv->hideAllActiveEditors();
            }
            break;
        }
        case 8: { // Encapsulate
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 5);
                cnv->encapsulateSelection();
            }
            break;
        }
        case 9: { // Connect selection in a row
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 4);
                cnv->connectSelection();
            }
            break;
        }
        case 10: { // Tidy
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 5);
                cnv->tidySelection();
            }
            break;
        }
        case 11: { // Move a random selection
            if (cnv->objects.size()) {
                selectRandomObjects(cnv, 3);
                SmallArray<t_gobj*> pointers;
                for (auto const* obj : cnv->getSelectionOfType<Object>()) {
                    if (auto* ptr = obj->getPointer())
                        pointers.add(ptr);
                }
                cnv->patch.moveObjects(pointers, rng.nextInt(41) - 20, rng.nextInt(41) - 20);
                cnv->synchronise();
            }
            break;
        }
        case 12:
        case 13: { // Undo (weighted up, so long undo chains get walked back)
            cnv->undo();
            break;
        }
        case 14: { // Redo
            cnv->redo();
            break;
        }
        }
    }
};
