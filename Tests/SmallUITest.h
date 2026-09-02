#include "Components/ColourPicker.h"
#include "Dialogs/SnapSettings.h"
#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/Dialogs.h"

// Covers the small, previously-untouched UI panels:
//
//  - ColourPicker: shown as a callout, driven through its hue/saturation
//    space, brightness selector, sliders and hex field by clicking children
//  - SnapSettings and OverlayDisplaySettings: the statusbar callouts; every
//    toggle in them is clicked (settings writes go to the isolated test
//    settings copy, never the user's settings.json)
//  - AudioExportDialog: fed a freshly generated wav "recording", its
//    destination redirected to a temp file through the same label binding
//    the browse button uses, exported, then cancelled
//
// All callouts are modal, so each section dismisses them before moving on.

class SmallUITest : public PlugDataUnitTest
{
public:
    SmallUITest(PluginEditor* editor) : PlugDataUnitTest(editor, "Small UI Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Colour picker");

        pickedColour = Colours::black;
        auto const screenBounds = editor->getScreenBounds().withSizeKeepingCentre(10, 10);
        ColourPicker::getInstance()->show(editor, editor->getTopLevelComponent(), false, Colours::red, screenBounds, [this](Colour const c) {
            pickedColour = c;
        });

        Timer::callAfterDelay(100, [this] {
            auto* picker = ColourPicker::getInstance();
            TestHelpers::clickThrough(picker);
            picker->createComponentSnapshot(picker->getLocalBounds());

            ModalComponentManager::getInstance()->cancelAllModalComponents();
            ColourPicker::getInstance()->hideEyedropper();
            expect(pickedColour != Colours::black, "interacting with the picker must produce colour callbacks");

            showSnapSettings();
        });
    }

    void showSnapSettings()
    {
        beginTest("Snap settings");
        auto const screenBounds = editor->getScreenBounds().withSizeKeepingCentre(10, 10);
        SnapSettings::show(editor, screenBounds);

        Timer::callAfterDelay(100, [this] {
            clickModalCalloutChildren();
            ModalComponentManager::getInstance()->cancelAllModalComponents();
            showOverlaySettings();
        });
    }

    void showOverlaySettings()
    {
        beginTest("Overlay display settings");
        auto const screenBounds = editor->getScreenBounds().withSizeKeepingCentre(10, 10);
        OverlayDisplaySettings::show(editor, screenBounds);

        Timer::callAfterDelay(100, [this] {
            clickModalCalloutChildren();
            ModalComponentManager::getInstance()->cancelAllModalComponents();
            showAudioExport();
        });
    }

    void showAudioExport()
    {
        beginTest("Audio export dialog");

        // Generate a short wav "recording" like the one plugdata's recorder produces
        recordingFile = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_test_recording.wav");
        recordingFile.deleteFile();
        {
            WavAudioFormat format;
            FileOutputStream* stream = new FileOutputStream(recordingFile);
            std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(stream, 44100.0, 2, 16, {}, 0));
            AudioBuffer<float> buffer(2, 4410);
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                auto const sample = std::sin(i * 0.05f) * 0.5f;
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        }

        exportedFile = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_test_export.wav");
        exportedFile.deleteFile();

        Dialogs::showAudioExportDialog(&editor->openedDialog, editor, recordingFile);

        Timer::callAfterDelay(100, [this] {
            auto* dialog = editor->openedDialog.get();
            if (!dialog) {
                expect(false, "audio export dialog must open");
                signalDone(false);
                return;
            }

            // Redirect the destination by editing the path label, exactly like
            // the browse button's callback does; the label's text value refers
            // to the dialog's destination Value
            auto* pathLabel = TestHelpers::findLabelContaining(dialog, "recording");
            expect(pathLabel != nullptr, "the destination path label must be found");
            if (pathLabel)
                pathLabel->setText(exportedFile.getFullPathName(), sendNotification);

            auto* exportButton = TestHelpers::findButtonWithText(dialog, "Export");
            expect(exportButton != nullptr, "the export button must be found");
            if (pathLabel && exportButton)
                exportButton->onClick();

            // The export runs on a thread; give it time, then verify and cancel
            Timer::callAfterDelay(800, [this] {
                expect(exportedFile.existsAsFile() && exportedFile.getSize() > 0, "the export must write the destination file");

                if (auto* dialog = editor->openedDialog.get()) {
                    if (auto* cancelButton = TestHelpers::findButtonWithText(dialog, "Cancel"))
                        cancelButton->onClick();
                }

                Timer::callAfterDelay(100, [this] {
                    editor->openedDialog.reset(nullptr);
                    expect(!recordingFile.existsAsFile(), "cancelling must delete the temporary recording");
                    exportedFile.deleteFile();
                    signalDone(true);
                });
            });
        });
    }

    // Clicks everything inside the currently active modal component (the callout)
    static void clickModalCalloutChildren()
    {
        if (auto* modal = Component::getCurrentlyModalComponent()) {
            TestHelpers::clickThrough(modal);
            modal->createComponentSnapshot(modal->getLocalBounds());
        }
    }

    Colour pickedColour;
    File recordingFile, exportedFile;
};
