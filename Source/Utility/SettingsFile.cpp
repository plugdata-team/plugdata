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

#include "Sidebar/CommandInput.h"

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
    // Backup previous corrupt settings file, so users can fix if they want to
    auto const corruptSettings = getInstance()->settingsFile;

    auto backupLocation = corruptSettings.getParentDirectory().getChildFile(".settings_damaged").getFullPathName();
    int counter = 1;

    // Increment backup settings file name if previous exists
    while (File(backupLocation).existsAsFile()) {
        backupLocation = corruptSettings.getParentDirectory().getChildFile(".settings_damaged_" + String(counter)).getFullPathName();
        counter++;
    }

    backupSettingsLocation = backupLocation;
    corruptSettings.moveFileTo(backupLocation);
}

String SettingsFile::getCorruptBackupSettingsLocation()
{
    return backupSettingsLocation;
}

SettingsFile* SettingsFile::initialise()
{
    if (isInitialised)
        return getInstance();

    isInitialised = true;

// #define DEBUG_CORRUPT_SETTINGS_DIALOG
#ifdef DEBUG_CORRUPT_SETTINGS_DIALOG
    backupSettingsLocation = ProjectInfo::appDataDir.getChildFile(".settings_damaged").getFullPathName();
#endif

    // Check if settings file exists, if not, create the default
    // This is expected behaviour for first run / deleting plugdata folder
    // No need to alert the user to this
    if (!settingsFile.existsAsFile()) {
        settingsFile.create();
    } else {
        std::unique_ptr<XmlElement> const xmlElement(XmlDocument::parse(settingsFile.loadFileAsString()));

        // First check if settings XML is valid
        if (verify(xmlElement.get())) {
            // Use user .settings file
            settingsTree = ValueTree::fromXml(*xmlElement);
            // Overwrite previous settings_bak with current good settings (don't touch it after this)
            settingsFile.copyFileTo(settingsFile.getFullPathName() + "_bak");
        } else {
            // Settings are invalid! Backup old file as .settings_damaged
            backupCorruptSettings();

            // See if there is a .settings_bak backup settings file from last run
            auto const backupSettings = File(settingsFile.getFullPathName() + "_bak");
            if (backupSettings.existsAsFile()) {
                std::unique_ptr<XmlElement> const xmlSettingsBackup(XmlDocument::parse(backupSettings.loadFileAsString()));
                if (verify(xmlSettingsBackup.get())) {
                    // If backup settings are good, use them
                    settingsTree = ValueTree::fromXml(*xmlSettingsBackup);
                    settingsState = BackupSettings;
                } else {
                    // Use default plugdata settings (worst case scenario)
                    settingsFile.create();
                    settingsState = DefaultSettings;
                }
            } else {
                // Use default plugdata settings (worst case scenario)
                settingsFile.create();
                settingsState = DefaultSettings;
            }
        }
    }

    // Make sure all the properties exist
    for (auto& [propertyName, propertyValue] : defaultSettings) {
        // If it doesn't exist, set it to the default value
        if (!settingsTree.hasProperty(propertyName) || settingsTree.getProperty(propertyName).toString() == "") {
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
    initialiseOverlayTree();

    initialiseCommandHistory();

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

    settingsTree.addListener(this);

    return this;
}

bool SettingsFile::verify(XmlElement const* xml)
{
    // Basic settings file verification
    // Verify if the xml is valid, and tags match correct name / order
    // Adjust tags here if future layout changes

    if (xml == nullptr || xml->getTagName() != "SettingsTree")
        return false;

    // These must at least be present in a valid save file!
    StringArray expectedNames = {
        "Paths",
        "KeyMap",
        "ColourThemes",
        "SelectedThemes",
        "RecentlyOpened",
        "Libraries",
        "Overlays"
    };

    StringArray const optionalNames {
        "HeavyState",
        "LastBrowserPaths",
        "CommandHistory",
        "EnabledMidiInputPorts",
        "EnabledMidiOutputPorts"
    };

    // Check if all expected elements are present and in the correct order
    bool unexpectedName = false;
    for (auto const* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement()) {
        auto name = child->getTagName();
        if (expectedNames.contains(name)) {
            expectedNames.removeString(name);
        } else if (!optionalNames.contains(name)) {
            std::cerr << "Unexpected settings file entry: " << name << std::endl;
            unexpectedName = true;
        }
    }

    for (auto const& expectedName : expectedNames) {
        std::cerr << "Expected settings file entry not found: " << expectedName << std::endl;
    }

    // Check if all expected elements were found
    return expectedNames.isEmpty() && !unexpectedName;
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

ValueTree SettingsFile::getKeyMapTree() const
{
    return settingsTree.getChildWithName("KeyMap");
}

ValueTree SettingsFile::getColourThemesTree() const
{
    return settingsTree.getChildWithName("ColourThemes");
}

ValueTree SettingsFile::getPathsTree() const
{
    return settingsTree.getChildWithName("Paths");
}

ValueTree SettingsFile::getSelectedThemesTree() const
{
    return settingsTree.getChildWithName("SelectedThemes");
}

ValueTree SettingsFile::getLibrariesTree() const
{
    return settingsTree.getChildWithName("Libraries");
}

ValueTree SettingsFile::getTheme(String const& name) const
{
    return getColourThemesTree().getChildWithProperty("theme", name);
}

void SettingsFile::setLastBrowserPathForId(String const& identifier, File const& path)
{
    if (identifier.isEmpty() || !path.exists() || path.isRoot())
        return;

    settingsTree.getChildWithName("LastBrowserPaths").setProperty(identifier, path.getFullPathName(), nullptr);
}

File SettingsFile::getLastBrowserPathForId(String const& identifier) const
{
    if (identifier.isEmpty())
        return {};

    return { settingsTree.getChildWithName("LastBrowserPaths").getProperty(identifier).toString() };
}

ValueTree SettingsFile::getCurrentTheme() const
{
    return getColourThemesTree().getChildWithProperty("theme", settingsTree.getProperty("theme"));
}

void SettingsFile::initialisePathsTree()
{

    // Make sure all the default paths are in place
    HeapArray<File> currentPaths;
    currentPaths.reserve(10);

    auto pathTree = getPathsTree();

    // on iOS, the containerisation of apps leads to problems with custom search paths
    // So we completely reset them every time
#if JUCE_IOS
    pathTree.removeAllChildren(nullptr);
#endif

    for (auto child : pathTree) {
        currentPaths.add(File(child.getProperty("Path").toString()));
    }

    for (auto const& path : pd::Library::defaultPaths) {
        if (!currentPaths.contains(path)) {
            auto pathSubTree = ValueTree("Path");
            pathSubTree.setProperty("Path", path.getFullPathName(), nullptr);
            pathTree.appendChild(pathSubTree, nullptr);
        }
    }

    // TODO: remove this later. this is to fix a mistake during v0.8.4 development
    for (auto child : pathTree) {
        if (child.getProperty("Path").toString().contains("Abstractions/Gem")) {
            pathTree.removeChild(child, nullptr);
            break;
        }
    }
}

void SettingsFile::saveCommandHistory()
{
    auto commandHistory = settingsTree.getChildWithName("CommandHistory");

    if (commandHistory.isValid()) {
        commandHistory.removeAllProperties(nullptr);
    } else {
        commandHistory = ValueTree("CommandHistory");
        SettingsFile::getInstance()->getValueTree().appendChild(commandHistory, nullptr);
    }

    int commandIndex = 0;
    for (auto& command : CommandInput::getCommandHistory()) {
        // Don't save more than 50 commands
        if (commandIndex > 50)
            break;
        // Set each command as a unique attribute
        commandHistory.setProperty("Command" + String(commandIndex), command, nullptr);
        commandIndex++;
    }
}

void SettingsFile::initialiseCommandHistory()
{
    auto const commandHistory = settingsTree.getChildWithName("CommandHistory");

    if (!commandHistory.isValid()) {
        return;
    }

    StringArray commands;
    String lastCommand;
    for (int i = 0; i < commandHistory.getNumProperties(); i++) {
        auto command = commandHistory.getProperty("Command" + String(i)).toString();
        // Filter out duplicate commands (we also do this in CommandInput, but not during loading)
        if (lastCommand != command) {
            commands.add(command);
            lastCommand = command;
        }
    }

    CommandInput::setCommandHistory(commands);
}

void SettingsFile::addToRecentlyOpened(File const& path)
{
    auto recentlyOpened = settingsTree.getChildWithName("RecentlyOpened");

    if (!recentlyOpened.isValid()) {
        recentlyOpened = ValueTree("RecentlyOpened");
        SettingsFile::getInstance()->getValueTree().appendChild(recentlyOpened, nullptr);
    }

    if (recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).isValid()) {

        recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);

        int const oldIdx = recentlyOpened.indexOf(recentlyOpened.getChildWithProperty("Path", path.getFullPathName()));
        recentlyOpened.moveChild(oldIdx, 0, nullptr);
    } else {
        ValueTree subTree("Path");
        subTree.setProperty("Path", path.getFullPathName(), nullptr);
        subTree.setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);
#if JUCE_MAC || JUCE_WINDOWS
        if (path.isOnRemovableDrive())
            subTree.setProperty("Removable", var(1), nullptr);
#endif
        recentlyOpened.addChild(subTree, 0, nullptr);
    }

    while (recentlyOpened.getNumChildren() > 15) {
        auto minTime = Time::getCurrentTime().toMilliseconds();
        int minIdx = -1;

        // Find oldest entry
        for (int i = 0; i < recentlyOpened.getNumChildren(); i++) {
            auto child = recentlyOpened.getChild(i);
            auto const pinned = child.hasProperty("Pinned") && static_cast<bool>(child.getProperty("Pinned"));
            auto const time = static_cast<int64>(child.getProperty("Time"));
            if (time < minTime && !pinned) {
                minIdx = i;
                minTime = time;
            }
        }

        recentlyOpened.removeChild(minIdx, nullptr);
    }

    // If we do this inside a plugin, it will add to the DAW's recently opened list!
    if (ProjectInfo::isStandalone) {
        RecentlyOpenedFilesList::registerRecentFileNatively(path);
    }
}

