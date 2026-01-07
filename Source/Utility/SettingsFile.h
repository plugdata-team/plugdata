/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Pd/Library.h"

class SettingsFileListener {
public:
    SettingsFileListener();

    virtual ~SettingsFileListener();

    virtual void settingsChanged(String const& name, var const& value) { }

    virtual void settingsFileReloaded() { }
};

// Class that manages the settings file
class SettingsFile final : public ValueTree::Listener
    , public FileSystemWatcher::Listener
    , public Timer
    , public DeletedAtShutdown {
public:
    ~SettingsFile() override;

    SettingsFile* initialise();

    void startChangeListener();

    ValueTree getKeyMapTree() const;
    ValueTree getColourThemesTree() const;
    ValueTree getPathsTree() const;
    ValueTree getSelectedThemesTree() const;
    ValueTree getLibrariesTree() const;

    ValueTree getTheme(String const& name) const;
    ValueTree getCurrentTheme() const;

    void setLastBrowserPathForId(String const& identifier, File const& path);
    File getLastBrowserPathForId(String const& identifier) const;

    void addToRecentlyOpened(File const& path);

    void saveCommandHistory();
    void initialiseCommandHistory();

    void initialisePathsTree();
    void initialiseThemesTree();
    void initialiseOverlayTree();

    void reloadSettings();

    void fileChanged(File file, FileSystemWatcher::FileSystemEvent fileEvent) override;

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

        if constexpr (std::is_same_v<T, String>) {
            return settingsTree.getProperty(name).toString();
        } else {
            return static_cast<T>(settingsTree.getProperty(name));
        }
    }

    bool hasProperty(String const& name) const;

    bool wantsNativeDialog() const;

    Value getPropertyAsValue(String const& name);

    ValueTree& getValueTree();

    void setGlobalScale(float newScale);

    String getCorruptBackupSettingsLocation();

    enum SettingsState { UserSettings,
        BackupSettings,
        DefaultSettings };
    SettingsState getSettingsState() const;
    void resetSettingsState();

private:
        
    bool acquireFileLock();
    void releaseFileLock();
        
    static bool verify(XmlElement const* settings);

    void backupCorruptSettings();
    String backupSettingsLocation;

    SettingsState settingsState = UserSettings;

    bool isInitialised = false;

    FileSystemWatcher settingsFileWatcher;

    HeapArray<SettingsFileListener*> listeners;

    File settingsFile = ProjectInfo::appDataDir.getChildFile(".settings");
    File lockFile = settingsFile.getSiblingFile(settingsFile.getFileNameWithoutExtension() + ".lock");
                
    ValueTree settingsTree = ValueTree("SettingsTree");
    bool settingsChangedInternally = false;
    bool settingsChangedExternally = false;
    int64 lastContentHash = 0;
    static constexpr int64 saveTimeoutMs = 100;
    static constexpr int64 lockTimeoutMs = 5000;

    HeapArray<std::pair<String, var>> defaultSettings {
        { "browser_path", var(ProjectInfo::appDataDir.getFullPathName()) },
        { "theme", var("light") },
        { "oversampling", var(0) },
        { "limiter_threshold", var(1) },
        { "protected", var(1) },
        { "debug_connections", var(1) },
        { "internal_synth", var(0) },
        { "grid_enabled", var(1) },
        { "grid_type", var(6) },
        { "grid_size", var(25) },
        { "default_font", var("Inter") },
        { "native_window", var(false) },
        { "autoconnect", var(true) },
        { "origin", var(0) },
        { "border", var(0) },
        { "index", var(0) },
        { "coordinate", var(0) },
        { "activation_state", var(0) },
        { "order", var(0) },
        { "direction", var(0) },
        { "global_scale", var(1.0f) },
        { "default_zoom", var(100.0f) },
        { "show_palettes", var(true) },
        { "cpu_meter_mapping_mode", var(0) },
        { "centre_resized_canvas", var(true) },
        { "centre_sidepanel_buttons", var(true) },
        { "show_all_audio_device_rates", var(false) },
        { "add_object_menu_pinned", var(false) },
        { "autosave_interval", var(5) },
        { "autosave_enabled", var(1) },
        { "patch_downwards_only", var(false) }, // Option to replicate PD-Vanilla patching downwards only
        // DEFAULT SETTINGS FOR TOGGLES
        { "search_order", var(true) },
        { "search_xy_show", var(true) },
        { "search_index_show", var(false) },
        { "open_patches_in_window", var(false) },
        { "cmd_click_switches_mode", var(true) },
        { "show_minimap", var(2) },
    };

    StringArray childTrees {
        "Paths",
        "KeyMap",
        "ColourThemes",
        "SelectedThemes",
        "RecentlyOpened",
        "Libraries",
        "EnabledMidiOutputPorts",
        "EnabledMidiInputPorts",
        "LastBrowserPaths"
    };

public:
    JUCE_DECLARE_SINGLETON(SettingsFile, false)

    friend class SettingsFileListener;
};
