#include "Deken.h"
#include "SearchPathComponent.h"
#include "../Utility/PropertiesPanel.h"

struct ThemePanel : public Component, public Value::Listener
{
    ValueTree settingsTree;
    Value fontValue;
    std::vector<std::vector<Value>> colours;

    TextButton resetButton = TextButton(Icons::Refresh);

    OwnedArray<PropertiesPanel::Property> panels;

    explicit ThemePanel(ValueTree globalSettings) : settingsTree(globalSettings)
    {
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);
        panels.add(new PropertiesPanel::FontComponent("Default font", fontValue, 0));

        // Get current colour
        auto numColours = PlugDataLook::colourNames[0].size();
        colours.resize(2);

        for (int i = 0; i < numColours; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                colours[j].resize(numColours);
                colours[j][i].setValue(PlugDataLook::colourSettings[j][i].toString());
                colours[j][i].addListener(this);
                panels.add(new PropertiesPanel::ColourComponent(PlugDataLook::colourNames[j][i], colours[j][i], 1));
            }
        }

        for (auto* panel : panels)
        {
            panel->setHideLabel(true);
            addAndMakeVisible(panel);
        }

        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:down");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]()
        {
            auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
            lnf.colourSettings = lnf.defaultColours;

            dynamic_cast<PropertiesPanel::FontComponent*>(panels[0])->setFont("Inter");
            fontValue = "Inter";
            lnf.setDefaultFont(fontValue.toString());
            settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);

            auto numColours = PlugDataLook::colourNames[0].size();
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < numColours; j++)
                {
                    colours[i][j] = lnf.colourSettings[i][j].toString();
                    settingsTree.setProperty(lnf.colourNames[i][j], lnf.colourSettings[i][j].toString(), nullptr);
                }
            }

            lnf.setTheme(lnf.isUsingLightTheme);
            getTopLevelComponent()->repaint();

            for (auto* panel : panels)
            {
                if (auto* colourPanel = dynamic_cast<PropertiesPanel::ColourComponent*>(panel))
                {
                    colourPanel->updateColour();
                }
            }
        };
    }

    void valueChanged(Value& v) override
    {
        auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());

        if (v.refersToSameSourceAs(fontValue))
        {
            lnf.setDefaultFont(fontValue.toString());
            settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);
            getTopLevelComponent()->repaint();
        }
        
        auto numColours = PlugDataLook::colourNames[0].size();
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < numColours; j++)
            {
                if (v.refersToSameSourceAs(colours[i][j]))
                {
                    lnf.colourSettings[i][j] = Colour::fromString(v.toString());
                    settingsTree.setProperty(lnf.colourNames[i][j], lnf.colourSettings[i][j].toString(), nullptr);
                    lnf.setTheme(lnf.isUsingLightTheme);
                    getTopLevelComponent()->repaint();
                }
            }
        }
    }

    void paint(Graphics& g) override
    {
        PlugDataLook::paintStripes(g, 23, getHeight() - 30, *this, -1, 0);

        auto bounds = getLocalBounds().removeFromLeft(getWidth() / 2).withTrimmedLeft(6);

        g.setColour(findColour(PlugDataColour::textColourId));
        g.drawText("Font", bounds.removeFromTop(23), Justification::left);

        auto themeRow = bounds.removeFromTop(23);
        g.drawText("Theme", themeRow, Justification::left);
        g.drawText("Dark", themeRow.withX(getWidth() * 0.5f).withWidth(getWidth() / 4), Justification::centred);
        g.drawText("Light", themeRow.withX(getWidth() * 0.75f).withWidth(getWidth() / 4), Justification::centred);

        g.drawText("Toolbar colour", bounds.removeFromTop(23), Justification::left);
        g.drawText("Canvas colour", bounds.removeFromTop(23), Justification::left);

        g.drawText("Text colour", bounds.removeFromTop(23), Justification::left);
        g.drawText("Data colour", bounds.removeFromTop(23), Justification::left);
        g.drawText("Outline colour", bounds.removeFromTop(23), Justification::left);
        g.drawText("Connection colour", bounds.removeFromTop(23), Justification::left);
        g.drawText("Signal colour", bounds.removeFromTop(23), Justification::left);

        bounds.removeFromTop(23);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().removeFromRight(getWidth() / 2);
        panels[0]->setBounds(bounds.removeFromTop(23));

        bounds.removeFromTop(23);

        for (int i = 0; i < panels.size() / 2; i++)
        {
            auto panelBounds = bounds.removeFromTop(23);
            panels[i * 2 + 1]->setBounds(panelBounds.removeFromRight(getWidth() / 4));
            panels[i * 2 + 2]->setBounds(panelBounds);
        }

        resetButton.setBounds(getWidth() - 40, getHeight() - 24, 28, 28);
    }
};

extern "C"
{
    EXTERN char* pd_version;
}

struct AboutPanel : public Component
{
    Image logo = ImageFileFormat::loadFrom(BinaryData::plugd_logo_png, BinaryData::plugd_logo_pngSize);

    void paint(Graphics& g) override
    {
        g.setFont(30);
        g.setColour(findColour(PlugDataColour::textColourId));
        g.drawFittedText("PlugData " + String(ProjectInfo::versionString), 150, 20, 300, 30, Justification::left, 1);

        g.setFont(16);
        g.drawFittedText("By Timothy Schoen", 150, 50, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false) + " by Miller Puckette and others", 150, 70, getWidth() - 150, 30, Justification::left, 1);

        g.drawFittedText("ELSE v1.0-rc2 by Alexandre Porres", 150, 110, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("cyclone v0.6.0 by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber and others", 150, 130, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Based on Camomile (Pierre Guillot)", 150, 170, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inspired by Kiwi (Elliot Paris, Pierre Guillot, Jean Millot)", 150, 190, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inter font by Rasmus Andersson", 150, 210, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Made with JUCE", 150, 230, getWidth() - 150, 50, Justification::left, 2);

        g.drawFittedText("Special thanks to: Deskew Technologies, ludnny and Joshua A.C. Newman for supporting this project", 150, 270, getWidth() - 150, 50, Justification::left, 2);
  

        g.drawFittedText("This program is published under the terms of the GPL3 license", 150, 300, getWidth() - 150, 50, Justification::left, 2);
        



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
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(audioProcessor.getActiveEditor()))
            {
                editor->settingsDialog.reset(nullptr);
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
            if (button == toolbarButtons.getLast())
            {
                button->setBounds(getWidth() - 100, 1, 70, toolbarHeight - 2);
                break;
            }

            button->setBounds(toolbarPosition, 1, 70, toolbarHeight - 2);
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
