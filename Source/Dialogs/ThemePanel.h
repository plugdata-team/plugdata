/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class NewThemeDialog : public Component {

public:
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

        ok.onClick = [this, parent]() mutable {
            StringArray allThemes = PlugDataLook::getAllThemes();

            if (nameEditor.getText().isEmpty()) {
                errorMessage = "Theme name cannot be empty";
                repaint();
                return;
            }
            if (allThemes.contains(nameEditor.getText())) {
                errorMessage = "Theme name already taken";
                repaint();
                return;
            }

            MessageManager::callAsync(
                [this, parent]() {
                    cb(1, nameEditor.getText(), baseThemeSelector.getText());
                    parent->closeDialog();
                });
        };

        auto allThemes = PlugDataLook::getAllThemes();
        int i = 1;
        for (auto& theme : allThemes) {
            baseThemeSelector.addItem(theme, i);
            i++;
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
        label.setBounds(10, 7, 200, 30);
        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

        nameEditor.setBounds(90, 45, getWidth() - 100, 25);
        baseThemeSelector.setBounds(90, 85, getWidth() - 100, 25);

        nameLabel.setBounds(8, 45, 80, 25);
        baseThemeLabel.setBounds(8, 85, 80, 25);
    }

    void paint(Graphics& g) override
    {
        if (errorMessage.isNotEmpty()) {
            Fonts::drawText(g, errorMessage, 0, getHeight() - 70, getWidth(), 23, Colours::red, 15, Justification::centred);
        }
    }

    std::function<void(int, String, String)> cb;

private:
    Label label = Label("", "Create a new theme");

    Label nameLabel = Label("", "Name:");
    Label baseThemeLabel = Label("", "Based on:");

    TextEditor nameEditor;
    ComboBox baseThemeSelector;

    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");

    String errorMessage;
};

