#include "Deken.h"
#include "SearchPathComponent.h"


struct DAWAudioSettings : public Component
{
    explicit DAWAudioSettings(AudioProcessor& p) : processor(p)
    {
        addAndMakeVisible(latencySlider);
        latencySlider.setRange(0, 88200, 1);
        latencySlider.setTextValueSuffix(" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);

        addAndMakeVisible(tailLengthSlider);
        tailLengthSlider.setRange(0, 10.0f, 0.01f);
        tailLengthSlider.setTextValueSuffix(" Seconds");
        tailLengthSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);

        addAndMakeVisible(tailLengthLabel);
        tailLengthLabel.setText("Tail Length", dontSendNotification);
        tailLengthLabel.attachToComponent(&tailLengthSlider, true);

        addAndMakeVisible(latencyLabel);
        latencyLabel.setText("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);

        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.onValueChange = [this, proc]() { proc->setLatencySamples(latencySlider.getValue()); };
        tailLengthSlider.onValueChange = [this, proc]() { proc->tailLength.setValue(tailLengthSlider.getValue()); };
    }

    void resized() override
    {
        latencySlider.setBounds(90, 5, getWidth() - 130, 20);
        tailLengthSlider.setBounds(90, 30, getWidth() - 130, 20);
    }

    void visibilityChanged() override
    {
        if (!isVisible()) return;

        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.setValue(processor.getLatencySamples());
        tailLengthSlider.setValue(static_cast<float>(proc->tailLength.getValue()));
    }

    AudioProcessor& processor;
    Label latencyLabel;
    Label tailLengthLabel;

    Slider latencySlider;
    Slider tailLengthSlider;
};


struct SettingsDialog : public Component
{

    SettingsDialog(AudioProcessor& processor, Dialog* dialog, AudioDeviceManager* manager, const ValueTree& settingsTree) : audioProcessor(processor)
    {
        setVisible(false);

        toolbarButtons = {new TextButton(Icons::Audio), new TextButton(Icons::Search), new TextButton(Icons::Keyboard), new TextButton(Icons::Externals)};
        
        currentPanel = std::clamp(lastPanel.load(), 0, toolbarButtons.size() - 1);

        auto* editor = dynamic_cast<ApplicationCommandManager*>(processor.getActiveEditor());

        if (manager)
        {
            panels.add(new AudioDeviceSelectorComponent(*manager, 1, 2, 1, 2, true, true, true, false));
        }
        else
        {
            panels.add(new DAWAudioSettings(processor));
        }

        panels.add(new SearchPathComponent(settingsTree.getChildWithName("Paths")));
        panels.add(new KeyMappingEditorComponent(*editor->getKeyMappings(), true));
        panels.add(new Deken());

        for (int i = 0; i < toolbarButtons.size(); i++)
        {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(0110);
            toolbarButtons[i]->setConnectedEdges(12);
            toolbarButtons[i]->setName("toolbar:settings");
            addAndMakeVisible(toolbarButtons[i]);

            addChildComponent(panels[i]);
            toolbarButtons[i]->onClick = [this, i]() mutable { showPanel(i); };
        }

        toolbarButtons[currentPanel]->setToggleState(true, sendNotification);

        dialog->onClose = [this, dialog](){
            // Check if deken is busy, else clean up settings dialog
            if(!dynamic_cast<Deken*>(panels[3])->isBusy()) {
                if(auto* editor = dynamic_cast<PlugDataPluginEditor*>(audioProcessor.getActiveEditor())) {
                    editor->settingsDialog.reset(nullptr);
                }
            }
            else {
                dialog->setVisible(false);
            }
        };

        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);
        
    }

    ~SettingsDialog() override
    {
        lastPanel = currentPanel;
        dynamic_cast<PlugDataAudioProcessor*>(&audioProcessor)->saveSettings();
    }

    void resized() override
    {
        
        auto b = getLocalBounds().reduced(3);
        auto panelBounds = Rectangle<int>(b.getX(), toolbarHeight, b.getWidth(), b.getHeight() - toolbarHeight - 4);
        
        int toolbarPosition = 2;
        for (auto& button : toolbarButtons)
        {
            button->setBounds(toolbarPosition, 2, 70, toolbarHeight - 2);
            toolbarPosition += 70;
        }

        panels[0]->setBounds(panelBounds);
        panels[1]->setBounds(panelBounds);
        panels[2]->setBounds(panelBounds.reduced(6, 0));
        panels[3]->setBounds(panelBounds);
    }

    void paint(Graphics& g) override
    {
        //g.fillAll(findColour(PlugDataColour::canvasColourId));

        g.setColour(findColour(PlugDataColour::canvasColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 5.0f);

        g.setColour(findColour(PlugDataColour::toolbarColourId));

        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, 5.0f);
        g.fillRect(toolbarBounds.withTop(10.0f));

        if (currentPanel > 0)
        {
            auto statusbarBounds = getLocalBounds().reduced(1).removeFromBottom(32).toFloat();
            g.setColour(findColour(PlugDataColour::toolbarColourId));

            g.fillRect(statusbarBounds.withHeight(20));
            g.fillRoundedRectangle(statusbarBounds, 5.0f);
        }

    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(1.0f, toolbarHeight, static_cast<float>(getWidth() - 2), toolbarHeight);

        if (currentPanel > 0)
        {
            g.drawLine(1.0f, getHeight() - 33, static_cast<float>(getWidth() - 2), getHeight() - 33);
        }
    }
    
    
    void showPanel(int idx)
    {
        panels[currentPanel]->setVisible(false);
        panels[idx]->setVisible(true);
        currentPanel = idx;
        repaint();
    }
    
    AudioProcessor& audioProcessor;
    ComponentBoundsConstrainer constrainer;

    static constexpr int toolbarHeight = 45;
    
    static inline std::atomic<int> lastPanel = 0;
    int currentPanel;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    OwnedArray<TextButton> toolbarButtons;
};

