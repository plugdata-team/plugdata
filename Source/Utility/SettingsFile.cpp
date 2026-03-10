/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/OSUtils.h"

#include "SettingsFile.h"
#include "LookAndFeel.h"

SettingsFileListener::SettingsFileListener()
{
    SettingsFile::getInstance()->listeners.add(this);
}

SettingsFileListener::~SettingsFileListener()
{
    SettingsFile::getInstance()->listeners.remove_one(this);
}

JUCE_IMPLEMENT_SINGLETON(SettingsFile)

SettingsFile::~SettingsFile()
{
    // Save current settings before quitting
    saveSettings();

    clearSingletonInstance();
}

void SettingsFile::backupCorruptSettings()
{
    auto const corruptSettings = getInstance()->settingsFile;
    auto backupLocation = corruptSettings.getParentDirectory().getChildFile(".settings_damaged").getFullPathName();
    backupSettingsLocation = backupLocation;
    OSUtils::moveFileTo(corruptSettings, backupLocation);
}

String SettingsFile::getCorruptBackupSettingsLocation()
{
    return backupSettingsLocation;
}

// Temporary function for backward compatibility with the old xml theme format
// TODO: remove before v1.0
DynamicObject::Ptr SettingsFile::xmlThemeToJson(ValueTree tree)
{
    DynamicObject::Ptr result { new DynamicObject };
    result->setProperty("name", tree.getProperty("theme"));
    for (int i = 0; i < tree.getNumProperties(); ++i) {
        auto const name = tree.getPropertyName(i);
        result->setProperty(name, tree.getProperty(name));
    }
    result->removeProperty("theme");
    return result;
}

