/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct ThemePanel : public Component
    , public Value::Listener {

    ValueTree settingsTree;
    Value fontValue;
    Value dashedSignalConnection;
    Value straightConnections;

    std::map<String, std::map<String, Value>> swatches;

    PropertiesPanel panel;
    Array<PropertyComponent*> allPanels;

    TextButton resetButton = TextButton(Icons::Refresh);

    std::unique_ptr<Dialog> confirmationDialog;

    explicit ThemePanel(ValueTree tree)
        : settingsTree(tree)
    {
        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:down");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&confirmationDialog, getParentComponent(), "Are you sure you want to reset to default theme settings?",
                [this](bool result) {
                    if (result) {
                        resetDefaults();
                    }
                });
        };

        addAndMakeVisible(panel);

        std::map<String, Array<PropertyComponent*>> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {

            auto& [colourName, colourId, colourCategory] = colourNames;

            Array<Value*> swatchesToAdd;

            // Loop over themes
            int i = 0;
            for (auto const& [themeName, themeColours] : PlugDataLook::colourSettings) {
                swatchesToAdd.add(&(swatches[themeName][colourId]));
                auto* swatch = swatchesToAdd.getLast();

                auto value = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName).getPropertyAsValue(colourId, nullptr);

                swatch->referTo(value);
                swatch->addListener(this);
            }
            i++;

            // Add a multi colour component to the properties panel
            panels[colourCategory].add(new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent>(colourName, swatchesToAdd));
        }

        // font setting
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);

        auto* fontPanel = new PropertiesPanel::FontComponent("Default font", fontValue);

        allPanels.add(fontPanel);
        addAndMakeVisible(*fontPanel);

        panel.addSection("Fonts", { fontPanel });

        // straight connections
        straightConnections.referTo(settingsTree.getPropertyAsValue("StraightConnections", nullptr));
        straightConnections.addListener(this);

        auto* useStraightConnections = new PropertiesPanel::BoolComponent("Use straight line for connections", straightConnections, { "No", "Yes" });
        allPanels.add(useStraightConnections);
        addAndMakeVisible(*useStraightConnections);

        // dashed signal setting
        dashedSignalConnection.referTo(settingsTree.getPropertyAsValue("DashedSignalConnection", nullptr));
        dashedSignalConnection.addListener(this);

        auto* useDashedSignalConnection = new PropertiesPanel::BoolComponent("Display signal connections dashed", dashedSignalConnection, { "No", "Yes" });

        allPanels.add(useDashedSignalConnection);
        addAndMakeVisible(*useDashedSignalConnection);

        panel.addSection("Connection Look", { useStraightConnections, useDashedSignalConnection });

        // Create the panels by category
        for (auto const& [sectionName, sectionColours] : panels) {
            for (auto* colourPanel : sectionColours)
                allPanels.add(colourPanel);
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

        if (v.refersToSameSourceAs(dashedSignalConnection)) {
            settingsTree.setProperty("DashedSignalConnection", dashedSignalConnection.getValue(), nullptr);
            getTopLevelComponent()->repaint();
            return;
        }

        if (v.refersToSameSourceAs(straightConnections)) {
            settingsTree.setProperty("StraightConnections", straightConnections.getValue(), nullptr);
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
        bounds.removeFromBottom(30);

        panel.setBounds(bounds);

        resetButton.setBounds(getWidth() - 36, getHeight() - 26, 32, 32);
    }

    void resetDefaults()
    {
        auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
        lnf.resetColours();

        dynamic_cast<PropertiesPanel::FontComponent*>(allPanels[0])->setFont("Inter");
        fontValue = "Inter";

        lnf.setDefaultFont(fontValue.toString());
        settingsTree.setProperty("DefaultFont", fontValue.getValue(), nullptr);

        straightConnections = false;
        settingsTree.setProperty("StraightConnections", straightConnections.getValue(), nullptr);

        dashedSignalConnection = false;
        settingsTree.setProperty("DashedSignalConnection", dashedSignalConnection.getValue(), nullptr);

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
            if (auto* multiPanel = dynamic_cast<PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent>*>(panel)) {
                for (auto* property : multiPanel->properties) {
                    property->updateColour();
                }
            }
        }
    }
};
