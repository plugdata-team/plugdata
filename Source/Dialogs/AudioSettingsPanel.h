/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


struct DAWAudioSettings : public Component, public Value::Listener {
    explicit DAWAudioSettings(AudioProcessor& p)
    : processor(p)
    {
        addAndMakeVisible(latencyNumberBox);
        addAndMakeVisible(tailLengthNumberBox);
        addAndMakeVisible(nativeDialogToggle);
        
        dynamic_cast<DraggableNumber*>(latencyNumberBox.label.get())->setMinimum(64);
        
        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        auto& settingsTree = dynamic_cast<PlugDataAudioProcessor&>(p).settingsTree;
        
        if(!settingsTree.hasProperty("NativeDialog")) {
            settingsTree.setProperty("NativeDialog", true, nullptr);
        }
        
        tailLengthValue.referTo(proc->tailLength);
        nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));
        
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
        nativeDialogToggle.setBounds(bounds.removeFromTop(23));
    }
    
    
    void valueChanged(Value& v) override
    {
        if(v.refersToSameSourceAs(latencyValue)) {
            processor.setLatencySamples(static_cast<int>(latencyValue.getValue()));
        }
    }
    
    AudioProcessor& processor;
    
    Value latencyValue;
    Value tailLengthValue;
    Value nativeDialogValue;
    
    PropertiesPanel::EditableComponent<int> latencyNumberBox = PropertiesPanel::EditableComponent<int>("Latency (samples)", latencyValue);
    PropertiesPanel::EditableComponent<float> tailLengthNumberBox = PropertiesPanel::EditableComponent<float>("Tail Length (seconds)", tailLengthValue);
    PropertiesPanel::BoolComponent nativeDialogToggle = PropertiesPanel::BoolComponent("Use Native Dialog", tailLengthValue,  {"No", "Yes"});
};



struct SimpleDeviceManagerInputLevelMeter  : public Component,
public Timer
{
    SimpleDeviceManagerInputLevelMeter (AudioDeviceManager& m)  : manager (m)
    {
        startTimerHz (20);
        inputLevelGetter = manager.getInputLevelGetter();
    }
    
    void timerCallback() override
    {
        if (isShowing())
        {
            auto newLevel = (float) inputLevelGetter->getCurrentLevel();
            
            if (std::abs (level - newLevel) > 0.005f)
            {
                level = newLevel;
                repaint();
            }
        }
        else
        {
            level = 0;
        }
    }
    
    void paint (Graphics& g) override
    {
        // (add a bit of a skew to make the level more obvious)
        getLookAndFeel().drawLevelMeter (g, getWidth(), getHeight(),
                                         (float) std::exp (std::log (level) / 3.0));
    }
    
    AudioDeviceManager& manager;
    AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
    float level = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDeviceManagerInputLevelMeter)
};

static void drawTextLayout (Graphics& g, Component& owner, StringRef text, const Rectangle<int>& textBounds, bool enabled)
{
    const auto textColour = owner.findColour (ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f);
    
    AttributedString attributedString { text };
    attributedString.setColour (textColour);
    attributedString.setFont ((float) textBounds.getHeight() * 0.6f);
    attributedString.setJustification (Justification::centredLeft);
    attributedString.setWordWrap (AttributedString::WordWrap::none);
    
    TextLayout textLayout;
    textLayout.createLayout (attributedString,
                             (float) textBounds.getWidth(),
                             (float) textBounds.getHeight());
    textLayout.draw (g, textBounds.toFloat());
}


//==============================================================================
class MidiInputSelectorComponentListBox  : public ListBox,
private ListBoxModel
{
public:
    MidiInputSelectorComponentListBox (AudioDeviceManager& dm, const String& noItems)
    : ListBox ({}, nullptr),
    deviceManager (dm),
    noItemsMessage (noItems)
    {
        updateDevices();
        setModel (this);
        setOutlineThickness (1);
    }
    
    void updateDevices()
    {
        items = MidiInput::getAvailableDevices();
    }
    
    int getNumRows() override
    {
        return items.size();
    }
    
    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                           .withMultipliedAlpha (0.3f));
            
            auto item = items[row];
            bool enabled = deviceManager.isMidiInputDeviceEnabled (item.identifier);
            
            auto x = getTickX();
            auto tickW = (float) height * 0.75f;
            
            getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                          enabled, true, true, false);
            
            drawTextLayout (g, *this, item.name, { x + 5, 0, width - x - 5, height }, enabled);
        }
    }
    
    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        selectRow (row);
        
        if (e.x < getTickX())
            flipEnablement (row);
    }
    
    void listBoxItemDoubleClicked (int row, const MouseEvent&) override
    {
        flipEnablement (row);
    }
    
    void returnKeyPressed (int row) override
    {
        flipEnablement (row);
    }
    
    void paint (Graphics& g) override
    {
        ListBox::paint (g);
        
        if (items.isEmpty())
        {
            g.setColour (Colours::grey);
            g.setFont (0.5f * (float) getRowHeight());
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        Justification::centred, true);
        }
    }
    
    int getBestHeight (int preferredHeight)
    {
        auto extra = getOutlineThickness() * 2;
        
        return jmax (getRowHeight() * 2 + extra,
                     jmin (getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }
    
private:
    //==============================================================================
    AudioDeviceManager& deviceManager;
    const String noItemsMessage;
    Array<MidiDeviceInfo> items;
    
    void flipEnablement (const int row)
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            auto identifier = items[row].identifier;
            deviceManager.setMidiInputDeviceEnabled (identifier, ! deviceManager.isMidiInputDeviceEnabled (identifier));
        }
    }
    
    int getTickX() const
    {
        return getRowHeight();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponentListBox)
};


