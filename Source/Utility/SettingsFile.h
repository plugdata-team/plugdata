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

template<typename T>
struct PropertyReturnType { using Type = T; };
template<> struct PropertyReturnType<Array<var>>   { using Type = Array<var>&; };
template<> struct PropertyReturnType<DynamicObject> { using Type = DynamicObject::Ptr; };

// Class that manages the settings file
class SettingsFile final
    : public FileSystemWatcher::Listener
    , public Timer
    , public Value::Listener
    , public DeletedAtShutdown {
public:
    ~SettingsFile() override;

    SettingsFile* initialise();

    void startChangeListener();

    void setKeyMap(String const& keymap);
    String getKeyMap() const;

    DynamicObject::Ptr getTheme(String const& name) const;
    DynamicObject::Ptr getCurrentTheme() const;

    bool isUsingTouchMode() const;

    void setLastBrowserPathForId(String const& identifier, File const& path);
    File getLastBrowserPathForId(String const& identifier) const;

    void addToRecentlyOpened(URL const& path);

    void initialisePathsTree();
    void initialiseThemesTree();
    void initialiseOverlayTree();

    void reloadSettings();

    void filesystemChanged() override;

    void triggerSettingsChange(String const&);
    void valueChanged(Value& v) override;

    void timerCallback() override;

    void saveSettings();

    void setProperty(String const& name, var const& value);

    template<typename T>
    typename PropertyReturnType<T>::Type getProperty(String const& name) const
    {
        if constexpr (std::is_same_v<T, String>)
            return settings.at(name).toString();
        else if constexpr (std::is_same_v<T, VarArray>)
            return *settings.at(name).getValue().getArray();
        else if constexpr (std::is_same_v<T, DynamicObject>)
            return settings.at(name).getValue().getDynamicObject();
        else
            return static_cast<T>(settings.at(name).getValue());
    }

    bool hasProperty(String const& name) const;

    bool wantsNativeDialog() const;

    Value getPropertyAsValue(String const& name);

    void setGlobalScale(float newScale);

    String getCorruptBackupSettingsLocation();

    enum SettingsState { UserSettings,
        BackupSettings,
        DefaultSettings };
    SettingsState getSettingsState() const;
    void resetSettingsState();

    static DynamicObject::Ptr xmlThemeToJson(ValueTree oldSettings);
    static DynamicObject::Ptr valueTreeToJsonObj (const ValueTree& tree);
    static ValueTree valueTreeFromJsonObj (DynamicObject* src, Identifier ident);

private:

    bool acquireFileLock();
    void releaseFileLock();

    void loadThemeFromDiff(Array<var>& savedThemes);

    void backupCorruptSettings();
    String backupSettingsLocation;

    SettingsState settingsState = UserSettings;

    bool isInitialised = false;

    FileSystemWatcher settingsFileWatcher;

    HeapArray<SettingsFileListener*> listeners;

    File oldSettingsFile = ProjectInfo::appDataDir.getChildFile(".settings");
    File settingsFile = ProjectInfo::appDataDir.getChildFile("settings.json");
    File lockFile = settingsFile.getSiblingFile(settingsFile.getFileNameWithoutExtension() + ".lock");

    bool settingsChangedInternally = false;
    bool settingsChangedExternally = false;
    int64 lastContentHash = 0;
    static constexpr int64 saveTimeoutMs = 100;
    static constexpr int64 lockTimeoutMs = 5000;

    UnorderedMap<String, Value> settings;

    UnorderedMap<String, var> const defaultSettings {
        { "browser_path", var(ProjectInfo::appDataDir.getFullPathName()) },
        { "theme", var("light") },
        { "oversampling", var(0) },
        { "limiter_threshold", var(1) },
        { "protected", var(true) },
        { "debug_connections", var(true) },
        { "internal_synth", var(0) },
        { "grid_enabled", var(true) },
        { "grid_type", var(6) },
        { "grid_size", var(25) },
        { "default_font", var("Inter") },
        { "native_window", var(false) },
        { "native_file_dialog", var(true) },
        { "autoconnect", var(true) },
        { "global_scale", var(1.0f) },
        { "default_zoom", var(100.0f) },
        { "show_palettes", var(true) },
        { "cpu_meter_mapping_mode", var(0) },
        { "centre_resized_canvas", var(true) },
        { "centre_sidepanel_buttons", var(true) },
        { "show_all_audio_device_rates", var(false) },
        { "add_object_menu_pinned", var(false) },
        { "autosave_interval", var(5) },
        { "autosave_enabled", var(true) },
        { "patch_downwards_only", var(false) },
        { "search_order", var(true) },
        { "search_xy_show", var(true) },
        { "search_index_show", var(false) },
        { "open_patches_in_window", var(false) },
        { "cmd_click_switches_mode", var(true) },
        { "show_minimap", var(2) },
        { "hvcc_mode", var(false) },
        { "heavy_state", var(0) },
        { "touch_mode", var(false) },
        { "keymap", var("") },
        { "last_welcome_panel", var(0) },
        { "active_themes", var(Array<var> { "light", "dark" }) },
        { "libraries", var(Array<var> { }) },
        { "recently_opened", var(Array<var> { }) },
        { "enabled_midi_outputs", var(Array<var> { }) },
        { "enabled_midi_inputs", var(Array<var> { }) },
        { "last_file_browser_paths", var(new DynamicObject()) },
        { "paths", var(Array<var> { }) },
        { "overlays", var(new DynamicObject()) },
        { "themes", var(Array<var> {}) },
        { "palettes", var(Array<var> {}) },
        { "palettes_version", var(1) },
        { "audio_setup", var(new DynamicObject()) },
        { "window_size", var(Array<var> { 1000, 660 }) },
        { "version", var("")},
    };

public:
    JUCE_DECLARE_SINGLETON(SettingsFile, false)

    friend class SettingsFileListener;
};