// Temporary function to convert to the new settings format
// Remove before v1.0!
static var convertFromLegacyFormat(ValueTree s)
{
    auto* root = new DynamicObject();
    var result(root);

    auto copyInt = [&](char const* xmlName, char const* jsonName) {
        if (s.hasProperty(xmlName))
            root->setProperty(jsonName, static_cast<int>(s[xmlName]));
    };
    auto copyBool = [&](char const* xmlName, char const* jsonName) {
        if (s.hasProperty(xmlName))
            root->setProperty(jsonName, static_cast<bool>(static_cast<int>(s[xmlName])));
    };
    auto copyFloat = [&](char const* xmlName, char const* jsonName) {
        if (s.hasProperty(xmlName))
            root->setProperty(jsonName, static_cast<float>(s[xmlName]));
    };
    auto copyString = [&](char const* xmlName, char const* jsonName) {
        if (s.hasProperty(xmlName))
            root->setProperty(jsonName, s[xmlName].toString());
    };

    copyString("browser_path",              "browser_path");
    copyString("theme",                     "theme");
    copyString("default_font",              "default_font");

    copyInt("oversampling",                 "oversampling");
    copyInt("limiter_threshold",            "limiter_threshold");
    copyInt("internal_synth",               "internal_synth");
    copyInt("grid_type",                    "grid_type");
    copyInt("grid_size",                    "grid_size");
    copyInt("cpu_meter_mapping_mode",       "cpu_meter_mapping_mode");
    copyInt("autosave_interval",            "autosave_interval");
    copyInt("show_minimap",                 "show_minimap");
    copyInt("hvcc_mode",                    "hvcc_mode");
    copyInt("last_welcome_panel",           "last_welcome_panel");

    copyFloat("global_scale",              "global_scale");
    copyFloat("default_zoom",              "default_zoom");

    copyBool("protected",                  "protected");
    copyBool("debug_connections",          "debug_connections");
    copyBool("grid_enabled",               "grid_enabled");
    copyBool("native_window",              "native_window");
    copyBool("native_file_dialog",         "native_file_dialog");
    copyBool("autoconnect",                "autoconnect");
    copyBool("show_palettes",              "show_palettes");
    copyBool("centre_resized_canvas",      "centre_resized_canvas");
    copyBool("centre_sidepanel_buttons",   "centre_sidepanel_buttons");
    copyBool("show_all_audio_device_rates","show_all_audio_device_rates");
    copyBool("add_object_menu_pinned",     "add_object_menu_pinned");
    copyBool("autosave_enabled",           "autosave_enabled");
    copyBool("patch_downwards_only",       "patch_downwards_only");
    copyBool("search_order",               "search_order");
    copyBool("search_xy_show",             "search_xy_show");
    copyBool("search_index_show",          "search_index_show");
    copyBool("open_patches_in_window",     "open_patches_in_window");
    copyBool("cmd_click_switches_mode",    "cmd_click_switches_mode");
    copyBool("touch_mode",                 "touch_mode");

    // Paths
    Array<var> paths;
    if (auto pathsTree = s.getChildWithName("Paths"); pathsTree.isValid())
        for (auto path : pathsTree)
            paths.add(path["Path"].toString());
    root->setProperty("paths", paths);

    // Keymap
    if (auto keymapTree = s.getChildWithName("KeyMap"); keymapTree.isValid()) {
        auto keyxml = keymapTree["keyxml"].toString();
        auto encoded = Base64::toBase64(keyxml.toRawUTF8(), keyxml.getNumBytesAsUTF8());
        root->setProperty("keymap", encoded);
    }

    // Themes
    Array<var> themes;
    if (auto colourThemes = s.getChildWithName("ColourThemes"); colourThemes.isValid())
        for (auto themeTree : colourThemes)
            themes.add(var(SettingsFile::xmlThemeToJson(themeTree).get()));
    root->setProperty("themes", themes);

    // Active themes
    if (auto sel = s.getChildWithName("SelectedThemes"); sel.isValid()) {
        Array<var> active;
        active.add(sel["first"].toString());
        active.add(sel["second"].toString());
        root->setProperty("active_themes", active);
    }

    // Recently opened
    Array<var> recent;
    if (auto recentTree = s.getChildWithName("RecentlyOpened"); recentTree.isValid()) {
        for (auto item : recentTree) {
            auto* entry = new DynamicObject();
            entry->setProperty("path", item["Path"].toString());
            entry->setProperty("time", static_cast<int64>(item["Time"]));
            recent.add(var(entry));
        }
    }
    root->setProperty("recently_opened", recent);

    // Libraries
    Array<var> libraries;
    if (auto libTree = s.getChildWithName("Libraries"); libTree.isValid())
        for (auto lib : libTree)
            libraries.add(lib["Path"].toString());
    root->setProperty("libraries", libraries);

    // Overlays
    if (auto overlaysTree = s.getChildWithName("Overlays"); overlaysTree.isValid()) {
        auto* overlays = new DynamicObject();
        overlays->setProperty("edit", static_cast<int>(overlaysTree["edit"]));
        overlays->setProperty("lock", static_cast<int>(overlaysTree["lock"]));
        overlays->setProperty("run",  static_cast<int>(overlaysTree["run"]));
        overlays->setProperty("alt",  static_cast<int>(overlaysTree["alt"]));
        root->setProperty("overlays", var(overlays));
    }

    return result;
}