//==============================================================================
struct AudioDeviceSetupDetails
{
    AudioDeviceManager* manager;
    int minNumInputChannels, maxNumInputChannels;
    int minNumOutputChannels, maxNumOutputChannels;
    bool useStereoPairs;
};

static String getNoDeviceString()   { return "<< " + TRANS("none") + " >>"; }


class StandaloneAudioSettings : public Component, private ChangeListener
{
public:
    //==============================================================================
    /** Creates the component.
     
     If your app needs only output channels, you might ask for a maximum of 0 input
     channels, and the component won't display any options for choosing the input
     channels. And likewise if you're doing an input-only app.
     
     @param deviceManager            the device manager that this component should control
     @param minAudioInputChannels    the minimum number of audio input channels that the application needs
     @param maxAudioInputChannels    the maximum number of audio input channels that the application needs
     @param minAudioOutputChannels   the minimum number of audio output channels that the application needs
     @param maxAudioOutputChannels   the maximum number of audio output channels that the application needs
     @param showMidiInputOptions     if true, the component will allow the user to select which midi inputs are enabled
     @param showMidiOutputSelector   if true, the component will let the user choose a default midi output device
     @param showChannelsAsStereoPairs    if true, channels will be treated as pairs; if false, channels will be
     treated as a set of separate mono channels.
     @param hideAdvancedOptionsWithButton    if true, only the minimum amount of UI components
     are shown, with an "advanced" button that shows the rest of them
     */
    StandaloneAudioSettings (AudioDeviceManager& dm,
                                  int minInputChannelsToUse,
                                  int maxInputChannelsToUse,
                                  int minOutputChannelsToUse,
                                  int maxOutputChannelsToUse,
                                  bool showMidiInputOptions,
                                  bool showMidiOutputSelector,
                                  bool showChannelsAsStereoPairsToUse,
                                  bool hideAdvancedOptionsWithButtonToUse)
    : deviceManager (dm),
    itemHeight (24),
    minOutputChannels (minOutputChannelsToUse),
    maxOutputChannels (maxOutputChannelsToUse),
    minInputChannels (minInputChannelsToUse),
    maxInputChannels (maxInputChannelsToUse),
    showChannelsAsStereoPairs (showChannelsAsStereoPairsToUse),
    hideAdvancedOptionsWithButton (hideAdvancedOptionsWithButtonToUse)
    {
        jassert (minOutputChannels >= 0 && minOutputChannels <= maxOutputChannels);
        jassert (minInputChannels >= 0 && minInputChannels <= maxInputChannels);
        
        const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();
        
        if (types.size() > 1)
        {
            deviceTypeDropDown.reset (new ComboBox());
            
            for (int i = 0; i < types.size(); ++i)
                deviceTypeDropDown->addItem (types.getUnchecked(i)->getTypeName(), i + 1);
            
            addAndMakeVisible (deviceTypeDropDown.get());
            deviceTypeDropDown->onChange = [this] { updateDeviceType(); };
            
            deviceTypeDropDownLabel.reset (new Label ({}, TRANS("Audio device type:")));
            deviceTypeDropDownLabel->setJustificationType (Justification::centredRight);
            deviceTypeDropDownLabel->attachToComponent (deviceTypeDropDown.get(), true);
        }
        
        if (showMidiInputOptions)
        {
            midiInputsList.reset (new MidiInputSelectorComponentListBox (deviceManager,
                                                                         "(" + TRANS("No MIDI inputs available") + ")"));
            addAndMakeVisible (midiInputsList.get());
            
            midiInputsLabel.reset (new Label ({}, TRANS ("Active MIDI inputs:")));
            midiInputsLabel->setJustificationType (Justification::topRight);
            midiInputsLabel->attachToComponent (midiInputsList.get(), true);
            
            if (BluetoothMidiDevicePairingDialogue::isAvailable())
            {
                bluetoothButton.reset (new TextButton (TRANS("Bluetooth MIDI"), TRANS("Scan for bluetooth MIDI devices")));
                addAndMakeVisible (bluetoothButton.get());
                bluetoothButton->onClick = [this] { handleBluetoothButton(); };
            }
        }
        else
        {
            midiInputsList.reset();
            midiInputsLabel.reset();
            bluetoothButton.reset();
        }
        
        if (showMidiOutputSelector)
        {
            midiOutputSelector.reset (new ComboBox());
            addAndMakeVisible (midiOutputSelector.get());
            midiOutputSelector->onChange = [this] { updateMidiOutput(); };
            
            midiOutputLabel.reset (new Label ("lm", TRANS("MIDI Output:")));
            midiOutputLabel->attachToComponent (midiOutputSelector.get(), true);
        }
        else
        {
            midiOutputSelector.reset();
            midiOutputLabel.reset();
        }
        
        deviceManager.addChangeListener (this);
        updateAllControls();
    }
    /** Destructor */
    ~StandaloneAudioSettings() override
    {
        deviceManager.removeChangeListener (this);
    }
    
