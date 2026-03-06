/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>
#include "Utility/FileSystemWatcher.h"
#include "Utility/Config.h"

#include <fuzzysearchdatabase/src/FuzzySearchDatabase.hpp>

namespace pd {

class Instance;
class Library final : public FileSystemWatcher::Listener
    , public Thread {

public:
    struct ObjectReferenceTable {
        struct ReferenceItem {
            String type;
            String description;
        };
        struct IoletReference {
            String tooltip;
            HeapArray<ReferenceItem> messages;
            bool variable;
        };
        using IoletsReference = HeapArray<IoletReference>;

        String title;
        String description;
        HeapArray<String> categories;
        IoletsReference inlets;
        IoletsReference outlets;
        HeapArray<ReferenceItem> arguments;
        HeapArray<ReferenceItem> methods;
        HeapArray<ReferenceItem> flags;
    };

    explicit Library(pd::Instance* instance);

    ~Library() override;

    void run() override;

    void ensureDatabaseInitialised() const;

    void updateLibrary();

    bool isGemObject(String const& query) const;

    StringArray autocomplete(String const& query, File const& patchDirectory) const;
    StringArray searchObjectDocumentation(String const& query);

    static File findPatch(String const& patchToFind);
    static File findFile(String const& fileToFind);

    static StackArray<StringArray, 2> parseIoletTooltips(ObjectReferenceTable::IoletsReference const& inlets, ObjectReferenceTable::IoletsReference const& outlets, String const& name, int numIn, int numOut);

    void filesystemChanged() override;

    static File findHelpfile(String const& name);
    static File findHelpfile(t_gobj* obj, File const& parentPatchFile);

    Library::ObjectReferenceTable const& getObjectInfo(String const& name);

    static String getObjectOrigin(t_gobj* obj);

    StringArray getAllObjects();

    std::function<void()> appDirChanged;

    // Paths to search for helpfiles
    // First, only search vanilla, then search all documentation
    // Lastly, check the deken folder
    static inline StackArray<File, 9> const helpPaths = { ProjectInfo::appDataDir.getChildFile("Documentation"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("5.reference"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("9.else"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("10.cyclone"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("11.heavylib"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("13.pdlua"),
        ProjectInfo::appDataDir.getChildFile("Documentation").getChildFile("14.gem"),
        ProjectInfo::appDataDir.getChildFile("Extra"),
        ProjectInfo::appDataDir.getChildFile("Externals") };

    static inline StackArray<File, 8> const defaultPaths = { ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("cyclone"),
        ProjectInfo::appDataDir.getChildFile("Abstractions").getChildFile("heavylib"),
        ProjectInfo::appDataDir.getChildFile("Abstractions"),
        ProjectInfo::appDataDir.getChildFile("Externals"),
        ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("else"),
        ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Gem"),
        ProjectInfo::appDataDir.getChildFile("Extra") };

    static inline StringArray objectOrigins = { "vanilla", "ELSE", "cyclone", "Gem", "heavylib", "pdlua", "MERDA" };

private:
    StringArray allObjects;
    StringArray gemObjects;

    FileSystemWatcher watcher;
    WaitableEvent initWait;
    pd::Instance* pd;

    ObjectReferenceTable parseObjectEntry(ValueTree const& objectEntry);

    HeapArray<ObjectReferenceTable> documentation;
    fuzzysearch::Database<ObjectReferenceTable*> searchDatabase;
    UnorderedMap<hash32, ObjectReferenceTable*> documentationIndex;
};

} // namespace pd
