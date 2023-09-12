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

    virtual void propertyChanged(String const& name, var const& value) {};

    virtual void settingsFileReloaded() {};
};

// Class that manages the settings file
class SettingsFile : public ValueTree::Listener
    , public Timer
    , public DeletedAtShutdown {
public:
    ~SettingsFile() override;

    SettingsFile* initialise();

    // TODO: instead of exposing these trees, try to encapsulate most of the interaction with those trees in functions
    ValueTree getKeyMapTree();
    ValueTree getColourThemesTree();
    ValueTree getPathsTree();
    ValueTree getSelectedThemesTree();
    ValueTree getLibrariesTree();

    ValueTree getTheme(String const& name);
    ValueTree getCurrentTheme();

    void addToRecentlyOpened(File const& path);

    void initialisePathsTree();
    void initialiseThemesTree();
    void initialiseOverlayTree();

    void reloadSettings();

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override;
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    void timerCallback() override;

    void saveSettings();

    void setProperty(String const& name, var const& value);

    template<typename T>
    T getProperty(String const& name)
    {
        if (!isInitialised) {
            initialise();
        }

        if constexpr (std::is_same<T, String>::value) {
            return settingsTree.getProperty(name).toString();
        } else {
            return static_cast<T>(settingsTree.getProperty(name));
        }
    }

    bool hasProperty(String const& name);

    bool wantsNativeDialog();

    Value getPropertyAsValue(String const& name);

    ValueTree getValueTree();

    void setGlobalScale(float newScale);

private:
    bool isInitialised = false;

    Array<SettingsFileListener*> listeners;

    File settingsFile = ProjectInfo::appDataDir.getChildFile(".settings");
    ValueTree settingsTree = ValueTree("SettingsTree");
    bool settingsChangedInternally = false;
    bool settingsChangedExternally = false;

    std::vector<std::pair<String, var>> defaultSettings {
        { "browser_path", var(ProjectInfo::appDataDir.getFullPathName()) },
        { "theme", var("light") },
        { "oversampling", var(0) },
        { "protected", var(1) },
        { "internal_synth", var(0) },
        { "grid_enabled", var(1) },
        { "grid_type", var(6) },
        { "grid_size", var(20) },
        { "default_font", var("Inter") },
        { "native_window", var(false) },
        { "reload_last_state", var(false) },
        { "autoconnect", var(true) },
        { "origin", var(0) },
        { "border", var(0) },
        { "index", var(0) },
        { "coordinate", var(0) },
        { "activation_state", var(0) },
        { "order", var(0) },
        { "direction", var(0) },
        { "global_scale", var(1.0f) },
        { "show_palettes", var(true) },
        { "infinite_drag", var(false) },
        { "show_all_audio_device_rates", var(false) },
        { "add_object_menu_pinned", var(false) },
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
        "EnabledMidiOutputPorts",
    };

public:
    JUCE_DECLARE_SINGLETON(SettingsFile, false)

    friend class SettingsFileListener;
};
