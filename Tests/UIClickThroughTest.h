#include "Dialogs/Dialogs.h"

// Clicks every visible UI element in the editor once.
//
// Opens a patch containing the common GUI objects, then walks the editor's
// entire component tree, collects every component that is currently showing,
// and delivers a synthetic mouseDown/mouseUp pair to each of them, one per
// message loop tick. This exercises every button, toolbar item, statusbar
// callout, sidebar panel, tab button, object and iolet that is reachable on
// the main editor surface.
//
// The component list is collected up-front: anything that only appears as a
// *result* of a click (popup menus, callouts, dialogs) is intentionally not
// clicked, both to keep the test deterministic and to avoid actions that
// can't be undone from a test (installing packages, overwriting files).
// All popups/dialogs that clicks leave behind are dismissed at the end.
//
// Native file choosers are suppressed by the ENABLE_TESTING guards in
// Dialogs::showOpenDialog/showSaveDialog, so no click can hang the run on an
// OS dialog. Run with AddressSanitizer to catch components whose click
// handlers touch freed state (e.g. handlers that fire after their target
// canvas was replaced).

class UIClickThroughTest : public PlugDataUnitTest
{
public:
    UIClickThroughTest(PluginEditor* editor) : PlugDataUnitTest(editor, "UI Click-Through Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Clicking every visible UI element must not crash");

        // A patch with one of every common GUI object, so canvas/object/iolet
        // clicks are part of the sweep
        auto* cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 700 500 12;\n"
            "#X obj 20 20 bng 25 250 50 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000;\n"
            "#X obj 60 20 tgl 25 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 100 20 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 100 50 vsl 17 128 0 127 0 0 empty empty empty 0 -9 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 250 20 nbx 5 14 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0 256;\n"
            "#X obj 320 20 vu 17 120 empty empty -1 -8 0 10 #404040 #000000 1 0;\n"
            "#X obj 360 20 cnv 15 100 60 empty empty empty 20 12 0 14 #e0e0e0 #404040 0;\n"
            "#X obj 20 200 vradio 17 1 0 8 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0;\n"
            "#X obj 60 200 hradio 17 1 0 8 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0;\n"
            "#X msg 250 200 click me;\n"
            "#X floatatom 250 240 5 0 0 0 - - -;\n"
            "#X symbolatom 250 280 10 0 0 0 - - -;\n"
            "#X text 400 200 a comment;\n"
            "#X obj 400 240 osc~ 440;\n"
            "#X obj 400 280 metro 200;\n"
            "#X connect 13 0 14 0;\n"));

        if (!cnv) {
            signalDone(false);
            return;
        }

        editor->pd->volume->store(0.0f);

        // Give the canvas a tick to lay itself out before collecting targets
        Timer::callAfterDelay(100, [this] {
            HeapArray<Component::SafePointer<Component>> targets;
            collectComponents(editor, targets);
            numTargets = targets.size();
            clickNext(std::move(targets));
        });
    }

    static void collectComponents(Component* c, HeapArray<Component::SafePointer<Component>>& targets)
    {
        for (auto* child : c->getChildren()) {
            if (!child->isShowing())
                continue;
            targets.add(child);
            collectComponents(child, targets);
        }
    }

    void clickNext(HeapArray<Component::SafePointer<Component>> targets)
    {
        // Components are deleted, hidden and replaced as a side effect of
        // earlier clicks (closing a tab kills the whole canvas tree), so
        // re-check each pointer right before clicking it
        while (!targets.empty()) {
            auto target = targets.back();
            targets.remove_at(targets.size() - 1);

            if (auto* component = target.getComponent()) {
                if (component->isShowing() && component->isEnabled()) {
                    simulateClick(component);
                    Timer::callAfterDelay(1, [this, targets = std::move(targets)]() mutable {
                        clickNext(std::move(targets));
                    });
                    return;
                }
            }
        }

        finish();
    }

    static void simulateClick(Component* c)
    {
        bool allowsMouseClicks, allowsMouseClicksOnChildren;
        c->getInterceptsMouseClicks(allowsMouseClicks, allowsMouseClicksOnChildren);
        if (!allowsMouseClicks)
            return;

        auto const centre = c->getLocalBounds().getCentre().toFloat();
        MouseEvent e(Desktop::getInstance().getMainMouseSource(),
                     centre, ModifierKeys::leftButtonModifier,
                     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                     Time::getCurrentTime(), centre, Time::getCurrentTime(), 1, false);
        c->mouseDown(e);
        c->mouseUp(e);
    }

    void finish()
    {
        // Clean up everything the clicks left behind: open menus, callouts,
        // dialogs and the tabs we (or a clicked button) created
        PopupMenu::dismissAllActiveMenus();
        ModalComponentManager::getInstance()->cancelAllModalComponents();
        Dialogs::dismissFileDialog();
        editor->openedDialog.reset(nullptr);

        auto& tabbar = editor->getTabComponent();
        while (auto* cnv = tabbar.getCurrentCanvas()) {
            tabbar.closeTab(cnv);
        }

        expect(numTargets > 0, "no clickable components were found");
        signalDone(numTargets > 0);
    }

    int numTargets = 0;
};