bool SettingsFile::wantsNativeDialog() const
{
    if (ProjectInfo::isStandalone) {
        return true;
    }

    if (!settingsTree.hasProperty("NativeDialog")) {
        return true;
    }

    return settingsTree.getProperty("NativeDialog");
}

void SettingsFile::initialiseThemesTree()
{
    // Initialise selected themes tree
    auto selectedThemes = getSelectedThemesTree();

    if (!selectedThemes.hasProperty("first"))
        selectedThemes.setProperty("first", "light", nullptr);
    if (!selectedThemes.hasProperty("second"))
        selectedThemes.setProperty("second", "dark", nullptr);

    if (selectedThemes.getProperty("first").toString() != getProperty<String>("theme") && selectedThemes.getProperty("second").toString() != getProperty<String>("theme")) {

        setProperty("theme", selectedThemes.getProperty("first").toString());
    }

    PlugDataLook::selectedThemes.set(0, selectedThemes.getProperty("first").toString());
    PlugDataLook::selectedThemes.set(1, selectedThemes.getProperty("second").toString());

    auto const defaultColourThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);
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
            // If it's one of the default themes, validate it against the default theme tree
            if (defaultColourThemesTree.getChildWithProperty("theme", themeName).isValid()) {
                auto defaultTree = defaultColourThemesTree.getChildWithProperty("theme", themeName);

                for (auto const& [colourId, colourInfo] : PlugDataColourNames) {
                    auto& [cId, colourName, colourCategory] = colourInfo;
                    // For when we add new colours in the future
                    if (!themeTree.hasProperty(colourName) || themeTree.getProperty(colourName).toString().isEmpty() || themeTree.getProperty(colourName).toString() == "00000000") {
                        themeTree.setProperty(colourName, defaultTree.getProperty(colourName).toString(), nullptr);
                    }
                }

                if (!themeTree.hasProperty("straight_connections")) {
                    themeTree.setProperty("straight_connections", defaultTree.getProperty("straight_connections"), nullptr);
                }
                if (!themeTree.hasProperty("connection_style")) {
                    themeTree.setProperty("connection_style", defaultTree.getProperty("connection_style"), nullptr);
                }
                if (!themeTree.hasProperty("square_iolets")) {
                    themeTree.setProperty("square_iolets", defaultTree.getProperty("square_iolets"), nullptr);
                }
                if (!themeTree.hasProperty("square_object_corners")) {
                    themeTree.setProperty("square_object_corners", defaultTree.getProperty("square_object_corners"), nullptr);
                }
                if (!themeTree.hasProperty("object_flag_outlined")) {
                    themeTree.setProperty("object_flag_outlined", defaultTree.getProperty("object_flag_outlined"), nullptr);
                }
                if (!themeTree.hasProperty("iolet_spacing_edge")) {
                    themeTree.setProperty("iolet_spacing_edge", defaultTree.getProperty("iolet_spacing_edge"), nullptr);
                }
                if (!themeTree.hasProperty("highlight_syntax")) {
                    themeTree.setProperty("highlight_syntax", defaultTree.getProperty("highlight_syntax"), nullptr);
                }
            }
            // Otherwise, just ensure these properties exist
            else {
                if (!themeTree.hasProperty("straight_connections")) {
                    themeTree.setProperty("straight_connections", false, nullptr);
                }
                if (!themeTree.hasProperty("connection_style")) {
                    themeTree.setProperty("connection_style", String(1), nullptr);
                }
                if (!themeTree.hasProperty("square_iolets")) {
                    themeTree.setProperty("square_iolets", false, nullptr);
                }
                if (!themeTree.hasProperty("square_object_corners")) {
                    themeTree.setProperty("square_object_corners", false, nullptr);
                }
                if (!themeTree.hasProperty("object_flag_outlined")) {
                    themeTree.setProperty("object_flag_outlined", false, nullptr);
                }
                if (!themeTree.hasProperty("iolet_spacing_edge")) {
                    themeTree.setProperty("iolet_spacing_edge", false, nullptr);
                }
                if (!themeTree.hasProperty("highlight_syntax")) {
                    themeTree.setProperty("highlight_syntax", true, nullptr);
                }
            }
        }
    }
}

