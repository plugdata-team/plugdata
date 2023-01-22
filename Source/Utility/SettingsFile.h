/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "LookAndFeel.h"
#include "Pd/PdLibrary.h"

struct SettingsFileListener
{
    SettingsFileListener();
    
    ~SettingsFileListener();
    
    virtual void propertyChanged(String name, var value) = 0;
};


// Graph bounds component
class SettingsFile : public ValueTree::Listener, public Timer
{
    std::vector<std::pair<String, var>> defaultSettings {
        { "Theme", var("light") },
        { "GridEnabled", var(1) }
    };
    
public:
    
    SettingsFile() {
    
    }
    
    ~SettingsFile() {
        
        // Save current settings before quitting
        saveSettings();
        
    }
    
    SettingsFile* initialise() {
        // Check if settings file exists, if not, create the default
        if (!settingsFile.existsAsFile()) {
            settingsFile.create();

            // Add default settings
            settingsTree.setProperty("BrowserPath", homeDir.getChildFile("Library").getFullPathName(), nullptr);
            settingsTree.setProperty("Theme", "light", nullptr);
            settingsTree.setProperty("GridEnabled", 1, nullptr);
            settingsTree.setProperty("Zoom", 1.0f, nullptr);

            auto pathTree = ValueTree("Paths");
            auto library = homeDir.getChildFile("Library");

            for (auto path : pd::Library::defaultPaths) {
                auto pathSubTree = ValueTree("Path");
                pathSubTree.setProperty("Path", path.getFullPathName(), nullptr);
                pathTree.appendChild(pathSubTree, nullptr);
            }

            settingsTree.appendChild(pathTree, nullptr);

            settingsTree.appendChild(ValueTree("Keymap"), nullptr);

            auto selectedThemes = ValueTree("SelectedThemes");
            selectedThemes.setProperty("first", "light", nullptr);
            selectedThemes.setProperty("second", "dark", nullptr);
            settingsTree.appendChild(selectedThemes, nullptr);

            settingsTree.setProperty("DefaultFont", "Inter", nullptr);
            auto colourThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);
            settingsTree.appendChild(colourThemesTree, nullptr);
            saveSettings();
        } else {
            // Or load the settings when they exist already
            settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());

            // Make sure all the default paths are in place
            StringArray currentPaths;

            auto pathTree = settingsTree.getChildWithName("Paths");
            for (auto child : pathTree) {
                currentPaths.add(child.getProperty("Path").toString());
            }

            for (auto path : pd::Library::defaultPaths) {
                if (!currentPaths.contains(path.getFullPathName())) {
                    auto pathSubTree = ValueTree("Path");
                    pathSubTree.setProperty("Path", path.getFullPathName(), nullptr);
                    pathTree.appendChild(pathSubTree, nullptr);
                }
            }


            if (!settingsTree.getChildWithName("SelectedThemes").isValid()) {
                auto selectedThemes = ValueTree("SelectedThemes");
                selectedThemes.setProperty("first", "light", nullptr);
                selectedThemes.setProperty("second", "dark", nullptr);
                settingsTree.appendChild(selectedThemes, nullptr);
            } else {
                auto selectedThemes = settingsTree.getChildWithName("SelectedThemes");
                PlugDataLook::selectedThemes.set(0, selectedThemes.getProperty("first").toString());
                PlugDataLook::selectedThemes.set(1, selectedThemes.getProperty("second").toString());
            }

