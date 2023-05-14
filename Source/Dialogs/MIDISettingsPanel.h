/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_audio_utils/juce_audio_utils.h>


class MidiSelectorComponentListBox : public ListBox
    , private ListBoxModel {

public:
        
    MidiSelectorComponentListBox(bool input, PluginProcessor* processor, AudioDeviceManager& dm, String const& noItems)
        : ListBox({}, nullptr)
        , deviceManager(dm)
        , noItemsMessage(noItems)
        , isInput(input)
        , audioProcessor(processor)
    {
        updateDevices();
        setModel(this);
        setOutlineThickness(1);
        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    }
        
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);
        
        if (items.isEmpty()) {
            Fonts::drawText(g, noItemsMessage, 0, 0, getWidth(), getHeight() / 2, Colours::grey, 0.5f * (float)getRowHeight(), Justification::centred);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::defaultCornerRadius, 1.0f);
    }
    
    static void drawTextLayout(Graphics& g, Component& owner, StringRef text, Rectangle<int> const& textBounds, bool enabled)
    {
        auto const textColour = owner.findColour(ListBox::textColourId, true).withMultipliedAlpha(enabled ? 1.0f : 0.6f);

        AttributedString attributedString { text };
        attributedString.setColour(textColour);
        attributedString.setFont((float)textBounds.getHeight() * 0.6f);
        attributedString.setJustification(Justification::centredLeft);
        attributedString.setWordWrap(AttributedString::WordWrap::none);

        TextLayout textLayout;
        textLayout.createLayout(attributedString,
            (float)textBounds.getWidth(),
            (float)textBounds.getHeight());
        textLayout.draw(g, textBounds.toFloat());
    }

    void resized() override
    {
        auto extra = getOutlineThickness() * 2;
        setSize(getWidth(), jmin(8 * getRowHeight(), items.size() * getRowHeight()) + extra);
        ListBox::resized();
    }

    void updateDevices()
    {
        items = isInput ? MidiInput::getAvailableDevices() : MidiOutput::getAvailableDevices();

        if (!isInput) {
            MidiDeviceInfo internalSynth;
            internalSynth.name = "Internal synth";
            internalSynth.identifier = "internal";
            items.insert(0, internalSynth);
        }
    }

    int getNumRows() override
    {
        return items.size();
    }

    void paintListBoxItem(int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow(row, items.size())) {
            auto item = items[row];

            bool enabled = isInput ? deviceManager.isMidiInputDeviceEnabled(item.identifier) : (getEnabledMidiOutputWithID(item.identifier) != nullptr);

            if (item.identifier == "internal") {
                enabled = audioProcessor->enableInternalSynth;
            }

            auto x = getTickX();
            auto tickW = (float)height * 0.75f;

            getLookAndFeel().drawTickBox(g, *this, (float)x - tickW, ((float)height - tickW) * 0.5f, tickW, tickW,
                enabled, true, true, false);

            drawTextLayout(g, *this, item.name, { x + 5, 0, width - x - 5, height }, enabled);
        }
    }

    void listBoxItemClicked(int row, MouseEvent const& e) override
    {
        selectRow(row);

        if (e.x < getTickX())
            flipEnablement(row);
    }

    void listBoxItemDoubleClicked(int row, MouseEvent const&) override
    {
        flipEnablement(row);
    }

    void returnKeyPressed(int row) override
    {
        flipEnablement(row);
    }

    int getBestHeight(int preferredHeight)
    {
        auto extra = getOutlineThickness() * 2;

        return jmax(getRowHeight() * 2 + extra,
            jmin(getRowHeight() * getNumRows() + extra,
                preferredHeight));
    }

private:
    AudioDeviceManager& deviceManager;
    PluginProcessor* audioProcessor;
    const String noItemsMessage;
    Array<MidiDeviceInfo> items;
    bool isInput;

    MidiOutput* getEnabledMidiOutputWithID(String identifier)
    {
        for (auto* midiOut : audioProcessor->midiOutputs) {
            if (midiOut->getIdentifier() == identifier) {
                return midiOut;
            }
        }

        return nullptr;
    }

    void flipEnablement(int const row)
    {
        if (isPositiveAndBelow(row, items.size())) {
            auto identifier = items[row].identifier;

            if (isInput) {
                deviceManager.setMidiInputDeviceEnabled(identifier, !deviceManager.isMidiInputDeviceEnabled(identifier));
            } else {
                if (identifier == "internal") {
                    audioProcessor->enableInternalSynth = !audioProcessor->enableInternalSynth;

                    audioProcessor->settingsFile->setProperty("internal_synth", static_cast<int>(audioProcessor->enableInternalSynth));
                } else if (auto* midiOut = getEnabledMidiOutputWithID(identifier)) {
                    audioProcessor->midiOutputs.removeObject(midiOut);
                } else {
                    auto* device = audioProcessor->midiOutputs.add(MidiOutput::openDevice(identifier));
                    device->startBackgroundThread();
                }
            }
            updateContent();
            repaint();
        }
    }

    int getTickX() const
    {
        return getRowHeight();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSelectorComponentListBox)
};

