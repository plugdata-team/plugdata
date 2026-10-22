#pragma once

class CanvasExportTest final : public PlugDataUnitTest {
public:
    explicit CanvasExportTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Canvas Export Test") {}

private:
    Component::SafePointer<Canvas> testCanvas;
    Component::SafePointer<Canvas> graphCanvas;
    int readinessChecks = 0;
    double firstFrameDeadlineMs = 0.0;

    void perform() override
    {
        if (!testCanvas) {
            if (editor->nvgSurface.getMaxTextureSize() == 0) {
                if (++readinessChecks >= 100) {
                    beginTest("Renderer initialization");
                    expect(false, "renderer did not initialize within five seconds");
                    signalDone(false);
                    return;
                }
                Timer::callAfterDelay(50, [this] { perform(); });
                return;
            }

            testCanvas = Component::SafePointer<Canvas>(editor->getTabComponent().openPatch(String(
                "#N canvas 100 100 650 450 12;\n"
                "#X obj 40 50 osc~ 440;\n"
                "#X obj 300 180 *~ 0.5;\n"
                "#N canvas 0 0 200 140 (subpatch) 0;\n"
                "#X obj 10 20 tgl 25 0 empty empty graph_toggle 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
                "#X coords 0 1 100 -1 120 80 1 0 0;\n"
                "#X restore 450 40 graph;\n"
                "#X connect 0 0 1 0;\n")));
            auto* canvas = testCanvas.getComponent();
            if (!canvas) {
                signalDone(false);
                return;
            }
            canvas->performSynchronise();
            if (canvas->objects.size() != 3 || !canvas->objects[2]->gui) {
                signalDone(false);
                return;
            }
            auto* graphObject = canvas->objects[2];
            for (auto* child : graphObject->gui->getChildren()) {
                if (auto* embedded = dynamic_cast<Canvas*>(child))
                    graphCanvas = embedded;
            }
            if (!graphCanvas) {
                signalDone(false);
                return;
            }
            beginTest("Graph fixture");
            expectEquals(graphCanvas->objects.size(), size_t(1));
            expect(graphCanvas->viewport == nullptr, "embedded graph must not have a viewport");
            expect(graphCanvas->getExportBounds().has_value(), "graph child needs exportable content");
            firstFrameDeadlineMs = Time::getMillisecondCounterHiRes() + 10000.0;

            beginTest("First display required");
            auto const directory = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("plugdata-canvas-first-display-test", "", false);
            if (!directory.createDirectory().wasOk()) {
                signalDone(false);
                return;
            }
            auto const output = directory.getChildFile("before-display.png");
            auto const result = canvas->exportAsPng(output);
            expect(result.error == Canvas::ExportError::Failed && result.message == "canvas has not been displayed", "a never-displayed canvas must not export");
            expect(!output.exists(), "rejected export must not create a PNG");
            auto const graphResult = graphCanvas->exportAsPng(output);
            expect(graphResult.error == Canvas::ExportError::Failed && graphResult.message == "canvas has not been displayed", "unseen graph child must not export");
            directory.deleteFile();
            Timer::callAfterDelay(50, [this] { perform(); });
            return;
        }

        auto* canvas = testCanvas.getComponent();
        if (!canvas) {
            signalDone(false);
            return;
        }
        auto const bounds = canvas->getExportBounds();
        if (!bounds) {
            beginTest("Full content and scale");
            expect(false, "authored canvas must have export bounds");
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
        if (result.error == Canvas::ExportError::Failed && result.message == "canvas has not been displayed") {
            directory.deleteFile();
            if (Time::getMillisecondCounterHiRes() < firstFrameDeadlineMs) {
                Timer::callAfterDelay(50, [this] { perform(); });
                return;
            }
            beginTest("Full content and scale");
            expect(false, "visible canvas did not render a frame within ten seconds");
            signalDone(false);
            return;
        }

        beginTest("Full content and scale");
        expect(Time::getMillisecondCounterHiRes() < firstFrameDeadlineMs, "first visible frame must arrive within ten seconds");
        expect(result.succeeded(), result.message);
        if (!result.succeeded()) {
            output.deleteFile();
            directory.deleteFile();
            signalDone(false);
            return;
        }
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

        beginTest("Editor-only marks stay out of PNG exports");
        if (canvas->objects.size() == 3 && canvas->connections.size() == 1 && canvas->objects[0]->numOutputs > 0) {
            auto* outlet = canvas->objects[0]->iolets[canvas->objects[0]->numInputs];
            auto* connection = canvas->connections[0];
            expect(outlet->isVisible(), "targeted outlet must be visible");
            expect(!connection->isSegmented() && connection->getPath().getLength() > 24.0f, "connection must be long enough for a direction arrow");

            bool const previousTargeted = outlet->isTargeted();
            bool const previousIndex = canvas->showIndex;
            bool const previousDirection = canvas->showConnectionDirection;
            bool const previousObjectActivity = canvas->showObjectActivity;
            bool const previousConnectionActivity = canvas->showConnectionActivity;
            outlet->setTargeted(false);
            canvas->showIndex = false;
            canvas->showConnectionDirection = false;
            canvas->showObjectActivity = false;
            canvas->showConnectionActivity = false;

            result = canvas->exportAsPng(output);
            expect(result.succeeded(), "baseline export must succeed: " + result.message);
            expect(!outlet->isTargeted() && !canvas->showIndex && !canvas->showConnectionDirection
                    && !canvas->showObjectActivity && !canvas->showConnectionActivity && !canvas->isDrawingForExport(),
                "baseline overlay state must survive export");
            auto const baseline = result.succeeded() ? ImageFileFormat::loadFrom(output) : Image();
            expect(!baseline.isNull() && containsContent(baseline), "baseline must contain authored pixels");

            auto const compareExport = [&](String const& state) {
                auto const overlayResult = canvas->exportAsPng(output);
                expect(overlayResult.succeeded(), state + " export must succeed: " + overlayResult.message);
                expect(outlet->isTargeted() == (state == "targeted iolet") && canvas->showIndex == (state == "object index")
                        && canvas->showConnectionDirection == (state == "connection direction") && !canvas->isDrawingForExport(),
                    state + " overlay state must survive export");
                if (!overlayResult.succeeded())
                    return;
                auto const current = ImageFileFormat::loadFrom(output);
                expect(!current.isNull() && current.getBounds() == baseline.getBounds(), state + " export must match baseline dimensions");
                if (current.isNull() || current.getBounds() != baseline.getBounds())
                    return;

                int changedPixels = 0;
                for (int y = 0; y < baseline.getHeight(); ++y) {
                    for (int x = 0; x < baseline.getWidth(); ++x) {
                        auto const before = baseline.getPixelAt(x, y);
                        auto const after = current.getPixelAt(x, y);
                        if (std::abs(int(before.getRed()) - int(after.getRed())) > 8
                            || std::abs(int(before.getGreen()) - int(after.getGreen())) > 8
                            || std::abs(int(before.getBlue()) - int(after.getBlue())) > 8
                            || std::abs(int(before.getAlpha()) - int(after.getAlpha())) > 8)
                            ++changedPixels;
                    }
                }
                expectEquals(changedPixels, 0, state + " must not alter authored pixels");
            };

            if (!baseline.isNull()) {
                outlet->setTargeted(true);
                expect(outlet->isTargeted(), "iolet hover must be active");
                compareExport("targeted iolet");
                outlet->setTargeted(false);

                canvas->showIndex = true;
                expect(canvas->shouldShowIndex(), "object index overlay must be active");
                compareExport("object index");
                canvas->showIndex = false;

                canvas->showConnectionDirection = true;
                expect(canvas->shouldShowConnectionDirection(), "direction overlay must be active");
                compareExport("connection direction");
            }

            outlet->setTargeted(previousTargeted);
            canvas->showIndex = previousIndex;
            canvas->showConnectionDirection = previousDirection;
            canvas->showObjectActivity = previousObjectActivity;
            canvas->showConnectionActivity = previousConnectionActivity;
            expect(outlet->isTargeted() == previousTargeted && canvas->showIndex == previousIndex && canvas->showConnectionDirection == previousDirection
                    && canvas->showObjectActivity == previousObjectActivity && canvas->showConnectionActivity == previousConnectionActivity
                    && !canvas->isDrawingForExport(),
                "editor overlay state must be restored after export");
        } else {
            expect(false, "overlay test requires two connected objects with an outlet and a graph");
        }

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

        beginTest("Previously displayed canvas remains exportable");
        canvas->setVisible(false);
        expect(!canvas->isShowing(), "canvas must be hidden for the persistence check");
        result = canvas->exportAsPng(output);
        expect(result.succeeded(), "a previously displayed canvas must still export while hidden");
        canvas->setVisible(true);

        output.deleteFile();
        scaledOutput.deleteFile();
        directory.deleteFile();

        beginTest("Graph child requires normal display");
        auto* graphObject = canvas->objects[2];
        graphObject->setType("graph", pd::WeakReference(graphObject->getPointer(), canvas->pd));
        graphCanvas = nullptr;
        for (auto* child : graphObject->gui->getChildren()) {
            if (auto* embedded = dynamic_cast<Canvas*>(child))
                graphCanvas = embedded;
        }
        if (!graphCanvas) {
            expect(false, "graph object must recreate its embedded canvas");
            signalDone(false);
            return;
        }
        auto const graphDirectory = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("plugdata-graph-display-test", "", false);
        if (!graphDirectory.createDirectory().wasOk()) {
            signalDone(false);
            return;
        }
        auto const parentOutput = graphDirectory.getChildFile("parent.png");
        auto const childOutput = graphDirectory.getChildFile("child.png");
        result = graphCanvas->exportAsPng(childOutput);
        expect(result.error == Canvas::ExportError::Failed && result.message == "canvas has not been displayed", "new graph child must be unseen");
        result = canvas->exportAsPng(parentOutput);
        expect(result.succeeded(), "parent export with a graph must succeed: " + result.message);
        result = graphCanvas->exportAsPng(childOutput);
        expect(result.error == Canvas::ExportError::Failed && result.message == "canvas has not been displayed", "parent export must not display its unseen child");
        expect(!childOutput.exists(), "rejected child export must not create a PNG");
        parentOutput.deleteFile();
        graphDirectory.deleteFile();
        firstFrameDeadlineMs = Time::getMillisecondCounterHiRes() + 10000.0;
        Timer::callAfterDelay(50, [this] { checkGraphDisplay(); });
    }

    void checkGraphDisplay()
    {
        if (!graphCanvas) {
            signalDone(false);
            return;
        }
        auto const directory = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("plugdata-graph-visible-test", "", false);
        if (!directory.createDirectory().wasOk()) {
            signalDone(false);
            return;
        }
        auto const output = directory.getChildFile("child.png");
        auto const result = graphCanvas->exportAsPng(output);
        if (result.error == Canvas::ExportError::Failed && result.message == "canvas has not been displayed"
            && Time::getMillisecondCounterHiRes() < firstFrameDeadlineMs) {
            directory.deleteFile();
            Timer::callAfterDelay(50, [this] { checkGraphDisplay(); });
            return;
        }
        expect(Time::getMillisecondCounterHiRes() < firstFrameDeadlineMs, "graph child must render within ten seconds");
        expect(result.succeeded(), "visible graph child must export: " + result.message);
        expect(output.existsAsFile(), "visible child export must create a PNG");
        output.deleteFile();
        directory.deleteFile();
        signalDone(true);
    }
};
