#include "Constants.h"

// Deterministic coverage of the core editing workflows, each followed by the
// undo/redo that a user relies on. Unlike EditActionStressTest (random, only
// checks for crashes), every step here asserts the expected object/connection
// counts and selection state, so a regression in any single operation fails
// loudly.
//
// Steps: multi-selection (direct + lasso drag), copy/paste, duplicate, delete,
// undo/redo after each, zoom in/out/reset, a nested subpatch opened into its
// own canvas, a split-view tab, and close-with-unsaved-changes through both
// the Cancel (keep open) and Don't Save (close) paths of the save dialog.

class EditWorkflowTest : public PlugDataUnitTest
{
public:
    EditWorkflowTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Edit Workflow Test")
    {
    }

private:
    void perform() override
    {
        cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 700 500 12;\n"
            "#X obj 50 50 osc~ 440;\n"
            "#X obj 50 100 *~ 0.5;\n"
            "#X obj 50 150 dac~;\n"
            "#X obj 300 50 metro 250;\n"
            "#X obj 300 100 print;\n"
            "#X connect 0 0 1 0;\n"
            "#X connect 1 0 2 0;\n"
            "#X connect 3 0 4 0;\n"));

        if (!cnv || cnv->objects.size() != 5) {
            signalDone(false);
            return;
        }
        cnv->locked.setValue(false);
        cnv->performSynchronise();

        buildSteps();
        runStep(0);
    }

    int objectCount() { return cnv ? static_cast<int>(cnv->objects.size()) : -1; }
    int connectionCount() { return cnv ? static_cast<int>(cnv->connections.size()) : -1; }

    void selectFirst(int const n)
    {
        cnv->deselectAll();
        int i = 0;
        for (auto* obj : cnv->objects) {
            if (i++ >= n)
                break;
            cnv->setSelected(obj, true);
        }
    }

    void buildSteps()
    {
        // --- Multi-selection ---
        steps.add([this] {
            beginTest("Multi-selection");
            selectFirst(3);
            expect(cnv->getSelectionOfType<Object>().size() == 3, "selecting three objects must report three selected");
        });

        // --- Copy / paste (+ undo / redo) ---
        // NB: Patch::copy writes to the system clipboard via MessageManager::callAsync,
        // and paste reads it back, so copy and paste must be in separate steps
        // (a message-loop tick apart) or paste sees a stale clipboard.
        steps.add([this] {
            beginTest("Copy and paste");
            selectFirst(2);
            baseline = objectCount();
            cnv->copySelection();
        });
        steps.add([this] {
            cnv->pasteSelection();
            cnv->performSynchronise();
            expect(objectCount() == baseline + 2, "pasting two copied objects must add two objects");
            cnv->undo();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline, "undo must remove the pasted objects");
            cnv->redo();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline + 2, "redo must bring the pasted objects back");
            cnv->undo();
            cnv->performSynchronise(); // back to baseline for the next step
        });

        // --- Duplicate (+ undo) ---
        steps.add([this] {
            beginTest("Duplicate");
            selectFirst(1);
            baseline = objectCount();
            cnv->duplicateSelection();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline + 1, "duplicating one object must add one object");
            cnv->undo();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline, "undo must remove the duplicate");
        });

        // --- Delete (+ undo restores objects and their connections) ---
        steps.add([this] {
            beginTest("Delete");
            baseline = objectCount();
            baselineConnections = connectionCount();
            // Select [*~ 0.5] (index 1), which sits between two connections
            cnv->deselectAll();
            int i = 0;
            for (auto* obj : cnv->objects) {
                if (i++ == 1)
                    cnv->setSelected(obj, true);
            }
            cnv->removeSelection();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline - 1, "deleting one object must remove one object");
            expect(connectionCount() < baselineConnections, "deleting a connected object must remove its connections");
            cnv->undo();
            cnv->performSynchronise();
        });
        steps.add([this] {
            expect(objectCount() == baseline, "undo must restore the deleted object");
            expect(connectionCount() == baselineConnections, "undo must restore the deleted object's connections");
        });

        // --- Zoom ---
        // The zoom commands drive an animated magnification, so assert on the
        // zoomScale model (which the canvas reads/writes directly) for
        // determinism, and invoke the commands separately for code coverage.
        steps.add([this] {
            beginTest("Zoom");
            cnv->zoomScale.setValue(1.0f);
            cnv->zoomScale.setValue(1.5f);
            expect(approximatelyEqual(getValue<float>(cnv->zoomScale), 1.5f), "setting the zoom scale must update it");
            cnv->zoomScale.setValue(0.5f);
            expect(approximatelyEqual(getValue<float>(cnv->zoomScale), 0.5f), "setting a smaller zoom scale must update it");
            cnv->zoomScale.setValue(1.0f);
        });
        steps.add([this] {
            // Exercise the zoom command handlers (magnify is animated, so we
            // don't assert exact values here - just that they run safely)
            editor->commandManager.invokeDirectly(CommandIDs::ZoomIn, false);
            editor->commandManager.invokeDirectly(CommandIDs::ZoomOut, false);
            editor->commandManager.invokeDirectly(CommandIDs::ZoomNormal, false);
            editor->commandManager.invokeDirectly(CommandIDs::ZoomToFitAll, false);
        });

        // --- Nested subpatch opened into its own canvas ---
        steps.add([this] {
            beginTest("Nested subpatch");
            tabCountBefore = static_cast<int>(editor->getTabComponent().getCanvases().size());
            cnv->patch.createObject(450, 250, "pd nested_sub");
            cnv->performSynchronise();
        });
        steps.add([this] {
            // Find the subpatch object and open it
            Object* subpatch = nullptr;
            for (auto* obj : cnv->objects) {
                if (obj->gui && obj->gui->getText().startsWith("pd"))
                    subpatch = obj;
            }
            expect(subpatch != nullptr, "the [pd nested_sub] subpatch object must be created");
            if (subpatch && subpatch->gui)
                subpatch->gui->openSubpatch();
        });
        steps.add([this] {
            auto const tabCountAfter = static_cast<int>(editor->getTabComponent().getCanvases().size());
            expect(tabCountAfter > tabCountBefore, "opening a subpatch must add a canvas");
        });

        // --- Split-view tab ---
        steps.add([this] {
            beginTest("Split-view tab");
            auto& tabbar = editor->getTabComponent();
            otherCanvas = tabbar.openPatch(String("#N canvas 100 100 300 200 12;\n#X obj 20 20 print split;\n"));
        });
        steps.add([this] {
            auto& tabbar = editor->getTabComponent();
            if (otherCanvas)
                tabbar.showTab(otherCanvas, 1); // show in the right split
        });
        steps.add([this] {
            auto& tabbar = editor->getTabComponent();
            expect(tabbar.getVisibleCanvases().size() == 2, "showing a tab in split 1 must make two canvases visible");
        });

        // --- Close with unsaved changes: Cancel keeps the tab, Don't Save closes it ---
        steps.add([this] {
            beginTest("Close with unsaved changes");
            // Make the split patch dirty by creating an object in it
            if (otherCanvas) {
                otherCanvas->patch.createObject(40, 60, "metro 100");
                otherCanvas->performSynchronise();
            }
            tabCountBefore = static_cast<int>(editor->getTabComponent().getCanvases().size());
            if (otherCanvas)
                editor->getTabComponent().askToCloseTab(otherCanvas);
        });
        steps.add([this] {
            // The save dialog opens asynchronously; click Cancel
            if (auto* cancel = TestHelpers::findButtonWithText(editor->openedDialog.get(), "Cancel")) {
                cancel->onClick();
            } else {
                expect(false, "the save dialog must offer a Cancel button");
            }
        });
        steps.add([this] {
            auto const count = static_cast<int>(editor->getTabComponent().getCanvases().size());
            expect(count == tabCountBefore, "cancelling the close must keep the tab open");
            editor->openedDialog.reset(nullptr);
            if (otherCanvas)
                editor->getTabComponent().askToCloseTab(otherCanvas);
        });
        steps.add([this] {
            if (auto* dontSave = TestHelpers::findButtonWithText(editor->openedDialog.get(), "Don't Save")) {
                dontSave->onClick();
            } else {
                expect(false, "the save dialog must offer a Don't Save button");
            }
        });
        steps.add([this] {
            auto const count = static_cast<int>(editor->getTabComponent().getCanvases().size());
            expect(count == tabCountBefore - 1, "Don't Save must close the tab");
            editor->openedDialog.reset(nullptr);
        });

        // --- Lasso multi-selection via canvas mouse drag ---
        steps.add([this] {
            beginTest("Lasso selection");
            // Bring the original canvas back to the front and make sure it's in
            // edit mode; the unlock propagates on the next message-loop tick.
            editor->getTabComponent().showTab(cnv, 0);
            cnv->locked.setValue(false);
            cnv->deselectAll();
        });
        steps.add([this] {
            // Objects live at canvasOrigin (a large offset), so the lasso must
            // cover their actual bounds, not the canvas's logical (0,0) corner.
            Rectangle<int> objectsArea;
            for (auto* obj : cnv->objects)
                objectsArea = objectsArea.getUnion(obj->getBounds());
            expect(!objectsArea.isEmpty(), "canvas must have objects to lasso");

            auto const start = objectsArea.getTopLeft().toFloat() - Point<float>(20, 20);
            auto const end = objectsArea.getBottomRight().toFloat() + Point<float>(20, 20);
            auto const event = [this](Point<float> pos, Point<float> down, bool dragged) {
                return MouseEvent(Desktop::getInstance().getMainMouseSource(), pos, ModifierKeys::leftButtonModifier,
                                  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, cnv, cnv,
                                  Time::getCurrentTime(), down, Time::getCurrentTime(), 1, dragged);
            };

            // Down/drag/up synchronously: JUCE's LassoComponent is synchronous,
            // and keeping it in one step avoids any inter-step state change.
            cnv->mouseDown(event(start, start, false));
            cnv->mouseDrag(event(end, start, true));
            cnv->mouseUp(event(end, start, true));

            expect(!cnv->getSelectionOfType<Object>().empty(), "a lasso over the objects must select them");
        });
    }

    void runStep(int const index)
    {
        if (!cnv) {
            signalDone(false);
            return;
        }
        if (index >= steps.size()) {
            auto& tabbar = editor->getTabComponent();
            while (auto* c = tabbar.getCurrentCanvas())
                tabbar.closeTab(c);
            signalDone(true);
            return;
        }

        steps[index]();
        Timer::callAfterDelay(40, [this, index] { runStep(index + 1); });
    }

    HeapArray<std::function<void()>> steps;
    Component::SafePointer<Canvas> cnv;
    Component::SafePointer<Canvas> otherCanvas;
    int baseline = 0, baselineConnections = 0, tabCountBefore = 0;
};
