/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>
#include "Utility/FileSystemWatcher.h"

namespace pd {

class Instance;
class Library : public FileSystemWatcher::Listener {

public:
    Library(pd::Instance* instance);

    ~Library() override
    {
        appDirChanged = nullptr;
        objectSearchThread.removeAllJobs(true, -1);
    }

    void updateLibrary();

    StringArray autocomplete(String const& query) const;
    void getExtraSuggestions(int currentNumSuggestions, String const& query, std::function<void(StringArray)> const& callback);

    static std::array<StringArray, 2> parseIoletTooltips(ValueTree const& iolets, String const& name, int numIn, int numOut);

    void fsChangeCallback() override;

    File findHelpfile(t_object* obj, File const& parentPatchFile) const;

    ValueTree getObjectInfo(String const& name);

    StringArray getAllObjects();
    StringArray getAllCategories();

    Array<File> helpPaths;

    std::function<void()> appDirChanged;

    static inline const File appDataDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    static inline Array<File> const defaultPaths = {
        appDataDir.getChildFile("Library").getChildFile("Abstractions").getChildFile("else"),
        appDataDir.getChildFile("Library").getChildFile("Abstractions").getChildFile("cyclone"),
        appDataDir.getChildFile("Library").getChildFile("Abstractions").getChildFile("heavylib"),
        appDataDir.getChildFile("Library").getChildFile("Abstractions"),
        appDataDir.getChildFile("Library").getChildFile("Deken"),
        appDataDir.getChildFile("Library").getChildFile("Extra").getChildFile("else"),
        appDataDir.getChildFile("Library").getChildFile("Extra")
    };

    static inline StringArray objectOrigins = { "vanilla", "ELSE", "cyclone", "heavylib", "pdlua" };

private:
    StringArray allObjects;
    StringArray allCategories;

    std::recursive_mutex libraryLock;

    FileSystemWatcher watcher;
    ThreadPool objectSearchThread = ThreadPool(1);

    ValueTree documentationTree;
};

} // namespace pd