class ThemePanel : public Component
    , public Value::Listener
    , public SettingsFileListener {

    Value fontValue;

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

    PluginProcessor* pd;

public:
    explicit ThemePanel(PluginProcessor* processor)
        : pd(processor)
    {
        resetButton.setTooltip("Reset to default");
        resetButton.getProperties().set("Style", "SmallIcon");
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

        StringArray allThemes = PlugDataLook::getAllThemes();

        newButton.setTooltip("New theme");
        newButton.getProperties().set("Style", "SmallIcon");
        addAndMakeVisible(newButton);
        newButton.setConnectedEdges(12);
        newButton.onClick = [this]() {
            auto callback = [this](int result, String name, String baseTheme) {
                if (!result)
                    return;

                auto colourThemes = SettingsFile::getInstance()->getColourThemesTree();
                auto newTheme = colourThemes.getChildWithProperty("theme", baseTheme).createCopy();
                newTheme.setProperty("theme", name, nullptr);
                colourThemes.appendChild(newTheme, nullptr);

                updateThemes();
                updateSwatches();
            };

            auto* d = new Dialog(&dialog, getParentComponent(), 400, 190, 220, false);
            auto* dialogContent = new NewThemeDialog(d, callback);

            d->setViewedComponent(dialogContent);
            dialog.reset(d);
        };

        loadButton.setTooltip("Load theme");
        loadButton.getProperties().set("Style", "SmallIcon");
        addAndMakeVisible(loadButton);
        loadButton.setConnectedEdges(12);
        loadButton.onClick = [this]() {
            auto constexpr folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

            openChooser = std::make_unique<FileChooser>("Choose theme to open", File::getSpecialLocation(File::userHomeDirectory), "*.plugdatatheme", true);

            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser) {
                auto result = fileChooser.getResult();
                auto themeXml = result.loadFileAsString();
                auto themeTree = ValueTree::fromXml(themeXml);
                auto themeName = themeTree.getProperty("theme").toString();

                auto allThemes = PlugDataLook::getAllThemes();
                if (allThemes.contains(themeName)) {
                    int i = 1;
                    auto finalThemeName = themeName + "_" + String(i);

                    while (allThemes.contains(finalThemeName)) {
                        i++;
                        finalThemeName = themeName + "_" + String(i);
                    }

                    themeName = finalThemeName;
                }

                SettingsFile::getInstance()->getColourThemesTree().appendChild(themeTree, nullptr);

                updateThemes();
                updateSwatches();
            });
        };

        saveButton.setTooltip("Save theme");
        saveButton.getProperties().set("Style", "SmallIcon");
        addAndMakeVisible(saveButton);
        saveButton.setConnectedEdges(12);
        saveButton.onClick = [this]() mutable {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&saveButton).withParentComponent(this), [this, allThemes](int result) {
                if (result < 1)
                    return;

                auto themeName = allThemes[result - 1];

                auto themeTree = SettingsFile::getInstance()->getColourThemesTree().getChildWithProperty("theme", themeName);

                auto themeXml = themeTree.toXmlString();

                saveChooser = std::make_unique<FileChooser>("Choose a location...", File::getSpecialLocation(File::userHomeDirectory), "*.plugdatatheme", true);

#if JUCE_LINUX
                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
#else
                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles;
#endif

                saveChooser->launchAsync(folderChooserFlags,
                    [this, themeXml](FileChooser const& fileChooser) mutable {
                        const auto file = fileChooser.getResult();
                        file.replaceWithText(themeXml);
                    });
            });
        };

        deleteButton.setTooltip("Delete theme");
        deleteButton.getProperties().set("Style", "SmallIcon");
        addAndMakeVisible(deleteButton);
        deleteButton.setConnectedEdges(12);
        deleteButton.onClick = [this]() mutable {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&deleteButton).withParentComponent(this), [this, allThemes](int result) {
                if (result < 1)
                    return;

                auto colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();
                auto selectedThemesTree = SettingsFile::getInstance()->getSelectedThemesTree();
                auto themeName = allThemes[result - 1];

                auto themeTree = colourThemesTree.getChildWithProperty("theme", themeName);

                colourThemesTree.removeChild(themeTree, nullptr);

                auto selectedThemes = selectedThemesTree;
                if (selectedThemes.getProperty("first").toString() == themeName) {
                    selectedThemes.setProperty("first", "light", nullptr);
                    PlugDataLook::selectedThemes.set(0, "light");
                }
                if (selectedThemes.getProperty("second").toString() == themeName) {
                    selectedThemes.setProperty("second", "dark", nullptr);
                    PlugDataLook::selectedThemes.set(1, "dark");
                }

                updateThemes();
                updateSwatches();
            });
        };

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < allThemes.size(); j++) {
                themeSelectors[i].addItem(allThemes[j], j + 1);
            }

            themeSelectors[i].setSelectedItemIndex(allThemes.indexOf(PlugDataLook::selectedThemes[i]), dontSendNotification);
            addAndMakeVisible(themeSelectors[i]);

            themeSelectors[i].setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::textColourId, findColour(PlugDataColour::panelTextColourId));

            themeSelectors[i].onChange = [this, i]() mutable {
                int themeIdx = PlugDataLook::selectedThemes.indexOf(PlugDataLook::currentTheme);

                String themeId = i == 0 ? "first" : "second";

                SettingsFile::getInstance()->getSelectedThemesTree().setProperty(themeId, themeSelectors[i].getText(), nullptr);

                auto selectedThemeName = themeSelectors[i].getText();

                if (selectedThemeName.isEmpty())
                    return;

                PlugDataLook::selectedThemes.set(i, selectedThemeName);
                updateSwatches();

                pd->setTheme(PlugDataLook::selectedThemes[themeIdx]);
                SettingsFile::getInstance()->setProperty("theme", PlugDataLook::selectedThemes[themeIdx]);

                getTopLevelComponent()->repaint();

                SettingsFile::getInstance()->saveSettings();
            };
        }

        addAndMakeVisible(panel);

        // font setting
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);

        updateSwatches();
    }

    void settingsFileReloaded() override
    {
        updateSwatches();
    };

    void updateSwatches()
    {
        panel.clear();
        allPanels.clear();

        std::map<String, Array<PropertyComponent*>> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {

            auto& [colourName, colourId, colourCategory] = colourNames;

            Array<Value*> swatchesToAdd;

            // Loop over themes
            for (int i = 0; i < 2; i++) {
                auto themeName = PlugDataLook::selectedThemes[i];
                swatchesToAdd.add(&(swatches[themeName][colourId]));
                auto* swatch = swatchesToAdd.getLast();

                auto value = SettingsFile::getInstance()->getColourThemesTree().getChildWithProperty("theme", themeName).getPropertyAsValue(colourId, nullptr);

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

        Array<Value*> dashedConnectionValues, straightConnectionValues, squareIoletsValues, squareObjectCornersValues, thinConnectionValues;

        for (int i = 0; i < 2; i++) {
            auto const& themeName = PlugDataLook::selectedThemes[i];
            auto& swatch = swatches[themeName];
            auto themeTree = SettingsFile::getInstance()->getTheme(themeName);

            swatch["dashed_signal_connections"].referTo(themeTree.getPropertyAsValue("dashed_signal_connections", nullptr));
            swatch["straight_connections"].referTo(themeTree.getPropertyAsValue("straight_connections", nullptr));
            swatch["square_iolets"].referTo(themeTree.getPropertyAsValue("square_iolets", nullptr));
            swatch["square_object_corners"].referTo(themeTree.getPropertyAsValue("square_object_corners", nullptr));
            swatch["thin_connections"].referTo(themeTree.getPropertyAsValue("thin_connections", nullptr));

            swatch["dashed_signal_connections"].addListener(this);
            swatch["straight_connections"].addListener(this);
            swatch["square_iolets"].addListener(this);
            swatch["square_object_corners"].addListener(this);
            swatch["thin_connections"].addListener(this);

            dashedConnectionValues.add(&swatch["dashed_signal_connections"]);
            straightConnectionValues.add(&swatch["straight_connections"]);
            squareIoletsValues.add(&swatch["square_iolets"]);
            squareObjectCornersValues.add(&swatch["square_object_corners"]);
            thinConnectionValues.add(&swatch["thin_connections"]);
        }

        auto* useStraightConnections = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Use straight line for connections", straightConnectionValues, { "No", "Yes" });
        allPanels.add(useStraightConnections);
        addAndMakeVisible(*useStraightConnections);

        auto* useDashedSignalConnection = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Display signal connections dashed", dashedConnectionValues, { "No", "Yes" });
        allPanels.add(useDashedSignalConnection);
        addAndMakeVisible(*useDashedSignalConnection);

        auto* useThinConnection = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Use thin connection style", thinConnectionValues, { "No", "Yes" });
        allPanels.add(useThinConnection);
        addAndMakeVisible(*useThinConnection);

        panel.addSection("Connection Look", { useStraightConnections, useDashedSignalConnection, useThinConnection });

        auto* useSquareObjectCorners = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Use squared object corners", squareObjectCornersValues, { "No", "Yes" });
        allPanels.add(useSquareObjectCorners);
        addAndMakeVisible(*useSquareObjectCorners);

        auto* useSquareIolets = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Use square iolets", squareIoletsValues, { "No", "Yes" });
        allPanels.add(useSquareIolets);
        addAndMakeVisible(*useSquareIolets);

        panel.addSection("Object Look", { useSquareObjectCorners, useSquareIolets });

        // Create the panels by category
        for (auto const& [sectionName, sectionColours] : panels) {
            for (auto* colourPanel : sectionColours)
                allPanels.add(colourPanel);
            panel.addSection(sectionName, sectionColours);
        }
    }

    void updateThemes()
    {
        for (int i = 0; i < 2; i++) {

            auto selectedText = themeSelectors[i].getText();
            themeSelectors[i].clear();

            StringArray allThemes = PlugDataLook::getAllThemes();
            for (int j = 0; j < allThemes.size(); j++) {
                themeSelectors[i].addItem(allThemes[j], j + 1);
            }

            int newIdx = allThemes.indexOf(selectedText);

            if (isPositiveAndBelow(newIdx, themeSelectors[i].getNumItems())) {
                themeSelectors[i].setSelectedItemIndex(newIdx, dontSendNotification);
            } else {
                themeSelectors[i].setSelectedItemIndex(i, dontSendNotification);
            }
        }

        if (!PlugDataLook::selectedThemes.contains(PlugDataLook::currentTheme)) {
            PlugDataLook::currentTheme = PlugDataLook::selectedThemes[0];
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::currentTheme);
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(fontValue)) {
            PlugDataLook::setDefaultFont(fontValue.toString());
            SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());
            getTopLevelComponent()->repaint();
            for (auto* panel : allPanels)
                panel->repaint();

            return;
        }

        if (v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["dashed_signal_connections"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["straight_connections"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["square_iolets"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["square_object_corners"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["thin_connections"])) {

            pd->setTheme(PlugDataLook::currentTheme, true);
            return;
        }

        auto themeTree = SettingsFile::getInstance()->getColourThemesTree();
        for (auto theme : themeTree) {
            auto themeName = theme.getProperty("theme").toString();

            for (auto [colourId, colourInfo] : PlugDataColourNames) {
                auto& [colId, colourName, colCat] = colourInfo;

                if (v.refersToSameSourceAs(swatches[themeName][colourName])) {
                    theme.setProperty(colourName, v.toString(), nullptr);
                    pd->setTheme(PlugDataLook::currentTheme, true);
                    return;
                }
            }
        }

        for (int i = 0; i < 2; i++) {
            themeSelectors[i].setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::textColourId, findColour(PlugDataColour::panelTextColourId));
        }
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().removeFromLeft(getWidth() / 2).withTrimmedLeft(6);

        auto themeRow = bounds.removeFromTop(23);
        Fonts::drawText(g, "theme", themeRow, findColour(PlugDataColour::panelTextColourId));

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
        auto colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();

        PlugDataLook::resetColours(colourThemesTree);

        dynamic_cast<PropertiesPanel::FontComponent*>(allPanels[0])->setFont("Inter");
        fontValue = "Inter";

        PlugDataLook::setDefaultFont(fontValue.toString());
        SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());

        pd->setTheme(PlugDataLook::currentTheme);
        updateSwatches();
    }
};
