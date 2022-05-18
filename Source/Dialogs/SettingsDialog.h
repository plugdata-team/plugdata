#include "Deken.h"
#include "SearchPathComponent.h"
#include "../Components/PropertiesPanel.h"

struct ThemePanel : public Component, private ListBoxModel, public Value::Listener
{
    Value fontValue;
    Value cnvColourValue;
    Value tbColourValue;
    ValueTree settingsTree;
    
    ThemePanel(ValueTree globalSettings) : settingsTree(globalSettings) {
        addAndMakeVisible(themeSettingsList);
        themeSettingsList.setRowHeight(23);
        themeSettingsList.updateContent();
        themeSettingsList.setOutlineThickness(0);
        themeSettingsList.setModel(this);
        
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        
        fontValue.addListener(this);
        cnvColourValue.addListener(this);
        tbColourValue.addListener(this);
        
        
       
    }
    
    void valueChanged(Value& v) override {
        if(v.refersToSameSourceAs(fontValue)) {
            auto newFont = Font(fontValue.toString(), 15, Font::plain);
            LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(newFont.getTypefacePtr());
            settingsTree.setProperty("DefaultFont", v.getValue(), nullptr);
            getTopLevelComponent()->repaint();
        }
        // TODO: implement this!
        if(v.refersToSameSourceAs(cnvColourValue)) {
            
        }
        if(v.refersToSameSourceAs(tbColourValue)) {
            
        }
    }
    
    
    int getNumRows() override
    {
        return 1;
    }

    /** This method must be implemented to draw a row of the list.
        Note that the rowNumber value may be greater than the number of rows in your
        list, so be careful that you don't assume it's less than getNumRows().
    */
    void paintListBoxItem (int rowNumber,
                                   Graphics& g,
                                   int width, int height,
                                   bool rowIsSelected) override
    {
    }
    
    Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                               Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        
        if(rowNumber == 0) {
            return new PropertiesPanel::FontComponent("Default font", fontValue, 0);
        }
        if(rowNumber == 1) {
            return new PropertiesPanel::ColourComponent("Canvas Colour", cnvColourValue, 1);
        }
        if(rowNumber == 2) {
            return new PropertiesPanel::ColourComponent("Toolbar Colour", tbColourValue, 2);
        }
    }

    
    void paint(Graphics& g) override
    {
        PlugDataLook::paintStripes(g, 23, getHeight(), themeSettingsList, -1, 0);
    }
    
    void resized() override {
        themeSettingsList.setBounds(getLocalBounds());
    }
    
    ListBox themeSettingsList;
};

extern "C"
{
    extern char* pd_version;
}

struct AboutPanel : public Component
{
    Image logo = ImageFileFormat::loadFrom(BinaryData::plugd_logo_png, BinaryData::plugd_logo_pngSize);
    
    AboutPanel() {
        
    }
    
    void paint(Graphics& g) override
    {
        g.setFont(30);
        g.drawFittedText("PlugData " + String(ProjectInfo::versionString), 150, 20, 300, 30, Justification::left, 1);
        
        g.setFont(16);
        g.drawFittedText("By Timothy Schoen", 150, 50, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false) + " by Miller Puckette and others", 150, 70, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("Includes:", 150, 90, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("ELSE v1.0-rc1 by Alexandre Porres", 150, 110, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("cyclone v0.6.0 by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber and others", 150, 130, getWidth() - 150, 50, Justification::left, 2);
        
        
        g.drawFittedText("This program is published under the terms of the GPLv3 license", 150, 180, getWidth() - 150, 50, Justification::left, 2);
       
        
        Rectangle<float> logoBounds = {40.0f, 20.0f, logo.getWidth() / 2.0f, logo.getHeight() / 2.0f};
        
        g.drawImage(logo, logoBounds);
        
    }
    
};

struct DAWAudioSettings : public Component
{
    explicit DAWAudioSettings(AudioProcessor& p) : processor(p)
    {
        addAndMakeVisible(latencySlider);
        latencySlider.setRange(0, 88200, 1);
        latencySlider.setTextValueSuffix(" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        latencySlider.setColour(Slider::trackColourId, findColour(PlugDataColour::textColourId));
        latencySlider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::toolbarColourId));
        
        addAndMakeVisible(tailLengthSlider);
        tailLengthSlider.setRange(0, 10.0f, 0.01f);
        tailLengthSlider.setTextValueSuffix(" Seconds");
        tailLengthSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        tailLengthSlider.setColour(Slider::trackColourId, findColour(PlugDataColour::textColourId));
        tailLengthSlider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::toolbarColourId));


        addAndMakeVisible(tailLengthLabel);
        tailLengthLabel.setText("Tail Length", dontSendNotification);
        tailLengthLabel.attachToComponent(&tailLengthSlider, true);

        addAndMakeVisible(latencyLabel);
        latencyLabel.setText("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);

        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.onValueChange = [this, proc]() { proc->setLatencySamples(latencySlider.getValue() + proc->pd::Instance::getBlockSize()); };
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

        
        toolbarButtons = {new TextButton(Icons::Audio), new TextButton(Icons::Pencil), new TextButton(Icons::Search), new TextButton(Icons::Keyboard), new TextButton(Icons::Externals), new TextButton(Icons::Info)};

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
        panels.add(new ThemePanel(settingsTree));
        panels.add(new SearchPathComponent(settingsTree.getChildWithName("Paths")));
        panels.add(new KeyMappingEditorComponent(*editor->getKeyMappings(), true));
        panels.add(new Deken());
        panels.add(new AboutPanel());

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

        dialog->onClose = [this, dialog]()
        {
            // Check if deken is busy, else clean up settings dialog
            if (!dynamic_cast<Deken*>(panels[4])->isBusy())
            {
                if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(audioProcessor.getActiveEditor()))
                {
                    editor->settingsDialog.reset(nullptr);
                }
            }
            else
            {
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
        auto b = getLocalBounds().withTrimmedTop(toolbarHeight).withTrimmedBottom(6);

        int toolbarPosition = 2;
        for (auto& button : toolbarButtons)
        {
            if(button == toolbarButtons.getLast())
            {
                button->setBounds(getWidth() - 100, 0, 70, toolbarHeight - 2);
                break;
            }
            
            button->setBounds(toolbarPosition, 0, 70, toolbarHeight - 2);
            toolbarPosition += 70;
            
           
        }

        panels[0]->setBounds(b);
        panels[1]->setBounds(b);
        panels[2]->setBounds(b.reduced(6, 0));
        panels[3]->setBounds(b);
        panels[4]->setBounds(b);
        panels[5]->setBounds(b);
    }

    void paint(Graphics& g) override
    {
        // g.fillAll(findColour(PlugDataColour::canvasColourId));

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
        g.drawLine(0.0f, toolbarHeight, getWidth(), toolbarHeight);

        if (currentPanel > 0)
        {
            g.drawLine(0.0f, getHeight() - 33, getWidth(), getHeight() - 33);
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
