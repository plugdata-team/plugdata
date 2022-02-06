/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once



#include <array>
#include <vector>

#include <JuceHeader.h>

namespace pd
{

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
    this->isLeaf = false;

    for (int i = 0; i < CHAR_SIZE; i++)
    {
      this->character[i] = nullptr;
    }
  }

  void insert(const std::string& key);
  bool deletion(Trie*&, std::string);
  bool search(const std::string&);
  bool hasChildren();

  void suggestionsRec(std::string currPrefix, std::vector<std::string>& result);
  int autocomplete(std::string query, std::vector<std::string>& result);
};

struct Library
{
  void initialiseLibrary(ValueTree pathTree);

  std::vector<std::string> autocomplete(std::string query);

  Trie searchTree;
};

}  // namespace pd
