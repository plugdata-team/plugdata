#include "Deken.h"
#include "SearchPathComponent.h"
#include "KeyMappingComponent.h"
#include "../Utility/PropertiesPanel.h"

struct ColourProperties : public Component, public Value::Listener
{
    ValueTree settingsTree;
    Value fontValue;
    std::map<String, std::map<String, Value>> swatches;
    OwnedArray<PropertiesPanel::Property> panels;
    
    explicit ColourProperties(ValueTree globalSettings)
        : settingsTree(globalSettings)
    {
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);
        panels.add(new PropertiesPanel::FontComponent("Default font", fontValue, 0));

        for (auto const& [themeName, themeColours] : PlugDataLook::colourSettings) {
            for (auto const& colour : themeColours) {
                auto colourName = PlugDataColourNames.at(colour.first).second;
                auto& swatch = swatches[themeName][colourName];
                
                auto value = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName).getPropertyAsValue(colourName, nullptr);
                
                swatch.referTo(value);
                swatch.addListener(this);
                auto* panel = panels.add(new PropertiesPanel::ColourComponent(colourName, swatch, 1));
                panel->setHideLabel(true);
                addAndMakeVisible(panel);
            }
            
        }
        
    }

    void valueChanged(Value& v) override
    {
        auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());

        if (v.refersToSameSourceAs(fontValue)) {
            lnf.setDefaultFont(fontValue.toString());
            settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);
            getTopLevelComponent()->repaint();
            return;
        }

        for (auto const& [themeName, theme] : lnf.colourSettings) {
            for (auto const& [colourId, value] : theme) {
                auto colourName = PlugDataColourNames.at(colourId).second;
                if (v.refersToSameSourceAs(swatches[themeName][colourName])) {
                    
                    lnf.setThemeColour(themeName, colourId, Colour::fromString(v.toString()));

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

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText("Font", bounds.removeFromTop(23), Justification::left);

        auto themeRow = bounds.removeFromTop(23);
        g.drawText("Theme", themeRow, Justification::left);
        g.drawText("Light", themeRow.withX(getWidth() * 0.5f).withWidth(getWidth() / 4), Justification::centred);
        g.drawText("Dark", themeRow.withX(getWidth() * 0.75f).withWidth(getWidth() / 4), Justification::centred);

        for (auto const& [identifier, name] : PlugDataColourNames)
        {
            g.drawText(name.first, bounds.removeFromTop(23), Justification::left);
        }
        bounds.removeFromTop(23);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().removeFromRight(getWidth() / 2);
        panels[0]->setBounds(bounds.removeFromTop(23));

        bounds.removeFromTop(23);

        int numRows = panels.size() / 2;
        for (int i = 1; i < numRows; i++) {
            auto panelBounds = bounds.removeFromTop(23);
            panels[i]->setBounds(panelBounds.removeFromRight(getWidth() / 4));
            panels[numRows + i]->setBounds(panelBounds);
        }
    }
    
    void resetColours() {
        auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
        lnf.resetColours();

        dynamic_cast<PropertiesPanel::FontComponent*>(panels[0])->setFont("Inter");
        fontValue = "Inter";
        
        lnf.setDefaultFont(fontValue.toString());
        settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);

        auto colourThemesTree = settingsTree.getChildWithName("ColourThemes");
        
        
        for (auto const& [themeName, theme] : lnf.colourSettings) {
            auto themeTree = colourThemesTree.getChildWithName(themeName);
            for (auto const& [colourId, colourValue] : theme) {
                auto colourName = PlugDataColourNames.at(colourId).second;
                swatches[themeName][colourName] = colourValue.toString();
                themeTree.setProperty(colourName, colourValue.toString(), nullptr);
            }
        }


        lnf.setTheme(lnf.isUsingLightTheme);
        getTopLevelComponent()->repaint();

        for (auto* panel : panels) {
            if (auto* colourPanel = dynamic_cast<PropertiesPanel::ColourComponent*>(panel)) {
                colourPanel->updateColour();
            }
        }
    }
    
};

struct ThemePanel : public Component
{

    ColourProperties colourProperties;
    Viewport viewport;
    
    TextButton resetButton = TextButton(Icons::Refresh);

    explicit ThemePanel(ValueTree settingsTree)
        : colourProperties(settingsTree)
    {
        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:down");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]() {
            colourProperties.resetColours();
        };
        
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&colourProperties, false);
        viewport.setScrollBarsShown(true, false);

        colourProperties.setVisible(true);
    }

    void resized() override
    {
        // Add a row for font as well
        int numRows = PlugDataColour::numberOfColours + 1;
        
        colourProperties.setBounds(0, 0, getWidth(), numRows * 23);
        viewport.setBounds(getLocalBounds().withTrimmedBottom(28));
        resetButton.setBounds(getWidth() - 40, getHeight() - 24, 28, 28);
    }
};