class StandaloneMIDISettings : public Component
    , private ChangeListener {
public:
        StandaloneMIDISettings(PluginProcessor* audioProcessor, AudioDeviceManager& dm)
        : deviceManager(dm)
        , itemHeight(24)
    {
        midiInputsList.reset(new MidiSelectorComponentListBox(true, audioProcessor, deviceManager,
            "(No MIDI inputs available)"));
        addAndMakeVisible(midiInputsList.get());

        midiInputsLabel.reset(new Label({}, "MIDI inputs:"));
        midiInputsLabel->setJustificationType(Justification::centredRight);
        midiInputsLabel->attachToComponent(midiInputsList.get(), true);

        // Temporarily disable this, it causes a crash at the moment
        if (false && BluetoothMidiDevicePairingDialogue::isAvailable()) {
            bluetoothButton.reset(new TextButton("Bluetooth MIDI", "Scan for bluetooth MIDI devices"));
            addAndMakeVisible(bluetoothButton.get());
            bluetoothButton->onClick = [this] { handleBluetoothButton(); };
        }

        midiOutputsList.reset(new MidiSelectorComponentListBox(false, audioProcessor, deviceManager,
            "(No MIDI outputs available)"));
        addAndMakeVisible(midiOutputsList.get());

        midiOutputLabel.reset(new Label("lm", "MIDI Outputs:"));
        midiOutputLabel->setJustificationType(Justification::centredRight);
        midiOutputLabel->attachToComponent(midiOutputsList.get(), true);

        deviceManager.addChangeListener(this);
        updateAllControls();
    }
    /** Destructor */
    ~StandaloneMIDISettings() override
    {
        deviceManager.removeChangeListener(this);
    }

    /** The device manager that this component is controlling */
    AudioDeviceManager& deviceManager;

    /** Sets the standard height used for items in the panel. */
    void setItemHeight(int newItemHeight)
    {
        itemHeight = newItemHeight;
        resized();
    }

    /** Returns the standard height used for items in the panel. */
    int getItemHeight() const noexcept { return itemHeight; }

    /** Returns the ListBox that's being used to show the midi inputs, or nullptr if there isn't one. */
    ListBox* getMidiInputSelectorListBox() const noexcept
    {
        return static_cast<ListBox*>(midiInputsList.get());
    }

    void resized() override
    {
        Rectangle<int> r(proportionOfWidth(0.35f), 15, proportionOfWidth(0.6f), 3000);
        auto space = itemHeight / 4;

        if (midiInputsList != nullptr) {
            midiInputsList->setRowHeight(jmin(22, itemHeight));
            midiInputsList->setBounds(r.removeFromTop(midiInputsList->getHeight()));
            r.removeFromTop(space);
        }

        if (bluetoothButton != nullptr) {
            bluetoothButton->setBounds(r.removeFromTop(24));
            r.removeFromTop(space);
        }

        if (midiOutputsList != nullptr) {
            midiOutputsList->setRowHeight(jmin(22, itemHeight));
            midiOutputsList->setBounds(r.removeFromTop(midiOutputsList->getHeight()));
            r.removeFromTop(space);
        }

        r.removeFromTop(itemHeight);
        setSize(getWidth(), r.getY());
    }

private:
    void handleBluetoothButton()
    {
        if (!RuntimePermissions::isGranted(RuntimePermissions::bluetoothMidi))
            RuntimePermissions::request(RuntimePermissions::bluetoothMidi, nullptr);

        if (RuntimePermissions::isGranted(RuntimePermissions::bluetoothMidi))
            BluetoothMidiDevicePairingDialogue::open();
    }

    void changeListenerCallback(ChangeBroadcaster*) override
    {
        updateAllControls();
    }

    void updateAllControls()
    {
        midiInputsList->updateDevices();
        midiInputsList->updateContent();
        midiInputsList->repaint();

        midiOutputsList->updateDevices();
        midiOutputsList->updateContent();
        midiOutputsList->repaint();

        resized();
    }

    std::unique_ptr<Component> audioDeviceSettingsComp;
    int itemHeight = 0;

    Array<MidiDeviceInfo> currentMidiOutputs;
    std::unique_ptr<MidiSelectorComponentListBox> midiInputsList;
    std::unique_ptr<MidiSelectorComponentListBox> midiOutputsList;
    std::unique_ptr<Label> midiInputsLabel, midiOutputLabel;
    std::unique_ptr<TextButton> bluetoothButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneMIDISettings)
};
