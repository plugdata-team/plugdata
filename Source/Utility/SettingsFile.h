/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Pd/Library.h"

struct SettingsFileListener {
    SettingsFileListener();

    ~SettingsFileListener();

    virtual void propertyChanged(String name, var value) {};

    virtual void settingsFileReloaded() {};
};

// Class that manages the settings file
class SettingsFile : public ValueTree::Listener
    , public Timer
    , public DeletedAtShutdown {
public:
    virtual ~SettingsFile();

    SettingsFile* initialise();

    // TODO: instead of exposing these trees, try to encapsulate most of the interaction with those trees in functions
    ValueTree getKeyMapTree();
    ValueTree getColourThemesTree();
    ValueTree getPathsTree();
    ValueTree getSelectedThemesTree();
    ValueTree getLibrariesTree();

    ValueTree getTheme(String name);
    ValueTree getCurrentTheme();

    void initialisePathsTree();

    void addToRecentlyOpened(File path);

    void initialiseThemesTree();

    void reloadSettings();

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override;
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    void timerCallback() override;

    void saveSettings();

    void setProperty(String name, var value);

    template<typename T>
    T getProperty(String name)
    {
        if(!isInitialised)
        {
            initialise();
        }
        
        if constexpr (std::is_same<T, String>::value) {
            return settingsTree.getProperty(name).toString();
        } else {
            return static_cast<T>(settingsTree.getProperty(name));
        }
    }

    bool hasProperty(String name);

    Value getPropertyAsValue(String name);

    ValueTree getValueTree();

private:
    bool isInitialised = false;

    Array<SettingsFileListener*> listeners;

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    File settingsFile = homeDir.getChildFile("Settings.xml");
    ValueTree settingsTree = ValueTree("SettingsTree");
    bool settingsChangedInternally = false;
    bool settingsChangedExternally = false;

    std::vector<std::pair<String, var>> defaultSettings {
        { "browser_path", var(homeDir.getChildFile("Library").getFullPathName()) },
        { "theme", var("light") },
        { "oversampling", var(0) },
        { "protected", var(1) },
        { "internal_synth", var(0) },
        { "grid_enabled", var(1) },
        { "grid_size", var(20) },
        { "zoom", var(1.0f) },
        { "split_zoom", var(1.0f) },
        { "default_font", var("Inter") },
        { "native_window", var(false) },
        { "reload_last_state", var(false) },
        { "autoconnect", var(true) },
        { "macos_buttons",
#if JUCE_MAC
            var(true)
#else
            var(false)
#endif
        },
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
