/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>

class DeviceManagerLevelMeter : public Component
    , public Timer {

public:
    DeviceManagerLevelMeter(AudioDeviceManager::LevelMeter::Ptr levelMeter) : levelGetter(levelMeter)
    {
        startTimerHz(20);
    }

    void timerCallback() override
    {
        if (isShowing()) {
            auto newLevel = (float)levelGetter->getCurrentLevel();

            if (std::abs(level - newLevel) > 0.002f || newLevel == 0.0f && level != 0.0f) {
                level = newLevel;
                repaint();
            }
        } else {
            level = 0;
        }
    }

    void paint(Graphics& g) override
    {
        // add a bit of a skew to make the level more obvious
        auto skewedLevel = (float)std::exp(std::log(level) / 3.0);
        auto levelWidth = skewedLevel * getWidth();
        
        g.setColour(findColour(TextButton::buttonColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() / 2.0f);
        
        g.setColour(findColour(PlugDataColour::levelMeterActiveColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().withWidth(levelWidth), getHeight() / 2.0f);
    }

    AudioDeviceManager::LevelMeter::Ptr levelGetter;
    float level = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceManagerLevelMeter)
};

struct AudioDeviceSetupDetails
{
    AudioDeviceManager* manager;
    int minNumInputChannels, maxNumInputChannels;
    int minNumOutputChannels, maxNumOutputChannels;
    bool useStereoPairs;
};

//class AudioDeviceSettingsPanel : public Component
//    , private ChangeListener {
//public:
//    AudioDeviceSettingsPanel(AudioIODeviceType& t, AudioDeviceSetupDetails& setupDetails)
//        : type(t)
//        , setup(setupDetails)
//    {
//
//        type.scanForDevices();
//
//        setup.manager->addChangeListener(this);
//    }
//
//    ~AudioDeviceSettingsPanel() override
//    {
//        setup.manager->removeChangeListener(this);
//    }
//
//    void resized() override
//    {
//        if (auto* parent = findParentComponentOfClass<StandaloneAudioSettingsComponent>()) {
//            Rectangle<int> r(proportionOfWidth(0.35f), 0, proportionOfWidth(0.6f), 3000);
//
//            int const maxListBoxHeight = 100;
//            int const h = parent->getItemHeight();
//            int const space = h / 4;
//
//            if (outputDeviceDropDown != nullptr) {
//                auto row = r.removeFromTop(h);
//
//                if (testButton != nullptr) {
//                    testButton->changeWidthToFitText(h);
//                    testButton->setBounds(row.removeFromRight(testButton->getWidth()));
//                    row.removeFromRight(space);
//                }
//
//                outputDeviceDropDown->setBounds(row);
//                r.removeFromTop(space);
//            }
//
//            if (inputDeviceDropDown != nullptr) {
//                auto row = r.removeFromTop(h);
//
//                inputLevelMeter->setBounds(row.removeFromRight(testButton != nullptr ? testButton->getWidth() : row.getWidth() / 6));
//                row.removeFromRight(space);
//                inputDeviceDropDown->setBounds(row);
//                r.removeFromTop(space);
//            }
//
//            if (outputChanList != nullptr) {
//                outputChanList->setRowHeight(jmin(22, h));
//                outputChanList->setBounds(r.removeFromTop(outputChanList->getBestHeight(maxListBoxHeight)));
//                outputChanLabel->setBounds(0, outputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
//                r.removeFromTop(space);
//            }
//
//            if (inputChanList != nullptr) {
//                inputChanList->setRowHeight(jmin(22, h));
//                inputChanList->setBounds(r.removeFromTop(inputChanList->getBestHeight(maxListBoxHeight)));
//                inputChanLabel->setBounds(0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
//                r.removeFromTop(space);
//            }
//
//            r.removeFromTop(space * 2);
//
//            if (showAdvancedSettingsButton != nullptr
//                && sampleRateDropDown != nullptr && bufferSizeDropDown != nullptr) {
//                showAdvancedSettingsButton->setBounds(r.removeFromTop(h));
//                r.removeFromTop(space);
//                showAdvancedSettingsButton->changeWidthToFitText();
//            }
//
//            auto advancedSettingsVisible = showAdvancedSettingsButton == nullptr
//                || showAdvancedSettingsButton->getToggleState();
//
//            if (sampleRateDropDown != nullptr) {
//                sampleRateDropDown->setVisible(advancedSettingsVisible);
//
//                if (advancedSettingsVisible) {
//                    sampleRateDropDown->setBounds(r.removeFromTop(h));
//                    r.removeFromTop(space);
//                }
//            }
//
//            if (bufferSizeDropDown != nullptr) {
//                bufferSizeDropDown->setVisible(advancedSettingsVisible);
//
//                if (advancedSettingsVisible) {
//                    bufferSizeDropDown->setBounds(r.removeFromTop(h));
//                    r.removeFromTop(space);
//                }
//            }
//
//            r.removeFromTop(space);
//
//            if (showUIButton != nullptr || resetDeviceButton != nullptr) {
//                auto buttons = r.removeFromTop(h);
//
//                if (showUIButton != nullptr) {
//                    showUIButton->setVisible(advancedSettingsVisible);
//                    showUIButton->changeWidthToFitText(h);
//                    showUIButton->setBounds(buttons.removeFromLeft(showUIButton->getWidth()));
//                    buttons.removeFromLeft(space);
//                }
//
//                if (resetDeviceButton != nullptr) {
//                    resetDeviceButton->setVisible(advancedSettingsVisible);
//                    resetDeviceButton->changeWidthToFitText(h);
//                    resetDeviceButton->setBounds(buttons.removeFromLeft(resetDeviceButton->getWidth()));
//                }
//
//                r.removeFromTop(space);
//            }
//
//            setSize(getWidth(), r.getY());
//        } else {
//            jassertfalse;
//        }
//    }
//
//    void updateConfig(bool updateOutputDevice, bool updateInputDevice, bool updateSampleRate, bool updateBufferSize)
//    {
//        auto config = setup.manager->getAudioDeviceSetup();
//        String error;
//
//        if (updateOutputDevice || updateInputDevice) {
//            if (outputDeviceDropDown != nullptr)
//                config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0 ? String()
//                                                                                    : outputDeviceDropDown->getText();
//
//            if (inputDeviceDropDown != nullptr)
//                config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0 ? String()
//                                                                                  : inputDeviceDropDown->getText();
//
//            if (!type.hasSeparateInputsAndOutputs())
//                config.inputDeviceName = config.outputDeviceName;
//
//            if (updateInputDevice)
//                config.useDefaultInputChannels = true;
//            else
//                config.useDefaultOutputChannels = true;
//
//            error = setup.manager->setAudioDeviceSetup(config, true);
//
//            showCorrectDeviceName(inputDeviceDropDown.get(), true);
//            showCorrectDeviceName(outputDeviceDropDown.get(), false);
//
//            updateControlPanelButton();
//            resized();
//        } else if (updateSampleRate) {
//            if (sampleRateDropDown->getSelectedId() > 0) {
//                config.sampleRate = sampleRateDropDown->getSelectedId();
//                error = setup.manager->setAudioDeviceSetup(config, true);
//            }
//        } else if (updateBufferSize) {
//            if (bufferSizeDropDown->getSelectedId() > 0) {
//                config.bufferSize = bufferSizeDropDown->getSelectedId();
//                error = setup.manager->setAudioDeviceSetup(config, true);
//            }
//        }
//
//        if (error.isNotEmpty())
//            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
//                "Error when trying to open audio device!",
//                error);
//    }
//
//    bool showDeviceControlPanel()
//    {
//        if (auto* device = setup.manager->getCurrentAudioDevice()) {
//            Component modalWindow;
//            modalWindow.setOpaque(true);
//            modalWindow.addToDesktop(0);
//            modalWindow.enterModalState();
//
//            return device->showControlPanel();
//        }
//
//        return false;
//    }
//
//    void toggleAdvancedSettings()
//    {
//        showAdvancedSettingsButton->setButtonText((showAdvancedSettingsButton->getToggleState() ? "Hide " : "Show ")
//            + String("advanced settings..."));
//        resized();
//    }
//
//    void showDeviceUIPanel()
//    {
//        if (showDeviceControlPanel()) {
//            setup.manager->closeAudioDevice();
//            setup.manager->restartLastAudioDevice();
//            getTopLevelComponent()->toFront(true);
//        }
//    }
//
//    void playTestSound()
//    {
//        setup.manager->playTestSound();
//    }
//
//    void updateAllControls()
//    {
//        updateOutputsComboBox();
//        updateInputsComboBox();
//
//        updateControlPanelButton();
//        updateResetButton();
//
//        if (auto* currentDevice = setup.manager->getCurrentAudioDevice()) {
//            if (setup.maxNumOutputChannels > 0
//                && setup.minNumOutputChannels < setup.manager->getCurrentAudioDevice()->getOutputChannelNames().size()) {
//                if (outputChanList == nullptr) {
//                    outputChanList.reset(new ChannelSelectorListBox(setup, ChannelSelectorListBox::audioOutputType,
//                        "(no audio output channels found)"));
//                    addAndMakeVisible(outputChanList.get());
//                    outputChanLabel.reset(new Label({}, "Active output channels:"));
//                    outputChanLabel->setJustificationType(Justification::centredRight);
//                    outputChanLabel->attachToComponent(outputChanList.get(), true);
//                }
//
//                outputChanList->refresh();
//            } else {
//                outputChanLabel.reset();
//                outputChanList.reset();
//            }
//
//            if (setup.maxNumInputChannels > 0
//                && setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()->getInputChannelNames().size()) {
//                if (inputChanList == nullptr) {
//                    inputChanList.reset(new ChannelSelectorListBox(setup, ChannelSelectorListBox::audioInputType,
//                        "(no audio input channels found)"));
//                    addAndMakeVisible(inputChanList.get());
//                    inputChanLabel.reset(new Label({}, "Active input channels:"));
//                    inputChanLabel->setJustificationType(Justification::centredRight);
//                    inputChanLabel->attachToComponent(inputChanList.get(), true);
//                }
//
//                inputChanList->refresh();
//            } else {
//                inputChanLabel.reset();
//                inputChanList.reset();
//            }
//
//            updateSampleRateComboBox(currentDevice);
//            updateBufferSizeComboBox(currentDevice);
//        } else {
//            jassert(setup.manager->getCurrentAudioDevice() == nullptr); // not the correct device type!
//
//            inputChanLabel.reset();
//            outputChanLabel.reset();
//            sampleRateLabel.reset();
//            bufferSizeLabel.reset();
//
//            inputChanList.reset();
//            outputChanList.reset();
//            sampleRateDropDown.reset();
//            bufferSizeDropDown.reset();
//
//            if (outputDeviceDropDown != nullptr)
//                outputDeviceDropDown->setSelectedId(-1, dontSendNotification);
//
//            if (inputDeviceDropDown != nullptr)
//                inputDeviceDropDown->setSelectedId(-1, dontSendNotification);
//        }
//
//        sendLookAndFeelChange();
//        resized();
//        setSize(getWidth(), getLowestY() + 4);
//    }
//
//    void changeListenerCallback(ChangeBroadcaster*) override
//    {
//        updateAllControls();
//    }
//
//    void resetDevice()
//    {
//        setup.manager->closeAudioDevice();
//        setup.manager->restartLastAudioDevice();
//    }
//
//private:
//    AudioIODeviceType& type;
//    const AudioDeviceSetupDetails setup;
//
//    std::unique_ptr<ComboBox> outputDeviceDropDown, inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
//    std::unique_ptr<Label> outputDeviceLabel, inputDeviceLabel, sampleRateLabel, bufferSizeLabel, inputChanLabel, outputChanLabel;
//    std::unique_ptr<TextButton> testButton;
//    std::unique_ptr<Component> inputLevelMeter;
//    std::unique_ptr<TextButton> showUIButton, showAdvancedSettingsButton, resetDeviceButton;
//
//    void showCorrectDeviceName(ComboBox* box, bool isInput)
//    {
//        if (box != nullptr) {
//            auto* currentDevice = setup.manager->getCurrentAudioDevice();
//            auto index = type.getIndexOfDevice(currentDevice, isInput);
//
//            box->setSelectedId(index < 0 ? index : index + 1, dontSendNotification);
//
//            if (testButton != nullptr && !isInput)
//                testButton->setEnabled(index >= 0);
//        }
//    }
//
//    void addNamesToDeviceBox(ComboBox& combo, bool isInputs)
//    {
//        const StringArray devs(type.getDeviceNames(isInputs));
//
//        combo.clear(dontSendNotification);
//
//        for (int i = 0; i < devs.size(); ++i)
//            combo.addItem(devs[i], i + 1);
//
//        combo.addItem(getNoDeviceString(), -1);
//        combo.setSelectedId(-1, dontSendNotification);
//    }
//
//    int getLowestY() const
//    {
//        int y = 0;
//
//        for (auto* c : getChildren())
//            y = jmax(y, c->getBottom());
//
//        return y;
//    }
//
//    void updateControlPanelButton()
//    {
//        auto* currentDevice = setup.manager->getCurrentAudioDevice();
//        showUIButton.reset();
//
//        if (currentDevice != nullptr && currentDevice->hasControlPanel()) {
//            showUIButton.reset(new TextButton("Control Panel",
//                "Opens the device's own control panel"));
//            addAndMakeVisible(showUIButton.get());
//            showUIButton->onClick = [this] { showDeviceUIPanel(); };
//        }
//
//        resized();
//    }
//
//    void updateResetButton()
//    {
//        if (auto* currentDevice = setup.manager->getCurrentAudioDevice()) {
//            if (currentDevice->hasControlPanel()) {
//                if (resetDeviceButton == nullptr) {
//                    resetDeviceButton.reset(new TextButton("Reset Device",
//                        "Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel"));
//                    addAndMakeVisible(resetDeviceButton.get());
//                    resetDeviceButton->onClick = [this] { resetDevice(); };
//                    resized();
//                }
//
//                return;
//            }
//        }
//
//        resetDeviceButton.reset();
//    }
//
//    void updateOutputsComboBox()
//    {
//        if (setup.maxNumOutputChannels > 0 || !type.hasSeparateInputsAndOutputs()) {
//            if (outputDeviceDropDown == nullptr) {
//                outputDeviceDropDown.reset(new ComboBox());
//                outputDeviceDropDown->onChange = [this] { updateConfig(true, false, false, false); };
//
//                addAndMakeVisible(outputDeviceDropDown.get());
//
//                outputDeviceLabel.reset(new Label({}, type.hasSeparateInputsAndOutputs() ? "Output:" : "Device:"));
//                outputDeviceLabel->attachToComponent(outputDeviceDropDown.get(), true);
//
//                if (setup.maxNumOutputChannels > 0) {
//                    testButton.reset(new TextButton("Test", "Plays a test tone"));
//                    addAndMakeVisible(testButton.get());
//                    testButton->onClick = [this] { playTestSound(); };
//                }
//            }
//
//            addNamesToDeviceBox(*outputDeviceDropDown, false);
//        }
//
//        showCorrectDeviceName(outputDeviceDropDown.get(), false);
//    }
//
//    void updateInputsComboBox()
//    {
//        if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs()) {
//            if (inputDeviceDropDown == nullptr) {
//                inputDeviceDropDown.reset(new ComboBox());
//                inputDeviceDropDown->onChange = [this] { updateConfig(false, true, false, false); };
//                addAndMakeVisible(inputDeviceDropDown.get());
//
//                inputDeviceLabel.reset(new Label({}, "Input:"));
//                inputDeviceLabel->attachToComponent(inputDeviceDropDown.get(), true);
//
//                inputLevelMeter.reset(new DeviceManagerLevelMeter(*setup.manager));
//                addAndMakeVisible(inputLevelMeter.get());
//            }
//
//            addNamesToDeviceBox(*inputDeviceDropDown, true);
//        }
//
//        showCorrectDeviceName(inputDeviceDropDown.get(), true);
//    }
//
//    void updateSampleRateComboBox(AudioIODevice* currentDevice)
//    {
//        if (sampleRateDropDown == nullptr) {
//            sampleRateDropDown.reset(new ComboBox());
//            addAndMakeVisible(sampleRateDropDown.get());
//
//            sampleRateLabel.reset(new Label({}, "Sample rate:"));
//            sampleRateLabel->attachToComponent(sampleRateDropDown.get(), true);
//        } else {
//            sampleRateDropDown->clear();
//            sampleRateDropDown->onChange = nullptr;
//        }
//
//        auto const getFrequencyString = [](int rate) { return String(rate) + " Hz"; };
//
//        for (auto rate : currentDevice->getAvailableSampleRates()) {
//            auto const intRate = roundToInt(rate);
//            sampleRateDropDown->addItem(getFrequencyString(intRate), intRate);
//        }
//
//        auto const intRate = roundToInt(currentDevice->getCurrentSampleRate());
//        sampleRateDropDown->setText(getFrequencyString(intRate), dontSendNotification);
//
//        sampleRateDropDown->onChange = [this] { updateConfig(false, false, true, false); };
//    }
//
//    void updateBufferSizeComboBox(AudioIODevice* currentDevice)
//    {
//        if (bufferSizeDropDown == nullptr) {
//            bufferSizeDropDown.reset(new ComboBox());
//            addAndMakeVisible(bufferSizeDropDown.get());
//
//            bufferSizeLabel.reset(new Label({}, "Audio buffer size:"));
//            bufferSizeLabel->attachToComponent(bufferSizeDropDown.get(), true);
//        } else {
//            bufferSizeDropDown->clear();
//            bufferSizeDropDown->onChange = nullptr;
//        }
//
//        auto currentRate = currentDevice->getCurrentSampleRate();
//
//        if (currentRate == 0)
//            currentRate = 48000.0;
//
//        for (auto bs : currentDevice->getAvailableBufferSizes())
//            bufferSizeDropDown->addItem(String(bs) + " samples (" + String(bs * 1000.0 / currentRate, 1) + " ms)", bs);
//
//        bufferSizeDropDown->setSelectedId(currentDevice->getCurrentBufferSizeSamples(), dontSendNotification);
//        bufferSizeDropDown->onChange = [this] { updateConfig(false, false, false, true); };
//    }
//
//public:
//    class ChannelSelectorListBox : public ListBox
//        , private ListBoxModel {
//    public:
//        enum BoxType {
//            audioInputType,
//            audioOutputType
//        };
//
//        ChannelSelectorListBox(AudioDeviceSetupDetails const& setupDetails, BoxType boxType, String const& noItemsText)
//            : ListBox({}, nullptr)
//            , setup(setupDetails)
//            , type(boxType)
//            , noItemsMessage(noItemsText)
//        {
//            setColour(ListBox::backgroundColourId, Colours::transparentBlack);
//            refresh();
//            setModel(this);
//            setOutlineThickness(1);
//        }
//
//        void refresh()
//        {
//            items.clear();
//
//            if (auto* currentDevice = setup.manager->getCurrentAudioDevice()) {
//                if (type == audioInputType)
//                    items = currentDevice->getInputChannelNames();
//                else if (type == audioOutputType)
//                    items = currentDevice->getOutputChannelNames();
//
//                if (setup.useStereoPairs) {
//                    StringArray pairs;
//
//                    for (int i = 0; i < items.size(); i += 2) {
//                        auto& name = items[i];
//
//                        if (i + 1 >= items.size())
//                            pairs.add(name.trim());
//                        else
//                            pairs.add(getNameForChannelPair(name, items[i + 1]));
//                    }
//
//                    items = pairs;
//                }
//            }
//
//            updateContent();
//            repaint();
//        }
//
//        int getNumRows() override
//        {
//            return items.size();
//        }
//
//        void paintListBoxItem(int row, Graphics& g, int width, int height, bool) override
//        {
//            if (isPositiveAndBelow(row, items.size())) {
//                auto item = items[row];
//                bool enabled = false;
//                auto config = setup.manager->getAudioDeviceSetup();
//
//                if (setup.useStereoPairs) {
//                    if (type == audioInputType)
//                        enabled = config.inputChannels[row * 2] || config.inputChannels[row * 2 + 1];
//                    else if (type == audioOutputType)
//                        enabled = config.outputChannels[row * 2] || config.outputChannels[row * 2 + 1];
//                } else {
//                    if (type == audioInputType)
//                        enabled = config.inputChannels[row];
//                    else if (type == audioOutputType)
//                        enabled = config.outputChannels[row];
//                }
//
//                auto x = getTickX();
//                auto tickW = (float)height * 0.75f;
//
//                getLookAndFeel().drawTickBox(g, *this, (float)x - tickW, ((float)height - tickW) * 0.5f, tickW, tickW,
//                    enabled, true, true, false);
//
//                drawTextLayout(g, *this, item, { x + 5, 0, width - x - 5, height }, enabled);
//            }
//        }
//
//        void listBoxItemClicked(int row, MouseEvent const& e) override
//        {
//            selectRow(row);
//
//            if (e.x < getTickX())
//                flipEnablement(row);
//        }
//
//        void listBoxItemDoubleClicked(int row, MouseEvent const&) override
//        {
//            flipEnablement(row);
//        }
//
//        void returnKeyPressed(int row) override
//        {
//            flipEnablement(row);
//        }
//
//        void paint(Graphics& g) override
//        {
//            g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
//            g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);
//
//            if (items.isEmpty()) {
//                Fonts::drawText(g, noItemsMessage,
//                    0, 0, getWidth(), getHeight() / 2,
//                    Colours::grey, 0.5f * (float)getRowHeight(), Justification::centred);
//            }
//        }
//
//        void paintOverChildren(Graphics& g) override
//        {
//            g.setColour(findColour(PlugDataColour::outlineColourId));
//            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::defaultCornerRadius, 1.0f);
//        }
//
//        int getBestHeight(int maxHeight)
//        {
//            return getRowHeight() * jlimit(2, jmax(2, maxHeight / getRowHeight()), getNumRows())
//                + getOutlineThickness() * 2;
//        }
//
//    private:
//        const AudioDeviceSetupDetails setup;
//        const BoxType type;
//        const String noItemsMessage;
//        StringArray items;
//
//        static String getNameForChannelPair(String const& name1, String const& name2)
//        {
//            String commonBit;
//
//            for (int j = 0; j < name1.length(); ++j)
//                if (name1.substring(0, j).equalsIgnoreCase(name2.substring(0, j)))
//                    commonBit = name1.substring(0, j);
//
//            // Make sure we only split the name at a space, because otherwise, things
//            // like "input 11" + "input 12" would become "input 11 + 2"
//            while (commonBit.isNotEmpty() && !CharacterFunctions::isWhitespace(commonBit.getLastCharacter()))
//                commonBit = commonBit.dropLastCharacters(1);
//
//            return name1.trim() + " + " + name2.substring(commonBit.length()).trim();
//        }
//
//        void flipEnablement(int row)
//        {
//            jassert(type == audioInputType || type == audioOutputType);
//
//            if (isPositiveAndBelow(row, items.size())) {
//                auto config = setup.manager->getAudioDeviceSetup();
//
//                if (setup.useStereoPairs) {
//                    BigInteger bits;
//                    auto& original = (type == audioInputType ? config.inputChannels
//                                                             : config.outputChannels);
//
//                    for (int i = 0; i < 256; i += 2)
//                        bits.setBit(i / 2, original[i] || original[i + 1]);
//
//                    if (type == audioInputType) {
//                        config.useDefaultInputChannels = false;
//                        flipBit(bits, row, setup.minNumInputChannels / 2, setup.maxNumInputChannels / 2);
//                    } else {
//                        config.useDefaultOutputChannels = false;
//                        flipBit(bits, row, setup.minNumOutputChannels / 2, setup.maxNumOutputChannels / 2);
//                    }
//
//                    for (int i = 0; i < 256; ++i)
//                        original.setBit(i, bits[i / 2]);
//                } else {
//                    if (type == audioInputType) {
//                        config.useDefaultInputChannels = false;
//                        flipBit(config.inputChannels, row, setup.minNumInputChannels, setup.maxNumInputChannels);
//                    } else {
//                        config.useDefaultOutputChannels = false;
//                        flipBit(config.outputChannels, row, setup.minNumOutputChannels, setup.maxNumOutputChannels);
//                    }
//                }
//
//                setup.manager->setAudioDeviceSetup(config, true);
//            }
//        }
//
//        static void flipBit(BigInteger& chans, int index, int minNumber, int maxNumber)
//        {
//            auto numActive = chans.countNumberOfSetBits();
//
//            if (chans[index]) {
//                if (numActive > minNumber)
//                    chans.setBit(index, false);
//            } else {
//                if (numActive >= maxNumber) {
//                    auto firstActiveChan = chans.findNextSetBit(0);
//                    chans.clearBit(index > firstActiveChan ? firstActiveChan : chans.getHighestBit());
//                }
//
//                chans.setBit(index, true);
//            }
//        }
//
//        int getTickX() const
//        {
//            return getRowHeight();
//        }
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
//    };
//
//private:
//    std::unique_ptr<ChannelSelectorListBox> inputChanList, outputChanList;
//
//    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceSettingsPanel)
//}

struct CallbackComboProperty : public PropertiesPanel::Property {
    CallbackComboProperty(String const& propertyName, StringArray options, String currentOption, std::function<void(String)> onChange)
    : Property(propertyName)
    {
        comboBox.addItemList(options, 1);
        comboBox.getProperties().set("Style", "Inspector");
        comboBox.onChange = [this, onChange](){
            onChange(comboBox.getText());
        };
        comboBox.setText(currentOption);
        addAndMakeVisible(comboBox);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
        comboBox.setBounds(bounds);
    }

    ComboBox comboBox;
};

struct CallbackComboPropertyWithTestButton : public CallbackComboProperty {
    
    CallbackComboPropertyWithTestButton(String const& propertyName,  StringArray options, String currentOption, std::function<void(String)> onChange, AudioDeviceManager& deviceManager) : CallbackComboProperty(propertyName, options, currentOption, onChange)
    {
        testButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        testButton.onClick = [&deviceManager]() mutable {
            deviceManager.playTestSound();
        };
        
        addAndMakeVisible(testButton);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
        auto testButtonBounds = bounds.removeFromRight(70);
        testButton.setBounds(testButtonBounds.reduced(8, 5));
        
        comboBox.setBounds(bounds);
    }
    
    TextButton testButton = TextButton("Test");
};

class ChannelToggleProperty : public PropertiesPanel::BoolComponent
{
public:
    ChannelToggleProperty(String channelName, bool isEnabled, std::function<void(bool)> onClick) : PropertiesPanel::BoolComponent(channelName, {"Disabled", "Enabled"}), callback(onClick)
    {
        toggleStateValue = isEnabled;
        setPreferredHeight(28);
    }
    
    void valueChanged(Value& v) override
    {
        repaint();
        callback(getValue<bool>(v));
    }
    
    // Make background slightly darker to make it appear more like a subcomponent
    void paint(Graphics& g) override
    {
        bool isDown = getValue<bool>(toggleStateValue);
        bool isHovered = isMouseOver();
        
        Path backgroundShape;
        backgroundShape.addRoundedRectangle (0, 0, getWidth(), getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTopCorner, roundTopCorner, roundBottomCorner, roundBottomCorner);
        
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId).darker(0.015f));
        g.fillPath(backgroundShape);
        
        auto buttonBounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2 - hideLabel));
        
        Path buttonShape;
        buttonShape.addRoundedRectangle (buttonBounds.getX(), buttonBounds.getY(), buttonBounds.getWidth(), buttonBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, false, roundTopCorner, false, roundBottomCorner);
        
        if (isDown || isHovered) {
            // Add some alpha to make it look good on any background...
            g.setColour(findColour(TextButton::buttonColourId).withAlpha(isDown ? 0.9f : 0.7f));
            g.fillPath(buttonShape);
        }
        
        auto textColour = isDown ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);
        Fonts::drawText(g, textOptions[isDown], buttonBounds, textColour, 14.0f, Justification::centred);
        
        // Paint label
        Property::paint(g);
    }
    
    std::function<void(bool)> callback;
};

