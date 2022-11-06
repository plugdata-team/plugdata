/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

#include "../Utility/FileSystemWatcher.h"

#include <array>
#include <vector>

namespace pd {

using IODescription = juce::Array<std::pair<String, bool>>;
using IODescriptionMap = std::unordered_map<String, IODescription>;

using Suggestion = std::pair<String, bool>;
using Suggestions = std::vector<Suggestion>;

using Arguments = std::vector<std::tuple<String, String, String>>;
using ArgumentMap = std::unordered_map<String, Arguments>;

using ObjectMap = std::unordered_map<String, String>;
using KeywordMap = std::unordered_map<String, StringArray>;

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

struct Library : public FileSystemWatcher::Listener {

    ~Library()
    {
        appDirChanged = nullptr;
        libraryUpdateThread.removeAllJobs(true, -1);
    }
    void initialiseLibrary();

    void updateLibrary();
    void parseDocumentation(String const& path);

    Suggestions autocomplete(String query) const;

    String getInletOutletTooltip(String objname, int idx, int total, bool isInlet);

    void fsChangeCallback() override;
    
    File findHelpfile(t_object* obj);

    std::vector<File> helpPaths;

    ThreadPool libraryUpdateThread = ThreadPool(1);

    ObjectMap getObjectDescriptions();
    KeywordMap getObjectKeywords();
    IODescriptionMap getInletDescriptions();
    IODescriptionMap getOutletDescriptions();
    ArgumentMap getArguments();

    std::function<void()> appDirChanged;

private:
    ObjectMap objectDescriptions;
    KeywordMap objectKeywords;
    IODescriptionMap inletDescriptions;
    IODescriptionMap outletDescriptions;
    ArgumentMap arguments;

    std::mutex libraryLock;

    std::unique_ptr<Trie> searchTree = nullptr;

    File appDataDir;
    FileSystemWatcher watcher;
};

} // namespace pd
