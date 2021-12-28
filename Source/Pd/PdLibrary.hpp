/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <array>
#include <vector>


namespace pd
{


// ASCII size
#define ALPHABET_SIZE 128
#define CHAR_TO_INDEX(c) ((int)c - (int)'\0')
    
// trie node
struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
  
    // isWordEnd is true if the node represents
    // end of a word
    bool isWordEnd;
};

struct Library
{
    Library();
    
    void initialiseLibrary(ValueTree pathTree);
    
    std::vector<std::string> autocomplete(std::string query);
    
    
    TrieNode* tree;
    
};


}
