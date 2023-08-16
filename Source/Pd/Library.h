/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>
#include "../Utility/FileSystemWatcher.h"
#include "../Utility/Config.h"

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

    StringArray autocomplete(String const& query, const File& patchDirectory) const;
    void getExtraSuggestions(int currentNumSuggestions, String const& query, std::function<void(StringArray)> const& callback);

    static std::array<StringArray, 2> parseIoletTooltips(ValueTree const& iolets, String const& name, int numIn, int numOut);

    void fsChangeCallback() override;
    
    File findHelpfile(t_object* obj, File const& parentPatchFile) const;

    ValueTree getObjectInfo(String const& name);

    StringArray getAllObjects();
    StringArray getAllCategories();

    Array<File> helpPaths;

    std::function<void()> appDirChanged;

    static inline Array<File> const defaultPaths = {
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("cyclone"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("heavylib"),
        ProjectInfo::appDataDir.getChildFile("Abstractions"),
        ProjectInfo::appDataDir.getChildFile("Externals"),
        ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Extra")
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


