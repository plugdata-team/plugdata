/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct NewThemeDialog : public Component {
    NewThemeDialog(Dialog* parent, std::function<void(int, String, String)> callback)
        : cb(callback)
    {
        setSize(400, 200);

        addAndMakeVisible(label);
        addAndMakeVisible(cancel);
        addAndMakeVisible(ok);

        cancel.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    cb(0, "", "");
                    parent->closeDialog();
                });
        };

        ok.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    cb(1, nameEditor.getText(), baseThemeSelector.getText());
                    parent->closeDialog();
                });
        };

        for(int i = 0; i < PlugDataLook::colourSettings.size(); i++) {
            auto it = PlugDataLook::colourSettings.begin();
            std::advance(it, i);
            auto [themeName, themeColours] = *it;
            
            baseThemeSelector.addItem(themeName, i + 1);
        }
        
        baseThemeSelector.setSelectedItemIndex(0);
        
        cancel.changeWidthToFitText();
        ok.changeWidthToFitText();

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(baseThemeLabel);

        addAndMakeVisible(nameEditor);
        addAndMakeVisible(baseThemeSelector);
        
        setOpaque(false);
    }

    void resized() override
    {
        label.setBounds(20, 7, 200, 30);
        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

        nameEditor.setBounds(90, 45, getWidth() - 85, 25);
        baseThemeSelector.setBounds(90, 85, getWidth() - 85, 25);
        nameLabel.setBounds(8, 45, 80, 25);
        baseThemeLabel.setBounds(8, 85, 80, 25);
    }

    std::function<void(int, String, String)> cb;

private:
    Label label = Label("", "Create a new theme");

    Label nameLabel = Label("", "Name:");
    Label baseThemeLabel = Label("", "Base theme:");

    TextEditor nameEditor;
    ComboBox baseThemeSelector;

    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");
};


