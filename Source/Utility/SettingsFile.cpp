/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "SettingsFile.h"
#include "../LookAndFeel.h"

SettingsFileListener::SettingsFileListener()
{
    SettingsFile::getInstance()->listeners.add(this);
}

SettingsFileListener::~SettingsFileListener()
{
    SettingsFile::getInstance()->listeners.removeFirstMatchingValue(this);
}

JUCE_IMPLEMENT_SINGLETON(SettingsFile)

SettingsFile::~SettingsFile()
{
    // Save current settings before quitting
    saveSettings();

    clearSingletonInstance();
}

SettingsFile* SettingsFile::initialise()
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

ValueTree SettingsFile::getKeyMapTree()
{
    return settingsTree.getChildWithName("KeyMap");
}

ValueTree SettingsFile::getColourThemesTree()
{
    return settingsTree.getChildWithName("ColourThemes");
}

ValueTree SettingsFile::getPathsTree()
{
    return settingsTree.getChildWithName("Paths");
}

ValueTree SettingsFile::getSelectedThemesTree()
{
    return settingsTree.getChildWithName("SelectedThemes");
}

ValueTree SettingsFile::getLibrariesTree()
{
    return settingsTree.getChildWithName("Libraries");
}

ValueTree SettingsFile::getTheme(String name)
{
    return getColourThemesTree().getChildWithProperty("theme", name);
}

ValueTree SettingsFile::getCurrentTheme()
{
    return getColourThemesTree().getChildWithProperty("theme", settingsTree.getProperty("theme"));
}

void SettingsFile::initialisePathsTree()
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

void SettingsFile::addToRecentlyOpened(File path)
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
        recentlyOpened.addChild(subTree, 0, nullptr);
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

            if (!themeTree.hasProperty("dashed_signal_connections")) {
                themeTree.setProperty("dashed_signal_connections", true, nullptr);
            }
            if (!themeTree.hasProperty("straight_connections")) {
                themeTree.setProperty("straight_connections", false, nullptr);
            }
            if (!themeTree.hasProperty("thin_connections")) {
                themeTree.setProperty("thin_connections", false, nullptr);
            }
            if (!themeTree.hasProperty("square_iolets")) {
                themeTree.setProperty("square_iolets", false, nullptr);
            }
            if (!themeTree.hasProperty("square_object_corners")) {
                themeTree.setProperty("square_object_corners", false, nullptr);
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

void SettingsFile::reloadSettings()
{
    if (settingsChangedInternally) {
        settingsChangedInternally = false;
        return;
    }

    settingsChangedExternally = true;

    jassert(isInitialised);

    auto newTree = ValueTree::fromXml(settingsFile.loadFileAsString());

    // Children shouldn't be overwritten as that would break some valueTree links
    for (auto child : settingsTree) {
        child.copyPropertiesAndChildrenFrom(newTree.getChildWithName(child.getType()), nullptr);
    }

    settingsTree.copyPropertiesFrom(newTree, nullptr);

    for (auto* listener : listeners) {
        listener->settingsFileReloaded();
    }
}

void SettingsFile::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property)
{
    for (auto* listener : listeners) {
        listener->propertyChanged(property.toString(), treeWhosePropertyHasChanged.getProperty(property));
    }

    if (!settingsChangedExternally)
        settingsChangedInternally = true;
    startTimer(700);
}

void SettingsFile::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
    if (!settingsChangedExternally)
        settingsChangedInternally = true;
    startTimer(700);
}

void SettingsFile::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if (!settingsChangedExternally)
        settingsChangedInternally = true;
    startTimer(700);
}

void SettingsFile::timerCallback()
{
    jassert(isInitialised);

    // Don't save again if we just loaded it from file
    if (settingsChangedExternally) {
        settingsChangedExternally = false;
        stopTimer();
        return;
    }

    // Save settings to file whenever valuetree state changes
    // Use timer to group changes together
    saveSettings();
    stopTimer();
}

void SettingsFile::saveSettings()
{
    jassert(isInitialised);
    // Save settings to file
    auto xml = settingsTree.toXmlString();
    settingsFile.replaceWithText(xml);
}

void SettingsFile::setProperty(String name, var value)
{
    jassert(isInitialised);
    settingsTree.setProperty(name, value, nullptr);
}

bool SettingsFile::hasProperty(String name)
{
    jassert(isInitialised);
    return settingsTree.hasProperty(name);
}

Value SettingsFile::getPropertyAsValue(String name)
{
    jassert(isInitialised);
    return settingsTree.getPropertyAsValue(name, nullptr);
}

ValueTree SettingsFile::getValueTree()
{
    jassert(isInitialised);
    return settingsTree;
}
