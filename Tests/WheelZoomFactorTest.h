#include "CanvasViewport.h"

// Covers CanvasViewport::wheelZoomFactor - the per-notch zoom multiplier from the
// discrete mouse wheel (Source/CanvasViewport.h, mouseWheelMove). The regression
// (issue #654) was that both directions used 1/(1-deltaY): 50% -> 62.1% -> 52.0%,
// so the wheel could never get back to where it started.
//
// This is pure arithmetic, so it runs without touching the canvas.

class WheelZoomFactorTest : public PlugDataUnitTest
{
public:
    WheelZoomFactorTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Wheel Zoom Factor Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Wheel notches cancel out");

        // 0.234 is a typical Windows notch (0.5 * 120 / 256)
        float scale = 1.0f;
        for (int i = 0; i < 5; i++)
            scale *= CanvasViewport::wheelZoomFactor(0.234f);
        for (int i = 0; i < 5; i++)
            scale *= CanvasViewport::wheelZoomFactor(-0.234f);
        expectWithinAbsoluteError(scale, 1.0f, 1.0e-4f, "five notches in then five out must return to the original zoom");
        expect(CanvasViewport::wheelZoomFactor(0.234f) > 1.0f, "wheel up must zoom in");
        expect(CanvasViewport::wheelZoomFactor(-0.234f) < 1.0f, "wheel down must zoom out");

        // |deltaY| can exceed 1 on Windows (jlimit(-1000, 1000, 0.5 * short) / 256)
        expectWithinAbsoluteError(CanvasViewport::wheelZoomFactor(0.0f), 1.0f, 1.0e-6f, "a zero delta must not zoom");
        expect(CanvasViewport::wheelZoomFactor(-3.9f) > 0.0f, "a large zoom-out delta must keep the factor positive");
        expect(CanvasViewport::wheelZoomFactor(-1.0f) > 0.0f, "a -1 delta must keep the factor positive");
        expect(CanvasViewport::wheelZoomFactor(1.0f) > 0.0f, "a +1 delta must keep the factor positive");
        expect(CanvasViewport::wheelZoomFactor(3.9f) > 0.0f, "a large zoom-in delta must keep the factor positive");

        signalDone(true);
    }
};