class StandaloneAudioSettings : public Component, private ChangeListener {
    
public:
    StandaloneAudioSettings(PluginProcessor* processor, AudioDeviceManager& audioDeviceManager) : deviceManager(audioDeviceManager), inputLevelMeter(audioDeviceManager.getInputLevelGetter()), outputLevelMeter(audioDeviceManager.getOutputLevelGetter())
    {
        deviceManager.addChangeListener(this);
        addAndMakeVisible(audioPropertiesPanel);
        
        setup = deviceManager.getAudioDeviceSetup();
        updateDevices();
        
        addAndMakeVisible(inputLevelMeter);
        addAndMakeVisible(outputLevelMeter);
    }
    
    ~StandaloneAudioSettings()
    {
        deviceManager.removeChangeListener(this);
    }
    
private:
    
    void updateDevices()
    {
        OwnedArray<AudioIODeviceType> const& types = deviceManager.getAvailableDeviceTypes();
        
        audioPropertiesPanel.clear();

        auto* currentType = deviceManager.getCurrentDeviceTypeObject();

        Array<PropertiesPanel::Property*> deviceConfigurationProperties;
        
        // Only show if there are multiple device types
        if (types.size() > 1) {
            // Get all device type names
            StringArray typeNames;
            for(int i = 0; i < types.size(); i++)
            {
                typeNames.add(types[i]->getTypeName());
            }
            
            // Create property
            deviceConfigurationProperties.add(new CallbackComboProperty("Device Type", typeNames, deviceManager.getCurrentAudioDeviceType(), [this](String newType){
                deviceManager.setCurrentAudioDeviceType(newType, true);
                updateConfig();
                updateDevices();
            }));
        }
        
        // Get all sample rate and buffer size options from the current audio device
        // If no device is enabled, don't show these
        if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
            auto sampleRates = currentDevice->getAvailableSampleRates();
            auto bufferSizes = currentDevice->getAvailableBufferSizes();
            
            StringArray sampleRateStrings;
            for(auto& rate : sampleRates)
            {
                sampleRateStrings.add(String(rate));
            }
            
            StringArray bufferSizeStrings;
            for(auto& size : bufferSizes)
            {
                bufferSizeStrings.add(String(size));
            }
            
            // Add sample rate property
            deviceConfigurationProperties.add(new CallbackComboProperty("Sample rate", sampleRateStrings, String(setup.sampleRate), [this](String selected){
                setup.sampleRate = selected.getFloatValue();
                updateConfig();
            }));
            
            // Add buffer size property
            deviceConfigurationProperties.add(new CallbackComboProperty("Buffer size", bufferSizeStrings, String(setup.bufferSize), [this](String selected){
                setup.bufferSize = selected.getIntValue();
                updateConfig();
            }));
        }
        