            if (!settingsTree.getChildWithName("ColourThemes").isValid()) {
                auto colourThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);
                settingsTree.appendChild(colourThemesTree, nullptr);
                saveSettings();
            } else {
                bool wasMissingColours = false;
                auto colourThemesTree = settingsTree.getChildWithName("ColourThemes");
                auto defaultThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);

                for (auto themeTree : colourThemesTree) {
                    auto themeName = themeTree.getProperty("theme");

                    if (!themeTree.hasProperty("DashedSignalConnection")) {
                        themeTree.setProperty("DashedSignalConnection", true, nullptr);
                    }
                    if (!themeTree.hasProperty("StraightConnections")) {
                        themeTree.setProperty("StraightConnections", false, nullptr);
                    }
                    if (!themeTree.hasProperty("ThinConnections")) {
                        themeTree.setProperty("ThinConnections", false, nullptr);
                    }
                    if (!themeTree.hasProperty("SquareIolets")) {
                        themeTree.setProperty("SquareIolets", false, nullptr);
                    }
                    if (!themeTree.hasProperty("SquareObjectCorners")) {
                        themeTree.setProperty("SquareObjectCorners", false, nullptr);
                    }

                    if (!defaultThemesTree.getChildWithProperty("theme", themeName).isValid()) {
                        continue;
                    }

                    for (auto const& [colourId, colourInfo] : PlugDataColourNames) {
                        auto& [cId, colourName, colourCategory] = colourInfo;

                        auto defaultTree = defaultThemesTree.getChildWithProperty("theme", themeName);

                        // For when we add new colours in the future
                        if (!themeTree.hasProperty(colourName)) {
                            themeTree.setProperty(colourName, defaultTree.getProperty(colourName).toString(), nullptr);
                            wasMissingColours = true;
                        }
                    }
                }

                if (wasMissingColours)
                    saveSettings();
            }
            
            return this;
        }

        if (!settingsTree.hasProperty("NativeWindow")) {
            settingsTree.setProperty("NativeWindow", false, nullptr);
        }

        if (!settingsTree.hasProperty("ReloadLastState")) {
            settingsTree.setProperty("ReloadLastState", false, nullptr);
        }
        if (!settingsTree.hasProperty("Zoom")) {
            settingsTree.setProperty("Zoom", 1.0f, nullptr);
        }

        if (!settingsTree.getChildWithName("Libraries").isValid()) {
            settingsTree.appendChild(ValueTree("Libraries"), nullptr);
        }

        if (!settingsTree.hasProperty("AutoConnect")) {
            settingsTree.setProperty("AutoConnect", true, nullptr);
        }
        
        settingsTree.addListener(this);
    }
    
    void reloadSettings() {

        auto newTree = ValueTree::fromXml(settingsFile.loadFileAsString());

        // Prevents causing an update loop
        settingsTree.removeListener(this);

        settingsTree.getChildWithName("Paths").copyPropertiesAndChildrenFrom(newTree.getChildWithName("Paths"), nullptr);

        // Direct children shouldn't be overwritten as that would break some valueTree links, for example in SettingsDialog
        for (auto child : settingsTree) {
            child.copyPropertiesAndChildrenFrom(newTree.getChildWithName(child.getType()), nullptr);
        }
        settingsTree.copyPropertiesFrom(newTree, nullptr);

        settingsTree.addListener(this);
    }
    
    ValueTree getChildTree(String name)
    {
        return settingsTree.getChildWithName(name);
    }

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
    {
        for(auto* listener : listeners) {
            listener->propertyChanged(property.toString(), treeWhosePropertyHasChanged.getProperty(property));
        }
        
        settingsChangedInternally = true;
        startTimer(300);
    }
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override
    {
        settingsChangedInternally = true;
        startTimer(300);
    }
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        settingsChangedInternally = true;
        startTimer(300);
    }

    void timerCallback() override
    {
        // Save settings to file whenever valuetree state changes
        // Use timer to group changes together
        saveSettings();
        stopTimer();
    }
    
    void saveSettings()
    {
        // Save settings to file
        auto xml = settingsTree.toXmlString();
        settingsFile.replaceWithText(xml);
    }
    
    void setProperty(String name, var value) {
        return settingsTree.setProperty(name, value, nullptr);
    }
    
    var getProperty(String name) {
        return settingsTree.getProperty(name);
    }
    
    bool hasProperty(String name) {
        return settingsTree.hasProperty(name);
    }
    
    Value getPropertyAsValue(String name) {
        return settingsTree.getPropertyAsValue(name, nullptr);
    }
    
    ValueTree getValueTree() {
        return settingsTree;
    }
    
private:
    
    Array<SettingsFileListener*> listeners;
    
    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    File settingsFile = homeDir.getChildFile("Settings.xml");
    ValueTree settingsTree;
    bool settingsChangedInternally = false;
    
public:
    
    JUCE_DECLARE_SINGLETON(SettingsFile, false)
    
    friend class SettingsFileListener;
};
