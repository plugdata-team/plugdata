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

    PropertiesPanel panel;
    Array<PropertyComponent*> allPanels;
    
    explicit ColourProperties(ValueTree globalSettings)
        : settingsTree(globalSettings)
    {
        addAndMakeVisible(panel);
                
        std::map<String, Array<PropertyComponent*>> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {
            
            auto& [colourName, colourId, colourCategory] = colourNames;
            
            Array<Value*> swatchesToAdd;
            
            // Loop over themes
            int i = 0;
            for (auto const& [themeName, themeColours] : PlugDataLook::colourSettings)
            {
                swatchesToAdd.add(&(swatches[themeName][colourId]));
                auto* swatch = swatchesToAdd.getLast();
                
                auto value = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName).getPropertyAsValue(colourId, nullptr);
                
                swatch->referTo(value);
                swatch->addListener(this);
            }
            i++;
            
            // Add a multi colour component to the properties panel
            panels[colourCategory].add(new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent, 2>(colourName, swatchesToAdd));
        }
        
        
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);
        
        auto* fontPanel = new PropertiesPanel::FontComponent("Default font", fontValue);
        
        allPanels.add(fontPanel);
        addAndMakeVisible(*fontPanel);
        
        panel.addSection("Fonts", {fontPanel});
        
        // Create the panels by category
        for (auto const& [sectionName, sectionColours] : panels)
        {
            for(auto* colourPanel : sectionColours) allPanels.add(colourPanel);
            panel.addSection(sectionName, sectionColours);
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
                auto [colId, colourName, colCat] = PlugDataColourNames.at(colourId);
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
        auto themeRow = bounds.removeFromTop(23);
        g.drawText("Theme", themeRow, Justification::left);
        g.drawText("Dark", themeRow.withX(getWidth() * 0.5f).withWidth(getWidth() / 4), Justification::centred);
        g.drawText("Light", themeRow.withX(getWidth() * 0.75f).withWidth(getWidth() / 4), Justification::centred);
        
        auto fullThemeRow = getLocalBounds().removeFromTop(23);
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(Line<int>(fullThemeRow.getBottomLeft(), fullThemeRow.getBottomRight()).toFloat(), -1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Space for dark/light labels
        bounds.removeFromTop(23);
        
        panel.setBounds(bounds);
    }
    
    void resetColours() {
        auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
        lnf.resetColours();

        dynamic_cast<PropertiesPanel::FontComponent*>(allPanels[0])->setFont("Inter");
        fontValue = "Inter";
        
        lnf.setDefaultFont(fontValue.toString());
        settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);

        auto colourThemesTree = settingsTree.getChildWithName("ColourThemes");
        
        for (auto const& [themeName, theme] : lnf.colourSettings) {
            auto themeTree = colourThemesTree.getChildWithName(themeName);
            for (auto const& [colourId, colourValue] : theme) {
                auto [colId, colourName, colCat] = PlugDataColourNames.at(colourId);
                swatches[themeName][colourName] = colourValue.toString();
                themeTree.setProperty(colourName, colourValue.toString(), nullptr);
            }
        }


        lnf.setTheme(lnf.isUsingLightTheme);
        getTopLevelComponent()->repaint();

        
        for (auto* panel : allPanels) {
            if (auto* colourPanel = dynamic_cast<PropertiesPanel::ColourComponent*>(panel)) {
                colourPanel->updateColour();
            }
        }
    }
    
};

struct ThemePanel : public Component
{

    ColourProperties colourProperties;
    
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
        
        
        addAndMakeVisible(colourProperties);
    }

    void resized() override
    {
        // Add a row for font and light/dark labels
        int numRows = PlugDataColour::numberOfColours + 2;
        
        colourProperties.setBounds(getLocalBounds().withTrimmedBottom(32));
        resetButton.setBounds(getWidth() - 36, getHeight() - 26, 32, 32);
    }
};
