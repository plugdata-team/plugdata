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
        
        // Expand the input/output channels is any channel higher than 8 is selected
        showAllInputChannels = setup.inputChannels.getHighestBit() > 8;
        showAllOutputChannels = setup.outputChannels.getHighestBit() > 8;
        
        addAndMakeVisible(inputLevelMeter);
        addAndMakeVisible(outputLevelMeter);
        
        audioPropertiesPanel.onLayoutChange = [this](){
            resized();
        };
    }
    
    ~StandaloneAudioSettings()
    {
        deviceManager.removeChangeListener(this);
    }
    
private:
    
    void updateDevices()
    {
        OwnedArray<AudioIODeviceType> const& types = deviceManager.getAvailableDeviceTypes();
        
        auto& viewport = audioPropertiesPanel.getViewport();
        auto viewY = viewport.getViewPositionY();
        
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
                if(!showAllInputChannels && idx > 8)
                {
                    inputProperties.add(new PropertiesPanel::ActionComponent([this](){
                        showAllInputChannels = true;
                        updateDevices();
                        
                    }, Icons::Down, "Show more input channels"));
                    break;
                }
                
                bool enabled = static_cast<bool>(setup.inputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, true);
                inputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled){
                    setup.useDefaultInputChannels = false;
                    setup.inputChannels.setBit(idx, isEnabled);
                }));
                idx++;
            }
            
            if(idx > 8 && showAllInputChannels)
            {
                inputProperties.add(new PropertiesPanel::ActionComponent([this](){
                    showAllInputChannels = false;
                    updateDevices();
                    
                }, Icons::Up, "Show fewer input channels"));
            }
            
            idx = 0;
            for(auto channel : currentDevice->getOutputChannelNames())
            {
                if(!showAllOutputChannels && idx > 8)
                {
                    outputProperties.add(new PropertiesPanel::ActionComponent([this](){
                        showAllOutputChannels = true;
                        updateDevices();
                        
                    }, Icons::Down, "Show more output channels"));
                    break;
                }
                
                bool enabled = static_cast<bool>(setup.outputChannels.getBitRangeAsInt(idx, 1));
                fixShortChannelName(channel, false);
                outputProperties.add(new ChannelToggleProperty(channel, enabled, [this, idx](bool isEnabled){
                    setup.useDefaultOutputChannels = false;
                    setup.outputChannels.setBit(idx, isEnabled);
                }));
                idx++;
            }
            
            if(idx > 8 && showAllOutputChannels)
            {
                outputProperties.add(new PropertiesPanel::ActionComponent([this](){
                    showAllOutputChannels = false;
                    updateDevices();
                    
                }, Icons::Up, "Show fewer output channels"));
            }
        }

        
        audioPropertiesPanel.addSection("Audio Output", outputProperties);
        audioPropertiesPanel.addSection("Audio Input", inputProperties);
        
        viewport.setViewPosition(0, viewY);
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
    SafePointer<PropertiesPanel::Property> outputSelectorProperty;
    SafePointer<PropertiesPanel::Property> inputSelectorProperty;
    
    AudioDeviceManager::AudioDeviceSetup setup;

    AudioDeviceManager& deviceManager;
    PropertiesPanel audioPropertiesPanel;
    
    bool showAllInputChannels = false;
    bool showAllOutputChannels = false;
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
        
        auto* proc = dynamic_cast<PluginProcessor*>(processor);
        
        nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));
        tailLengthValue.referTo(proc->tailLength);
        
        tailLengthValue.addListener(this);
        latencyValue.addListener(this);
        nativeDialogValue.addListener(this);

        latencyValue = proc->getLatencySamples();
        
        latencyNumberBox = new PropertiesPanel::EditableComponent<int>("Latency (samples)", latencyValue);
        tailLengthNumberBox = new PropertiesPanel::EditableComponent<float>("Tail length (seconds)", tailLengthValue);
        nativeDialogToggle = new PropertiesPanel::BoolComponent("Use system dialog", nativeDialogValue, StringArray{ "No", "Yes" });
        
        dawSettingsPanel.addSection("Audio", {latencyNumberBox, tailLengthNumberBox});
        dawSettingsPanel.addSection("Other", {nativeDialogToggle});

        addAndMakeVisible(dawSettingsPanel);
        
        dynamic_cast<DraggableNumber*>(latencyNumberBox->label.get())->setMinimum(64);
    }

    void resized() override
    {
        dawSettingsPanel.setBounds(getLocalBounds());
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
        
    PropertiesPanel dawSettingsPanel;

    PropertiesPanel::EditableComponent<int>* latencyNumberBox;
        PropertiesPanel::EditableComponent<float>* tailLengthNumberBox;
    PropertiesPanel::BoolComponent* nativeDialogToggle;
};