struct ThemePanel : public Component
    , public Value::Listener {

    ValueTree settingsTree;
    Value fontValue;
    Value dashedSignalConnection;
    Value straightConnections;
        
    ComboBox themeSelectors[2];

    std::map<String, std::map<String, Value>> swatches;

    PropertiesPanel panel;
    Array<PropertyComponent*> allPanels;

    TextButton resetButton = TextButton(Icons::Refresh);
        
    TextButton newButton = TextButton(Icons::New);
    TextButton loadButton = TextButton(Icons::Open);
    TextButton saveButton = TextButton(Icons::Save);
    TextButton deleteButton = TextButton(Icons::Clear);
    
    std::unique_ptr<Dialog> dialog;
        
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    explicit ThemePanel(ValueTree tree)
        : settingsTree(tree)
    {
        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:reset");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&dialog, getParentComponent(), "Are you sure you want to reset to default theme settings?",
                [this](bool result) {
                    if (result) {
                        resetDefaults();
                    }
                });
        };
        
        StringArray allThemes;
        for(int i = 0; i < PlugDataLook::colourSettings.size(); i++) {
            auto it = PlugDataLook::colourSettings.begin();
            std::advance(it, i);
            auto [themeName, themeColours] = *it;
            allThemes.add(themeName);
        }
        
        newButton.setTooltip("New theme");
        newButton.setName("statusbar:new");
        addAndMakeVisible(newButton);
        newButton.setConnectedEdges(12);
        newButton.onClick = [this](){
            
            auto callback = [this](int result, String name, String baseTheme){
                
                if(!result) return;
                
                auto colourThemes = settingsTree.getChildWithName("ColourThemes");
                auto newTheme = colourThemes.getChildWithProperty("theme", baseTheme).createCopy();
                newTheme.setProperty("theme", name, nullptr);
                colourThemes.appendChild(newTheme, nullptr);
                
                updateThemes();
                updateSwatches();
            };
            
            auto* d = new Dialog(&dialog, getParentComponent(), 400, 170, 200, false);
            auto* dialogContent = new NewThemeDialog(d, callback);

            d->setViewedComponent(dialogContent);
            dialog.reset(d);
        };
        
        loadButton.setTooltip("Load theme");
        loadButton.setName("statusbar:load");
        addAndMakeVisible(loadButton);
        loadButton.setConnectedEdges(12);
        loadButton.onClick = [this](){
            
            auto constexpr folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
            
            openChooser = std::make_unique<FileChooser>("Choose theme to open", File::getSpecialLocation(File::userHomeDirectory), "*.plugdatatheme", true);

            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser) {
                
                auto result = fileChooser.getResult();
                auto themeXml = result.loadFileAsString();
                auto themeTree = ValueTree::fromXml(themeXml);
                auto themeName = themeTree.getProperty("theme").toString();
                
                settingsTree.getChildWithName("ColourThemes").appendChild(themeTree, nullptr);
                
                for (auto const& [colourId, colourNames] : PlugDataColourNames) {
                    auto [id, colourName, category] = colourNames;
                    PlugDataLook::colourSettings[themeName][colourId] = Colour::fromString(themeTree.getProperty(colourName).toString());
                }
                
                updateThemes();
                updateSwatches();
            });
        };
        
        saveButton.setTooltip("Save theme");
        saveButton.setName("statusbar:save");
        addAndMakeVisible(saveButton);
        saveButton.setConnectedEdges(12);
        saveButton.onClick = [this, allThemes]() mutable {
            
            PopupMenu menu;
            
            for(int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }
            
            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&saveButton).withParentComponent(this), [this, allThemes](int result) {
                
                if(result < 1) return;
                
                auto themeName = allThemes[result - 1];
                
                auto themeTree = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName);
                
                auto themeXml = themeTree.toXmlString();
                
                saveChooser = std::make_unique<FileChooser>("Choose a location...", File::getSpecialLocation(File::userHomeDirectory), "*.plugdatatheme", true);

                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
                
                saveChooser->launchAsync(folderChooserFlags,
                    [this, themeXml](FileChooser const& fileChooser) mutable {
                        const auto file = fileChooser.getResult();
                        file.replaceWithText(themeXml);
                    });
                
            });
        };
        
        
        deleteButton.setTooltip("Delete theme");
        deleteButton.setName("statusbar:save");
        addAndMakeVisible(deleteButton);
        deleteButton.setConnectedEdges(12);
        deleteButton.onClick = [this, allThemes]() mutable {
            PopupMenu menu;
            
            for(int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }
            
            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&deleteButton).withParentComponent(this), [this, allThemes](int result) {
                
                if(result < 1) return;
                
                auto themeName = allThemes[result - 1];
                
                auto themeTree = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName);
                
                settingsTree.getChildWithName("ColourThemes").removeChild(themeTree, nullptr);
                
                PlugDataLook::colourSettings.erase(themeName);
                
                auto selectedThemes = settingsTree.getChildWithName("SelectedThemes");
                if(selectedThemes.getProperty("first").toString() == themeName) {
                    selectedThemes.setProperty("first", "light", nullptr);
                    PlugDataLook::selectedThemes.set(0, "light");
                }
                if(selectedThemes.getProperty("second").toString() == themeName) {
                    selectedThemes.setProperty("second", "dark", nullptr);
                    PlugDataLook::selectedThemes.set(0, "dark");
                }
                
                updateThemes();
                updateSwatches();
            });
        };
        
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < allThemes.size(); j++) {
                themeSelectors[i].addItem(allThemes[j], j + 1);
            }
            
            themeSelectors[i].setSelectedItemIndex(allThemes.indexOf(PlugDataLook::selectedThemes[i]));
            addAndMakeVisible(themeSelectors[i]);
            
            themeSelectors[i].setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            
            themeSelectors[i].onChange = [this, i]() mutable {
                
                int themeIdx = PlugDataLook::selectedThemes.indexOf(PlugDataLook::currentTheme);
                
                String themeId = i == 0 ? "first" : "second";
                
                settingsTree.getChildWithName("SelectedThemes").setProperty(themeId, themeSelectors[i].getText(), nullptr);
                
                auto selectedThemeName = themeSelectors[i].getText();
                
                if(selectedThemeName.isEmpty()) return;
                
                PlugDataLook::selectedThemes.set(i, selectedThemeName);
                updateSwatches();
                
                auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
                
                lnf.setTheme(PlugDataLook::selectedThemes[themeIdx]);
                settingsTree.setProperty("Theme", PlugDataLook::selectedThemes[themeIdx], nullptr);
                
                getTopLevelComponent()->repaint();
            };
        }
        
        addAndMakeVisible(panel);
    
        
        // straight connections
        straightConnections.referTo(settingsTree.getPropertyAsValue("StraightConnections", nullptr));
        straightConnections.addListener(this);

        // dashed signal setting
        dashedSignalConnection.referTo(settingsTree.getPropertyAsValue("DashedSignalConnection", nullptr));
        dashedSignalConnection.addListener(this);
        
        // font setting
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);
        
        updateSwatches();
    }
        
    void updateSwatches() {
        
        panel.clear();
        allPanels.clear();
        
        std::map<String, Array<PropertyComponent*>> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {

            auto& [colourName, colourId, colourCategory] = colourNames;

            Array<Value*> swatchesToAdd;

            // Loop over themes
            for(int i = 0; i < 2; i++) {
                auto themeName = PlugDataLook::selectedThemes[i];
                auto const& themeColours = PlugDataLook::colourSettings[themeName];
                swatchesToAdd.add(&(swatches[themeName][colourId]));
                auto* swatch = swatchesToAdd.getLast();

                auto value = settingsTree.getChildWithName("ColourThemes").getChildWithProperty("theme", themeName).getPropertyAsValue(colourId, nullptr);

                swatch->referTo(value);
                swatch->addListener(this);
            }

            // Add a multi colour component to the properties panel
            panels[colourCategory].add(new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent>(colourName, swatchesToAdd));
        }
        
        auto* fontPanel = new PropertiesPanel::FontComponent("Default font", fontValue);

        allPanels.add(fontPanel);
        addAndMakeVisible(*fontPanel);

        panel.addSection("Fonts", { fontPanel });

        auto* useStraightConnections = new PropertiesPanel::BoolComponent("Use straight line for connections", straightConnections, { "No", "Yes" });
        allPanels.add(useStraightConnections);
        addAndMakeVisible(*useStraightConnections);

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
    
    void updateThemes() {
        for(int i = 0; i < 2; i++) {
            
            auto selectedText = themeSelectors[i].getText();
            themeSelectors[i].clear();
            
            StringArray allThemes;
            for(int j = 0; j < PlugDataLook::colourSettings.size(); j++) {
                auto it = PlugDataLook::colourSettings.begin();
                std::advance(it, j);
                auto [themeName, themeColours] = *it;
   
                themeSelectors[i].addItem(themeName, j + 1);
                allThemes.add(themeName);
            }
            
            int newIdx = allThemes.indexOf(selectedText);
            
            if(isPositiveAndBelow(newIdx, themeSelectors[i].getNumItems())) {
                themeSelectors[i].setSelectedItemIndex(newIdx, dontSendNotification);
            }
            else {
                themeSelectors[i].setSelectedItemIndex(i, dontSendNotification);
            }
        }
        
        if(!PlugDataLook::selectedThemes.contains(PlugDataLook::currentTheme)) {
            PlugDataLook::currentTheme = PlugDataLook::selectedThemes[0];
            settingsTree.setProperty("Theme", PlugDataLook::currentTheme, nullptr);
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

                    lnf.setTheme(lnf.currentTheme);
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


        auto fullThemeRow = getLocalBounds().removeFromTop(23);
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(Line<int>(fullThemeRow.getBottomLeft(), fullThemeRow.getBottomRight()).toFloat(), -1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto themeRow = bounds.removeFromTop(23);
        themeSelectors[0].setBounds(themeRow.withX(getWidth() * 0.5f).withWidth(getWidth() / 4));
        themeSelectors[1].setBounds(themeRow.withX(getWidth() * 0.75f).withWidth(getWidth() / 4));

        bounds.removeFromBottom(30);

        panel.setBounds(bounds);

        resetButton.setBounds(getWidth() - 36, getHeight() - 26, 32, 32);
        
        newButton.setBounds(4, getHeight() - 26, 32, 32);
        loadButton.setBounds(40, getHeight() - 26, 32, 32);
        saveButton.setBounds(76, getHeight() - 26, 32, 32);
        deleteButton.setBounds(112, getHeight() - 26, 32, 32);
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
            auto themeTree = colourThemesTree.getChildWithProperty("theme", themeName);
            for (auto const& [colourId, colourValue] : theme) {
                auto [colId, colourName, colCat] = PlugDataColourNames.at(colourId);
                swatches[themeName][colourName] = colourValue.toString();
                themeTree.setProperty(colourName, colourValue.toString(), nullptr);
            }
        }

        lnf.setTheme(lnf.currentTheme);
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
