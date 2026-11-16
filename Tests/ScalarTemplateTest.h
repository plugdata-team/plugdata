// Saved data can define a template without a live [struct] drawing canvas.
// Opening its scalar must not dereference template_findcanvas()'s null result.
class ScalarTemplateTest : public PlugDataUnitTest {
public:
    ScalarTemplateTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Scalar Template Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Open scalar data without a drawing canvas");
        auto& tabs = editor->getTabComponent();
        tabs.openPatch(String(
            "#N struct regression-orphan-scalar float x float y;\n"
            "#N canvas 0 0 400 300 12;\n"
            "#X scalar regression-orphan-scalar 10 20 \\;;\n"));
        auto* canvas = tabs.getCurrentCanvas();
        expect(canvas != nullptr);
        if (canvas) {
            expectEquals(static_cast<int>(canvas->objects.size()), 1);
            expect(canvas->drawables.empty());
            tabs.closeTab(canvas);
        }
        signalDone(canvas != nullptr);
    }
};
