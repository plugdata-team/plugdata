#include <vector>
#include <string>
#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <s_stuff.h>

#include <JuceHeader.h>

#include "PdLibrary.hpp"



struct _canvasenvironment
{
    t_symbol *ce_dir;      /* directory patch lives in */
    int ce_argc;           /* number of "$" arguments */
    t_atom *ce_argv;       /* array of "$" arguments */
    int ce_dollarzero;     /* value of "$0" */
    t_namelist *ce_path;   /* search path */
};

namespace pd
{


// Returns new trie node (initialized to NULLs)
static TrieNode *getNode(void)
{
    struct TrieNode *pNode = new TrieNode;
    pNode->isWordEnd = false;
    
    for (int i = 0; i < ALPHABET_SIZE; i++)
        pNode->children[i] = NULL;
    
    return pNode;
}

// If not present, inserts key into trie.  If the
// key is prefix of trie node, just marks leaf node
static void insert(struct TrieNode *root,  const std::string key)
{
    struct TrieNode *pCrawl = root;
    
    for (int level = 0; level < key.length(); level++)
    {
        int index = CHAR_TO_INDEX(key[level]);
        if (!pCrawl->children[index])
            pCrawl->children[index] = getNode();
        
        pCrawl = pCrawl->children[index];
    }
    
    // mark last node as leaf
    pCrawl->isWordEnd = true;
}

// Returns true if key presents in trie, else false
static bool search(struct TrieNode *root, const std::string key)
{
    int length = key.length();
    struct TrieNode *pCrawl = root;
    for (int level = 0; level < length; level++)
    {
        int index = CHAR_TO_INDEX(key[level]);
        
        if (!pCrawl->children[index])
            return false;
        
        pCrawl = pCrawl->children[index];
    }
    
    return (pCrawl != NULL && pCrawl->isWordEnd);
}

// Returns 0 if current node has a child
// If all children are NULL, return 1.
static bool isLastNode(struct TrieNode* root)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (root->children[i])
            return 0;
    return 1;
}

// Recursive function to print auto-suggestions for given
// node.
static void suggestionsRec(struct TrieNode* root, std::string currPrefix, std::vector<std::string>& result)
{
    // found aString in Trie with the given prefix
    if (root->isWordEnd)
    {
        result.push_back(currPrefix);
    }
    
    // All children struct node pointers are NULL
    if (isLastNode(root))
        return;
    
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i])
        {
            // append current character to currPrefixString
            currPrefix.push_back(i);
            
            // recur over the rest
            suggestionsRec(root->children[i], currPrefix, result);
            // remove last character
            currPrefix.pop_back();
        }
    }
}



// print suggestions for given query prefix.
static int autocomplete(TrieNode* root, std::string query, std::vector<std::string>& result)
{
    struct TrieNode* pCrawl = root;
    
    // Check if prefix is present and find the
    // the node (of last level) with last character
    // of givenString.
    int level;
    int n = query.length();
    for (level = 0; level < n; level++)
    {
        int index = CHAR_TO_INDEX(query[level]);
        
        // noString in the Trie has this prefix
        if (!pCrawl->children[index])
            return 0;
        
        pCrawl = pCrawl->children[index];
    }
    
    // If prefix is present as a word.
    bool isWord = (pCrawl->isWordEnd == true);
    
    // If prefix is last node of tree (has no
    // children)
    bool isLast = isLastNode(pCrawl);
    
    // If prefix is present as a word, but
    // there is no subtree below the last
    // matching node.
    if (isWord && isLast)
    {
        result.push_back(query);
        return -1;
    }
    
    // If there are are nodes below last
    // matching character.
    if (!isLast)
    {
        std::string prefix = query;
        suggestionsRec(pCrawl, prefix, result);
        return 1;
    }
    return 0;
    
}


Library::Library() {
    tree = getNode();
}

void Library::initialiseLibrary(ValueTree pathTree)
{
    
    int i;
    t_class* o = pd_objectmaker;
    
    t_methodentry *mlist, *m;
    
#ifdef PDINSTANCE
    mlist = o->c_methods[pd_this->pd_instanceno];
#else
    mlist = o->c_methods;
#endif
    
    for (i = o->c_nmethod, m = mlist; i--; m++) {
        String name(m->me_name->s_name);
        insert(tree, m->me_name->s_name);
    }
    
    insert(tree, "graph");
    
    for(auto path : pathTree) {
        auto filePath = File(path.getProperty("Path").toString());
        
        for(auto& iter : RangedDirectoryIterator(filePath, false)) {
            auto file = iter.getFile();
            if(file.getFileExtension() == ".pd")
                insert(tree, file.getFileNameWithoutExtension().toStdString());

        }
    }


}

std::vector<std::string> Library::autocomplete(std::string query) {
    std::vector<std::string> result;
    pd::autocomplete(tree, query, result);
    return result;
}


}