SettingsFile* SettingsFile::initialise()
{
    if (isInitialised)
        return getInstance();

    isInitialised = true;

    FileSystemWatcher::addGlobalIgnorePath(lockFile);

    // Check if settings file exists, if not, create the default
    // This is expected behaviour for first run / deleting plugdata folder
    // No need to alert the user to this
    if (!settingsFile.existsAsFile()) {
        for (auto& [name, var] : defaultSettings) {
            settings[name] = var;
        }
        settings["themes"] = JSON::parse(PlugDataLook::defaultThemesJSON);
        saveSettings();
    } else {
        // Start out with default settings, when overwrite with whatever settings we find in the settings file
        for (auto& [name, var] : defaultSettings) {
            if(name == "themes")
            {
                settings["themes"] = JSON::fromString(PlugDataLook::defaultThemesJSON);
                loadThemeFromDiff(*var.getArray());
                continue;
            }
            settings[name] = var;
        }
        for (auto& [name, value] : settings) {
            value.addListener(this);
        }

        auto settingsFileToUse = settingsFile;
        bool xmlSettings = false;
        if (oldSettingsFile.existsAsFile() && !settingsFile.existsAsFile()) {
            settingsFileToUse = oldSettingsFile;
            xmlSettings = true;
        }

        auto const newSettings = settingsFileToUse.loadFileAsString();
        var settingsToLoad;
        if (xmlSettings) {
            settingsToLoad = convertFromLegacyFormat(ValueTree::fromXml(newSettings));
        } else {
            settingsToLoad = JSON::fromString(newSettings);
        }

        bool validSettings = settingsToLoad.isObject();
        if(validSettings) {
            for(auto& property : settingsToLoad.getDynamicObject()->getProperties())
            {
                auto name = property.name.toString();
                if(settings.contains(name) && !property.value.hasSameTypeAs(settings.at(name)))
                {
                    validSettings = false;
                    break;
                }
            }
        }
        if (!validSettings) {
            backupCorruptSettings();
            auto backupFile = settingsFile.getSiblingFile(".settings_bak");
            settingsToLoad = JSON::fromString(backupFile.loadFileAsString());
            jassertfalse;
        }

        auto* jsonObject = settingsToLoad.getDynamicObject();
        if (jsonObject) {
            for (auto& var : jsonObject->getProperties()) {
                if(var.name == Identifier("themes"))
                {
                    loadThemeFromDiff(*var.value.getArray());
                    continue;
                }
                settings[var.name.toString()] = var.value;
            }
        }

        if (oldSettingsFile.existsAsFile()) {
            saveSettings();
            oldSettingsFile.deleteFile();
        }
    }

    initialisePathsTree();
    initialiseThemesTree();
    initialiseOverlayTree();

    setProperty("version", PLUGDATA_VERSION);

#if JUCE_IOS
    if (!ProjectInfo::isStandalone) {
        Desktop::getInstance().setGlobalScaleFactor(1.0f); // scaling inside AUv3 is a bad idea
    } else if (OSUtils::isIPad()) {
        Desktop::getInstance().setGlobalScaleFactor(1.125f);
    } else {
        Desktop::getInstance().setGlobalScaleFactor(0.825f);
    }

#else
    Desktop::getInstance().setGlobalScaleFactor(getProperty<float>("global_scale"));
#endif
    saveSettings();

    return this;
}

SettingsFile::SettingsState SettingsFile::getSettingsState() const
{
    return settingsState;
}

void SettingsFile::resetSettingsState()
{
    settingsState = UserSettings;
}

void SettingsFile::startChangeListener()
{
    settingsFileWatcher.addFolder(settingsFile.getParentDirectory());
    settingsFileWatcher.addListener(this);
}

void SettingsFile::setKeyMap(String const& keymap)
{
    settings["keymap"] = Base64::toBase64(keymap);
}

String SettingsFile::getKeyMap() const
{
    MemoryOutputStream memOut;
    Base64::convertFromBase64(memOut, settings.at("keymap").toString());
    return memOut.toString();
}

DynamicObject::Ptr SettingsFile::getTheme(String const& name) const
{
    for (auto& theme : getProperty<VarArray>("themes")) {
        auto themeObject = theme.getDynamicObject();
        if (themeObject->getProperty("name") == name) {
            return themeObject;
        }
    }

    return nullptr;
}

DynamicObject::Ptr SettingsFile::getCurrentTheme() const
{
    return getTheme(settings.at("theme").toString());
}

bool SettingsFile::isUsingTouchMode() const
{
#if JUCE_IOS
    return true;
#else
    return getValue<bool>(settings.at("touch_mode"));
#endif
}

