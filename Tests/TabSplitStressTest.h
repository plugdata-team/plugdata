// Randomized stress test for tab, split and patch lifecycle management.
//
// Rapidly opens, shows, splits, saves and closes patches in random order,
// including multiple views of the same on-disk file and an abstraction with a
// live instance in another patch.
//
// This generalizes the two deterministic regression tests
// (SaveCloseRaceTest, AbstractionReloadRaceTest) into a randomized search of
// the same problem space: both of those crashes were specific interleavings
// of savePatch()'s async reloadAbstractions() with tab closing, found in the
// wild. The fragile ingredients are all here:
//
//  - TabComponent::splits holds two SafePointer<Canvas> plus a per-split
//    "last shown tab" history (addLastShownTab/getLastShownTab); closing the
//    canvas a split points at must fall back to a still-alive tab.
//  - closeTab()/showTab()/closeEmptySplits() run synchronously while
//    savePatch()'s reload lambdas are still queued on the message thread.
//  - Saving an abstraction file while an instance of it is open in another
//    patch recreates that instance, invalidating canvases mid-flight.
//
// The action sequence is deterministic for a fixed UnitTestRunner seed.
// Run with AddressSanitizer to catch canvases or patches touched after a
// close on any of these paths.

class TabSplitStressTest : public PlugDataUnitTest
{
public:
    TabSplitStressTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Tab/Split Stress Test")
    {
    }

private:
    static constexpr int numSteps = 200;

    void perform() override
    {
        beginTest("Random open/show/split/save/close sequences must not crash");

        auto const dir = File::getSpecialLocation(File::tempDirectory);

        // An abstraction, so saving it triggers the async reload of any open
        // instances - historically the most crash-prone path in this area
        abstractionFile = dir.getChildFile("tab_stress_abs.pd");
        abstractionFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 40 40 osc~ 440;\n"
            "#X obj 40 80 +~;\n"
            "#X connect 0 0 1 0;\n");

        // A patch instantiating that abstraction
        parentFile = dir.getChildFile("tab_stress_parent.pd");
        parentFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 60 60 tab_stress_abs;\n"
            "#X obj 60 120 metro 100;\n");

        // A plain patch
        plainFile = dir.getChildFile("tab_stress_plain.pd");
        plainFile.replaceWithText(
            "#N canvas 100 100 450 300 12;\n"
            "#X obj 50 50 tgl 25 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 50 100 print;\n"
            "#X connect 0 0 1 0;\n");

        editor->pd->volume->store(0.0f);

        performStep(numSteps);
    }

    void performStep(int stepsLeft)
    {
        auto& tabbar = editor->getTabComponent();

        if (stepsLeft == 0) {
            // The final queued reload lambdas from savePatch() must still be
            // delivered safely after everything is closed
            while (auto* cnv = tabbar.getCurrentCanvas())
                tabbar.closeTab(cnv);
            MessageManager::callAsync([this] {
                abstractionFile.deleteFile();
                parentFile.deleteFile();
                plainFile.deleteFile();
                signalDone(true);
            });
            return;
        }

        performRandomAction(tabbar);

        Timer::callAfterDelay(1, [this, stepsLeft] {
            performStep(stepsLeft - 1);
        });
    }

    Canvas* getRandomCanvas(TabComponent& tabbar)
    {
        auto canvases = tabbar.getCanvases();
        if (canvases.empty())
            return nullptr;
        return canvases[rng.nextInt(static_cast<int>(canvases.size()))];
    }

    void performRandomAction(TabComponent& tabbar)
    {
        switch (rng.nextInt(10)) {
        case 0: { // Open one of the files (possibly a second view of an already-open file)
            File const files[] = { abstractionFile, parentFile, plainFile };
            tabbar.openPatch(editor->pd->loadPatch(URL(files[rng.nextInt(3)])));
            break;
        }
        case 1: { // New untitled patch
            tabbar.newPatch();
            break;
        }
        case 2:
        case 3: { // Close a random tab, regardless of pending async work
            if (auto* cnv = getRandomCanvas(tabbar))
                tabbar.closeTab(cnv);
            break;
        }
        case 4:
        case 5: { // Show a random tab in a random split
            if (auto* cnv = getRandomCanvas(tabbar))
                tabbar.showTab(cnv, rng.nextInt(2));
            break;
        }
        case 6: { // Cycle tabs
            rng.nextBool() ? tabbar.nextTab() : tabbar.previousTab();
            break;
        }
        case 7: { // Make a random canvas the active split
            if (auto* cnv = getRandomCanvas(tabbar))
                tabbar.setActiveSplit(cnv);
            break;
        }
        case 8:
        case 9: { // Save a random file-backed patch: queues the async
                  // abstraction reload that both regression tests target
            if (auto* cnv = getRandomCanvas(tabbar)) {
                if (cnv->refCountedPatch->getCurrentFile().existsAsFile())
                    cnv->refCountedPatch->savePatch();
            }
            break;
        }
        }
    }

    File abstractionFile, parentFile, plainFile;
};
