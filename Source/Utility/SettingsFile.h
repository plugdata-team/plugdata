/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "LookAndFeel.h"
#include "Pd/PdLibrary.h"

struct SettingsFileListener {
    SettingsFileListener();

    ~SettingsFileListener();

    virtual void propertyChanged(String name, var value) = 0;
};

// Class that manages the settings file
class SettingsFile : public ValueTree::Listener
    , public Timer
    , public DeletedAtShutdown {
public:
    virtual ~SettingsFile()
    {
        // Save current settings before quitting
        saveSettings();

        clearSingletonInstance();
    }

    SettingsFile* initialise()
    {

        if (isInitialised)
            return getInstance();

        isInitialised = true;

        // Check if settings file exists, if not, create the default
        if (!settingsFile.existsAsFile()) {
            settingsFile.create();
        } else {
            // Or load the settings when they exist already
            settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());
        }

        // Make sure all the properties exist
        for (auto& [propertyName, propertyValue] : defaultSettings) {
            // If it doesn't exists, set it to the default value
            if (!settingsTree.hasProperty(propertyName)) {
                settingsTree.setProperty(propertyName, propertyValue, nullptr);
            }
        }
        // Make sure all the child trees exist
        for (auto& childName : childTrees) {
            if (!settingsTree.getChildWithName(childName).isValid()) {
                settingsTree.appendChild(ValueTree(childName), nullptr);
            }
        }

        initialisePathsTree();
        initialiseThemesTree();

        saveSettings();

        settingsTree.addListener(this);

        return this;
    }

    // TODO: instead of exposing these trees, try to encapsulate most of the interaction with those trees in functions
    ValueTree getKeyMapTree()
    {
        return settingsTree.getChildWithName("KeyMap");
    }

    ValueTree getColourThemesTree()
    {
        return settingsTree.getChildWithName("ColourThemes");
    }

    ValueTree getPathsTree()
    {
        return settingsTree.getChildWithName("Paths");
    }

    ValueTree getSelectedThemesTree()
    {
        return settingsTree.getChildWithName("SelectedThemes");
    }

    ValueTree getLibrariesTree()
    {
        return settingsTree.getChildWithName("Libraries");
    }

    ValueTree getTheme(String name)
    {
        return getColourThemesTree().getChildWithProperty("theme", name);
    }

    void initialisePathsTree()
    {

        // Make sure all the default paths are in place
        StringArray currentPaths;

        auto pathTree = getPathsTree();

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
    }

    void addToRecentlyOpened(File path)
    {
        auto recentlyOpened = settingsTree.getChildWithName("RecentlyOpened");

        if (!recentlyOpened.isValid()) {
            recentlyOpened = ValueTree("RecentlyOpened");
            SettingsFile::getInstance()->getValueTree().appendChild(recentlyOpened, nullptr);
        }

        if (recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).isValid()) {

            recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);

            int oldIdx = recentlyOpened.indexOf(recentlyOpened.getChildWithProperty("Path", path.getFullPathName()));
            recentlyOpened.moveChild(oldIdx, 0, nullptr);
        } else {
            ValueTree subTree("Path");
            subTree.setProperty("Path", path.getFullPathName(), nullptr);
            subTree.setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);
            recentlyOpened.appendChild(subTree, nullptr);
        }

        while (recentlyOpened.getNumChildren() > 10) {
            auto minTime = Time::getCurrentTime().toMilliseconds();
            int minIdx = -1;

            // Find oldest entry
            for (int i = 0; i < recentlyOpened.getNumChildren(); i++) {
                auto time = static_cast<int>(recentlyOpened.getChild(i).getProperty("Time"));
                if (time < minTime) {
                    minIdx = i;
                    minTime = time;
                }
            }

            recentlyOpened.removeChild(minIdx, nullptr);
        }
    }

    void initialiseThemesTree()
    {

        // Initialise selected themes tree
        auto selectedThemes = getSelectedThemesTree();

        if (!selectedThemes.hasProperty("first"))
            selectedThemes.setProperty("first", "light", nullptr);
        if (!selectedThemes.hasProperty("second"))
            selectedThemes.setProperty("second", "dark", nullptr);
        
        if(selectedThemes.getProperty("first").toString() != getProperty("Theme").toString() && selectedThemes.getProperty("second").toString() != getProperty("Theme").toString()) {
            
            setProperty("Theme", selectedThemes.getProperty("first").toString());
        }
        
        PlugDataLook::selectedThemes.set(0,  selectedThemes.getProperty("first").toString());
        PlugDataLook::selectedThemes.set(1,  selectedThemes.getProperty("second").toString());

        auto defaultColourThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);
        auto colourThemesTree = getColourThemesTree();

        if (colourThemesTree.getNumChildren() == 0) {
            colourThemesTree.copyPropertiesAndChildrenFrom(defaultColourThemesTree, nullptr);
        } else {

            // Check if all the default themes are still there
            // Mostly for updating from v0.6.3 -> v0.7.0
            for (auto themeTree : defaultColourThemesTree) {
                if (!colourThemesTree.getChildWithProperty("theme", themeTree.getProperty("theme")).isValid()) {

                    colourThemesTree.appendChild(themeTree.createCopy(), nullptr);
                }
            }

            // Ensure each theme is valid
            for (auto themeTree : colourThemesTree) {

                auto themeName = themeTree.getProperty("theme");

                if (!defaultColourThemesTree.getChildWithProperty("theme", themeName).isValid()) {
                    continue;
                }

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

                if (!defaultColourThemesTree.getChildWithProperty("theme", themeName).isValid()) {
                    continue;
                }

                for (auto const& [colourId, colourInfo] : PlugDataColourNames) {
                    auto& [cId, colourName, colourCategory] = colourInfo;

                    auto defaultTree = defaultColourThemesTree.getChildWithProperty("theme", themeName);

                    // For when we add new colours in the future
                    if (!themeTree.hasProperty(colourName)) {
                        themeTree.setProperty(colourName, defaultTree.getProperty(colourName).toString(), nullptr);
                    }
                }
            }
        }
    }

    void reloadSettings()
    {

        if(settingsChangedInternally) {
            settingsChangedInternally = false;
            return;
        }
        
        jassert(isInitialised);

        auto newTree = ValueTree::fromXml(settingsFile.loadFileAsString());

        // Prevents causing an update loop
        settingsTree.removeListener(this);

        // Direct children shouldn't be overwritten as that would break some valueTree links, for example in SettingsDialog
        for (auto child : settingsTree) {
            child.copyPropertiesAndChildrenFrom(newTree.getChildWithName(child.getType()), nullptr);
        }
        settingsTree.copyPropertiesFrom(newTree, nullptr);

        settingsTree.addListener(this);
    }

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
    {
        for (auto* listener : listeners) {
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
        jassert(isInitialised);

        // Save settings to file whenever valuetree state changes
        // Use timer to group changes together
        saveSettings();
        stopTimer();
    }

    void saveSettings()
    {
        jassert(isInitialised);
        // Save settings to file
        auto xml = settingsTree.toXmlString();
        settingsFile.replaceWithText(xml);
    }

    void setProperty(String name, var value)
    {
        jassert(isInitialised);
        settingsTree.setProperty(name, value, nullptr);
    }

    var getProperty(String name)
    {
        jassert(isInitialised);
        return settingsTree.getProperty(name);
    }

    bool hasProperty(String name)
    {
        jassert(isInitialised);
        return settingsTree.hasProperty(name);
    }

    Value getPropertyAsValue(String name)
    {
        jassert(isInitialised);
        return settingsTree.getPropertyAsValue(name, nullptr);
    }

    ValueTree getValueTree()
    {
        jassert(isInitialised);
        return settingsTree;
    }

private:
    bool isInitialised = false;

    Array<SettingsFileListener*> listeners;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    File settingsFile = homeDir.getChildFile("Settings.xml");
    ValueTree settingsTree = ValueTree("SettingsTree");
    bool settingsChangedInternally = false;

    std::vector<std::pair<String, var>> defaultSettings {
        { "BrowserPath", var(homeDir.getChildFile("Library").getFullPathName()) },
        { "Theme", var("light") },
        { "GridEnabled", var(1) },
        { "Zoom", var(1.0f) },
        { "DefaultFont", var("Inter") },
        { "NativeWindow", var(false) },
        { "ReloadLastState", var(false) },
        { "AutoConnect", var(true) }
    };

    StringArray childTrees {
        "Paths",
        "KeyMap",
        "ColourThemes",
        "SelectedThemes",
        "RecentlyOpened",
        "Libraries",
    };

public:
    JUCE_DECLARE_SINGLETON(SettingsFile, false)

    friend class SettingsFileListener;
};
