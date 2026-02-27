/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <utility>

class NewThemeDialog final : public Component {

public:
    NewThemeDialog(Dialog* parent, std::function<void(int, String, String)> callback)
        : cb(std::move(callback))
    {
        setSize(400, 170);

        label.setFont(Fonts::getBoldFont().withHeight(14.0f));
        label.setJustificationType(Justification::centred);

        nameEditor.setJustification(Justification::centredLeft);

        auto const backgroundColour = PlugDataColours::dialogBackgroundColour;
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
                [this, parent] {
                    cb(0, "", "");
                    parent->closeDialog();
                });
        };

        ok.onClick = [this, parent]() mutable {
            StringArray const allThemes = PlugDataLook::getAllThemes();

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
                [this, parent] {
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

struct ThemeSelectorProperty final : public PropertiesPanelProperty {
    ThemeSelectorProperty(String const& propertyName, std::function<void(String const&)> const& callback)
        : PropertiesPanelProperty(propertyName)
        , cb(callback)
    {
        comboBox.getProperties().set("Style", "Inspector");
        comboBox.onChange = [this, callback] {
            callback(comboBox.getText());
        };

        addAndMakeVisible(comboBox);

        setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
        lookAndFeelChanged();
    }

    PropertiesPanelProperty* createCopy() override
    {
        auto* themeSelector = new ThemeSelectorProperty(getName(), cb);
        themeSelector->setOptions(items);
        themeSelector->setSelectedItem(comboBox.getSelectedItemIndex());
        return themeSelector;
    }

    void lookAndFeelChanged() override
    {
        comboBox.setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
        comboBox.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        comboBox.setColour(ComboBox::textColourId, PlugDataColours::panelTextColour);
    }

    String getText() const
    {
        return comboBox.getText();
    }

    void setSelectedItem(int const idx)
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

class ThemeSettingsPanel final : public SettingsDialogPanel
    , public Value::Listener
    , public SettingsFileListener
    , public AsyncUpdater {

    Value fontValue;

    UnorderedSegmentedMap<String, UnorderedSegmentedMap<String, Value>> swatches;

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
    explicit ThemeSettingsPanel(PluginProcessor* processor)
        : pd(processor)
    {

        StringArray allThemes = PlugDataLook::getAllThemes();

        addAndMakeVisible(panel);

        // font setting
        fontValue.setValue(LookAndFeel::getDefaultLookAndFeel().getTypefaceForFont(Font(FontOptions()))->getName());
        fontValue.addListener(this);

        updateSwatches();
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &panel;
    }

    void updateThemeNames(String const& firstTheme, String const& secondTheme)
    {
        auto const sections = panel.getSectionNames();
        for (int i = 3; i < sections.size(); i++) {
            panel.setExtraHeaderNames(i, { firstTheme, secondTheme });
        }
    }

    void handleAsyncUpdate() override
    {
        for (int i = 0; i < 2; i++) {
            for (auto& [name, swatch] : swatches[PlugDataLook::selectedThemes[i]]) {
                swatch.addListener(this);
            }
        }
    }

    void updateSwatches()
    {
        auto scrollPosition = panel.getViewport().getViewPositionY();

        panel.clear();
        allPanels.clear();

        UnorderedSegmentedMap<String, PropertiesArray> panels;

        // Loop over colours
        for (auto const& [colour, colourNames] : PlugDataColourNames) {

            auto& [colourName, colourId, colourCategory] = colourNames;

            SmallArray<Value*> swatchesToAdd;

            // Loop over themes
            for (int i = 0; i < 2; i++) {
                auto const& themeName = PlugDataLook::selectedThemes[i];
                auto& themeSwatches = swatches[themeName];
                auto& swatch = themeSwatches[colourId];
                swatchesToAdd.add(&swatch);

                auto value = SettingsFile::getInstance()->getColourThemesTree().getChildWithProperty("theme", themeName).getPropertyAsValue(colourId, nullptr);

                swatch.referTo(value);
            }

            // Add a multi colour component to the properties panel
            panels[colourCategory].add(new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent>(colourName, swatchesToAdd));
        }

        auto* fontPanel = new PropertiesPanel::FontComponent("Default font", fontValue);

        std::function<void(int, String)> onThemeChange = [this](int const themeSlot, String const& newThemeName) {
            auto allThemes = PlugDataLook::getAllThemes();
            int const themeIdx = PlugDataLook::selectedThemes.indexOf(PlugDataLook::currentTheme);

            String const themeId = themeSlot ? "second" : "first";

            SettingsFile::getInstance()->getSelectedThemesTree().setProperty(themeId, newThemeName, nullptr);

            if (newThemeName.isEmpty())
                return;

            PlugDataLook::selectedThemes.set(themeSlot, newThemeName);

            updateThemeNames(primaryThemeSelector->getText(), secondaryThemeSelector->getText());

            pd->setTheme(PlugDataLook::selectedThemes[themeIdx]);
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::selectedThemes[themeIdx]);

            getTopLevelComponent()->repaint();

            SettingsFile::getInstance()->saveSettings();

            MessageManager::callAsync([_this = SafePointer(this)] {
                _this->updateSwatches();
            });
        };

        auto* resetButton = new PropertiesPanel::ActionComponent([this] {
            Dialogs::showMultiChoiceDialog(&dialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset to default theme settings?",
                [this](int const result) {
                    if (!result) {
                        resetDefaults();
                    }
                });
        },
            Icons::Reset, "Reset all themes to default", true);

        newButton = new PropertiesPanel::ActionComponent([this] {
            auto callback = [this](int const result, String const& name, String const& baseTheme) {
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

        loadButton = new PropertiesPanel::ActionComponent([this] {
            Dialogs::showOpenDialog([this](URL const& url) {
                auto const result = url.getLocalFile();
                if (!result.exists())
                    return;

                auto const themeXml = result.loadFileAsString();
                auto themeTree = ValueTree::fromXml(themeXml);
                auto themeName = themeTree.getProperty("theme").toString();

                auto const allThemes = PlugDataLook::getAllThemes();
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

        saveButton = new PropertiesPanel::ActionComponent([this] {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(saveButton).withParentComponent(this), [this, allThemes](int const result) {
                if (result < 1)
                    return;

                auto const& themeName = allThemes[result - 1];

                auto const themeTree = SettingsFile::getInstance()->getColourThemesTree().getChildWithProperty("theme", themeName);

                auto themeXml = themeTree.toXmlString();

                Dialogs::showSaveDialog([themeXml](URL const& url) {
                    auto result = url.getLocalFile();
                    if (result.getParentDirectory().exists()) {
                        result = result.withFileExtension(".plugdatatheme");
                        result.replaceWithText(themeXml);
                    }
                },
                    "*.plugdatatheme", "ThemeLocation", getTopLevelComponent());
            });
        },
            Icons::Save, "Export theme...");

        deleteButton = new PropertiesPanel::ActionComponent([this] {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i]);
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(deleteButton).withParentComponent(this), [this, allThemes](int const result) {
                if (result < 1)
                    return;

                auto colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();
                auto const selectedThemesTree = SettingsFile::getInstance()->getSelectedThemesTree();
                auto const& themeName = allThemes[result - 1];

                auto const themeTree = colourThemesTree.getChildWithProperty("theme", themeName);

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

        SmallArray<Value*> straightConnectionValues, connectionStyle, connectionLook, ioletSpacingEdge, squareIolets, squareObjectCorners, objectFlagOutlined, highlightSyntax;

        for (int i = 0; i < 2; i++) {
            auto const& themeName = PlugDataLook::selectedThemes[i];
            auto& swatch = swatches[themeName];
            auto themeTree = SettingsFile::getInstance()->getTheme(themeName);

            // settings for connections
            swatch["straight_connections"].referTo(themeTree.getPropertyAsValue("straight_connections", nullptr));
            swatch["connection_style"].referTo(themeTree.getPropertyAsValue("connection_style", nullptr));
            swatch["connection_look"].referTo(themeTree.getPropertyAsValue("connection_look", nullptr));

            // settings for object & iolets
            swatch["iolet_spacing_edge"].referTo(themeTree.getPropertyAsValue("iolet_spacing_edge", nullptr));
            swatch["square_iolets"].referTo(themeTree.getPropertyAsValue("square_iolets", nullptr));
            swatch["square_object_corners"].referTo(themeTree.getPropertyAsValue("square_object_corners", nullptr));
            swatch["object_flag_outlined"].referTo(themeTree.getPropertyAsValue("object_flag_outlined", nullptr));
            swatch["highlight_syntax"].referTo(themeTree.getPropertyAsValue("highlight_syntax", nullptr));

            straightConnectionValues.add(&swatch["straight_connections"]);
            connectionStyle.add(&swatch["connection_style"]);
            connectionLook.add(&swatch["connection_look"]);

            ioletSpacingEdge.add(&swatch["iolet_spacing_edge"]);
            squareIolets.add(&swatch["square_iolets"]);
            squareObjectCorners.add(&swatch["square_object_corners"]);
            objectFlagOutlined.add(&swatch["object_flag_outlined"]);
            highlightSyntax.add(&swatch["highlight_syntax"]);
        }

        auto* useObjectCorners = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Object corners", squareObjectCorners, { "Round", "Square" });
        allPanels.add(useObjectCorners);
        addAndMakeVisible(*useObjectCorners);

        auto* useObjectFlagOutlined = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Object flag style", objectFlagOutlined, { "Filled", "Outlined" });
        allPanels.add(useObjectFlagOutlined);
        addAndMakeVisible(*useObjectFlagOutlined);

        auto* useSyntaxHighlighting = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Enable syntax highlighting", highlightSyntax, { "No", "Yes" });
        allPanels.add(useSyntaxHighlighting);
        addAndMakeVisible(*useSyntaxHighlighting);

        auto* useIoletCorners = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Iolet corners", squareIolets, { "Round", "Square" });
        allPanels.add(useIoletCorners);
        addAndMakeVisible(*useIoletCorners);

        auto* useIoletSpacingEdge = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Iolet spacing", ioletSpacingEdge, { "Centre", "Edge" });
        allPanels.add(useIoletSpacingEdge);
        addAndMakeVisible(*useIoletSpacingEdge);

        auto* useStraightConnections = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Connection path", straightConnectionValues, { "Curved", "Line" });
        allPanels.add(useStraightConnections);
        addAndMakeVisible(*useStraightConnections);

        auto* useConnectionLook = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::BoolComponent>("Connection look", connectionLook, { "Filled", "Gradient" });
        allPanels.add(useConnectionLook);
        addAndMakeVisible(*useConnectionLook);

        auto* useConnectionStyle = new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ComboComponent>("Connection style", connectionStyle, { "Default", "Vanilla", "Thin" });
        allPanels.add(useConnectionStyle);
        addAndMakeVisible(*useConnectionStyle);

        panel.addSection("Object & Connection Look", { useObjectCorners, useObjectFlagOutlined, useSyntaxHighlighting, useIoletCorners, useIoletSpacingEdge, useStraightConnections, useConnectionLook, useConnectionStyle });

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
        triggerAsyncUpdate();
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(fontValue)) {
            auto const previousFontName = Fonts::getCurrentFont().toString();

            PlugDataLook::setDefaultFont(fontValue.toString());
            SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());

            if (previousFontName != Fonts::getCurrentFont().toString())
                pd->updateAllEditorsLNF();

            CachedStringWidth<14>::clearCache();
            CachedStringWidth<15>::clearCache();

            return;
        }

        auto const themeTree = SettingsFile::getInstance()->getColourThemesTree();
        bool isInTheme = false;
        bool ioletGeometryNeedsUpdate = false;
        for (auto theme : PlugDataLook::selectedThemes) {
            if (v.refersToSameSourceAs(swatches[theme]["straight_connections"])
                || v.refersToSameSourceAs(swatches[theme]["iolet_spacing_edge"])
                || v.refersToSameSourceAs(swatches[theme]["square_iolets"])
                || v.refersToSameSourceAs(swatches[theme]["square_object_corners"])
                || v.refersToSameSourceAs(swatches[theme]["connection_look"])
                || v.refersToSameSourceAs(swatches[theme]["connection_style"])
                || v.refersToSameSourceAs(swatches[theme]["object_flag_outlined"])
                || v.refersToSameSourceAs(swatches[theme]["highlight_syntax"])) {
                if (v.refersToSameSourceAs(swatches[theme]["iolet_spacing_edge"]))
                    ioletGeometryNeedsUpdate = true;

                isInTheme = true;
                break;
            }
        }

        if (isInTheme) {
            for (auto theme : themeTree) {
                auto themeName = theme.getProperty("theme").toString();
                if (v.refersToSameSourceAs(swatches[themeName]["straight_connections"])) {
                    theme.setProperty("straight_connections", v.toString(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["connection_style"])) {
                    theme.setProperty("connection_style", v.toString().getIntValue(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["connection_look"])) {
                    theme.setProperty("connection_look", v.toString(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["iolet_spacing_edge"])) {
                    theme.setProperty("iolet_spacing_edge", v.toString().getIntValue(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["square_iolets"])) {
                    theme.setProperty("square_iolets", v.toString().getIntValue(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["square_object_corners"])) {
                    theme.setProperty("square_object_corners", v.toString().getIntValue(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["object_flag_outlined"])) {
                    theme.setProperty("object_flag_outlined", v.toString().getIntValue(), nullptr);
                } else if (v.refersToSameSourceAs(swatches[themeName]["highlight_syntax"])) {
                    theme.setProperty("highlight_syntax", v.toString().getIntValue(), nullptr);
                }
            }

            pd->setTheme(PlugDataLook::currentTheme, true);

            if (ioletGeometryNeedsUpdate)
                PluginEditor::updateIoletGeometryForAllObjects(pd);

            return;
        }

        for (auto theme : themeTree) {
            auto themeName = theme.getProperty("theme").toString();

            for (auto [colourId, colourInfo] : PlugDataColourNames) {
                auto& [colId, colourName, colCat] = colourInfo;

                if (v.refersToSameSourceAs(swatches[themeName][colourName])) {
                    theme.setProperty(colourName, v.toString(), nullptr);
                    pd->setTheme(PlugDataLook::currentTheme, true);
                    sendLookAndFeelChange();
                    return;
                }
            }
        }
    }

    void resized() override
    {
        auto const bounds = getLocalBounds();
        panel.setBounds(bounds);
    }

    void resetDefaults()
    {
        auto const colourThemesTree = SettingsFile::getInstance()->getColourThemesTree();
        auto const currentIoletSpacing = getValue<bool>(swatches[PlugDataLook::currentTheme]["iolet_spacing_edge"]);

        PlugDataLook::resetColours(colourThemesTree);

        dynamic_cast<PropertiesPanel::FontComponent*>(allPanels[0])->setFont("Inter");
        fontValue = "Inter";

        PlugDataLook::setDefaultFont(fontValue.toString());
        SettingsFile::getInstance()->setProperty("default_font", fontValue.getValue());

        auto const allThemes = PlugDataLook::getAllThemes();
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
        sendLookAndFeelChange();

        auto const newIoletSpacing = getValue<bool>(swatches[PlugDataLook::currentTheme]["iolet_spacing_edge"]);
        if (currentIoletSpacing != newIoletSpacing)
            PluginEditor::updateIoletGeometryForAllObjects(pd);
    }
};
