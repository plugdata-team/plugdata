/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class Recorder {
public:
    Recorder()
        : backgroundThread("Audio Recorder Thread")
    {
        formatManager.registerBasicFormats();
        backgroundThread.startThread();
    }

    ~Recorder() { stopWriting(); }

    void prepare(double const sampleRateToUse, int const numChannelsToUse)
    {
        sampleRate = sampleRateToUse;
        numChannels = numChannelsToUse;
    }

    // Real-time safe. Call from the end of processBlock().
    void write(AudioBuffer<float> const& buffer)
    {
        ScopedLock const sl(writerLock);
        if (activeWriter != nullptr) {
            activeWriter->write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
            samplesRecorded += static_cast<int64>(buffer.getNumSamples());
        }
    }

    void toggleRecording(PluginEditor* editor)
    {
        if (editor && isRecording()) {
            stopWriting();
            showExportDialog(editor);
        } else {
            startRecordingToTemp();
        }
    }

    bool isRecording() const noexcept { return activeWriter != nullptr; }
    double getElapsedSeconds() const noexcept { return sampleRate > 0 ? samplesRecorded / sampleRate : 0.0; }

private:
    void startRecordingToTemp()
    {
        stopWriting();

        // Record to a temp WAV; on stop we re-encode to the user's chosen format.
        tempFile = std::make_unique<TemporaryFile>(".wav");
        startWriting(tempFile->getFile(), formatManager.findFormatForFileExtension("wav"));
    }

    void startWriting(File const& file, AudioFormat* format)
    {
        if (format == nullptr)
            return;

        file.deleteFile();
        auto fileStream = std::unique_ptr<OutputStream>(file.createOutputStream());
        if (fileStream == nullptr)
            return;

        int const bitDepth = format->getPossibleBitDepths().contains(24) ? 24 : 16;

        auto const options = AudioFormatWriterOptions { }
                                 .withSampleRate(sampleRate)
                                 .withNumChannels(numChannels)
                                 .withBitsPerSample(bitDepth);

        if (auto writer = format->createWriterFor(fileStream, options)) {
            threadedWriter.reset(new AudioFormatWriter::ThreadedWriter(
                writer.release(), backgroundThread, 32768));

            samplesRecorded = 0;
            ScopedLock const sl(writerLock);
            activeWriter = threadedWriter.get();
        }
    }

    void stopWriting()
    {
        {
            ScopedLock const sl(writerLock);
            activeWriter = nullptr;
        }
        threadedWriter.reset(); // flushes FIFO and finalises file
    }

    void showExportDialog(PluginEditor* editor)
    {
        if (tempFile == nullptr)
            return;

        auto recordingFile = tempFile->getFile();
        tempFile.release(); // don't let ~TemporaryFile delete it out from under the dialog

        Dialogs::showAudioExportDialog(&editor->openedDialog, editor, recordingFile);
    }

    AudioFormat* findFormatForFile(File const& file) const
    {
        for (int i = 0; i < formatManager.getNumKnownFormats(); ++i) {
            auto* f = formatManager.getKnownFormat(i);
            if (f->getFileExtensions().contains(file.getFileExtension(), true))
                return f;
        }
        return nullptr;
    }

    AudioFormatManager formatManager;
    TimeSliceThread backgroundThread;
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter;

    CriticalSection writerLock;
    AudioFormatWriter::ThreadedWriter* activeWriter = nullptr; // guarded by writerLock

    double sampleRate = 0.0;
    int numChannels = 0;
    std::atomic<int64> samplesRecorded { 0 };

    std::unique_ptr<TemporaryFile> tempFile;
};