void SettingsFile::initialiseOverlayTree()
{
    UnorderedMap<String, int> defaults = {
        { "edit", Origin | ActivationState },
        { "lock", Behind },
        { "run", None },
        { "alt", Origin | Border | ActivationState | Index | Coordinate | Order | Direction }
    };

    auto overlayTree = settingsTree.getChildWithName("Overlays");

    if (!overlayTree.isValid()) {
        overlayTree = ValueTree("Overlays");

        for (auto& [name, settings] : defaults) {
            overlayTree.setProperty(name, settings, nullptr);
        }

        settingsTree.appendChild(overlayTree, nullptr);
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

        auto const newTree = ValueTree::fromXml(newSettings);
        if (!newTree.isValid()) {
            releaseFileLock();
            return;
        }

        // Children shouldn't be overwritten as that would break some valueTree links
        for (auto child : settingsTree) {
            child.copyPropertiesAndChildrenFrom(newTree.getChildWithName(child.getType()), nullptr);
        }

        settingsTree.copyPropertiesFrom(newTree, nullptr);

        for (auto* listener : listeners) {
            listener->settingsFileReloaded();
        }
        lastContentHash = contentHash;
        releaseFileLock();
    }
}

void SettingsFile::fileChanged(File const file, FileSystemWatcher::FileSystemEvent fileEvent)
{
    if (file == settingsFile) {
        reloadSettings();
    }
}

