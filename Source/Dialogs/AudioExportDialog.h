class AudioExportDialog final : public Component
    , public Value::Listener
    , private Thread
    , private Timer {
public:
    AudioExportDialog(Dialog* parentDialog, File const& recordingToExport)
        : Thread("Audio Export")
        , dialog(parentDialog)
        , recording(recordingToExport)
    {
        formatManager.registerBasicFormats();

        for (int i = 0; i < formatManager.getNumKnownFormats(); i++) {
            formatNames.add(formatManager.getKnownFormat(i)->getFormatName());
            formatExtensions.add(formatManager.getKnownFormat(i)->getFileExtensions()[0]);
        }

        destinationValue = File::getSpecialLocation(File::userMusicDirectory).getChildFile("recording.wav").getFullPathName();
        formatValue = 1;
        sampleRateValue = 1;
        bitDepthValue = 2;
        normaliseValue = false;

        auto* destinationComponent = new PropertiesPanel::FilePathComponent("Destination", destinationValue);
        auto* formatComponent = new PropertiesPanel::ComboComponent("Format", formatValue, formatNames);
        auto* sampleRateComponent = new PropertiesPanel::ComboComponent("Sample rate", sampleRateValue, { "44100", "48000", "88200", "96000", "192000" });
        auto* bitDepthComponent = new PropertiesPanel::ComboComponent("Bit depth", bitDepthValue, { "16", "24", "32" });
        auto* normaliseComponent = new PropertiesPanel::BoolComponent("Normalize", normaliseValue, { "No", "Yes" });

        panel.addSection(" ", { destinationComponent, formatComponent, sampleRateComponent, bitDepthComponent, normaliseComponent });
        addAndMakeVisible(panel);

        auto const backgroundColour = PlugDataColours::panelBackgroundColour;
        cancelButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        cancelButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        cancelButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this] {
            if (isThreadRunning()) {
                signalThreadShouldExit();
                waitForThreadToExit(2000);
            }
            recording.deleteFile();
            dialog->closeDialog();
        };
        addAndMakeVisible(cancelButton);

        exportButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        exportButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        exportButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        exportButton.setButtonText("Export");
        exportButton.onClick = [this] { beginExport(); };
        addAndMakeVisible(exportButton);

        progressBar = std::make_unique<ProgressBar>(progress);
        progressBar->setTextToDisplay("Exporting...");
        addChildComponent(*progressBar);

        formatValue.addListener(this);

        setSize(520, 285);
    }

    ~AudioExportDialog() override
    {
        stopTimer();
        if (isThreadRunning()) {
            signalThreadShouldExit();
            waitForThreadToExit(2000);
        }
    }

    void valueChanged(Value& v) override
    {
        int const formatIdx = jlimit(0, (int)formatNames.size() - 1, getValue<int>(formatValue) - 1);
        auto formatExtension = formatExtensions[formatIdx];
        destinationValue = File(getValue<String>(destinationValue)).withFileExtension(formatExtension).getFullPathName();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().withTrimmedTop(16);
        auto buttonRow = bounds.removeFromBottom(56).reduced(8);
        int const buttonWidth = (buttonRow.getWidth() - 8) / 2;
        cancelButton.setBounds(buttonRow.removeFromLeft(buttonWidth).withSizeKeepingCentre(84, 26));
        buttonRow.removeFromLeft(8);
        exportButton.setBounds(buttonRow.withSizeKeepingCentre(84, 26));

        panel.setContentWidth(bounds.getWidth() - 32);
        panel.setBounds(bounds);
    }

