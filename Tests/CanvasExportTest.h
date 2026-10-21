#pragma once

class CanvasExportTest final : public PlugDataUnitTest {
public:
    explicit CanvasExportTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Canvas Export Test") {}

private:
    Component::SafePointer<Canvas> testCanvas;
    int readinessChecks = 0;

    void perform() override
    {
        if (readinessChecks == 0) {
            testCanvas = Component::SafePointer<Canvas>(editor->getTabComponent().openPatch(String(
                "#N canvas 100 100 650 450 12;\n"
                "#X obj 40 50 osc~ 440;\n"
                "#X obj 300 180 *~ 0.5;\n"
                "#X connect 0 0 1 0;\n")));
            if (auto* canvas = testCanvas.getComponent())
                canvas->performSynchronise();
        }

        auto* canvas = testCanvas.getComponent();
        if (!canvas) {
            signalDone(false);
            return;
        }
        if (canvas->editor->nvgSurface.getMaxTextureSize() == 0) {
            if (++readinessChecks >= 100) {
                beginTest("Renderer initialization");
                expect(false, "renderer did not initialize within five seconds");
                signalDone(false);
                return;
            }
            Timer::callAfterDelay(50, [this] { perform(); });
            return;
        }

        beginTest("Full content and scale");

        auto const bounds = canvas->getExportBounds();
        expect(bounds.has_value(), "authored canvas must have export bounds");
        if (!bounds) {
            signalDone(false);
            return;
        }

        auto const directory = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("plugdata-canvas-export-test", "", false);
        if (!directory.createDirectory().wasOk()) {
            signalDone(false);
            return;
        }
        auto const output = directory.getChildFile("canvas.png");
        auto const scaledOutput = directory.getChildFile("canvas-2x.png");

        auto result = canvas->exportAsPng(output);
        expect(result.succeeded(), result.message);
        auto image = ImageFileFormat::loadFrom(output);
        auto const containsContent = [](Image const& exportImage) {
            auto const background = exportImage.getPixelAt(0, 0);
            for (int y = 0; y < exportImage.getHeight(); ++y)
                for (int x = 0; x < exportImage.getWidth(); ++x)
                    if (exportImage.getPixelAt(x, y) != background)
                        return true;
            return false;
        };
        expect(!image.isNull(), "export must be a decodable PNG");
        if (!image.isNull()) {
            expectEquals(image.getWidth(), bounds->getWidth());
            expectEquals(image.getHeight(), bounds->getHeight());
            expect(containsContent(image), "export must contain more than the canvas background");
        }

        auto const previousZoom = canvas->zoomScale.getValue();
        canvas->zoomScale.setValue(1.75f);
        expect(canvas->getExportBounds() == bounds, "viewport zoom must not alter logical export bounds");
        result = canvas->exportAsPng(scaledOutput, 2);
        expect(result.succeeded(), result.message);
        auto const scaledImage = ImageFileFormat::loadFrom(scaledOutput);
        expect(!scaledImage.isNull(), "scaled export must be a decodable PNG");
        if (!scaledImage.isNull()) {
            expectEquals(scaledImage.getWidth(), bounds->getWidth() * 2);
            expectEquals(scaledImage.getHeight(), bounds->getHeight() * 2);
        }
        result = canvas->exportAsPng(scaledOutput, 4);
        expect(result.succeeded(), result.message);
        auto const highImage = ImageFileFormat::loadFrom(scaledOutput);
        expect(!highImage.isNull(), "4x export must be a decodable PNG");
        if (!highImage.isNull()) {
            expectEquals(highImage.getWidth(), bounds->getWidth() * 4);
            expectEquals(highImage.getHeight(), bounds->getHeight() * 4);
            expect(containsContent(highImage), "4x export must contain authored pixels");
        }
        canvas->zoomScale.setValue(previousZoom);

        beginTest("Failed publication preserves previous PNG");
        MemoryBlock original;
        expect(output.loadFileAsData(original) && !original.isEmpty());
        canvas->failNextExportWriteForTesting();
        result = canvas->exportAsPng(output);
        expect(result.error == Canvas::ExportError::Failed, "injected write failure must not report success");
        MemoryBlock after;
        expect(output.loadFileAsData(after) && after == original, "existing PNG bytes must survive the failure");
        expectEquals(directory.getNumberOfChildFiles(File::findFiles), 2);

        canvas->failNextExportRenameForTesting();
        result = canvas->exportAsPng(output);
        expect(result.error == Canvas::ExportError::Failed, "injected rename failure must not report success");
        MemoryBlock afterRename;
        expect(output.loadFileAsData(afterRename) && afterRename == original, "rename failure must preserve the previous PNG");
        expectEquals(directory.getNumberOfChildFiles(File::findFiles), 2);

        auto const blocked = directory.getChildFile("blocked.png");
        expect(blocked.createDirectory().wasOk());
        result = canvas->exportAsPng(blocked);
        expect(result.error == Canvas::ExportError::Failed, "renaming over a directory must fail");
        expect(blocked.isDirectory(), "failed rename must preserve its destination");
        expectEquals(directory.getNumberOfChildFiles(File::findFiles), 2);
        blocked.deleteFile();

        result = canvas->exportAsPng(output);
        expect(result.succeeded(), "healthy retry must publish after failed write");

        output.deleteFile();
        scaledOutput.deleteFile();
        directory.deleteFile();
        signalDone(true);
    }
};
