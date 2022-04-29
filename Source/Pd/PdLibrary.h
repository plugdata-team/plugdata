/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

namespace pd
{

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

// A class to store a Trie node
class Trie
{
   public:
    bool isLeaf;
    Trie* character[CHAR_SIZE];

    // Constructor
    Trie()
    {
        isLeaf = false;

        for (int i = 0; i < CHAR_SIZE; i++)
        {
            character[i] = nullptr;
        }
    }

    ~Trie()
    {
        for (int i = 0; i < CHAR_SIZE; i++)
        {
            if (character[i])
            {
                delete character[i];
            }
        }
    }

    void insert(const std::string& key);
    bool deletion(Trie*&, std::string);
    bool search(const std::string&);
    bool hasChildren();

    void suggestionsRec(std::string currPrefix, Suggestions& result);
    int autocomplete(std::string query, Suggestions& result);
};

struct Library : public Timer
{
    void initialiseLibrary();

    void updateLibrary();
    void parseDocumentation(const String& path);

    Suggestions autocomplete(std::string query);
    
    String getInletOutletTooltip(String boxname, int idx, int total, bool isInlet);

    void timerCallback() override;

    ObjectMap objectDescriptions;
    KeywordMap objectKeywords;
    IODescriptionMap inletDescriptions;
    IODescriptionMap outletDescriptions;
    ArgumentMap arguments;
    
    std::unordered_map<String, std::pair<StringArray, StringArray>> edgeDescriptions;

    std::unique_ptr<Trie> searchTree;

    File appDataDir;

    std::function<void()> appDirChanged;

    Time lastAppDirModificationTime;
};

}  // namespace pd
