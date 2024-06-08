/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>

#include <utility>

class DeviceManagerLevelMeter : public Component
    , public Timer {

public:
    explicit DeviceManagerLevelMeter(AudioDeviceManager::LevelMeter::Ptr levelMeter)
        : levelGetter(std::move(levelMeter))
    {
        startTimerHz(20);
    }

    void timerCallback() override
    {
        if (isShowing()) {
            auto newLevel = (float)levelGetter->getCurrentLevel();

            if (std::abs(level - newLevel) > 0.002f || (newLevel == 0.0f && level != 0.0f)) {
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

struct CallbackComboProperty : public PropertiesPanelProperty {
    CallbackComboProperty(String const& propertyName, StringArray const& comboOptions, String const& currentOption, std::function<void(String)> const& onChange)
        : PropertiesPanelProperty(propertyName)
        , changeCallback(onChange)
        , options(comboOptions)
    {
        lastValue = currentOption;
        comboBox.addItemList(options, 1);
        comboBox.getProperties().set("Style", "Inspector");
        comboBox.setText(currentOption);
        comboBox.onChange = [this, onChange]() {
            // Combobox onChange is a bit too sensitive, so if we don't test this we can accidentally get into a feedback loop
            auto newValue = comboBox.getText();
            if (newValue != lastValue) {
                onChange(newValue);
            }
        };
        addAndMakeVisible(comboBox);
    }

    PropertiesPanelProperty* createCopy() override
    {
        return new CallbackComboProperty(getName(), options, lastValue, changeCallback);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
        comboBox.setBounds(bounds);
    }

    std::function<void(String)> changeCallback;
    StringArray options;
    String lastValue;
    ComboBox comboBox;
};

struct CallbackComboPropertyWithTestButton : public CallbackComboProperty {

    CallbackComboPropertyWithTestButton(String const& propertyName, StringArray const& options, String const& currentOption, std::function<void(String)> const& onChange, AudioDeviceManager& deviceManager)
        : CallbackComboProperty(propertyName, options, currentOption, onChange)
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

class ChannelToggleProperty : public PropertiesPanel::BoolComponent {
public:
    ChannelToggleProperty(String const& channelName, bool isEnabled, std::function<void(bool)> onClick)
        : PropertiesPanel::BoolComponent(channelName, isEnabled, { "Disabled", "Enabled" })
        , callback(std::move(onClick))
    {
        setPreferredHeight(28);
        repaint();
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
        backgroundShape.addRoundedRectangle(0, 0, getWidth(), getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTopCorner, roundTopCorner, roundBottomCorner, roundBottomCorner);

        g.setColour(findColour(PlugDataColour::panelForegroundColourId).darker(0.015f));
        g.fillPath(backgroundShape);

        auto buttonBounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2.0f - hideLabel));

        Path buttonShape;
        buttonShape.addRoundedRectangle(buttonBounds.getX(), buttonBounds.getY(), buttonBounds.getWidth(), buttonBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, false, roundTopCorner, false, roundBottomCorner);

        if (isDown || isHovered) {
            // Add some alpha to make it look good on any background...
            g.setColour(findColour(TextButton::buttonColourId).withAlpha(isDown ? 0.9f : 0.7f));
            g.fillPath(buttonShape);
        }

        Fonts::drawText(g, textOptions[isDown], buttonBounds, findColour(PlugDataColour::panelTextColourId), 14.0f, Justification::centred);

        // Paint label
        PropertiesPanelProperty::paint(g);
    }

    std::function<void(bool)> callback;
};

class StandaloneAudioSettings : public SettingsDialogPanel
    , private ChangeListener
    , public Value::Listener {

public:
    explicit StandaloneAudioSettings(AudioDeviceManager& audioDeviceManager)
        : inputLevelMeter(audioDeviceManager.getInputLevelGetter())
        , outputLevelMeter(audioDeviceManager.getOutputLevelGetter())
        , deviceManager(audioDeviceManager)
    {
        deviceManager.addChangeListener(this);
        addAndMakeVisible(audioPropertiesPanel);

        setup = deviceManager.getAudioDeviceSetup();
        updateDevices();

        // Expand the input/output channels is any channel higher than 8 is selected
        showAllInputChannels = setup.inputChannels.getHighestBit() > 8;
        showAllOutputChannels = setup.outputChannels.getHighestBit() > 8;

        showAllAudioDeviceValues.addListener(this);
        showAllAudioDeviceValues.referTo(SettingsFile::getInstance()->getPropertyAsValue("show_all_audio_device_rates"));
    }

    ~StandaloneAudioSettings() override
    {
        deviceManager.removeChangeListener(this);
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &audioPropertiesPanel;
    }

private:
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(showAllAudioDeviceValues))
            updateDevices();
    }

    void updateDevices()
    {
        OwnedArray<AudioIODeviceType> const& types = deviceManager.getAvailableDeviceTypes();

        auto& viewport = audioPropertiesPanel.getViewport();
        auto viewY = viewport.getViewPositionY();

        audioPropertiesPanel.clear();

        auto* currentType = deviceManager.getCurrentDeviceTypeObject();

        Array<PropertiesPanelProperty*> deviceConfigurationProperties;

        // Only show if there are multiple device types
        if (types.size() > 1) {
            // Get all device type names
            StringArray typeNames;
            for (auto type : types) {
                typeNames.add(type->getTypeName());
            }

            // Create property
            deviceConfigurationProperties.add(new CallbackComboProperty("Device Type", typeNames, deviceManager.getCurrentAudioDeviceType(), [this](String const& newType) {
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
            for (auto& rate : sampleRates) {
                auto rateAsString = String(rate);
                if (::getValue<bool>(showAllAudioDeviceValues) || standardSampleRates.contains(rateAsString)) {
                    sampleRateStrings.add(rateAsString);
                }
            }

            // if the audio device has no sample rates that are standard rates, list all rates (highly unlikely)
            if (sampleRateStrings.size() == 0) {
                for (auto& rate : sampleRates) {
                    sampleRateStrings.add(String(rate));
                }
            }

            // also make sure that setup.sampleRate is set to a supported rate
            if (!sampleRates.contains(setup.sampleRate)) {
                for (auto& rate : sampleRates) {
                    setup.sampleRate = rate;
                    break;
                }
            }

            StringArray bufferSizeStrings;
            for (auto& size : bufferSizes) {
                auto sizeAsString = String(size);
                if (::getValue<bool>(showAllAudioDeviceValues) || standardBufferSizes.contains(sizeAsString)) {
                    bufferSizeStrings.add(sizeAsString);
                }
            }
            // if the audio device has no buffer sizes that are powers of 2 (standard sizes), list all the sizes (highly unlikely)
            if (bufferSizeStrings.size() == 0) {
                for (auto& size : bufferSizes) {
                    bufferSizeStrings.add(String(size));
                }
            }

            // Add sample rate property
            deviceConfigurationProperties.add(new CallbackComboProperty("Sample rate", sampleRateStrings, String(setup.sampleRate), [this](String const& selected) {
                setup.sampleRate = selected.getFloatValue();
                updateConfig();
            }));

            // Add buffer size property
            deviceConfigurationProperties.add(new CallbackComboProperty("Buffer size", bufferSizeStrings, String(setup.bufferSize), [this](String const& selected) {
                setup.bufferSize = selected.getIntValue();
                updateConfig();
            }));
        }

        // This can possibly be empty if only one device type is available, and there is no device currently selected
        if (!deviceConfigurationProperties.isEmpty()) {
            audioPropertiesPanel.addSection("Device Configuration", deviceConfigurationProperties);
        }

        // Add output device selector
        Array<PropertiesPanelProperty*> outputProperties;
        StringArray const outputDevices(currentType->getDeviceNames(false));
        outputSelectorProperty = new CallbackComboPropertyWithTestButton(
            "Output Device", outputDevices, setup.outputDeviceName, [this](String selectedDevice) {
                setup.outputDeviceName = std::move(selectedDevice);
                updateConfig();
            },
            deviceManager);
        outputProperties.add(outputSelectorProperty);

        // Add input device selector
        Array<PropertiesPanelProperty*> inputProperties;
        StringArray const inputDevices(currentType->getDeviceNames(true));
        inputSelectorProperty = new CallbackComboProperty("Input Device", inputDevices, setup.inputDeviceName, [this](String selectedDevice) {
            setup.inputDeviceName = std::move(selectedDevice);
            updateConfig();
        });
        inputProperties.add(inputSelectorProperty);

        // Add channel toggles for input and output
        if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
            int idx = 0;
            for (auto channel : currentDevice->getInputChannelNames()) {
                if (!showAllInputChannels && idx > 8) {
                    inputProperties.add(new PropertiesPanel::ActionComponent([this]() {
                        showAllInputChannels = true;
                        updateDevices();
                    },
                        Icons::Down, "Show more input channels"));
                    break;
                }

                bool enabled = static_cast<bool>(setup.inputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, true);
                inputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled) {
                    setup.useDefaultInputChannels = false;
                    setup.inputChannels.setBit(idx, isEnabled);
                    updateConfig();
                }));
                idx++;
            }

            if (idx > 8 && showAllInputChannels) {
                inputProperties.add(new PropertiesPanel::ActionComponent([this]() {
                    showAllInputChannels = false;
                    updateDevices();
                },
                    Icons::Up, "Show fewer input channels"));
            }

            idx = 0;
            for (auto channel : currentDevice->getOutputChannelNames()) {
                if (!showAllOutputChannels && idx > 8) {
                    outputProperties.add(new PropertiesPanel::ActionComponent([this]() {
                        showAllOutputChannels = true;
                        updateDevices();
                    },
                        Icons::Down, "Show more output channels"));
                    break;
                }

                bool enabled = static_cast<bool>(setup.outputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, false);
                outputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled) {
                    setup.useDefaultOutputChannels = false;
                    setup.outputChannels.setBit(idx, isEnabled);
                    updateConfig();
                }));
                idx++;
            }

            if (idx > 8 && showAllOutputChannels) {
                outputProperties.add(new PropertiesPanel::ActionComponent([this]() {
                    showAllOutputChannels = false;
                    updateDevices();
                },
                    Icons::Up, "Show fewer output channels"));
            }
        }

        audioPropertiesPanel.addSection("Audio Output", outputProperties);
        audioPropertiesPanel.addSection("Audio Input", inputProperties);

        auto* outputSection = audioPropertiesPanel.getSectionByName("Audio Output");
        auto* inputSection = audioPropertiesPanel.getSectionByName("Audio Input");

        if (outputLevelMeter.getParentComponent()) {
            outputLevelMeter.getParentComponent()->removeChildComponent(&outputLevelMeter);
            inputLevelMeter.getParentComponent()->removeChildComponent(&inputLevelMeter);
        }

        outputSection->addAndMakeVisible(outputLevelMeter);
        inputSection->addAndMakeVisible(inputLevelMeter);

        viewport.setViewPosition(0, viewY);
    }

    // On macOS, output channel names will be "1" instead of "Output 1", so here we fix that
    static void fixShortChannelName(String& currentDeviceName, bool isInput)
    {
        auto prefix = isInput ? "Input " : "Output ";

        if (currentDeviceName.length() == 1) {
            currentDeviceName = prefix + currentDeviceName;
        }
    }

    // Updates the configuration, called when we change any settings
    void updateConfig()
    {

        String error = deviceManager.setAudioDeviceSetup(setup, true);

        if (error.isNotEmpty()) {
            std::cerr << error << std::endl;
        }
    }

    void changeListenerCallback(ChangeBroadcaster* origin) override
    {
        updateDevices();
    }

    void resized() override
    {
        audioPropertiesPanel.setBounds(getLocalBounds());

        auto [x, width] = audioPropertiesPanel.getContentXAndWidth();

        if (inputSelectorProperty) {
            inputLevelMeter.setBounds((x + width) - 60, 12, 60, 6);
        }

        if (outputSelectorProperty) {
            outputLevelMeter.setBounds((x + width) - 60, 12, 60, 6);
        }
    }

    DeviceManagerLevelMeter inputLevelMeter;
    DeviceManagerLevelMeter outputLevelMeter;

    // Used for positioning the levelmeters
    SafePointer<PropertiesPanelProperty> outputSelectorProperty;
    SafePointer<PropertiesPanelProperty> inputSelectorProperty;

    AudioDeviceManager::AudioDeviceSetup setup;

    AudioDeviceManager& deviceManager;
    PropertiesPanel audioPropertiesPanel;

    bool showAllInputChannels = false;
    bool showAllOutputChannels = false;

    Value showAllAudioDeviceValues;

    StringArray standardBufferSizes = { "16", "32", "64", "128", "256", "512", "1024", "2048" };
    StringArray standardSampleRates = { "44100", "48000", "88200", "96000", "176400", "192000" };
};

