
extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <s_stuff.h>
}

#include <utility>
#include <vector>

#include "PdLibrary.h"

struct _canvasenvironment
{
    t_symbol* ce_dir;    /* directory patch lives in */
    int ce_argc;         /* number of "$" arguments */
    t_atom* ce_argv;     /* array of "$" arguments */
    int ce_dollarzero;   /* value of "$0" */
    t_namelist* ce_path; /* search path */
};

namespace pd
{

// Iterative function to insert a key into a Trie
void Trie::insert(const std::string& key)
{
    // Names with spaces not supported yet by the suggestor
    if (std::count(key.begin(), key.end(), ' ')) return;

    // start from the root node
    Trie* curr = this;
    for (char i : key)
    {
        // create a new node if the path doesn't exist
        if (curr->character[i] == nullptr)
        {
            curr->character[i] = new Trie();
        }

        // go to the next node
        curr = curr->character[i];
    }

    // mark the current node as a leaf
    curr->isLeaf = true;
}

// Iterative function to search a key in a Trie. It returns true
// if the key is found in the Trie; otherwise, it returns false
bool Trie::search(const std::string& key)
{
    Trie* curr = this;
    for (char i : key)
    {
        // go to the next node
        curr = curr->character[i];

        // if the string is invalid (reached end of a path in the Trie)
        if (curr == nullptr)
        {
            return false;
        }
    }

    // return true if the current node is a leaf and the
    // end of the string is reached
    return curr->isLeaf;
}

// Returns true if a given node has any children
bool Trie::hasChildren()
{
    for (int i = 0; i < CHAR_SIZE; i++)
    {
        if (character[i])
        {
            return true;  // child found
        }
    }

    return false;
}

// Recursive function to delete a key in the Trie
bool Trie::deletion(Trie*& curr, std::string key)
{
    // return if Trie is empty
    if (curr == nullptr)
    {
        return false;
    }

    // if the end of the key is not reached
    if (key.length())
    {
        // recur for the node corresponding to the next character in the key
        // and if it returns true, delete the current node (if it is non-leaf)

        if (curr != nullptr && curr->character[key[0]] != nullptr && deletion(curr->character[key[0]], key.substr(1)) && !curr->isLeaf)
        {
            if (!curr->hasChildren())
            {
                delete curr;
                curr = nullptr;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    // if the end of the key is reached
    if (key.length() == 0 && curr->isLeaf)
    {
        // if the current node is a leaf node and doesn't have any children
        if (!curr->hasChildren())
        {
            // delete the current node
            delete curr;
            curr = nullptr;

            // delete the non-leaf parent nodes
            return true;
        }

        // if the current node is a leaf node and has children
        else
        {
            // mark the current node as a non-leaf node (DON'T DELETE IT)
            curr->isLeaf = false;

            // don't delete its parent nodes
            return false;
        }
    }

    return false;
}

void Trie::suggestionsRec(std::string currPrefix, Suggestions& result)
{
    // found aString in Trie with the given prefix
    if (isLeaf)
    {
        result.push_back({currPrefix, true});
    }

    // All children struct node pointers are NULL
    if (!hasChildren()) return;

    for (int i = 0; i < CHAR_SIZE; i++)
    {
        if (character[i])
        {
            // append current character to currPrefixString
            currPrefix.push_back(i);

            // recur over the rest
            character[i]->suggestionsRec(currPrefix, result);

            // remove last character
            currPrefix.pop_back();
        }
    }
}

// print suggestions for given query prefix.
int Trie::autocomplete(std::string query, Suggestions& result)
{
    auto* pCrawl = this;

    // Check if prefix is present and find the
    // the node (of last level) with last character
    // of givenString.
    int level;
    int n = query.length();
    for (level = 0; level < n; level++)
    {
        int index = CHAR_TO_INDEX(query[level]);

        // noString in the Trie has this prefix
        if (!pCrawl->character[index]) return 0;

        pCrawl = pCrawl->character[index];
    }

    // If prefix is present as a word.
    bool isWord = pCrawl->isLeaf;

    // If prefix is last node of tree (has no
    // children)
    bool isLast = !pCrawl->hasChildren();

    // If prefix is present as a word, but
    // there is no subtree below the last
    // matching node.
    if (isWord && isLast)
    {
        result.push_back({query, true});
        return -1;
    }

    // If there are are nodes below last
    // matching character.
    if (!isLast)
    {
        const std::string& prefix = query;
        pCrawl->suggestionsRec(prefix, result);
        return 1;
    }
    return 0;
}

void Library::initialiseLibrary()
{
    appDataDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");

    lastAppDirModificationTime = appDataDir.getLastModificationTime();

    updateLibrary();

    auto pddocPath = appDataDir.getChildFile("Library").getChildFile("Documentation").getChildFile("pddp").getFullPathName();

    parseDocumentation(pddocPath);

    startTimer(1300);
}

void Library::updateLibrary()
{
    auto settingsTree = ValueTree::fromXml(appDataDir.getChildFile("Settings.xml").loadFileAsString());

    auto pathTree = settingsTree.getChildWithName("Paths");

    searchTree = std::make_unique<Trie>();

    int i;
    t_class* o = pd_objectmaker;

    t_methodentry *mlist, *m;

#if PDINSTANCE
    mlist = o->c_methods[pd_this->pd_instanceno];
#else
    mlist = o->c_methods;
#endif

    for (i = o->c_nmethod, m = mlist; i--; m++)
    {
        String name(m->me_name->s_name);
        searchTree->insert(m->me_name->s_name);
    }

    searchTree->insert("graph");

    for (auto path : pathTree)
    {
        auto filePath = File(path.getProperty("Path").toString());

        for (auto& iter : RangedDirectoryIterator(filePath, true))
        {
            auto file = iter.getFile();
            // Get pd files but not help files
            if (file.getFileExtension() == ".pd" && !(file.getFileNameWithoutExtension().startsWith("help-") || file.getFileNameWithoutExtension().endsWith("-help")))
            {
                searchTree->insert(file.getFileNameWithoutExtension().toStdString());
            }
        }
    }
}

void Library::parseDocumentation(const String& path)
{
    // Function to get sections from a text file based on a section name
    // Let it know which sections exists, and it will order them and put them in a map by name
    auto getSections = [](String contents, StringArray sectionNames)
    {
        Array<std::pair<String, int>> positions;

        for (auto& section : sectionNames)
        {
            positions.add({section, contents.indexOf(section + ':')});
        }

        std::sort(positions.begin(), positions.end(), [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

        std::map<String, std::pair<String, int>> sections;

        int i = 0;
        for (auto it = positions.begin(); it < positions.end(); it++)
        {
            auto& [name, currentPosition] = *it;
            if (currentPosition < 0) continue;  // section not found

            currentPosition += name.length();

            String sectionContent;
            if (it == positions.end() - 1)
            {
                sectionContent = contents.substring(currentPosition);
            }
            else
            {
                sectionContent = contents.substring(currentPosition + 1, (*(it + 1)).second);
            }

            sections[name.trim()] = {sectionContent.substring(1).trim().unquoted(), i};
            i++;
        }

        return sections;
    };

    auto formatText = [](String text)
    {
        text = text.trim();
        // Start sentences with uppercase
        if (text.length() > 1 && (*text.toRawUTF8()) >= 'a' && (*text.toRawUTF8()) <= 'z')
        {
            text = text.replaceSection(0, 1, text.substring(0, 1).toUpperCase());
        }

        // Don't end with a period
        if (text.getLastCharacter() == '.')
        {
            text = text.upToLastOccurrenceOf(".", false, false);
        }

        return text;
    };

    auto parseFile = [this, getSections, formatText](File& f)
    {
        String contents = f.loadFileAsString();
        auto sections = getSections(contents, {"\ntitle", "\ndescription", "\npdcategory", "\ncategories", "\narguments", "\nlast_update", "\ninlets", "\noutlets", "\ndraft"});

        if (!sections.count("title")) return;

        String name = sections["title"].first;

        if (sections.count("description"))
        {
            objectDescriptions[name] = sections["description"].first;
        }

        if (sections.count("arguments"))
        {
            Arguments args;

            for (auto& argument : StringArray::fromTokens(sections["arguments"].first.fromFirstOccurrenceOf("-", false, false), "-", "\""))
            {
                auto sectionMap = getSections(argument, {"type", "description", "default"});
                args.push_back({sectionMap["type"].first, sectionMap["description"].first, sectionMap["default"].first});
            }

            arguments[name] = args;
        }

        auto numbers = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "nth"};
        if (sections.count("inlets"))
        {
            auto section = getSections(sections["inlets"].first, numbers);
            inletDescriptions[name].resize(section.size());
            for (auto [number, content] : section)
            {
                String tooltip;
                for (auto& argument : StringArray::fromTokens(content.first, "-", "\""))
                {
                    auto sectionMap = getSections(argument, {"type", "description"});
                    if (sectionMap["type"].first.isEmpty()) continue;

                    tooltip += "(" + sectionMap["type"].first + ") " + sectionMap["description"].first + "\n";
                }

                inletDescriptions[name].getReference(content.second) = {tooltip, number == "nth"};
            }
        }
        if (sections.count("outlets"))
        {
            auto section = getSections(sections["outlets"].first, numbers);
            outletDescriptions[name].resize(section.size());
            for (auto [number, content] : section)
            {
                String tooltip;

                for (auto& argument : StringArray::fromTokens(content.first, "-", "\""))
                {
                    auto sectionMap = getSections(argument, {"type", "description"});
                    if (sectionMap["type"].first.isEmpty()) continue;
                    tooltip += "(" + sectionMap["type"].first + ") " + sectionMap["description"].first + "\n";
                }

                outletDescriptions[name].getReference(content.second) = {tooltip, number == "nth"};
            }
        }
    };

    for (auto& iter : RangedDirectoryIterator(path, true))
    {
        auto file = iter.getFile();

        if (file.hasFileExtension("md"))
        {
            parseFile(file);
        }
    }
}

Suggestions Library::autocomplete(std::string query)
{
    Suggestions result;
    searchTree->autocomplete(std::move(query), result);
    return result;
}

String Library::getInletOutletTooltip(String boxname, int idx, int total, bool isInlet)
{
    auto name = boxname.upToFirstOccurrenceOf(" ", false, false);
    auto args = StringArray::fromTokens(boxname.fromFirstOccurrenceOf(" ", false, false), true);

    auto findInfo = [&name, &args, &total, &idx](IODescriptionMap& map)
    {
        if (map.count(name))
        {
            auto descriptions = map.at(name);

            // if the amount of inlets is not equal to the amount in the spec, look for repeating inlets
            if (descriptions.size() < total)
            {
                for (int i = 0; i < descriptions.size(); i++)
                {
                    if (descriptions[i].second)
                    {  // repeating inlet found
                        for (int j = 0; j < total - descriptions.size(); j++)
                        {
                            descriptions.insert(i, descriptions[i]);
                        }
                    }
                }
            }

            auto result = isPositiveAndBelow(idx, descriptions.size()) ? descriptions[idx].first : String();
            result = result.replace("$mth", String(idx));
            result = result.replace("$nth", String(idx + 1));
            result = result.replace("$arg", args[idx]);

            return result;
        }

        return String();
    };

    return isInlet ? findInfo(inletDescriptions) : findInfo(outletDescriptions);
}

void Library::timerCallback()
{
    if (lastAppDirModificationTime.getMillisecondCounter() < appDataDir.getLastModificationTime().getMillisecondCounter())
    {
        appDirChanged();
        updateLibrary();
        lastAppDirModificationTime = appDataDir.getLastModificationTime();
    }
}

}  // namespace pd

/* Code for generating library markdown files, not in usage but useful for later

 // wait for pd to initialise
 Timer::callAfterDelay(1200, [this](){

     enqueueFunction([this](){
     t_class* o = pd_objectmaker;
#if PDINSTANCE
     t_methodentry* mlist = o->c_methods[pd_this->pd_instanceno];
#else
     t_methodentry* mlist = o->c_methods;
#endif

 t_methodentry* m;

 int i;
 for (i = o->c_nmethod, m = mlist; i--; m++)
 {
     String name(m->me_name->s_name);
     StringArray arguments;

     if(name == "onebang_proxy" || name == "midi") continue;

     t_atom args[9];
     int nargs = 3;
     for(int i = 0; i < 6; i++) {
         auto atomtype = (t_atomtype)m->me_arg[i];
         String type;
         auto* target = args + i + 3;

         if(atomtype == A_NULL) {
             break;
         }

         nargs++;
         if(atomtype == A_FLOAT) {
             type = "float";
             SETFLOAT(target, 0);
         }
         else if(atomtype == A_SYMBOL) {
             type = "symbol";
             SETSYMBOL(target, gensym("0"));
         }
         else if(atomtype == A_GIMME) {
             type = "gimme";
             SETFLOAT(target, 0);
             arguments.add(type);
             break;
         }
         else if(atomtype == A_POINTER) {
             type = "pointer";
             SETPOINTER(target, 0);
         }
         else if(atomtype == A_SEMI) {
             type = "semi";
             SETSEMI(target);
         }
         else if(atomtype == A_COMMA) {
             type = "comma";
             SETCOMMA(target);
         }
         else if(atomtype == A_DEFFLOAT) {
             type = "float";
             SETFLOAT(target, 0);
         }
         else if(atomtype == A_DEFSYM) {
             type = "symbol";
             SETSYMBOL(target, gensym("0"));
         }
         else if(atomtype == A_DOLLSYM) {
             type = "dollsym";
             SETDOLLSYM(target, gensym("$1"));
         }

         arguments.add(type);
     }

     SETFLOAT(args, 20.0f);
     SETFLOAT(args + 1, 20.0f);
     SETSYMBOL(args + 2, gensym(name.toRawUTF8()));

     auto* obj = pd_checkobject(libpd_createobj(patches[0]->getPointer(), gensym("obj"), nargs, args));

     if(!obj) continue;
     int nin = libpd_ninlets(obj);
     int nout = libpd_noutlets(obj);

     t_inlet* i;
     t_outlet* i_out;

     StringArray inletTypes;
     StringArray outletTypes;

     if(name == "metro") {
         std::cout << "hey" << std::endl;
     }

     for (i = (t_inlet*)obj->ob_inlet; i; i = i->i_next) {
         if(!i->i_symfrom) {
             inletTypes.add("?");
             continue;
         }
         inletTypes.add(i->i_symfrom->s_name);
     }
     while(inletTypes.size() && nin > inletTypes.size()) {
         inletTypes.add(inletTypes.getReference(inletTypes.size() - 1));
         nin--;
     }

     for (i_out = (t_outlet*)obj->ob_outlet; i_out; i_out = i_out->o_next) {
         if(!i_out->o_sym) {
             outletTypes.add("?");
             continue;
         }
         outletTypes.add(i_out->o_sym->s_name);
     }

     while(outletTypes.size() && nout > outletTypes.size()) {
         outletTypes.add(outletTypes.getReference(outletTypes.size() - 1));
         nout--;
     }

     auto file = File("/Users/timschoen/Projecten/PlugData/Resources/pddp/NEW/" + name + ".md");

     String newFile;
     newFile += "---\n";
     newFile += "title: " + name + "\n";
     newFile += "description:\n";
     newFile += "categories:\n";
     newFile += " - object\n";
     newFile += "pdcategory: General\n";
     newFile += "arguments:\n";

     StringArray numbers = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th"};


     for(auto& arg : arguments) {

         newFile += "- type: " + arg + "\n";
         newFile += "  description:\n";
         newFile += "  default:\n";
     }

     int idx = 0;
     newFile += "inlets:\n";
     for(auto& inlet : inletTypes) {
         newFile += "  " + numbers[idx] + ":\n";
         newFile += "  - type: " + inlet + "\n";
         newFile += "    description:\n";
         idx++;
     }

     idx = 0;
     newFile += "outlets:\n";
     for(auto& outlet : outletTypes) {
         newFile += "  " + numbers[idx] + ":\n";
         newFile += "  - type: " + outlet + "\n";
         newFile += "    description:\n";
         idx++;
     }

     file.create();
     file.replaceWithText(newFile);

 }

     });
 });

 */
