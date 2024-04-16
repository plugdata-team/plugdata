#include <utility>

/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class NewThemeDialog : public Component {

public:
    NewThemeDialog(Dialog* parent, std::function<void(int, String, String)> callback)
        : cb(std::move(callback))
    {
        setSize(400, 170);

        label.setFont(Fonts::getBoldFont().withHeight(14.0f));
        label.setJustificationType(Justification::centred);

        nameEditor.setJustification(Justification::centredLeft);

        auto backgroundColour = findColour(PlugDataColour::dialogBackgroundColourId);
        ok.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        ok.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        ok.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        cancel.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        cancel.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        cancel.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

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

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(baseThemeLabel);

        addAndMakeVisible(nameEditor);
        addAndMakeVisible(baseThemeSelector);

        setOpaque(false);
    }

    void resized() override
    {
        label.setBounds(0, 7, getWidth(), 30);
        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

        nameEditor.setBounds(90, 43, getWidth() - 100, 28);
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

struct ThemeSelectorProperty : public PropertiesPanelProperty {
    ThemeSelectorProperty(String const& propertyName, std::function<void(String const&)> const& callback)
        : PropertiesPanelProperty(propertyName)
        , cb(callback)
    {
        comboBox.getProperties().set("Style", "Inspector");
        comboBox.onChange = [this, callback]() {
            callback(comboBox.getText());
        };

        comboBox.setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
        comboBox.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        comboBox.setColour(ComboBox::textColourId, findColour(PlugDataColour::panelTextColourId));

        addAndMakeVisible(comboBox);
    }

    PropertiesPanelProperty* createCopy() override
    {
        auto* themeSelector = new ThemeSelectorProperty(getName(), cb);
        themeSelector->setOptions(items);
        themeSelector->setSelectedItem(comboBox.getSelectedItemIndex());
        return themeSelector;
    }

    String getText() const
    {
        return comboBox.getText();
    }

    void setSelectedItem(int idx)
    {
        comboBox.setSelectedItemIndex(idx, dontSendNotification);
    }

    void setOptions(StringArray const& options)
    {
        items = options;
        comboBox.clear();
        comboBox.addItemList(options, 1);
    }

    void resized() override
    {
        comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
    }

    StringArray items;
    std::function<void(String const&)> cb;
    ComboBox comboBox;
};

class ThemePanel : public SettingsDialogPanel
    , public Value::Listener
    , public SettingsFileListener {

    Value fontValue;

    std::map<String, std::map<String, Value>> swatches;

    PropertiesPanel::ActionComponent* newButton = nullptr;
    PropertiesPanel::ActionComponent* loadButton = nullptr;
    PropertiesPanel::ActionComponent* saveButton = nullptr;
    PropertiesPanel::ActionComponent* deleteButton = nullptr;

    ThemeSelectorProperty* primaryThemeSelector = nullptr;
    ThemeSelectorProperty* secondaryThemeSelector = nullptr;

    PropertiesPanel panel;
    Array<PropertyComponent*> allPanels;

    std::unique_ptr<Dialog> dialog;

    PluginProcessor* pd;

public:
    explicit ThemePanel(PluginProcessor* processor)
        : pd(processor)
    {

        StringArray allThemes = PlugDataLook::getAllThemes();

        addAndMakeVisible(panel);

        // font setting
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font())->getName());
        fontValue.addListener(this);

        updateSwatches();
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &panel;
    }

    void updateThemeNames(String const& firstTheme, String const& secondTheme)
    {
        auto sections = panel.getSectionNames();
        for (int i = 3; i < sections.size(); i++) {
            panel.setExtraHeaderNames(i, { firstTheme, secondTheme });
        }
    }

    void updateSwatches()
    {
        auto scrollPosition = panel.getViewport().getViewPositionY();
        
        panel.clear();
        allPanels.clear();

        std::map<String, Array<PropertiesPanelProperty*>> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {

            auto& [colourName, colourId, colourCategory] = colourNames;

            Array<Value*> swatchesToAdd;

            // Loop over themes
            for (int i = 0; i < 2; i++) {
                auto const& themeName = PlugDataLook::selectedThemes[i];
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

        std::function<void(int, String)> onThemeChange = [this](int themeSlot, String const& newThemeName) {
            auto allThemes = PlugDataLook::getAllThemes();
            int themeIdx = PlugDataLook::selectedThemes.indexOf(PlugDataLook::currentTheme);

            String themeId = themeSlot ? "second" : "first";

            SettingsFile::getInstance()->getSelectedThemesTree().setProperty(themeId, newThemeName, nullptr);

            if (newThemeName.isEmpty())
                return;

            PlugDataLook::selectedThemes.set(themeSlot, newThemeName);

            updateThemeNames(primaryThemeSelector->getText(), secondaryThemeSelector->getText());

            pd->setTheme(PlugDataLook::selectedThemes[themeIdx]);
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::selectedThemes[themeIdx]);

            getTopLevelComponent()->repaint();

            SettingsFile::getInstance()->saveSettings();

            MessageManager::callAsync([_this = SafePointer(this)]() {
                _this->updateSwatches();
            });
        };

        auto* resetButton = new PropertiesPanel::ActionComponent([this]() {
            Dialogs::showOkayCancelDialog(&dialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset to default theme settings?",
                [this](bool result) {
                    if (result) {
                        resetDefaults();
                    }
                });
        },
            Icons::Reset, "Reset all themes to default", true);

        newButton = new PropertiesPanel::ActionComponent([this]() {
            auto callback = [this](int result, const String& name, const String& baseTheme) {
                if (!result)
                    return;

                auto colourThemes = SettingsFile::getInstance()->getColourThemesTree();
                auto newTheme = colourThemes.getChildWithProperty("theme", baseTheme).createCopy();
                newTheme.setProperty("theme", name, nullptr);
                colourThemes.appendChild(newTheme, nullptr);

                updateSwatches();
            };

            auto* d = new Dialog(&dialog, getParentComponent(), 400, 170, false);
            auto* dialogContent = new NewThemeDialog(d, callback);

            d->setViewedComponent(dialogContent);
            dialog.reset(d);
        },
            Icons::New, "New theme...");

        loadButton = new PropertiesPanel::ActionComponent([this]() {
            Dialogs::showOpenDialog([this](URL url) {
                auto result = url.getLocalFile();
                if (!result.exists())
                    return;

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

                themeTree.setProperty("theme", themeName, nullptr);
                SettingsFile::getInstance()->getColourThemesTree().appendChild(themeTree, nullptr);
                updateSwatches();
            },
                true, false, "*.plugdatatheme", "ThemeLocation", getTopLevelComponent());
        },
            Icons::Open, "Import theme...");

        saveButton = new PropertiesPanel::ActionComponent([this]() {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(saveButton).withParentComponent(this), [this, allThemes](int result) {
                if (result < 1)
                    return;

                const auto& themeName = allThemes[result - 1];

                auto themeTree = SettingsFile::getInstance()->getColourThemesTree().getChildWithProperty("theme", themeName);

                auto themeXml = themeTree.toXmlString();

                Dialogs::showSaveDialog([themeXml](URL url) {
                    auto result = url.getLocalFile();
                    if (result.getParentDirectory().exists()) {
                        result.replaceWithText(themeXml);
                    }
                },
                    "*.plugdatatheme", "ThemeLocation", getTopLevelComponent());
            });
        },
            Icons::Save, "Export theme...");

        deleteButton = new PropertiesPanel::ActionComponent([this]() {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(deleteButton).withParentComponent(this), [this, allThemes](int result) {
                if (result < 1)
                    return;

                auto colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();
                auto selectedThemesTree = SettingsFile::getInstance()->getSelectedThemesTree();
                const auto& themeName = allThemes[result - 1];

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

                updateSwatches();
            });
        },
            Icons::Trash, "Delete theme...", false, true);

        panel.addSection("Manage themes", { resetButton, newButton, loadButton, saveButton, deleteButton });

        primaryThemeSelector = new ThemeSelectorProperty("Primary Theme", [onThemeChange](String const& selectedThemeName) {
            onThemeChange(0, selectedThemeName);
        });

        secondaryThemeSelector = new ThemeSelectorProperty("Secondary Theme", [onThemeChange](String const& selectedThemeName) {
            onThemeChange(1, selectedThemeName);
        });

        auto allThemes = PlugDataLook::getAllThemes();
        auto firstThemes = allThemes;
        auto secondThemes = allThemes;
        
        // Remove theme selected in other combobox (so you can't pick the same theme twice)
        firstThemes.removeString(PlugDataLook::selectedThemes[1]);
        secondThemes.removeString(PlugDataLook::selectedThemes[0]);
        
        primaryThemeSelector->setOptions(firstThemes);
        secondaryThemeSelector->setOptions(secondThemes);
        
        primaryThemeSelector->setSelectedItem(firstThemes.indexOf(PlugDataLook::selectedThemes[0]));
        secondaryThemeSelector->setSelectedItem(secondThemes.indexOf(PlugDataLook::selectedThemes[1]));

        allPanels.add(fontPanel);
        allPanels.add(primaryThemeSelector);
        allPanels.add(secondaryThemeSelector);

        addAndMakeVisible(*fontPanel);

        panel.addSection("Fonts", { fontPanel });

        panel.addSection("Active Themes", { primaryThemeSelector, secondaryThemeSelector });

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

        if (!PlugDataLook::selectedThemes.contains(PlugDataLook::currentTheme)) {
            PlugDataLook::currentTheme = PlugDataLook::selectedThemes[0];
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::currentTheme);
        }

        updateThemeNames(primaryThemeSelector->getText(), secondaryThemeSelector->getText());

        panel.repaint();
        panel.getViewport().setViewPosition(0, scrollPosition);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(fontValue)) {
            PlugDataLook::setDefaultFont(fontValue.toString());
            SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());
            pd->updateAllEditorsLNF();
            return;
        }

        auto themeTree = SettingsFile::getInstance()->getColourThemesTree();
        if (v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["dashed_signal_connections"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["straight_connections"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["square_iolets"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["square_object_corners"])
            || v.refersToSameSourceAs(swatches[PlugDataLook::currentTheme]["thin_connections"])) {
            for (auto theme : themeTree) {
                auto themeName = theme.getProperty("theme").toString();
                if (v.refersToSameSourceAs(swatches[themeName]["dashed_signal_connections"])) {
                    theme.setProperty("dashed_signal_connections", v.toString(), nullptr);
                }
                else if(v.refersToSameSourceAs(swatches[themeName]["straight_connections"]))
                {
                    theme.setProperty("straight_connections", v.toString(), nullptr);
                }
                else if(v.refersToSameSourceAs(swatches[themeName]["square_iolets"]))
                {
                    theme.setProperty("square_iolets", v.toString(), nullptr);
                }
                else if(v.refersToSameSourceAs(swatches[themeName]["square_object_corners"]))
                {
                    theme.setProperty("square_object_corners", v.toString(), nullptr);
                }
                else if(v.refersToSameSourceAs(swatches[themeName]["thin_connections"]))
                {
                    theme.setProperty("thin_connections", v.toString(), nullptr);
                }
            }
            
            pd->setTheme(PlugDataLook::currentTheme, true);
            return;
        }
        
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

        /* TODO: Fix this, maybe with a LookAndFeelChanged on the property component?
        for (int i = 0; i < 2; i++) {
            themeSelectors[i].setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            themeSelectors[i].setColour(ComboBox::textColourId, findColour(PlugDataColour::panelTextColourId));
        }*/
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        panel.setBounds(bounds);
    }

    void resetDefaults()
    {
        auto colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();

        PlugDataLook::resetColours(colourThemesTree);

        dynamic_cast<PropertiesPanel::FontComponent*>(allPanels[0])->setFont("Inter");
        fontValue = "Inter";

        PlugDataLook::setDefaultFont(fontValue.toString());
        SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());
        
        auto allThemes = PlugDataLook::getAllThemes();
        auto firstThemes = allThemes;
        auto secondThemes = allThemes;
        
        firstThemes.removeString(PlugDataLook::selectedThemes[1]);
        secondThemes.removeString(PlugDataLook::selectedThemes[0]);
        
        primaryThemeSelector->setSelectedItem(firstThemes.indexOf(PlugDataLook::selectedThemes[0]));
        secondaryThemeSelector->setSelectedItem(secondThemes.indexOf(PlugDataLook::selectedThemes[1]));
        
        SettingsFile::getInstance()->getSelectedThemesTree().setProperty("first", "light", nullptr);
        SettingsFile::getInstance()->getSelectedThemesTree().setProperty("second", "dark", nullptr);
        SettingsFile::getInstance()->setProperty("theme", "light");
        
        updateSwatches();
        pd->setTheme(PlugDataLook::selectedThemes[0], true);
    }
};
