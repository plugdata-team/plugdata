// Saving a patch to a file that already exists must replace it. On iOS, savePatch(URL)
// writes through URL::createOutputStream, which appends to an existing file: the result
// held two root canvases and grew with every save.

#include "Canvas.h"

class SaveOverwriteTest : public PlugDataUnitTest {
public:
    SaveOverwriteTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Save Overwrite Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Saving to an existing file replaces it");

        auto const target = File::createTempFile(".pd");
        target.replaceWithText("#N canvas 0 50 450 300 12;\n#X obj 10 10 stale_patch_marker;\n");

        auto* cnv = editor->getTabComponent().openPatch(String("#N canvas 0 50 450 300 12;\n#X obj 30 30 osc~ 331;\n"));
        if (!cnv) {
            signalDone(false);
            return;
        }

        cnv->patch.savePatch(URL(target));
        auto const firstSize = target.getSize();
        cnv->patch.savePatch(URL(target));

        auto const saved = target.loadFileAsString();
        expect(saved.contains("osc~ 331"), "the file must contain the patch");
        expect(!saved.contains("stale_patch_marker"), "the old content must be gone");
        expectEquals(target.getSize(), firstSize, "saving twice must not grow the file");

        editor->getTabComponent().closeTab(cnv);
        target.deleteFile();
        signalDone(true);
    }
};
