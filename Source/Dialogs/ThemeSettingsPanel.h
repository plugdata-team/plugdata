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

        auto const backgroundColour = getThemeColours(*this).dialogBackgroundColour;
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
                [_this = SafePointer(this), owner = SafePointer(parent)] {
                    if (!_this || !owner)
                        return;
                    _this->cb(0, "", "");
                    if (owner)
                        owner->closeDialog();
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
                [_this = SafePointer(this), owner = SafePointer(parent)] {
                    if (!_this || !owner)
                        return;
                    _this->cb(1, _this->nameEditor.getText(), _this->baseThemeSelector.getText());
                    if (owner)
                        owner->closeDialog();
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

        setLookAndFeel(nullptr);
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
        comboBox.setColour(ComboBox::textColourId, getThemeColours(*this).panelTextColour);
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
    , public SettingsFileListener
    , public AsyncUpdater {

    class ThemePropertyValue final : public Value::ValueSource {
    public:
        ThemePropertyValue(String theme, String property, bool colour = false)
            : themeName(std::move(theme)), propertyName(std::move(property)), isColour(colour)
        {
            lastNotifiedValue = getValue();
        }

        var getValue() const override
        {
            if (auto theme = SettingsFile::getInstance()->getTheme(themeName)) {
                auto const value = theme->getProperty(propertyName);
                return isColour ? colourToVar(Colour::fromString(value.toString())) : value;
            }
            return {};
        }

        void setValue(var const& value) override
        {
            auto* settings = SettingsFile::getInstance();
            if (auto theme = settings->getTheme(themeName)) {
                // Colour controls use packed integer Values; theme files store ARGB hex strings.
                auto const propertyValue = isColour ? var(std::bit_cast<Colour>(static_cast<int>(value)).toString()) : value;
                if (theme->getProperty(propertyName).equalsWithSameType(propertyValue))
                    return;
                theme->setProperty(propertyName, propertyValue);
                settings->triggerSettingsChange("themes");
                refresh();
            }
        }

        void refresh()
        {
            auto const value = getValue();
            if (lastNotifiedValue.equalsWithSameType(value))
                return;
            lastNotifiedValue = value;
            sendChangeMessage(false);
        }

    private:
        String themeName, propertyName;
        bool isColour;
        var lastNotifiedValue;
    };

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

    StringArray displayedThemes, availableThemes;

public:
    explicit ThemeSettingsPanel(PluginProcessor*)
    {

        addAndMakeVisible(panel);
        fontValue.referTo(SettingsFile::getInstance()->getPropertyAsValue("default_font"));

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
        if (displayedThemes != PlugDataLook::selectedThemes || availableThemes != PlugDataLook::getAllThemes()) {
            updateSwatches();
            return;
        }
        for (auto& [theme, properties] : swatches)
            for (auto& [name, swatch] : properties)
                static_cast<ThemePropertyValue&>(swatch.getValueSource()).refresh();
        panel.repaint();
    }

    void settingsChanged(String const& name, var const&) override
    {
        if (name == "themes" || name == "active_themes")
            triggerAsyncUpdate();
    }

    void settingsFileReloaded() override
    {
        triggerAsyncUpdate();
    }

    void updateSwatches()
    {
        auto scrollPosition = panel.getViewport().getViewPositionY();

        panel.clear();
        allPanels.clear();
        swatches.clear();
        displayedThemes = PlugDataLook::selectedThemes;
        availableThemes = PlugDataLook::getAllThemes();

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
                swatch.referTo(Value(new ThemePropertyValue(themeName, colourId, true)));
            }

            // Add a multi colour component to the properties panel
            panels[colourCategory].add(new PropertiesPanel::MultiPropertyComponent<PropertiesPanel::ColourComponent>(colourName, swatchesToAdd));
        }

        auto* fontPanel = new PropertiesPanel::FontComponent("Default font", fontValue);

        std::function<void(int, String)> onThemeChange = [](int const themeSlot, String const& newThemeName) {
            auto* settings = SettingsFile::getInstance();
            if (newThemeName.isEmpty() || newThemeName == PlugDataLook::selectedThemes[themeSlot]
                || newThemeName == PlugDataLook::selectedThemes[1 - themeSlot] || !settings->getTheme(newThemeName))
                return;

            auto const replacesCurrent = settings->getProperty<String>("theme") == PlugDataLook::selectedThemes[themeSlot];
            settings->getProperty<VarArray>("active_themes").set(themeSlot, newThemeName);
            PlugDataLook::selectedThemes.set(themeSlot, newThemeName);
            if (replacesCurrent)
                settings->setProperty("theme", newThemeName);
            settings->triggerSettingsChange("active_themes");
        };

        auto* resetButton = new PropertiesPanel::ActionComponent([this] {
            Dialogs::showMultiChoiceDialog(&dialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset to default theme settings?",
                [_this = SafePointer(this)](int const result) {
                    if (_this && !result) {
                        _this->resetDefaults();
                    }
                });
        },
            Icons::Reset, "Reset all themes to default", true);

        newButton = new PropertiesPanel::ActionComponent([this] {
            auto callback = [](int const result, String const& name, String const& baseTheme) {
                if (!result)
                    return;

                auto theme = SettingsFile::getInstance()->getTheme(baseTheme);
                if (!theme)
                    return;
                DynamicObject::Ptr newTheme = theme->clone().release();
                newTheme->setProperty("name", name);

                SettingsFile::getInstance()->getProperty<VarArray>("themes").add(var(newTheme.get()));
                SettingsFile::getInstance()->triggerSettingsChange("themes");
            };

            auto* d = new Dialog(&dialog, getParentComponent(), 400, 170, false);
            auto* dialogContent = new NewThemeDialog(d, callback);

            d->setViewedComponent(dialogContent);
            dialog.reset(d);
        },
            Icons::New, "New theme...");

        loadButton = new PropertiesPanel::ActionComponent([this] {
            Dialogs::showOpenDialog([](URL const& url) {
                auto const result = url.getLocalFile();
                if (!result.exists())
                    return;

                auto const themeFile = result.loadFileAsString();
                auto themeJson = JSON::parse(themeFile);

                String themeName;
                if (!themeJson.isObject()) {
                    themeJson = var(SettingsFile::xmlThemeToJson(ValueTree::fromXml(themeFile)).get());
                }
                if (themeJson.isObject()) {
                    auto themeObj = themeJson.getDynamicObject();
                    auto themeName = themeObj->getProperty("name").toString();

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
                    themeObj->setProperty("name", themeName);
                    SettingsFile::getInstance()->getProperty<VarArray>("themes").add(themeJson);
                }
                SettingsFile::getInstance()->triggerSettingsChange("themes");
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

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(saveButton).withParentComponent(this), [_this = SafePointer(this), allThemes](int const result) {
                if (!_this || result < 1)
                    return;

                auto const& themeName = allThemes[result - 1];

                auto const themeTree = SettingsFile::getInstance()->getTheme(themeName);
                if (!themeTree)
                    return;
                auto themeJson = JSON::toString(var(themeTree.get()));

                Dialogs::showSaveDialog([themeJson](URL const& url) {
                    auto result = url.getLocalFile();
                    if (result.getParentDirectory().exists()) {
                        result = result.withFileExtension(".plugdatatheme");
                        result.replaceWithText(themeJson);
                    }
                },
                    "*.plugdatatheme", "ThemeLocation", _this->getTopLevelComponent());
            });
        },
            Icons::Save, "Export theme...");

        deleteButton = new PropertiesPanel::ActionComponent([this] {
            auto allThemes = PlugDataLook::getAllThemes();

            PopupMenu menu;

            for (int i = 0; i < allThemes.size(); i++) {
                menu.addItem(i + 1, allThemes[i], allThemes[i] != "light" && allThemes[i] != "dark");
            }

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(deleteButton).withParentComponent(this), [allThemes](int const result) {
                if (result < 1)
                    return;

                auto& selectedThemes = SettingsFile::getInstance()->getProperty<VarArray>("active_themes");
                auto const& themeName = allThemes[result - 1];
                auto currentTheme = SettingsFile::getInstance()->getProperty<String>("theme");

                auto const currentIndex = PlugDataLook::getAllThemes().indexOf(themeName);
                if (currentIndex < 0)
                    return;
                SettingsFile::getInstance()->getProperty<VarArray>("themes").remove(currentIndex);
                if (selectedThemes[0].toString() == themeName) {
                    selectedThemes.set(0, "light");
                    PlugDataLook::selectedThemes.set(0, "light");
                    if (themeName == currentTheme)
                        SettingsFile::getInstance()->setProperty("theme", "light");
                }
                if (selectedThemes[1].toString() == themeName) {
                    selectedThemes.set(1, "dark");
                    PlugDataLook::selectedThemes.set(1, "dark");
                    if (themeName == currentTheme)
                        SettingsFile::getInstance()->setProperty("theme", "dark");
                }

                SettingsFile::getInstance()->triggerSettingsChange("active_themes");
                SettingsFile::getInstance()->triggerSettingsChange("themes");
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
            // settings for connections
            swatch["straight_connections"].referTo(Value(new ThemePropertyValue(themeName, "straight_connections")));
            swatch["connection_style"].referTo(Value(new ThemePropertyValue(themeName, "connection_style")));
            swatch["connection_look"].referTo(Value(new ThemePropertyValue(themeName, "connection_look")));

            // settings for object & iolets
            swatch["iolet_spacing_edge"].referTo(Value(new ThemePropertyValue(themeName, "iolet_spacing_edge")));
            swatch["square_iolets"].referTo(Value(new ThemePropertyValue(themeName, "square_iolets")));
            swatch["square_object_corners"].referTo(Value(new ThemePropertyValue(themeName, "square_object_corners")));
            swatch["object_flag_outlined"].referTo(Value(new ThemePropertyValue(themeName, "object_flag_outlined")));
            swatch["highlight_syntax"].referTo(Value(new ThemePropertyValue(themeName, "highlight_syntax")));

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

        updateThemeNames(primaryThemeSelector->getText(), secondaryThemeSelector->getText());

        panel.repaint();
        panel.getViewport().setViewPosition(0, scrollPosition);
    }

    void resized() override
    {
        auto const bounds = getLocalBounds();
        panel.setBounds(bounds);
    }

    void resetDefaults()
    {
        auto* settings = SettingsFile::getInstance();
        settings->setProperty("themes", JSON::fromString(PlugDataLook::defaultThemesJSON));
        settings->setProperty("active_themes", Array<var> { "light", "dark" });
        settings->setProperty("theme", "light");
        settings->setProperty("default_font", "Inter");
        settings->initialiseThemesTree();
        triggerAsyncUpdate();
    }
};