void SettingsFile::setLastBrowserPathForId(String const& identifier, File const& path)
{
    if (identifier.isEmpty() || !path.exists() || path.isRoot())
        return;

    getProperty<DynamicObject>("last_file_browser_paths")->setProperty(identifier, path.getFullPathName());
}

File SettingsFile::getLastBrowserPathForId(String const& identifier) const
{
    if (identifier.isEmpty())
        return { };

    return File(getProperty<DynamicObject>("last_file_browser_paths")->getProperty(identifier).toString());
}

void SettingsFile::initialisePathsTree()
{

    // Make sure all the default paths are in place
    HeapArray<File> currentPaths;
    currentPaths.reserve(10);

    auto& pathTree = getProperty<VarArray>("paths");

    // on iOS, the containerisation of apps leads to problems with custom search paths
    // So we completely reset them every time
#if JUCE_IOS
    pathTree.removeAllChildren(nullptr);
#endif

    for (auto child : pathTree) {
        currentPaths.add(child.toString());
    }

    for (auto const& path : pd::Library::defaultPaths) {
        if (!currentPaths.contains(path)) {
            pathTree.add(path.getFullPathName());
        }
    }
}

void SettingsFile::addToRecentlyOpened(URL const& url)
{
    auto& recentlyOpened = getProperty<VarArray>("recently_opened");
    auto path = url.getLocalFile().getFullPathName();

    for (auto& item : recentlyOpened) {
        auto* obj = item.getDynamicObject();
        auto path = obj->getProperty("path").toString();
        if (File(path) == url.getLocalFile()) {
            obj->setProperty("time", (int64)Time::getMillisecondCounter());
#if JUCE_IOS
            auto bookmarkData = url.getBookmarkData();
            if (bookmarkData.isNotEmpty()) {
                obj->setProperty("bookmark", bookmarkData);
            }
#endif
            recentlyOpened.move(recentlyOpened.indexOf(item), 0);
            return;
        }
    }

    auto* obj = new DynamicObject();
    obj->setProperty("path", url.getLocalFile().getFullPathName());
    obj->setProperty("time", (int64)Time::getMillisecondCounter());

#if JUCE_IOS
    // Store iOS bookmark so that we can recover file permissions later
    auto bookmarkData = url.getBookmarkData();
    if (bookmarkData.isNotEmpty()) {
        obj->setProperty("bookmark", bookmarkData);
    }
#endif
    recentlyOpened.insert(0, var(obj));

    while (recentlyOpened.size() > 15) {
        auto minTime = Time::getCurrentTime().toMilliseconds();
        int minIdx = -1;

        // Find oldest entry
        for (int i = 0; i < recentlyOpened.size(); i++) {
            auto const pinned = recentlyOpened[i].getProperty("pinned", false);
            auto const time = static_cast<int64>(recentlyOpened[i].getProperty("time", 0));
            if (time < minTime && !pinned) {
                minIdx = i;
                minTime = time;
            }
        }
        recentlyOpened.remove(minIdx);
    }

    // If we do this inside a plugin, it will add to the DAW's recently opened list!
    if (ProjectInfo::isStandalone) {
        RecentlyOpenedFilesList::registerRecentFileNatively(path);
    }
    SettingsFile::getInstance()->triggerSettingsChange("recently_opened");
}

bool SettingsFile::wantsNativeDialog() const
{
    if (ProjectInfo::isStandalone) {
        return true;
    }

    return getValue<bool>(settings.at("native_file_dialog"));
}

void SettingsFile::initialiseThemesTree()
{
    // Initialise selected themes tree
    auto selectedThemes = getProperty<VarArray>("active_themes");
    auto currentTheme = getProperty<String>("theme");

    if(!getTheme(currentTheme))
    {
        currentTheme = "light";
        setProperty("theme", selectedThemes[0].toString());
    }
    if(!getTheme(selectedThemes[0]))
    {
        selectedThemes.set(0, "light");
    }
    if(!getTheme(selectedThemes[1]))
    {
        selectedThemes.set(1, "dark");
    }
    if (selectedThemes[0].toString() != currentTheme && selectedThemes[1].toString() != currentTheme) {
        setProperty("theme", selectedThemes[0].toString());
    }

    PlugDataLook::selectedThemes.set(0, selectedThemes[0].toString());
    PlugDataLook::selectedThemes.set(1, selectedThemes[1].toString());
}

