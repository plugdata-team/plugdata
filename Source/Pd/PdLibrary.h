/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>
#include <m_pd.h>

#include "../Utility/FileSystemWatcher.h"

#include <array>
#include <vector>

namespace pd {

using IODescription = Array<std::pair<String, bool>>;
using IODescriptionMap = std::unordered_map<String, std::array<IODescription, 2>>;

using Suggestions = StringArray;

using Arguments = std::vector<std::tuple<String, String, String>>;
using ArgumentMap = std::unordered_map<String, Arguments>;

using Methods = std::vector<std::pair<String, String>>;
using MethodMap = std::unordered_map<String, Methods>;

using ObjectMap = std::unordered_map<String, String>;
using KeywordMap = std::unordered_map<String, StringArray>;
using CategoryMap = std::unordered_map<String, StringArray>;
// Define the character size
#define CHAR_SIZE 128
#define CHAR_TO_INDEX(c) (static_cast<int>(c) - static_cast<int>('\0'))
#define INDEX_TO_CHAR(c) static_cast<char>(c + static_cast<int>('\0'))
// A class to store a Trie node
class Trie {
public:
    bool isLeaf;
    Trie* character[CHAR_SIZE];

    // Constructor
    Trie()
    {
        isLeaf = false;

        for (int i = 0; i < CHAR_SIZE; i++) {
            character[i] = nullptr;
        }
    }

    ~Trie()
    {
        for (int i = 0; i < CHAR_SIZE; i++) {
            if (character[i]) {
                delete character[i];
            }
        }
    }

    void insert(String const& key);
    bool deletion(Trie*&, String);
    bool search(String const&);
    bool hasChildren();

    void suggestionsRec(String currPrefix, Suggestions& result);
    int autocomplete(String query, Suggestions& result);
};

class Library : public FileSystemWatcher::Listener {

public:
    ~Library()
    {
        appDirChanged = nullptr;
        libraryUpdateThread.removeAllJobs(true, -1);
    }
    void initialiseLibrary();

    void updateLibrary();
    void parseDocumentation(String const& path);

    Suggestions autocomplete(String query) const;
    void getExtraSuggestions(int currentNumSuggestions, String query, std::function<void(Suggestions)> callback);

    std::array<StringArray, 2> getIoletTooltips(String type, String name, int numIn, int numOut);

    void fsChangeCallback() override;

    File findHelpfile(t_object* obj, File parentPatchFile);

    Array<File> helpPaths;

    ThreadPool libraryUpdateThread = ThreadPool(1);

    ObjectMap getObjectDescriptions();
    KeywordMap getObjectKeywords();
    CategoryMap getObjectCategories();
    IODescriptionMap getIoletDescriptions();
    StringArray getAllObjects();
    ArgumentMap getArguments();
    MethodMap getMethods();

    std::function<void()> appDirChanged;

    static inline const File appDataDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    static inline Array<File> const defaultPaths = {
        appDataDir.getChildFile("Library").getChildFile("Abstractions").getChildFile("else"),
        appDataDir.getChildFile("Library").getChildFile("Abstractions").getChildFile("heavylib"),
        appDataDir.getChildFile("Library").getChildFile("Abstractions"),
        appDataDir.getChildFile("Library").getChildFile("Deken"),
        appDataDir.getChildFile("Library").getChildFile("Extra").getChildFile("else")
    };
    
    static inline StringArray objectOrigins = {"vanilla", "ELSE", "cyclone", "heavylib", "pdlua"};

private:
    ObjectMap objectDescriptions;
    KeywordMap objectKeywords;
    CategoryMap objectCategories;
    IODescriptionMap ioletDescriptions;
    ArgumentMap arguments;
    MethodMap methods;

    StringArray allObjects;

    std::mutex libraryLock;

    std::unique_ptr<Trie> searchTree = nullptr;

    FileSystemWatcher watcher;
};

} // namespace pd