        // This can possibly be empty if only one device type is available, and there is no device currently selected
        if(!deviceConfigurationProperties.isEmpty())
        {
            audioPropertiesPanel.addSection("Device Configuration", deviceConfigurationProperties);
        }
        
        // Add output device selector
        Array<PropertiesPanel::Property*> outputProperties;
        const StringArray outputDevices(currentType->getDeviceNames(false));
        outputSelectorProperty = new CallbackComboPropertyWithTestButton("Output Device",  outputDevices, setup.outputDeviceName, [this](String selectedDevice){
            setup.outputDeviceName = selectedDevice;
            updateConfig();
        }, deviceManager);
        outputProperties.add(outputSelectorProperty);

        // Add input device selector
        Array<PropertiesPanel::Property*> inputProperties;
        const StringArray inputDevices(currentType->getDeviceNames(true));
        inputSelectorProperty = new CallbackComboProperty("Input Device", inputDevices, setup.inputDeviceName, [this](String selectedDevice){
            setup.inputDeviceName = selectedDevice;
            updateConfig();
        });
        inputProperties.add(inputSelectorProperty);
        
        // Add channel toggles for input and output
        if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
            int idx = 0;
            for(auto channel : currentDevice->getInputChannelNames())
            {
                bool enabled = static_cast<bool>(setup.inputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, true);
                inputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled){
                    setup.useDefaultInputChannels = false;
                    setup.inputChannels.setBit(idx, isEnabled);
                }));
                idx++;
            }
            
            idx = 0;
            for(auto channel : currentDevice->getOutputChannelNames())
            {
                bool enabled = static_cast<bool>(setup.outputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, false);
                outputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled){
                    setup.useDefaultOutputChannels = false;
                    setup.outputChannels.setBit(idx, isEnabled);
                }));
                idx++;
            }
        }
        
        audioPropertiesPanel.addSection("Audio Output", outputProperties);
        audioPropertiesPanel.addSection("Audio Input", inputProperties);
    }
    
    // On macOS, output channel names will be "1" instead of "Output 1", so here we fix that
    void fixShortChannelName(String& currentDeviceName, bool isInput)
    {
        auto prefix = isInput ? "Input " : "Output ";
        
        if(currentDeviceName.length() == 1)
        {
            currentDeviceName = prefix + currentDeviceName;
        }
    }
    
    // Updates the configuration, called when we change any settings
    void updateConfig()
    {
        
        String error = deviceManager.setAudioDeviceSetup(setup, true);
        
        if(error.isNotEmpty())
        {
            std::cerr << error << std::endl;
        }
    }
    
    void changeListenerCallback(ChangeBroadcaster* origin)
    {
        updateDevices();
    }
    
    void resized()
    {
        audioPropertiesPanel.setBounds(getLocalBounds());
        
        auto [x, width] = audioPropertiesPanel.getContentXAndWidth();
        
        if(inputSelectorProperty)
        {
            auto inputSelectorBounds = getLocalArea(nullptr, inputSelectorProperty->getScreenBounds());
            inputLevelMeter.setBounds((x + width) - 60, inputSelectorBounds.getY() - 16, 60, 6);
        }
        
        if(outputSelectorProperty)
        {
            auto outputSelectorBounds = getLocalArea(nullptr, outputSelectorProperty->getScreenBounds());
            outputLevelMeter.setBounds((x + width) - 60, outputSelectorBounds.getY() - 16, 60, 6);
        }
    }
    
    DeviceManagerLevelMeter inputLevelMeter;
    DeviceManagerLevelMeter outputLevelMeter;
    
    // Used for positioning the levelmeters
    PropertiesPanel::Property* outputSelectorProperty = nullptr;
    PropertiesPanel::Property* inputSelectorProperty = nullptr;
    
    AudioDeviceManager::AudioDeviceSetup setup;

    AudioDeviceManager& deviceManager;
    PropertiesPanel audioPropertiesPanel;
};


