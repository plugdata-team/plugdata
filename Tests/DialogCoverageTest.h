#include "Dialogs/Dialogs.h"
#include "Dialogs/AboutPanel.h"

// Opens, paints and closes every dialog that can be shown without network
// access or native OS windows.
//
// The Dialogs/ folder is almost entirely unreachable from the other tests
// (helpfiles and canvas edits never open dialogs), so each dialog is shown
// here explicitly: all six settings panels (audio, MIDI, theme, paths, key
// mappings, advanced - which also run the bulk of PropertiesPanel),
// about, object browser, object reference, Heavy export, save prompt,
// multi-choice prompt, and the popup menus (main menu, add-object menu,
// canvas right-click menu).
//
// Each dialog gets a tick to lay itself out, is force-painted via
// createComponentSnapshot() so paint() coverage doesn't depend on vblank
// timing, and is then closed through the same openedDialog pointer the real
// UI uses. Deken and the patch store are deliberately skipped: they fetch
// from the network on open, which would make the suite flaky offline.

class DialogCoverageTest : public PlugDataUnitTest
{
public:
    DialogCoverageTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Dialog Coverage Test")
    {
    }

private:
    void perform() override
    {
        // An open canvas, so canvas-dependent menus have something to target
        cnv = editor->getTabComponent().newPatch();

        steps.clear();

        // All settings panels: 0=audio, 1=MIDI, 2=theme, 3=paths, 4=key mappings, 5=advanced
        for (int panel = 0; panel < 6; panel++) {
            steps.add([this, panel] {
                beginTest("Settings panel " + String(panel));
                Dialogs::showSettingsDialog(editor, panel);
            });
        }

        steps.add([this] {
            beginTest("About panel");
            auto* dialog = new Dialog(&editor->openedDialog, editor, 360, 490, true);
            dialog->setViewedComponent(new AboutPanel());
            editor->openedDialog.reset(dialog);
        });

        steps.add([this] {
            beginTest("Object browser");
            Dialogs::showObjectBrowserDialog(&editor->openedDialog, editor);
        });

        steps.add([this] {
            beginTest("Object reference");
            Dialogs::showObjectReferenceDialog(&editor->openedDialog, editor, "osc~");
        });

        steps.add([this] {
            beginTest("Heavy export dialog");
            Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
        });

        steps.add([this] {
            beginTest("Ask-to-save dialog");
            Dialogs::showAskToSaveDialog(&editor->openedDialog, editor, "test.pd", [](int) { }, 0, true);
        });

        steps.add([this] {
            beginTest("Multi-choice dialog");
            Dialogs::showMultiChoiceDialog(&editor->openedDialog, editor, "Test question?", [](int) { }, { "Yes", "No", "Maybe" });
        });

        steps.add([this] {
            beginTest("Main menu");
            Dialogs::showMainMenu(editor, editor);
        });

        steps.add([this] {
            beginTest("Add-object menu");
            Dialogs::showObjectMenu(editor, editor);
        });

        steps.add([this] {
            beginTest("Canvas right-click menu");
            if (cnv)
                Dialogs::showCanvasRightClickMenu(cnv, cnv, cnv->getScreenBounds().getCentre());
        });

        runNextStep(0);
    }

    void runNextStep(int const stepIndex)
    {
        if (stepIndex >= steps.size()) {
            auto& tabbar = editor->getTabComponent();
            while (auto* c = tabbar.getCurrentCanvas())
                tabbar.closeTab(c);
            signalDone(true);
            return;
        }

        steps[stepIndex]();

        // Give the dialog a tick to lay out and run any async setup, then
        // force a paint pass and close it again
        Timer::callAfterDelay(100, [this, stepIndex] {
            if (editor->openedDialog) {
                editor->openedDialog->createComponentSnapshot(editor->openedDialog->getLocalBounds());
            }

            PopupMenu::dismissAllActiveMenus();
            ModalComponentManager::getInstance()->cancelAllModalComponents();
            editor->openedDialog.reset(nullptr);

            Timer::callAfterDelay(50, [this, stepIndex] {
                runNextStep(stepIndex + 1);
            });
        });
    }

    HeapArray<std::function<void()>> steps;
    Component::SafePointer<Canvas> cnv;
};