extern "C" {
EXTERN char* pd_version;
}

struct AboutPanel : public Component {
    
    Image logo = ImageFileFormat::loadFrom(BinaryData::plugd_logo_png, BinaryData::plugd_logo_pngSize);

    void paint(Graphics& g) override
    {
        g.setFont(30);
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawFittedText("PlugData " + String(ProjectInfo::versionString), 150, 20, 300, 30, Justification::left, 1);

        g.setFont(16);
        g.drawFittedText("By Timothy Schoen", 150, 50, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false) + " by Miller Puckette and others", 150, 70, getWidth() - 150, 30, Justification::left, 1);

        g.drawFittedText("ELSE v1.0-rc3 by Alexandre Porres", 150, 110, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("cyclone v0.6.1 by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber and others", 150, 130, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Based on Camomile (Pierre Guillot)", 150, 170, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inspired by Kiwi (Elliot Paris, Pierre Guillot, Jean Millot)", 150, 190, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inter font by Rasmus Andersson", 150, 210, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Made with JUCE", 150, 230, getWidth() - 150, 50, Justification::left, 2);

        g.drawFittedText("Special thanks to: Deskew Technologies, ludnny, kreth608 and Joshua A.C. Newman for supporting this project", 150, 270, getWidth() - 200, 80, Justification::left, 3);

        g.drawFittedText("This program is published under the terms of the GPL3 license", 150, 340, getWidth() - 150, 50, Justification::left, 2);

        Rectangle<float> logoBounds = { 10.0f, 20.0f, 128, 128 };

        g.drawImage(logo, logoBounds);
    }
};

struct DAWAudioSettings : public Component {
    explicit DAWAudioSettings(AudioProcessor& p)
        : processor(p)
    {
        addAndMakeVisible(latencySlider);
        latencySlider.setRange(0, 88200, 1);
        latencySlider.setTextValueSuffix(" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        latencySlider.setColour(Slider::trackColourId, findColour(PlugDataColour::scrollbarThumbColourId));
        latencySlider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));

        addAndMakeVisible(tailLengthSlider);
        tailLengthSlider.setRange(0, 10.0f, 0.01f);
        tailLengthSlider.setTextValueSuffix(" Seconds");
        tailLengthSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        tailLengthSlider.setColour(Slider::trackColourId, findColour(PlugDataColour::scrollbarThumbColourId));
        tailLengthSlider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));

        addAndMakeVisible(tailLengthLabel);
        tailLengthLabel.setText("Tail Length", dontSendNotification);
        tailLengthLabel.attachToComponent(&tailLengthSlider, true);

        addAndMakeVisible(latencyLabel);
        latencyLabel.setText("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);
       
        addAndMakeVisible(nativeDialogLabel);
        nativeDialogLabel.setText("Use native file dialog", dontSendNotification);
        nativeDialogLabel.attachToComponent(&nativeDialogToggle, true);
        
        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.onValueChange = [this, proc]() { proc->setLatencySamples(latencySlider.getValue() + proc->pd::Instance::getBlockSize()); };
        tailLengthSlider.onValueChange = [this, proc]() { proc->tailLength.setValue(tailLengthSlider.getValue()); };
        
        auto& settingsTree = dynamic_cast<PlugDataAudioProcessor&>(p).settingsTree;
        
        if(!settingsTree.hasProperty("NativeDialog")) {
            settingsTree.setProperty("NativeDialog", true, nullptr);
        }
        
        nativeDialogToggle.getToggleStateValue().referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));
        addAndMakeVisible(nativeDialogToggle);
    }

    void resized() override
    {
        latencySlider.setBounds(120, 5, getWidth() - 130, 20);
        tailLengthSlider.setBounds(120, 30, getWidth() - 130, 20);
        nativeDialogToggle.setBounds(120, 55, 25, 25);
    }

    void visibilityChanged() override
    {
        if (!isVisible())
            return;

        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.setValue(processor.getLatencySamples());
        tailLengthSlider.setValue(static_cast<float>(proc->tailLength.getValue()));
    }

    AudioProcessor& processor;
    Label latencyLabel;
    Label tailLengthLabel;
    Label nativeDialogLabel;

    Slider latencySlider;
    Slider tailLengthSlider;
    
    ToggleButton nativeDialogToggle;
};