void SettingsFile::initialiseOverlayTree()
{
    UnorderedMap<String, int> defaults = {
        { "edit", Origin | ActivationState },
        { "lock", Behind },
        { "run", None },
        { "alt", Origin | Border | ActivationState | Index | Coordinate | Order | Direction }
    };

    auto overlays = getProperty<DynamicObject>("overlays");
    if(!overlays->getProperties().size()) {
        for (auto& [name, settings] : defaults) {
            overlays->setProperty(name, settings);
        }
    }
}

bool SettingsFile::acquireFileLock()
{
    auto const startTime = Time::getCurrentTime().toMilliseconds();

    while (Time::getCurrentTime().toMilliseconds() - startTime < lockTimeoutMs) {
        if (!lockFile.exists()) {
            // Try to create lock file with our process info
            auto processInfo = String(Time::getCurrentTime().toMilliseconds());

            if (lockFile.replaceWithText(processInfo)) {
                // Double-check we successfully created it (atomic operation)
                Thread::sleep(1); // Brief pause to ensure file system consistency
                if (lockFile.exists() && lockFile.loadFileAsString() == processInfo) {
                    return true;
                }
            }
        } else {
            auto lockAge = Time::getCurrentTime().toMilliseconds() - lockFile.loadFileAsString().getLargeIntValue();
            if (lockAge > lockTimeoutMs * 2) {
                lockFile.deleteFile();
                continue; // Try to acquire again
            }
        }

        Thread::sleep(10); // Brief wait before retry
    }

    return false; // Timeout
}

void SettingsFile::releaseFileLock()
{
    lockFile.deleteFile();
}

void SettingsFile::loadThemeFromDiff(Array<var>& savedThemes)
{
   auto& currentThemes = *settings["themes"].getValue().getArray();
   for (auto const& savedTheme : savedThemes)
   {
       auto const* savedThemeObj = savedTheme.getDynamicObject();
       if (!savedThemeObj) continue;

       auto const themeName = savedThemeObj->getProperty("name").toString();

       for (auto& current : currentThemes)
       {
           auto* currentThemeObj = current.getDynamicObject();
           if (!currentThemeObj || currentThemeObj->getProperty("name").toString() != themeName) continue;

           for (auto const& [propName, propValue] : savedThemeObj->getProperties())
               if (propName != Identifier("name"))
                   currentThemeObj->setProperty(propName, propValue);
           break;
       }
   }
}

void SettingsFile::reloadSettings()
{
    jassert(isInitialised);

    if (acquireFileLock()) {
        auto const newSettings = settingsFile.loadFileAsString();
        auto const contentHash = newSettings.hashCode64();

        if (contentHash == lastContentHash) {
            releaseFileLock();
            return;
        }

        var settingsToLoad = JSON::fromString(newSettings);
        if (settingsToLoad.isVoid()) {
            releaseFileLock();
            return;
        }

        for (auto& [name, var] : defaultSettings) {
            if(name == "themes")
            {
                settings["themes"] = JSON::fromString(PlugDataLook::defaultThemesJSON);
                loadThemeFromDiff(*var.getArray());
                continue;
            }
            settings[name] = var;
        }

        auto* jsonObject = settingsToLoad.getDynamicObject();
        for (auto& var : jsonObject->getProperties()) {
            if(var.name == Identifier("themes"))
            {
                loadThemeFromDiff(*var.value.getArray());
                continue;
            }
            settings[var.name.toString()] = var.value;
        }

        for (auto* listener : listeners) {
            listener->settingsFileReloaded();
        }

        lastContentHash = contentHash;
        releaseFileLock();
    }
}