    /** The device manager that this component is controlling */
    AudioDeviceManager& deviceManager;
    
    /** Sets the standard height used for items in the panel. */
    void setItemHeight (int newItemHeight)
    {
        itemHeight = newItemHeight;
        resized();
    }
    
    /** Returns the standard height used for items in the panel. */
    int getItemHeight() const noexcept      { return itemHeight; }
    
    /** Returns the ListBox that's being used to show the midi inputs, or nullptr if there isn't one. */
    ListBox* getMidiInputSelectorListBox() const noexcept
    {
        return static_cast<ListBox*>(midiInputsList.get());
    }
    
    //==============================================================================
    /** @internal */
    void resized() override
    {
        Rectangle<int> r (proportionOfWidth (0.35f), 15, proportionOfWidth (0.6f), 3000);
        auto space = itemHeight / 4;
        
        if (deviceTypeDropDown != nullptr)
        {
            deviceTypeDropDown->setBounds (r.removeFromTop (itemHeight));
            r.removeFromTop (space * 3);
        }
        
        if (audioDeviceSettingsComp != nullptr)
        {
            audioDeviceSettingsComp->resized();
            audioDeviceSettingsComp->setBounds (r.removeFromTop (audioDeviceSettingsComp->getHeight())
                                                .withX (0).withWidth (getWidth()));
            r.removeFromTop (space);
        }
        
        if (midiInputsList != nullptr)
        {
            midiInputsList->setRowHeight (jmin (22, itemHeight));
            midiInputsList->setBounds (r.removeFromTop (midiInputsList->getBestHeight (jmin (itemHeight * 8,
                                                                                             getHeight() - r.getY() - space - itemHeight))));
            r.removeFromTop (space);
        }
        
        if (bluetoothButton != nullptr)
        {
            bluetoothButton->setBounds (r.removeFromTop (24));
            r.removeFromTop (space);
        }
        
        if (midiOutputSelector != nullptr)
            midiOutputSelector->setBounds (r.removeFromTop (itemHeight));
        
        r.removeFromTop (itemHeight);
        setSize (getWidth(), r.getY());
    }
    
private:
    //==============================================================================
    void handleBluetoothButton()
    {
        if (! RuntimePermissions::isGranted (RuntimePermissions::bluetoothMidi))
            RuntimePermissions::request (RuntimePermissions::bluetoothMidi, nullptr);
        
        if (RuntimePermissions::isGranted (RuntimePermissions::bluetoothMidi))
            BluetoothMidiDevicePairingDialogue::open();
    }
    