class DAWAudioSettings : public SettingsDialogPanel
    , public Value::Listener {

public:
    explicit DAWAudioSettings(PluginProcessor* p)
        : processor(p)
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();

        auto* proc = dynamic_cast<PluginProcessor*>(processor);
        tailLengthValue.referTo(proc->tailLength);

        latencyValue.addListener(this);

        latencyValue = proc->getLatencySamples() - pd::Instance::getBlockSize();

        latencyNumberBox = new PropertiesPanel::EditableComponent<int>("Latency (samples)", latencyValue);
        tailLengthNumberBox = new PropertiesPanel::EditableComponent<float>("Tail length (seconds)", tailLengthValue);

        dawSettingsPanel.addSection("Audio", { latencyNumberBox, tailLengthNumberBox });

        addAndMakeVisible(dawSettingsPanel);

        latencyNumberBox->setRangeMin(64);
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &dawSettingsPanel;
    }

    void resized() override
    {
        dawSettingsPanel.setBounds(getLocalBounds());
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(latencyValue)) {
            processor->performLatencyCompensationChange(getValue<int>(latencyValue));
        }
    }

    PluginProcessor* processor;

    Value latencyValue;
    Value tailLengthValue;

    PropertiesPanel dawSettingsPanel;

    PropertiesPanel::EditableComponent<int>* latencyNumberBox;
    PropertiesPanel::EditableComponent<float>* tailLengthNumberBox;
};