class DAWAudioSettings : public Component
    , public Value::Listener {

public:
    explicit DAWAudioSettings(AudioProcessor* p)
        : processor(p)
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();

        if (!settingsTree.hasProperty("NativeDialog")) {
            settingsTree.setProperty("NativeDialog", true, nullptr);
        }

        nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));

        nativeDialogToggle = std::make_unique<PropertiesPanel::BoolComponent>("Use system dialog", nativeDialogValue, StringArray{ "No", "Yes" });

        addAndMakeVisible(latencyNumberBox);
        addAndMakeVisible(tailLengthNumberBox);
        addAndMakeVisible(*nativeDialogToggle);

        dynamic_cast<DraggableNumber*>(latencyNumberBox.label.get())->setMinimum(64);

        auto* proc = dynamic_cast<PluginProcessor*>(processor);

        tailLengthValue.referTo(proc->tailLength);

        tailLengthValue.addListener(this);
        latencyValue.addListener(this);
        nativeDialogValue.addListener(this);

        latencyValue = proc->getLatencySamples();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        latencyNumberBox.setBounds(bounds.removeFromTop(23));
        tailLengthNumberBox.setBounds(bounds.removeFromTop(23));
        nativeDialogToggle->setBounds(bounds.removeFromTop(23));
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(latencyValue)) {
            processor->setLatencySamples(getValue<int>(latencyValue));
        }
    }
        
    AudioProcessor* processor;

    Value latencyValue;
    Value tailLengthValue;
    Value nativeDialogValue;

    PropertiesPanel::EditableComponent<int> latencyNumberBox = PropertiesPanel::EditableComponent<int>("Latency (samples)", latencyValue);
    PropertiesPanel::EditableComponent<float> tailLengthNumberBox = PropertiesPanel::EditableComponent<float>("Tail length (seconds)", tailLengthValue);

    std::unique_ptr<PropertiesPanel::BoolComponent> nativeDialogToggle;
};