void SettingsFile::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property)
{
    for (auto* listener : listeners) {
        listener->settingsChanged(property.toString(), treeWhosePropertyHasChanged.getProperty(property));
    }

    startTimer(saveTimeoutMs);
}

void SettingsFile::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
    startTimer(saveTimeoutMs);
}

void SettingsFile::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
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

    saveCommandHistory();

    // Check if content actually changed
    auto const xml = settingsTree.toXmlString();
    auto const contentHash = xml.hashCode64();

    if (contentHash == lastContentHash) {
        return; // No changes to save
    }

    // Attempt to acquire file lock
    if (acquireFileLock()) {
        if (settingsFile.replaceWithText(xml)) {
            lastContentHash = contentHash;
        }
        releaseFileLock();
    }
}

void SettingsFile::setProperty(String const& name, var const& value)
{
    jassert(isInitialised);
    settingsTree.setProperty(name, value, nullptr);
}

bool SettingsFile::hasProperty(String const& name) const
{
    jassert(isInitialised);
    return settingsTree.hasProperty(name);
}

Value SettingsFile::getPropertyAsValue(String const& name)
{
    jassert(isInitialised);
    return settingsTree.getPropertyAsValue(name, nullptr);
}

ValueTree& SettingsFile::getValueTree()
{
    jassert(isInitialised);
    return settingsTree;
}