struct SettingsDialog : public Component {
    
    SettingsDialog(AudioProcessor& processor, Dialog* dialog, AudioDeviceManager* manager, ValueTree const& settingsTree)
        : audioProcessor(processor)
    {
        setVisible(false);

        toolbarButtons = { new TextButton(Icons::Audio), new TextButton(Icons::Pencil), new TextButton(Icons::Search), new TextButton(Icons::Keyboard), new TextButton(Icons::Externals)};

        currentPanel = std::clamp(lastPanel.load(), 0, toolbarButtons.size() - 1);

        auto* editor = dynamic_cast<ApplicationCommandManager*>(processor.getActiveEditor());

        if (manager) {
            panels.add(new AudioDeviceSelectorComponent(*manager, 1, 2, 1, 2, true, true, true, false));
        } else {
            panels.add(new DAWAudioSettings(processor));
        }


        panels.add(new ThemePanel(settingsTree));
        panels.add(new SearchPathComponent(settingsTree.getChildWithName("Paths")));
        panels.add(new KeyMappingComponent(*editor->getKeyMappings()));
        panels.add(new Deken());

        for (int i = 0; i < toolbarButtons.size(); i++) {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(0110);
            toolbarButtons[i]->setConnectedEdges(12);
            toolbarButtons[i]->setName("toolbar:settings");
            addAndMakeVisible(toolbarButtons[i]);

            addChildComponent(panels[i]);
            toolbarButtons[i]->onClick = [this, i]() mutable { showPanel(i); };
        }

        toolbarButtons[currentPanel]->setToggleState(true, sendNotification);

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
        for (auto& button : toolbarButtons) {
            button->setBounds(toolbarPosition, 1, 70, toolbarHeight - 2);
            toolbarPosition += 70;
        }

        panels[0]->setBounds(b);
        panels[1]->setBounds(b);
        panels[2]->setBounds(b.reduced(6, 0));
        panels[3]->setBounds(b);
        panels[4]->setBounds(b);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 5.0f);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, 5.0f);
        g.fillRect(toolbarBounds.withTop(10.0f));

        if (currentPanel > 0) {
            auto statusbarBounds = getLocalBounds().reduced(1).removeFromBottom(32).toFloat();
            g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));

            g.fillRect(statusbarBounds.withHeight(20));
            g.fillRoundedRectangle(statusbarBounds, 5.0f);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, toolbarHeight, getWidth(), toolbarHeight);

        if (currentPanel > 0) {
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

struct SettingsPopup : public PopupMenu {
    

    SettingsPopup(AudioProcessor& processor, ValueTree tree) :
    settingsTree(tree),
    themeSelector(tree),
    zoomSelector(tree),
    gridSelector(tree, "GridEnabled", "Enable grid")
    {
        addCustomItem(1, themeSelector, 70, 45, false);
        addCustomItem(2, zoomSelector, 70, 30, false);
        addCustomItem(3, gridSelector, 70, 30, false);
        
        addSeparator();
        addItem(4, "Settings");
        addItem(5, "About");
    }
    
    static void showSettingsPopup(AudioProcessor& processor, AudioDeviceManager* manager, Component* centre, ValueTree settingsTree) {
        auto* popup = new SettingsPopup(processor, settingsTree);
        auto* editor = dynamic_cast<PlugDataPluginEditor*>(processor.getActiveEditor());
        
        popup->showMenuAsync(PopupMenu::Options().withMinimumWidth(170).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
            [editor, &processor, popup, manager, centre, settingsTree](int result) {
            
                if (result == 4) {
                    
                    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                    auto* settingsDialog = new SettingsDialog(processor, dialog, manager, settingsTree);
                    dialog->setViewedComponent(settingsDialog);
                    editor->openedDialog.reset(dialog);
                }
                if (result == 5) {
                    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                    auto* aboutPanel = new AboutPanel();
                    dialog->setViewedComponent(aboutPanel);
                    editor->openedDialog.reset(dialog);
                }
             
            
            MessageManager::callAsync([popup](){
                delete popup;
            });
            
            });
    }
    
    struct ZoomSelector : public Component
    {
        TextButton zoomIn;
        TextButton zoomOut;
        TextButton zoomReset;
        
        Value zoomValue;
        
        ZoomSelector(ValueTree settingsTree)
        {
            zoomValue = settingsTree.getPropertyAsValue("Zoom", nullptr);
            
            zoomIn.setButtonText("+");
            zoomReset.setButtonText(String(static_cast<float>(zoomValue.getValue()) * 100, 1) + "%");
            zoomOut.setButtonText("-");
            
            addAndMakeVisible(zoomIn);
            addAndMakeVisible(zoomReset);
            addAndMakeVisible(zoomOut);
            
            zoomIn.setConnectedEdges(Button::ConnectedOnLeft);
            zoomOut.setConnectedEdges(Button::ConnectedOnRight);
            zoomReset.setConnectedEdges(12);
            
            zoomIn.onClick = [this](){
                applyZoom(true);
            };
            zoomOut.onClick = [this](){
                applyZoom(false);
            };
            zoomReset.onClick = [this](){
                resetZoom();
            };
        }
        
        void applyZoom(bool zoomIn)
        {
            float value = static_cast<float>(zoomValue.getValue());

            // Apply limits
            value = std::clamp(zoomIn ? value + 0.1f : value - 0.1f, 0.5f, 2.0f);

            // Round in case we zoomed with scrolling
            value = static_cast<float>(static_cast<int>(round(value * 10.))) / 10.;

            zoomValue = value;

            zoomReset.setButtonText(String(value * 100.0f, 1) + "%");
        }
        
        void resetZoom() {
            zoomValue = 1.0f;
            zoomReset.setButtonText("100.0%");
        }
        
        void resized() override
        {
            auto bounds = getLocalBounds().reduced(8, 4);
            int buttonWidth = (getWidth() - 8) / 3;
            
            zoomOut.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
            zoomReset.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
            zoomIn.setBounds(bounds.removeFromLeft(buttonWidth).expanded(1, 0));
        }
    };
    
    struct PopupToggleComponent : public Component
    {
        ToggleButton toggle;
        
        String labelText;
        
        PopupToggleComponent(ValueTree settingsTree, String propertyName, String name) {
            labelText = name;

            toggle.getToggleStateValue().referTo(settingsTree.getPropertyAsValue(propertyName, nullptr));
            
            addAndMakeVisible(toggle);
        }
        
        void resized() override
        {
            auto bounds = getLocalBounds().reduced(5, 4);
            toggle.setBounds(bounds.removeFromLeft(25));
        }
        
        void paint(Graphics& g) override
        {
            auto textBounds = getLocalBounds().reduced(5, 4);
            textBounds.removeFromLeft(30);
            
            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
            g.setFont(15);
            g.drawText(labelText, textBounds, Justification::centredLeft);
        }
    };

    struct ThemeSelector : public Component
    {
        ThemeSelector(ValueTree settingsTree) {
            theme.referTo(settingsTree.getPropertyAsValue("Theme", nullptr));
        }
        
        void paint(Graphics& g)
        {
            auto firstBounds = getLocalBounds();
            auto secondBounds = firstBounds.removeFromLeft(getWidth() / 2.0f);
            
            firstBounds = firstBounds.withSizeKeepingCentre(30, 30);
            secondBounds = secondBounds.withSizeKeepingCentre(30, 30);
            
            g.setColour(Colour(25, 25, 25));
            g.fillEllipse(firstBounds.toFloat());
            
            g.setColour(Colour(240, 240, 240));
            g.fillEllipse(secondBounds.toFloat());

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawEllipse(firstBounds.toFloat(), 1.0f);
            g.drawEllipse(secondBounds.toFloat(), 1.0f);
            
            auto tick = getLookAndFeel().getTickShape(0.6f);
            auto tickBounds = Rectangle<int>();
            
            if(!static_cast<bool>(theme.getValue())) {
                g.setColour(Colour(240, 240, 240));
                tickBounds = firstBounds;
            }
            else {
                g.setColour(Colour(25, 25, 25));
                tickBounds = secondBounds;
            }
            
            g.fillPath (tick, tick.getTransformToScaleToFit (tickBounds.reduced (9, 9).toFloat(), false));

        }
        
        void mouseUp(const MouseEvent& e)
        {
            auto firstBounds = getLocalBounds();
            auto secondBounds = firstBounds.removeFromLeft(getWidth() / 2.0f);
            
            firstBounds = firstBounds.withSizeKeepingCentre(30, 30);
            secondBounds = secondBounds.withSizeKeepingCentre(30, 30);
            
            if(firstBounds.contains(e.x, e.y)) {
                theme = false;
                repaint();
            }
            else if(secondBounds.contains(e.x, e.y)) {
                theme = true;
                repaint();
            }
        }
        
        Value theme;
    };
    
    ThemeSelector themeSelector;
    PopupToggleComponent gridSelector;
    ZoomSelector zoomSelector;

    
    ValueTree settingsTree;
};