    void updateDeviceType()
    {
        if (auto* type = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown->getSelectedId() - 1])
        {
            audioDeviceSettingsComp.reset();
            deviceManager.setCurrentAudioDeviceType (type->getTypeName(), true);
            updateAllControls(); // needed in case the type hasn't actually changed
        }
    }
    void updateMidiOutput()
    {
        auto selectedId = midiOutputSelector->getSelectedId();
        
        if (selectedId == -1)
            deviceManager.setDefaultMidiOutputDevice ({});
        else
            deviceManager.setDefaultMidiOutputDevice (currentMidiOutputs[selectedId - 1].identifier);
    }
    
    void changeListenerCallback (ChangeBroadcaster*) override
    {
        updateAllControls();
    }
    
    void updateAllControls()
    {
        if (deviceTypeDropDown != nullptr)
            deviceTypeDropDown->setText (deviceManager.getCurrentAudioDeviceType(), dontSendNotification);
        
        if (audioDeviceSettingsComp == nullptr
            || audioDeviceSettingsCompType != deviceManager.getCurrentAudioDeviceType())
        {
            audioDeviceSettingsCompType = deviceManager.getCurrentAudioDeviceType();
            audioDeviceSettingsComp.reset();
            
            if (auto* type = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown == nullptr
                                                                      ? 0 : deviceTypeDropDown->getSelectedId() - 1])
            {
                AudioDeviceSetupDetails details;
                details.manager = &deviceManager;
                details.minNumInputChannels = minInputChannels;
                details.maxNumInputChannels = maxInputChannels;
                details.minNumOutputChannels = minOutputChannels;
                details.maxNumOutputChannels = maxOutputChannels;
                details.useStereoPairs = showChannelsAsStereoPairs;
                
                auto* sp = new AudioDeviceSettingsPanel (*type, details, hideAdvancedOptionsWithButton);
                audioDeviceSettingsComp.reset (sp);
                addAndMakeVisible (sp);
                sp->updateAllControls();
            }
        }
        
        if (midiInputsList != nullptr)
        {
            midiInputsList->updateDevices();
            midiInputsList->updateContent();
            midiInputsList->repaint();
        }
        
        if (midiOutputSelector != nullptr)
        {
            midiOutputSelector->clear();
            
            currentMidiOutputs = MidiOutput::getAvailableDevices();
            
            midiOutputSelector->addItem (getNoDeviceString(), -1);
            midiOutputSelector->addSeparator();
            
            auto defaultOutputIdentifier = deviceManager.getDefaultMidiOutputIdentifier();
            int i = 0;
            
            for (auto& out : currentMidiOutputs)
            {
                midiOutputSelector->addItem (out.name, i + 1);
                
                if (defaultOutputIdentifier.isNotEmpty() && out.identifier == defaultOutputIdentifier)
                    midiOutputSelector->setSelectedId (i + 1);
                
                ++i;
            }
        }
        
        resized();
    }
    
    std::unique_ptr<ComboBox> deviceTypeDropDown;
    std::unique_ptr<Label> deviceTypeDropDownLabel;
    std::unique_ptr<Component> audioDeviceSettingsComp;
    String audioDeviceSettingsCompType;
    int itemHeight = 0;
    const int minOutputChannels, maxOutputChannels, minInputChannels, maxInputChannels;
    const bool showChannelsAsStereoPairs;
    const bool hideAdvancedOptionsWithButton;
    
    Array<MidiDeviceInfo> currentMidiOutputs;
    std::unique_ptr<MidiInputSelectorComponentListBox> midiInputsList;
    std::unique_ptr<ComboBox> midiOutputSelector;
    std::unique_ptr<Label> midiInputsLabel, midiOutputLabel;
    std::unique_ptr<TextButton> bluetoothButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneAudioSettings)
    
    //==============================================================================
    class AudioDeviceSettingsPanel : public Component,
    private ChangeListener
    {
    public:
        AudioDeviceSettingsPanel (AudioIODeviceType& t, AudioDeviceSetupDetails& setupDetails,
                                  const bool hideAdvancedOptionsWithButton)
        : type (t), setup (setupDetails)
        {
            if (hideAdvancedOptionsWithButton)
            {
                showAdvancedSettingsButton.reset (new TextButton (TRANS("Show advanced settings...")));
                addAndMakeVisible (showAdvancedSettingsButton.get());
                showAdvancedSettingsButton->setClickingTogglesState (true);
                showAdvancedSettingsButton->onClick = [this] { toggleAdvancedSettings(); };
            }
            
            type.scanForDevices();
            
            setup.manager->addChangeListener (this);
        }
        
        ~AudioDeviceSettingsPanel() override
        {
            setup.manager->removeChangeListener (this);
        }
        
        void resized() override
        {
            if (auto* parent = findParentComponentOfClass<StandaloneAudioSettings>())
            {
                Rectangle<int> r (proportionOfWidth (0.35f), 0, proportionOfWidth (0.6f), 3000);
                
                const int maxListBoxHeight = 100;
                const int h = parent->getItemHeight();
                const int space = h / 4;
                
                if (outputDeviceDropDown != nullptr)
                {
                    auto row = r.removeFromTop (h);
                    
                    if (testButton != nullptr)
                    {
                        testButton->changeWidthToFitText (h);
                        testButton->setBounds (row.removeFromRight (testButton->getWidth()));
                        row.removeFromRight (space);
                    }
                    
                    outputDeviceDropDown->setBounds (row);
                    r.removeFromTop (space);
                }
                
                if (inputDeviceDropDown != nullptr)
                {
                    auto row = r.removeFromTop (h);
                    
                    inputLevelMeter->setBounds (row.removeFromRight (testButton != nullptr ? testButton->getWidth() : row.getWidth() / 6));
                    row.removeFromRight (space);
                    inputDeviceDropDown->setBounds (row);
                    r.removeFromTop (space);
                }
                
                if (outputChanList != nullptr)
                {
                    outputChanList->setRowHeight (jmin (22, h));
                    outputChanList->setBounds (r.removeFromTop (outputChanList->getBestHeight (maxListBoxHeight)));
                    outputChanLabel->setBounds (0, outputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
                    r.removeFromTop (space);
                }
                
                if (inputChanList != nullptr)
                {
                    inputChanList->setRowHeight (jmin (22, h));
                    inputChanList->setBounds (r.removeFromTop (inputChanList->getBestHeight (maxListBoxHeight)));
                    inputChanLabel->setBounds (0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
                    r.removeFromTop (space);
                }
                
                r.removeFromTop (space * 2);
                
                if (showAdvancedSettingsButton != nullptr
                    && sampleRateDropDown != nullptr && bufferSizeDropDown != nullptr)
                {
                    showAdvancedSettingsButton->setBounds (r.removeFromTop (h));
                    r.removeFromTop (space);
                    showAdvancedSettingsButton->changeWidthToFitText();
                }
                
                auto advancedSettingsVisible = showAdvancedSettingsButton == nullptr
                || showAdvancedSettingsButton->getToggleState();
                
                if (sampleRateDropDown != nullptr)
                {
                    sampleRateDropDown->setVisible (advancedSettingsVisible);
                    
                    if (advancedSettingsVisible)
                    {
                        sampleRateDropDown->setBounds (r.removeFromTop (h));
                        r.removeFromTop (space);
                    }
                }
                
                if (bufferSizeDropDown != nullptr)
                {
                    bufferSizeDropDown->setVisible (advancedSettingsVisible);
                    
                    if (advancedSettingsVisible)
                    {
                        bufferSizeDropDown->setBounds (r.removeFromTop (h));
                        r.removeFromTop (space);
                    }
                }
                
                r.removeFromTop (space);
                
                if (showUIButton != nullptr || resetDeviceButton != nullptr)
                {
                    auto buttons = r.removeFromTop (h);
                    
                    if (showUIButton != nullptr)
                    {
                        showUIButton->setVisible (advancedSettingsVisible);
                        showUIButton->changeWidthToFitText (h);
                        showUIButton->setBounds (buttons.removeFromLeft (showUIButton->getWidth()));
                        buttons.removeFromLeft (space);
                    }
                    
                    if (resetDeviceButton != nullptr)
                    {
                        resetDeviceButton->setVisible (advancedSettingsVisible);
                        resetDeviceButton->changeWidthToFitText (h);
                        resetDeviceButton->setBounds (buttons.removeFromLeft (resetDeviceButton->getWidth()));
                    }
                    
                    r.removeFromTop (space);
                }
                
                setSize (getWidth(), r.getY());
            }
            else
            {
                jassertfalse;
            }
        }
        
        void updateConfig (bool updateOutputDevice, bool updateInputDevice, bool updateSampleRate, bool updateBufferSize)
        {
            auto config = setup.manager->getAudioDeviceSetup();
            String error;
            
            if (updateOutputDevice || updateInputDevice)
            {
                if (outputDeviceDropDown != nullptr)
                    config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0 ? String()
                    : outputDeviceDropDown->getText();
                
                if (inputDeviceDropDown != nullptr)
                    config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0 ? String()
                    : inputDeviceDropDown->getText();
                
                if (! type.hasSeparateInputsAndOutputs())
                    config.inputDeviceName = config.outputDeviceName;
                
                if (updateInputDevice)
                    config.useDefaultInputChannels = true;
                else
                    config.useDefaultOutputChannels = true;
                
                error = setup.manager->setAudioDeviceSetup (config, true);
                
                showCorrectDeviceName (inputDeviceDropDown.get(), true);
                showCorrectDeviceName (outputDeviceDropDown.get(), false);
                
                updateControlPanelButton();
                resized();
            }
            else if (updateSampleRate)
            {
                if (sampleRateDropDown->getSelectedId() > 0)
                {
                    config.sampleRate = sampleRateDropDown->getSelectedId();
                    error = setup.manager->setAudioDeviceSetup (config, true);
                }
            }
            else if (updateBufferSize)
            {
                if (bufferSizeDropDown->getSelectedId() > 0)
                {
                    config.bufferSize = bufferSizeDropDown->getSelectedId();
                    error = setup.manager->setAudioDeviceSetup (config, true);
                }
            }
            
            if (error.isNotEmpty())
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon,
                                                  TRANS("Error when trying to open audio device!"),
                                                  error);
        }
        
        bool showDeviceControlPanel()
        {
            if (auto* device = setup.manager->getCurrentAudioDevice())
            {
                Component modalWindow;
                modalWindow.setOpaque (true);
                modalWindow.addToDesktop (0);
                modalWindow.enterModalState();
                
                return device->showControlPanel();
            }
            
            return false;
        }
        
        void toggleAdvancedSettings()
        {
            showAdvancedSettingsButton->setButtonText ((showAdvancedSettingsButton->getToggleState() ? "Hide " : "Show ")
                                                       + String ("advanced settings..."));
            resized();
        }
        
        void showDeviceUIPanel()
        {
            if (showDeviceControlPanel())
            {
                setup.manager->closeAudioDevice();
                setup.manager->restartLastAudioDevice();
                getTopLevelComponent()->toFront (true);
            }
        }
        
        void playTestSound()
        {
            setup.manager->playTestSound();
        }
        
        void updateAllControls()
        {
            updateOutputsComboBox();
            updateInputsComboBox();
            
            updateControlPanelButton();
            updateResetButton();
            
            if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
            {
                if (setup.maxNumOutputChannels > 0
                    && setup.minNumOutputChannels < setup.manager->getCurrentAudioDevice()->getOutputChannelNames().size())
                {
                    if (outputChanList == nullptr)
                    {
                        outputChanList.reset (new ChannelSelectorListBox (setup, ChannelSelectorListBox::audioOutputType,
                                                                          TRANS ("(no audio output channels found)")));
                        addAndMakeVisible (outputChanList.get());
                        outputChanLabel.reset (new Label ({}, TRANS("Active output channels:")));
                        outputChanLabel->setJustificationType (Justification::centredRight);
                        outputChanLabel->attachToComponent (outputChanList.get(), true);
                    }
                    
                    outputChanList->refresh();
                }
                else
                {
                    outputChanLabel.reset();
                    outputChanList.reset();
                }
                
                if (setup.maxNumInputChannels > 0
                    && setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()->getInputChannelNames().size())
                {
                    if (inputChanList == nullptr)
                    {
                        inputChanList.reset (new ChannelSelectorListBox (setup, ChannelSelectorListBox::audioInputType,
                                                                         TRANS("(no audio input channels found)")));
                        addAndMakeVisible (inputChanList.get());
                        inputChanLabel.reset (new Label ({}, TRANS("Active input channels:")));
                        inputChanLabel->setJustificationType (Justification::centredRight);
                        inputChanLabel->attachToComponent (inputChanList.get(), true);
                    }
                    
                    inputChanList->refresh();
                }
                else
                {
                    inputChanLabel.reset();
                    inputChanList.reset();
                }
                
                updateSampleRateComboBox (currentDevice);
                updateBufferSizeComboBox (currentDevice);
            }
            else
            {
                jassert (setup.manager->getCurrentAudioDevice() == nullptr); // not the correct device type!
                
                inputChanLabel.reset();
                outputChanLabel.reset();
                sampleRateLabel.reset();
                bufferSizeLabel.reset();
                
                inputChanList.reset();
                outputChanList.reset();
                sampleRateDropDown.reset();
                bufferSizeDropDown.reset();
                
                if (outputDeviceDropDown != nullptr)
                    outputDeviceDropDown->setSelectedId (-1, dontSendNotification);
                
                if (inputDeviceDropDown != nullptr)
                    inputDeviceDropDown->setSelectedId (-1, dontSendNotification);
            }
            
            sendLookAndFeelChange();
            resized();
            setSize (getWidth(), getLowestY() + 4);
        }
        
        void changeListenerCallback (ChangeBroadcaster*) override
        {
            updateAllControls();
        }
        
        void resetDevice()
        {
            setup.manager->closeAudioDevice();
            setup.manager->restartLastAudioDevice();
        }
        
    private:
        AudioIODeviceType& type;
        const AudioDeviceSetupDetails setup;
        
        std::unique_ptr<ComboBox> outputDeviceDropDown, inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
        std::unique_ptr<Label> outputDeviceLabel, inputDeviceLabel, sampleRateLabel, bufferSizeLabel, inputChanLabel, outputChanLabel;
        std::unique_ptr<TextButton> testButton;
        std::unique_ptr<Component> inputLevelMeter;
        std::unique_ptr<TextButton> showUIButton, showAdvancedSettingsButton, resetDeviceButton;
        
        void showCorrectDeviceName (ComboBox* box, bool isInput)
        {
            if (box != nullptr)
            {
                auto* currentDevice = setup.manager->getCurrentAudioDevice();
                auto index = type.getIndexOfDevice (currentDevice, isInput);
                
                box->setSelectedId (index < 0 ? index : index + 1, dontSendNotification);
                
                if (testButton != nullptr && ! isInput)
                    testButton->setEnabled (index >= 0);
            }
        }
        
        void addNamesToDeviceBox (ComboBox& combo, bool isInputs)
        {
            const StringArray devs (type.getDeviceNames (isInputs));
            
            combo.clear (dontSendNotification);
            
            for (int i = 0; i < devs.size(); ++i)
                combo.addItem (devs[i], i + 1);
            
            combo.addItem (getNoDeviceString(), -1);
            combo.setSelectedId (-1, dontSendNotification);
        }
        
        int getLowestY() const
        {
            int y = 0;
            
            for (auto* c : getChildren())
                y = jmax (y, c->getBottom());
            
            return y;
        }
        
        void updateControlPanelButton()
        {
            auto* currentDevice = setup.manager->getCurrentAudioDevice();
            showUIButton.reset();
            
            if (currentDevice != nullptr && currentDevice->hasControlPanel())
            {
                showUIButton.reset (new TextButton (TRANS ("Control Panel"),
                                                    TRANS ("Opens the device's own control panel")));
                addAndMakeVisible (showUIButton.get());
                showUIButton->onClick = [this] { showDeviceUIPanel(); };
            }
            
            resized();
        }
        
        void updateResetButton()
        {
            if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
            {
                if (currentDevice->hasControlPanel())
                {
                    if (resetDeviceButton == nullptr)
                    {
                        resetDeviceButton.reset (new TextButton (TRANS ("Reset Device"),
                                                                 TRANS ("Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel")));
                        addAndMakeVisible (resetDeviceButton.get());
                        resetDeviceButton->onClick = [this] { resetDevice(); };
                        resized();
                    }
                    
                    return;
                }
            }
            
            resetDeviceButton.reset();
        }
        
        void updateOutputsComboBox()
        {
            if (setup.maxNumOutputChannels > 0 || ! type.hasSeparateInputsAndOutputs())
            {
                if (outputDeviceDropDown == nullptr)
                {
                    outputDeviceDropDown.reset (new ComboBox());
                    outputDeviceDropDown->onChange = [this] { updateConfig (true, false, false, false); };
                    
                    addAndMakeVisible (outputDeviceDropDown.get());
                    
                    outputDeviceLabel.reset (new Label ({}, type.hasSeparateInputsAndOutputs() ? TRANS("Output:")
                                                        : TRANS("Device:")));
                    outputDeviceLabel->attachToComponent (outputDeviceDropDown.get(), true);
                    
                    if (setup.maxNumOutputChannels > 0)
                    {
                        testButton.reset (new TextButton (TRANS("Test"), TRANS("Plays a test tone")));
                        addAndMakeVisible (testButton.get());
                        testButton->onClick = [this] { playTestSound(); };
                    }
                }
                
                addNamesToDeviceBox (*outputDeviceDropDown, false);
            }
            
            showCorrectDeviceName (outputDeviceDropDown.get(), false);
        }
        
        void updateInputsComboBox()
        {
            if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
            {
                if (inputDeviceDropDown == nullptr)
                {
                    inputDeviceDropDown.reset (new ComboBox());
                    inputDeviceDropDown->onChange = [this] { updateConfig (false, true, false, false); };
                    addAndMakeVisible (inputDeviceDropDown.get());
                    
                    inputDeviceLabel.reset (new Label ({}, TRANS("Input:")));
                    inputDeviceLabel->attachToComponent (inputDeviceDropDown.get(), true);
                    
                    inputLevelMeter.reset (new SimpleDeviceManagerInputLevelMeter (*setup.manager));
                    addAndMakeVisible (inputLevelMeter.get());
                }
                
                addNamesToDeviceBox (*inputDeviceDropDown, true);
            }
            
            showCorrectDeviceName (inputDeviceDropDown.get(), true);
        }
        
        void updateSampleRateComboBox (AudioIODevice* currentDevice)
        {
            if (sampleRateDropDown == nullptr)
            {
                sampleRateDropDown.reset (new ComboBox());
                addAndMakeVisible (sampleRateDropDown.get());
                
                sampleRateLabel.reset (new Label ({}, TRANS("Sample rate:")));
                sampleRateLabel->attachToComponent (sampleRateDropDown.get(), true);
            }
            else
            {
                sampleRateDropDown->clear();
                sampleRateDropDown->onChange = nullptr;
            }
            
            const auto getFrequencyString = [] (int rate) { return String (rate) + " Hz"; };
            
            for (auto rate : currentDevice->getAvailableSampleRates())
            {
                const auto intRate = roundToInt (rate);
                sampleRateDropDown->addItem (getFrequencyString (intRate), intRate);
            }
            
            const auto intRate = roundToInt (currentDevice->getCurrentSampleRate());
            sampleRateDropDown->setText (getFrequencyString (intRate), dontSendNotification);
            
            sampleRateDropDown->onChange = [this] { updateConfig (false, false, true, false); };
        }
        
        void updateBufferSizeComboBox (AudioIODevice* currentDevice)
        {
            if (bufferSizeDropDown == nullptr)
            {
                bufferSizeDropDown.reset (new ComboBox());
                addAndMakeVisible (bufferSizeDropDown.get());
                
                bufferSizeLabel.reset (new Label ({}, TRANS("Audio buffer size:")));
                bufferSizeLabel->attachToComponent (bufferSizeDropDown.get(), true);
            }
            else
            {
                bufferSizeDropDown->clear();
                bufferSizeDropDown->onChange = nullptr;
            }
            
            auto currentRate = currentDevice->getCurrentSampleRate();
            
            if (currentRate == 0)
                currentRate = 48000.0;
            
            for (auto bs : currentDevice->getAvailableBufferSizes())
                bufferSizeDropDown->addItem (String (bs) + " samples (" + String (bs * 1000.0 / currentRate, 1) + " ms)", bs);
            
            bufferSizeDropDown->setSelectedId (currentDevice->getCurrentBufferSizeSamples(), dontSendNotification);
            bufferSizeDropDown->onChange = [this] { updateConfig (false, false, false, true); };
        }
        
    public:
        //==============================================================================
        class ChannelSelectorListBox  : public ListBox,
        private ListBoxModel
        {
        public:
            enum BoxType
            {
                audioInputType,
                audioOutputType
            };
            
            //==============================================================================
            ChannelSelectorListBox (const AudioDeviceSetupDetails& setupDetails, BoxType boxType, const String& noItemsText)
            : ListBox ({}, nullptr), setup (setupDetails), type (boxType), noItemsMessage (noItemsText)
            {
                refresh();
                setModel (this);
                setOutlineThickness (1);
            }
            
            void refresh()
            {
                items.clear();
                
                if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
                {
                    if (type == audioInputType)
                        items = currentDevice->getInputChannelNames();
                    else if (type == audioOutputType)
                        items = currentDevice->getOutputChannelNames();
                    
                    if (setup.useStereoPairs)
                    {
                        StringArray pairs;
                        
                        for (int i = 0; i < items.size(); i += 2)
                        {
                            auto& name = items[i];
                            
                            if (i + 1 >= items.size())
                                pairs.add (name.trim());
                            else
                                pairs.add (getNameForChannelPair (name, items[i + 1]));
                        }
                        
                        items = pairs;
                    }
                }
                
                updateContent();
                repaint();
            }
            
            int getNumRows() override
            {
                return items.size();
            }
            
            void paintListBoxItem (int row, Graphics& g, int width, int height, bool) override
            {
                if (isPositiveAndBelow (row, items.size()))
                {
                    g.fillAll (findColour (ListBox::backgroundColourId));
                    
                    auto item = items[row];
                    bool enabled = false;
                    auto config = setup.manager->getAudioDeviceSetup();
                    
                    if (setup.useStereoPairs)
                    {
                        if (type == audioInputType)
                            enabled = config.inputChannels[row * 2] || config.inputChannels[row * 2 + 1];
                        else if (type == audioOutputType)
                            enabled = config.outputChannels[row * 2] || config.outputChannels[row * 2 + 1];
                    }
                    else
                    {
                        if (type == audioInputType)
                            enabled = config.inputChannels[row];
                        else if (type == audioOutputType)
                            enabled = config.outputChannels[row];
                    }
                    
                    auto x = getTickX();
                    auto tickW = (float) height * 0.75f;
                    
                    getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                                  enabled, true, true, false);
                    
                    drawTextLayout (g, *this, item, { x + 5, 0, width - x - 5, height }, enabled);
                }
            }
            
            void listBoxItemClicked (int row, const MouseEvent& e) override
            {
                selectRow (row);
                
                if (e.x < getTickX())
                    flipEnablement (row);
            }
            
            void listBoxItemDoubleClicked (int row, const MouseEvent&) override
            {
                flipEnablement (row);
            }
            
            void returnKeyPressed (int row) override
            {
                flipEnablement (row);
            }
            
            void paint (Graphics& g) override
            {
                ListBox::paint (g);
                
                if (items.isEmpty())
                {
                    g.setColour (Colours::grey);
                    g.setFont (0.5f * (float) getRowHeight());
                    g.drawText (noItemsMessage,
                                0, 0, getWidth(), getHeight() / 2,
                                Justification::centred, true);
                }
            }
            
            int getBestHeight (int maxHeight)
            {
                return getRowHeight() * jlimit (2, jmax (2, maxHeight / getRowHeight()),
                                                getNumRows())
                + getOutlineThickness() * 2;
            }
            
        private:
            //==============================================================================
            const AudioDeviceSetupDetails setup;
            const BoxType type;
            const String noItemsMessage;
            StringArray items;
            
            static String getNameForChannelPair (const String& name1, const String& name2)
            {
                String commonBit;
                
                for (int j = 0; j < name1.length(); ++j)
                    if (name1.substring (0, j).equalsIgnoreCase (name2.substring (0, j)))
                        commonBit = name1.substring (0, j);
                
                // Make sure we only split the name at a space, because otherwise, things
                // like "input 11" + "input 12" would become "input 11 + 2"
                while (commonBit.isNotEmpty() && ! CharacterFunctions::isWhitespace (commonBit.getLastCharacter()))
                    commonBit = commonBit.dropLastCharacters (1);
                
                return name1.trim() + " + " + name2.substring (commonBit.length()).trim();
            }
            
            void flipEnablement (int row)
            {
                jassert (type == audioInputType || type == audioOutputType);
                
                if (isPositiveAndBelow (row, items.size()))
                {
                    auto config = setup.manager->getAudioDeviceSetup();
                    
                    if (setup.useStereoPairs)
                    {
                        BigInteger bits;
                        auto& original = (type == audioInputType ? config.inputChannels
                                          : config.outputChannels);
                        
                        for (int i = 0; i < 256; i += 2)
                            bits.setBit (i / 2, original[i] || original[i + 1]);
                        
                        if (type == audioInputType)
                        {
                            config.useDefaultInputChannels = false;
                            flipBit (bits, row, setup.minNumInputChannels / 2, setup.maxNumInputChannels / 2);
                        }
                        else
                        {
                            config.useDefaultOutputChannels = false;
                            flipBit (bits, row, setup.minNumOutputChannels / 2, setup.maxNumOutputChannels / 2);
                        }
                        
                        for (int i = 0; i < 256; ++i)
                            original.setBit (i, bits[i / 2]);
                    }
                    else
                    {
                        if (type == audioInputType)
                        {
                            config.useDefaultInputChannels = false;
                            flipBit (config.inputChannels, row, setup.minNumInputChannels, setup.maxNumInputChannels);
                        }
                        else
                        {
                            config.useDefaultOutputChannels = false;
                            flipBit (config.outputChannels, row, setup.minNumOutputChannels, setup.maxNumOutputChannels);
                        }
                    }
                    
                    setup.manager->setAudioDeviceSetup (config, true);
                }
            }
            
            static void flipBit (BigInteger& chans, int index, int minNumber, int maxNumber)
            {
                auto numActive = chans.countNumberOfSetBits();
                
                if (chans[index])
                {
                    if (numActive > minNumber)
                        chans.setBit (index, false);
                }
                else
                {
                    if (numActive >= maxNumber)
                    {
                        auto firstActiveChan = chans.findNextSetBit (0);
                        chans.clearBit (index > firstActiveChan ? firstActiveChan : chans.getHighestBit());
                    }
                    
                    chans.setBit (index, true);
                }
            }
            
            int getTickX() const
            {
                return getRowHeight();
            }
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelSelectorListBox)
        };
        
    private:
        std::unique_ptr<ChannelSelectorListBox> inputChanList, outputChanList;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceSettingsPanel)
    };
};