void SettingsFile::filesystemChanged()
{
    reloadSettings();
}

void SettingsFile::triggerSettingsChange(String const& name)
{
    for (auto* listener : listeners) {
        listener->settingsChanged(name, settings[name]);
    }
    startTimer(saveTimeoutMs);
}

void SettingsFile::valueChanged(Value& v)
{
    for (auto& [name, var] : settings) {
        if (var.refersToSameSourceAs(v)) {
            for (auto* listener : listeners) {
                listener->settingsChanged(name, v);
            }
            break;
        }
    }
    startTimer(saveTimeoutMs);
}

void SettingsFile::timerCallback()
{
    jassert(isInitialised);

    // Save settings to file whenever valuetree state changes
    // Use timer to group changes together
    saveSettings();
    stopTimer();
}

void SettingsFile::setGlobalScale(float const newScale)
{
    setProperty("global_scale", newScale);
    Desktop::getInstance().setGlobalScaleFactor(newScale);
}

void SettingsFile::saveSettings()
{
    jassert(isInitialised);

    auto* properties = new DynamicObject();

    for (auto& [name, value] : settings) {
        if (name == "themes")
        {
            auto const defaultThemes = JSON::fromString(PlugDataLook::defaultThemesJSON);
            auto const* currentThemesArray = value.getValue().getArray();
            auto const* defaultThemesArray = defaultThemes.getArray();
            if (!currentThemesArray) continue;

            Array<var> themesToWrite;
            for (auto const& currentTheme : *currentThemesArray)
            {
                auto const* currentObj = currentTheme.getDynamicObject();
                if (!currentObj) continue;

                // Find matching default theme by name
                DynamicObject const* defaultObj = nullptr;
                if (defaultThemesArray)
                {
                    for (auto const& defaultTheme : *defaultThemesArray)
                    {
                        auto const* candidate = defaultTheme.getDynamicObject();
                        if (candidate && candidate->getProperty("name") == currentObj->getProperty("name"))
                        {
                            defaultObj = candidate;
                            break;
                        }
                    }
                }

                // Collect only properties that differ from the default
                auto* diffObj = new DynamicObject();
                diffObj->setProperty("name", currentObj->getProperty("name"));

                bool hasDiff = false;
                for (auto const& [propName, propValue] : currentObj->getProperties())
                {
                    if (propName == Identifier("name")) continue;

                    bool const matchesDefault = defaultObj
                        && defaultObj->hasProperty(propName)
                        && defaultObj->getProperty(propName).equalsWithSameType(propValue);

                    if (!matchesDefault)
                    {
                        diffObj->setProperty(propName, propValue);
                        hasDiff = true;
                    }
                }
                if (hasDiff)
                    themesToWrite.add(var(diffObj));
                else
                    delete diffObj;
            }

            if (!themesToWrite.isEmpty())
                properties->setProperty("themes", themesToWrite);
        }
        else if(defaultSettings[name].isArray())
        {
            properties->setProperty(name, value);
        }
        else if(!defaultSettings[name].equalsWithSameType(value)) {
            properties->setProperty(name, value);
        }
    }
    // Check if content actually changed
    auto const json = JSON::toString(var(properties));
    auto const contentHash = json.hashCode64();

    if (contentHash == lastContentHash) {
        return; // No changes to save
    }

    // Attempt to acquire file lock
    if (acquireFileLock()) {
        if (settingsFile.replaceWithText(json)) {
            lastContentHash = contentHash;
        }
        releaseFileLock();
    }
}

void SettingsFile::setProperty(String const& name, var const& value)
{
    jassert(isInitialised);
    settings[name] = value;
}

bool SettingsFile::hasProperty(String const& name) const
{
    jassert(isInitialised);
    return settings.contains(name);
}

Value SettingsFile::getPropertyAsValue(String const& name)
{
    jassert(isInitialised);
    return settings[name];
}