private:
    StringArray formatNames;
    StringArray formatExtensions;
    static constexpr std::array sampleRates { 44100, 48000, 88200, 96000, 192000 };
    static constexpr std::array bitDepths { 16, 24, 32 };

    void beginExport()
    {
        // Snapshot current settings into members so the background thread can
        // read them without touching Value (which isn't thread-safe).
        int const formatIdx = jlimit(0, (int)formatNames.size() - 1, getValue<int>(formatValue) - 1);
        chosenFormatName = formatNames[formatIdx];
        chosenDestination = File(getValue<String>(destinationValue)).withFileExtension(formatExtensions[formatIdx]);
        chosenSampleRate = sampleRates[jlimit(0, (int)sampleRates.size() - 1, getValue<int>(sampleRateValue) - 1)];
        chosenBitDepth = bitDepths[jlimit(0, (int)bitDepths.size() - 1, getValue<int>(bitDepthValue) - 1)];
        chosenNormalise = getValue<bool>(normaliseValue);

        if (chosenDestination == File { })
            return;

        panel.setEnabled(false);
        exportButton.setEnabled(false);
        progress = 0.0;
        progressBar->setVisible(true);
        resized();

        startThread();
        startTimerHz(30);
    }

    void run() override
    {
        WavAudioFormat wav;
        std::unique_ptr<AudioFormatReader> reader(
            wav.createReaderFor(recording.createInputStream().release(), true));
        if (reader == nullptr)
            return;

        progress = 0.05;

        AudioBuffer<float> audio(static_cast<int>(reader->numChannels),
            static_cast<int>(reader->lengthInSamples));
        reader->read(&audio, 0, audio.getNumSamples(), 0, true, true);
        if (threadShouldExit())
            return;
        progress = 0.3;

        if (chosenNormalise) {
            float peak = 0.0f;
            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                peak = jmax(peak, audio.getMagnitude(ch, 0, audio.getNumSamples()));
            if (peak > 0.0f)
                audio.applyGain(1.0f / peak);
        }
        if (threadShouldExit())
            return;
        progress = 0.45;

        AudioBuffer<float> const* sourceBuffer = &audio;
        AudioBuffer<float> resampled;
        if (chosenSampleRate != static_cast<int>(reader->sampleRate)) {
            double const ratio = reader->sampleRate / static_cast<double>(chosenSampleRate);
            int const outSamples = static_cast<int>(audio.getNumSamples() / ratio);
            resampled.setSize(audio.getNumChannels(), outSamples);

            for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
                LagrangeInterpolator interp;
                interp.process(ratio, audio.getReadPointer(ch), resampled.getWritePointer(ch), outSamples);
                if (threadShouldExit())
                    return;
            }
            sourceBuffer = &resampled;
        }
        progress = 0.65;

        auto* format = formatForName(chosenFormatName);
        if (format == nullptr)
            return;

        chosenDestination.deleteFile();
        auto out = std::unique_ptr<OutputStream>(chosenDestination.createOutputStream());
        if (out == nullptr)
            return;

        auto const options = AudioFormatWriterOptions { }
                                 .withSampleRate(static_cast<double>(chosenSampleRate))
                                 .withNumChannels(sourceBuffer->getNumChannels())
                                 .withBitsPerSample(chosenBitDepth);

        auto writer = format->createWriterFor(out, options);
        if (writer == nullptr)
            return;
        out.release();

        int const totalSamples = sourceBuffer->getNumSamples();
        int const chunkSize = 8192;
        int samplesWritten = 0;
        while (samplesWritten < totalSamples) {
            if (threadShouldExit())
                return;
            int const thisChunk = jmin(chunkSize, totalSamples - samplesWritten);
            writer->writeFromAudioSampleBuffer(*sourceBuffer, samplesWritten, thisChunk);
            samplesWritten += thisChunk;
            progress = 0.65 + 0.35 * (samplesWritten / static_cast<double>(totalSamples));
        }
        progress = 1.0;
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto const titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(p);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(40, 0.0f, getWidth());

        Fonts::drawStyledText(g, "Export recording", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), PlugDataColours::panelTextColour, Semibold, 15, Justification::centred);
    }

    void timerCallback() override
    {
        if (!isThreadRunning()) {
            stopTimer();
            progressBar->setVisible(false);
            recording.deleteFile();
            dialog->closeDialog();
        }
    }

    AudioFormat* formatForName(String const& name) const
    {
        for (int i = 0; i < formatManager.getNumKnownFormats(); ++i) {
            auto* f = formatManager.getKnownFormat(i);
            if (f->getFormatName().containsIgnoreCase(name))
                return f;
        }
        return nullptr;
    }

    Dialog* dialog;
    File recording;

    AudioFormatManager formatManager;

    Value destinationValue, formatValue, sampleRateValue, bitDepthValue, normaliseValue;

    File chosenDestination;
    String chosenFormatName;
    int chosenSampleRate = 44100;
    int chosenBitDepth = 24;
    bool chosenNormalise = false;

    PropertiesPanel panel;
    TextButton cancelButton;
    TextButton exportButton;

    std::unique_ptr<ProgressBar> progressBar;
    double progress = 0.0;
};
