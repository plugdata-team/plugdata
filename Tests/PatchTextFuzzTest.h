// Fuzzes the patch loader with structurally damaged patch files.
//
// Users routinely paste patch text from forums/chats and open patches written
// by other (sometimes buggy) software, so the loader regularly sees text that
// no saver would produce. This test generates randomly malformed patch text
// and runs it through the same path as TabComponent::openPatch: a random mix
// of valid object/message/atom/array lines with classic loader hazards:
//
//  - "#X connect" with out-of-range or negative object indices
//  - "#X restore" without a matching "#N canvas" (canvas stack underflow)
//  - "#N canvas" that is never restored (truncated file)
//  - "#A" array-data lines with no array in sight
//  - "#X coords" on canvases with and without graph-on-parent
//  - absurd coordinates/sizes, multi-kilobyte single atoms, unterminated
//    final lines, raw garbage lines, and "\$0"-style dollar args at the
//    top level
//
// Whatever the loader produces is then synchronised into a Canvas (so
// plugdata's object/connection sync sees the damaged glist too) and closed
// again. Every generated patch is deterministic for a fixed UnitTestRunner
// seed; the patch text of the current iteration is logged so a crash can be
// reproduced directly.

class PatchTextFuzzTest : public PlugDataUnitTest
{
public:
    PatchTextFuzzTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Patch Text Fuzz Test")
    {
    }

private:
    static constexpr int numPatches = 80;

    void perform() override
    {
        editor->pd->volume->store(0.0f);
        performStep(numPatches);
    }

    void performStep(int stepsLeft)
    {
        auto& tabbar = editor->getTabComponent();

        if (stepsLeft == 0) {
            while (auto* cnv = tabbar.getCurrentCanvas())
                tabbar.closeTab(cnv);
            signalDone(true);
            return;
        }

        beginTest(String(stepsLeft) + " -> random malformed patch");

        auto const patchText = generatePatchText();
        // Log the patch so a crash in this iteration can be reproduced
        logMessage(patchText);

        tabbar.openPatch(patchText);
        if (auto* cnv = tabbar.getCurrentCanvas())
            cnv->performSynchronise();

        while (auto* cnv = tabbar.getCurrentCanvas())
            tabbar.closeTab(cnv);

        Timer::callAfterDelay(1, [this, stepsLeft] {
            performStep(stepsLeft - 1);
        });
    }

    String generatePatchText()
    {
        static constexpr char const* objectNames[] = {
            "osc~", "dac~", "metro", "f", "t b b", "print", "tgl", "bng",
            "list store", "until", "select 1", "route a b", "text define",
            "not_an_object_xyz", "$1", "\\$0-foo"
        };

        String text = "#N canvas 100 100 600 400 12;\n";
        int openCanvases = 0;

        int const numLines = rng.nextInt(30) + 1;
        for (int i = 0; i < numLines; i++) {
            switch (rng.nextInt(12)) {
            case 0:
            case 1: // A (mostly) valid object line
                text += "#X obj " + randomCoord() + " " + randomCoord() + " "
                    + objectNames[rng.nextInt(static_cast<int>(std::size(objectNames)))] + ";\n";
                break;
            case 2: // A message box, sometimes with dollar args and escapes
                text += "#X msg " + randomCoord() + " " + randomCoord() + " set \\$1 " + randomToken(rng.nextInt(20) + 1) + ";\n";
                break;
            case 3: // Atom boxes
                text += rng.nextBool()
                    ? "#X floatatom " + randomCoord() + " " + randomCoord() + " 5 0 0 0 - - -;\n"
                    : "#X symbolatom " + randomCoord() + " " + randomCoord() + " 10 0 0 0 - - -;\n";
                break;
            case 4: // Connection with random, frequently out-of-range indices
                text += "#X connect " + String(rng.nextInt(20) - 4) + " " + String(rng.nextInt(8) - 2) + " "
                    + String(rng.nextInt(20) - 4) + " " + String(rng.nextInt(8) - 2) + ";\n";
                break;
            case 5: // Open a subcanvas (may never be restored)
                text += "#N canvas 0 0 300 200 sub" + String(i) + " 0;\n";
                openCanvases++;
                break;
            case 6: // Restore - sometimes without a canvas to restore
                text += "#X restore " + randomCoord() + " " + randomCoord() + " pd sub;\n";
                openCanvases--;
                break;
            case 7: // Array definition, sometimes followed by mismatched data
                text += "#X array fuzzarray" + String(i) + " " + String(rng.nextInt(1000) - 100) + " float " + String(rng.nextInt(4)) + ";\n";
                if (rng.nextBool())
                    text += "#A " + String(rng.nextInt(2000)) + " 0.1 0.2 0.3 0.4;\n";
                break;
            case 8: // Stray array data with no array in sight
                text += "#A 0 " + randomToken(rng.nextInt(40) + 1) + ";\n";
                break;
            case 9: // Graph-on-parent coords, valid or not
                text += "#X coords 0 1 100 -1 " + String(rng.nextInt(400) - 100) + " " + String(rng.nextInt(400) - 100) + " " + String(rng.nextInt(3)) + " 0 0;\n";
                break;
            case 10: // A single absurdly long atom
                text += "#X obj 10 10 " + randomToken(rng.nextInt(4000) + 1000) + ";\n";
                break;
            case 11: // Raw garbage line
                text += randomToken(rng.nextInt(60) + 1) + "\n";
                break;
            }
        }

        // Sometimes close the canvases that are still open, sometimes leave
        // the file truncated; sometimes pop one canvas too many
        int toClose = openCanvases + (rng.nextInt(4) == 0 ? 1 : 0);
        if (rng.nextInt(4) == 0)
            toClose = 0;
        for (int i = 0; i < toClose; i++)
            text += "#X restore 10 10 pd sub;\n";

        // Sometimes chop the end off mid-line
        if (rng.nextInt(4) == 0)
            text = text.dropLastCharacters(rng.nextInt(jmin(20, text.length())));

        return text;
    }

    String randomCoord()
    {
        switch (rng.nextInt(4)) {
        case 0:
            return String(rng.nextInt(600));
        case 1:
            return String(-rng.nextInt(100000));
        case 2:
            return String(rng.nextInt(std::numeric_limits<int>::max()));
        default:
            return String(rng.nextFloat() * 1e9f);
        }
    }

    String randomToken(int length)
    {
        static constexpr char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789$\\,;{}()[]<>~#-.";
        String result;
        result.preallocateBytes(length);
        for (int i = 0; i < length; i++)
            result += charset[rng.nextInt(sizeof(charset) - 1)];
        return result;
    }
};
