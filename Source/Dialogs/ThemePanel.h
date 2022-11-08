/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

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
        
        auto* fontPanel = panels.add(new PropertiesPanel::FontComponent("Default font", fontValue));
        fontPanel->setHideLabel(true);
        
        addAndMakeVisible(fontPanel);

        
        for (auto const& [themeName, themeColours] : PlugDataLook::colourSettings) {
            for (auto const& colour : themeColours) {
                auto colourName = PlugDataColourNames.at(colour.first).second;
                auto& swatch = swatches[themeName][colourName];
                
                auto value = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName).getPropertyAsValue(colourName, nullptr);
                
                swatch.referTo(value);
                swatch.addListener(this);
                auto* panel = panels.add(new PropertiesPanel::ColourComponent(colourName, swatch));
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

        // Space for dark/light labels
        bounds.removeFromTop(23);

        for (int i = 1; i < numberOfColours + 1; i++) {
            auto panelBounds = bounds.removeFromTop(23);
            panels[i]->setBounds(panelBounds.removeFromRight(getWidth() / 4));
            panels[numberOfColours + i]->setBounds(panelBounds);
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
    
    std::unique_ptr<Dialog> confirmationDialog;

    explicit ThemePanel(ValueTree settingsTree)
        : colourProperties(settingsTree)
    {
        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:down");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&confirmationDialog, getParentComponent(), "Are you sure you want to reset to default theme settings?",
                                          [this](bool result){
                if(result) {
                    colourProperties.resetColours();
                }
            });
        };
        
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&colourProperties, false);
        viewport.setScrollBarsShown(true, false);

        colourProperties.setVisible(true);
    }

    void resized() override
    {
        // Add a row for font and light/dark labels
        int numRows = PlugDataColour::numberOfColours + 2;
        
        colourProperties.setBounds(0, 0, getWidth(), numRows * 23);
        viewport.setBounds(getLocalBounds().withTrimmedBottom(32));
        resetButton.setBounds(getWidth() - 36, getHeight() - 26, 32, 32);
    }
};
