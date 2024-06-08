/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>
#include "Utility/FileSystemWatcher.h"
#include "Utility/Config.h"

namespace pd {

class Instance;
class Library : public FileSystemWatcher::Listener {

public:
    explicit Library(pd::Instance* instance);

    ~Library() override
    {
        appDirChanged = nullptr;
        objectSearchThread.removeAllJobs(true, -1);
    }

    void updateLibrary();

    StringArray autocomplete(String const& query, File const& patchDirectory) const;
    void getExtraSuggestions(int currentNumSuggestions, String const& query, std::function<void(StringArray)> const& callback);

    static std::array<StringArray, 2> parseIoletTooltips(ValueTree const& iolets, String const& name, int numIn, int numOut);

    void filesystemChanged() override;

    static File findHelpfile(t_gobj* obj, File const& parentPatchFile);

    ValueTree getObjectInfo(String const& name);

    static String getObjectOrigin(t_gobj* obj);

    StringArray getAllObjects();

    std::function<void()> appDirChanged;

    // Paths to search for helpfiles
    // First, only search vanilla, then search all documentation
    // Lastly, check the deken folder
    static inline Array<File> const helpPaths = {
        ProjectInfo::appDataDir.getChildFile("Documentation"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("5.reference"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("9.else"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("10.cyclone"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("11.heavylib"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("13.pdlua"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("14.gem"),
        ProjectInfo::appDataDir.getChildFile("Extra"),
        ProjectInfo::appDataDir.getChildFile("Externals")
    };

    static inline Array<File> const defaultPaths = {
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("cyclone"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("heavylib"),
        ProjectInfo::appDataDir.getChildFile("Abstractions"),
        ProjectInfo::appDataDir.getChildFile("Externals"),
        ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Gem"),
        ProjectInfo::appDataDir.getChildFile("Extra")
    };

    static inline StringArray objectOrigins = { "vanilla", "ELSE", "cyclone", "Gem", "heavylib", "pdlua" };

private:
    StringArray allObjects;

    std::recursive_mutex libraryLock;

    FileSystemWatcher watcher;
    ThreadPool objectSearchThread = ThreadPool(1);

    ValueTree documentationTree;
    std::unordered_map<hash32, ValueTree> documentationIndex;
};

} // namespace pd
