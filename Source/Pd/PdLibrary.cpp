/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <s_stuff.h>
#include <z_libpd.h>
}

#include <utility>
#include <vector>

#include "PdLibrary.h"

struct _canvasenvironment {
    t_symbol* ce_dir;    /* directory patch lives in */
    int ce_argc;         /* number of "$" arguments */
    t_atom* ce_argv;     /* array of "$" arguments */
    int ce_dollarzero;   /* value of "$0" */
    t_namelist* ce_path; /* search path */
};

namespace pd {

// Iterative function to insert a key into a Trie
void Trie::insert(String key)
{
    // Names with spaces not supported yet by the suggestor
    if (key.containsChar(' '))
        key = key.upToFirstOccurrenceOf(" ", false, false);

    // start from the root node
    Trie* curr = this;
    for (char i : key) {
        if (!curr)
            continue;

        // create a new node if the path doesn't exist
        if (curr->character[i] == nullptr) {
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
bool Trie::search(String const& key)
{
    Trie* curr = this;
    for (char i : key) {
        // go to the next node
        curr = curr->character[i];

        // if the string is invalid (reached end of a path in the Trie)
        if (curr == nullptr) {
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
    for (auto& i : character) {
        if (i) {
            return true; // child found
        }
    }

    return false;
}

// Recursive function to delete a key in the Trie
bool Trie::deletion(Trie*& curr, String key)
{
    // return if Trie is empty
    if (curr == nullptr) {
        return false;
    }

    // if the end of the key is not reached
    if (key.length()) {
        // recur for the node corresponding to the next character in the key
        // and if it returns true, delete the current node (if it is non-leaf)

        if (curr != nullptr && curr->character[key[0]] != nullptr && deletion(curr->character[key[0]], key.substring(1)) && !curr->isLeaf) {
            if (!curr->hasChildren()) {
                delete curr;
                curr = nullptr;
                return true;
            }

            return false;
        }
    }

    // if the end of the key is reached
    if (key.length() == 0 && curr->isLeaf) {
        // if the current node is a leaf node and doesn't have any children
        if (!curr->hasChildren()) {
            // delete the current node
            delete curr;
            curr = nullptr;

            // delete the non-leaf parent nodes
            return true;
        }

        // mark the current node as a non-leaf node (DON'T DELETE IT)
        curr->isLeaf = false;

        // don't delete its parent nodes
        return false;
    }

    return false;
}

void Trie::suggestionsRec(String currPrefix, Suggestions& result)
{
    // found aString in Trie with the given prefix
    if (isLeaf) {
        result.add(currPrefix);
    }

    // All children struct node pointers are nullptr
    if (!hasChildren())
        return;

    for (int i = 0; i < CHAR_SIZE; i++) {
        if (character[i]) {
            // append current character to currPrefixString
            // currPrefix += i;
            char letter = INDEX_TO_CHAR(i);
            currPrefix += String(&letter, 1);

            // recur over the rest
            character[i]->suggestionsRec(currPrefix, result);

            // remove last character
            currPrefix = currPrefix.dropLastCharacters(1);
        }
    }
}

// print suggestions for given query prefix.
int Trie::autocomplete(String query, Suggestions& result)
{
    auto* pCrawl = this;

    // Check if prefix is present and find the
    // the node (of last level) with last character
    // of givenString.
    int level;
    int n = query.length();
    for (level = 0; level < n; level++) {
        int index = CHAR_TO_INDEX(query[level]);

        // no String in the Trie has this prefix
        if (index < 0 || index >= 127 || !pCrawl->character[index])
            return 0;

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
    if (isWord && isLast) {
        result.add(query);
        return -1;
    }

    // If there are are nodes below last
    // matching character.
    if (!isLast) {
        String const& prefix = query;
        pCrawl->suggestionsRec(prefix, result);
        return 1;
    }
    return 0;
}

void Library::initialiseLibrary()
{
    auto* pdinstance = pd_this;

    auto updateFn = [this, pdinstance]() {
        libraryLock.lock();

// Make sure instance is set correctly for this thread
#ifdef PDINSTANCE
        pd_setinstance(pdinstance);
#endif

        auto pddocPath = appDataDir.getChildFile("Library").getChildFile("Documentation").getChildFile("pddp").getFullPathName();

        updateLibrary();
        parseDocumentation(pddocPath);

        // Paths to search
        // First, only search vanilla, then search all documentation
        // Lastly, check the deken folder
        helpPaths = { appDataDir.getChildFile("Library").getChildFile("Documentation").getChildFile("5.reference"), appDataDir.getChildFile("Library").getChildFile("Documentation"),
            appDataDir.getChildFile("Deken") };

        libraryLock.unlock();

        // Update docs in GUI
        MessageManager::callAsync([this]() {
            watcher.addFolder(appDataDir);
            watcher.addListener(this);

            if (appDirChanged)
                appDirChanged();
        });
    };

    libraryUpdateThread.addJob(updateFn);
}

void Library::updateLibrary()
{
    auto* pdinstance = pd_this;
    auto updateFn = [this, pdinstance]() {
        auto settingsTree = ValueTree::fromXml(appDataDir.getChildFile("Settings.xml").loadFileAsString());

        auto pathTree = settingsTree.getChildWithName("Paths");

        searchTree = std::make_unique<Trie>();

        // Get available objects directly from pd
        int i;
        t_class* o = pd_objectmaker;

        t_methodentry *mlist, *m;

#if PDINSTANCE
        mlist = o->c_methods[pdinstance->pd_instanceno];
#else
        mlist = o->c_methods;
#endif

        allObjects.clear();

        for (i = o->c_nmethod, m = mlist; i--; m++) {

            auto newName = String(m->me_name->s_name);
            if (!(newName.startsWith("else/") || newName.startsWith("cyclone/"))) {
                allObjects.add(newName);
                searchTree->insert(m->me_name->s_name);
            }
        }

        searchTree->insert("graph");

        for (auto& path : defaultPaths) {
            for (const auto& iter : RangedDirectoryIterator(path, false)) {
                auto file = iter.getFile();
                // Get pd files but not help files
                if (file.getFileExtension() == ".pd" && !(file.getFileNameWithoutExtension().startsWith("help-") || file.getFileNameWithoutExtension().endsWith("-help"))) {
                    searchTree->insert(file.getFileNameWithoutExtension().toStdString());
                    allObjects.add(file.getFileNameWithoutExtension().toStdString());
                }
            }
        }

        // Find patches in our search tree
        for (auto path : pathTree) {
            auto filePath = File(path.getProperty("Path").toString());

            for (const auto& iter : RangedDirectoryIterator(filePath, false)) {
                auto file = iter.getFile();
                // Get pd files but not help files
                if (file.getFileExtension() == ".pd" && !(file.getFileNameWithoutExtension().startsWith("help-") || file.getFileNameWithoutExtension().endsWith("-help"))) {
                    searchTree->insert(file.getFileNameWithoutExtension().toStdString());
                    allObjects.add(file.getFileNameWithoutExtension().toStdString());
                }
            }
        }
    };

    libraryUpdateThread.addJob(updateFn);
}

void Library::parseDocumentation(String const& path)
{
    // Function to get sections from a text file based on a section name
    // Let it know which sections exists, and it will order them and put them in a map by name
    auto getSections = [](String contents, StringArray sectionNames) {
        Array<std::pair<String, int>> positions;

        for (auto& section : sectionNames) {
            positions.add({ section, contents.indexOf(section + ':') });
        }

        std::sort(positions.begin(), positions.end(), [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

        std::map<String, std::pair<String, int>> sections;

        int i = 0;
        for (auto it = positions.begin(); it < positions.end(); it++) {
            auto& [name, currentPosition] = *it;
            if (currentPosition < 0)
                continue; // section not found

            currentPosition += name.length();

            String sectionContent;
            if (it == positions.end() - 1) {
                sectionContent = contents.substring(currentPosition);
            } else {
                sectionContent = contents.substring(currentPosition + 1, (*(it + 1)).second);
            }

            sections[name.trim()] = { sectionContent.substring(1).trim().unquoted(), i };
            i++;
        }

        return sections;
    };

    auto formatText = [](String text) {
        text = text.trim();
        // Start sentences with uppercase
        if (text.length() > 1 && (*text.toRawUTF8()) >= 'a' && (*text.toRawUTF8()) <= 'z') {
            text = text.replaceSection(0, 1, text.substring(0, 1).toUpperCase());
        }

        // Don't end with a period
        if (text.getLastCharacter() == '.') {
            text = text.upToLastOccurrenceOf(".", false, false);
        }

        return text;
    };

    auto sectionsFromHyphens = [](String text) {
        StringArray lines = StringArray::fromLines(text);

        int lastIdx = 0;
        for (int i = 0; i < lines.size(); i++) {
            auto& line = lines.getReference(i);
            auto& lastLine = lines.getReference(lastIdx);

            if (line.trim().startsWith("-")) {
                line = line.fromFirstOccurrenceOf("-", false, false);
                lastIdx = i;
            } else {
                lastLine += line;
                line.clear();
            }
        }

        lines.removeEmptyStrings();

        return lines;
    };

    auto parseFile = [this, getSections, formatText, sectionsFromHyphens](File& f) {
        String contents = f.loadFileAsString();
        auto sections = getSections(contents, { "\ntitle", "\ndescription", "\npdcategory", "\ncategories", "\nflags", "\narguments", "\nlast_update", "\ninlets", "\noutlets", "\ndraft", "\nsee_also", "\nmethods" });

        if (!sections.count("title"))
            return;

        // Allow multiple names separated by a comma
        for (auto& name : StringArray::fromTokens(sections["title"].first, ",", "")) {

            name = name.trim();

            if (sections.count("description")) {
                objectDescriptions[name] = sections["description"].first;
            }
            if (sections.count("methods")) {

                Methods methodList;
                for (auto& method : sectionsFromHyphens(sections["methods"].first)) {
                    auto sectionMap = getSections(method, { "type", "description" });
                    methodList.push_back({ sectionMap["type"].first, sectionMap["description"].first });
                }

                methods[name] = methodList;
            }

            if (sections.count("pdcategory")) {
                auto categories = sections["pdcategory"].first;
                if (categories.isEmpty())
                    categories = "Unknown";
                for (auto category : StringArray::fromTokens(categories, ",", "")) {
                    objectCategories[category.trim()].add(name);
                }
            }

            if (sections.count("arguments") || sections.count("flags")) {
                Arguments args;

                for (auto& argument : sectionsFromHyphens(sections["arguments"].first)) {
                    auto sectionMap = getSections(argument, { "type", "description", "default" });
                    args.push_back({ sectionMap["type"].first, sectionMap["description"].first, sectionMap["default"].first });
                }

                for (auto& flag : sectionsFromHyphens(sections["flags"].first)) {
                    auto sectionMap = getSections(flag, { "name", "description" });
                    args.push_back({ sectionMap["name"].first, sectionMap["description"].first, "" });
                }

                arguments[name] = args;
            }

            auto numbers = { "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "nth" };
            if (sections.count("inlets")) {
                auto section = getSections(sections["inlets"].first, numbers);
                ioletDescriptions[name][0].resize(static_cast<int>(section.size()));
                for (auto [number, content] : section) {
                    String tooltip;
                    for (auto& argument : sectionsFromHyphens(content.first)) {
                        auto sectionMap = getSections(argument, { "type", "description" });
                        if (sectionMap["type"].first.isEmpty())
                            continue;

                        tooltip += "(" + sectionMap["type"].first + ") " + sectionMap["description"].first + "\n";
                    }

                    ioletDescriptions[name][0].getReference(content.second) = { tooltip, number == "nth" };
                }
            }
            if (sections.count("outlets")) {
                auto section = getSections(sections["outlets"].first, numbers);
                ioletDescriptions[name][1].resize(static_cast<int>(section.size()));
                for (auto [number, content] : section) {
                    String tooltip;

                    for (auto& argument : sectionsFromHyphens(content.first)) {
                        auto sectionMap = getSections(argument, { "type", "description" });
                        if (sectionMap["type"].first.isEmpty())
                            continue;
                        tooltip += "(" + sectionMap["type"].first + ") " + sectionMap["description"].first + "\n";
                    }

                    ioletDescriptions[name][1].getReference(content.second) = { tooltip, number == "nth" };
                }
            }
        }
    };

    for (auto& iter : RangedDirectoryIterator(path, true)) {
        auto file = iter.getFile();

        if (file.hasFileExtension("md")) {
            parseFile(file);
        }
    }

    for (auto& [category, objects] : objectCategories) {
        objects.removeDuplicates(true);
    }
}

Suggestions Library::autocomplete(String query) const
{
    Suggestions result;
    if (searchTree)
        searchTree->autocomplete(std::move(query), result);

    return result;
}

void Library::getExtraSuggestions(int currentNumSuggestions, String query, std::function<void(Suggestions)> callback)
{
    int const maxSuggestions = 20;
    if (currentNumSuggestions > maxSuggestions)
        return;

    libraryUpdateThread.addJob([this, callback, currentNumSuggestions, query]() mutable {
        // if(!libraryLock.try_lock()) {
        //    return;
        //}

        Suggestions result;

        Suggestions matches;
        for (const auto& object : allObjects) {
            // Whitespace is not supported by our autocompletion, because normally it indicates the start of the arguments
            if (object.contains(" "))
                continue;

            if (object.contains(query)) {
                matches.add(object);
            }
        }

        matches.sort(true);
        result.addArray(matches);
        matches.clear();

        if (currentNumSuggestions + result.size() < maxSuggestions) {
            for (const auto& [object, keywords] : objectKeywords) {
                if (object.contains(" "))
                    continue;
                for (const auto& keyword : keywords) {
                    if (keyword.contains(query)) {
                        matches.add(object);
                    }
                }
            }
        }

        matches.sort(true);
        result.addArray(matches);
        matches.clear();

        if (currentNumSuggestions + result.size() < maxSuggestions) {
            for (const auto& [object, description] : objectDescriptions) {
                if (object.contains(" "))
                    continue;
                if (description.contains(query)) {
                    matches.add(object);
                }
            }
        }

        matches.sort(true);
        result.addArray(matches);
        matches.clear();

        if (currentNumSuggestions + result.size() > maxSuggestions) {
            for (auto& [object, iolets] : ioletDescriptions) {
                if (object.contains(" "))
                    continue;
                for (int type = 0; type < 2; type++) {
                    auto descriptions = iolets[type];
                    for (auto& [description, type] : descriptions) {
                        if (description.contains(query)) {
                            matches.add(object);
                        }
                    }
                }
            }
        }

        matches.sort(true);
        result.addArray(matches);
        matches.clear();

        // libraryLock.unlock();

        MessageManager::callAsync([callback, result]() {
            callback(result);
        });
    });
}

String Library::getObjectTooltip(String const& type)
{
    if (libraryLock.try_lock()) {
        return objectDescriptions[type];
        libraryLock.unlock();
    }

    return "";
}

std::array<StringArray, 2> Library::getIoletTooltips(String type, String name, int numIn, int numOut)
{
    auto args = StringArray::fromTokens(name.fromFirstOccurrenceOf(" ", false, false), true);

    IODescriptionMap const* map = nullptr;
    if (libraryLock.try_lock()) {
        map = &ioletDescriptions;
        libraryLock.unlock();
    }

    auto result = std::array<StringArray, 2>();

    if (!map) {
        return result;
    }

    // TODO: replace with map.contains once all compilers support this!
    if (map->count(type)) {
        auto const& ioletDescriptions = map->at(type);

        for (int type = 0; type < 2; type++) {
            int total = type ? numOut : numIn;
            auto descriptions = ioletDescriptions[type];
            // if the amount of inlets is not equal to the amount in the spec, look for repeating iolets
            if (descriptions.size() < total) {
                for (int i = 0; i < descriptions.size(); i++) {
                    if (descriptions[i].second) { // repeating inlet found
                        for (int j = 0; j < (total - descriptions.size()) + 1; j++) {

                            auto description = descriptions[i].first;
                            description = description.replace("$mth", String(j));
                            description = description.replace("$nth", String(j + 1));

                            if (isPositiveAndBelow(j, args.size())) {
                                description = description.replace("$arg", args[j]);
                            }

                            result[type].add(description);
                        }
                    } else {
                        result[type].add(descriptions[i].first);
                    }
                }
            } else {
                for (int i = 0; i < descriptions.size(); i++) {
                    result[type].add(descriptions[i].first);
                }
            }
        }
    }

    return result;
}

StringArray Library::getAllObjects()
{
    return allObjects;
}

void Library::fsChangeCallback()
{
    appDirChanged();
}

File Library::findHelpfile(t_object* obj, File parentPatchFile)
{
    String helpName;
    String helpDir;
    
    auto* pdclass = pd_class(reinterpret_cast<t_pd*>(obj));

    if (pdclass == canvas_class && canvas_isabstraction(reinterpret_cast<t_canvas*>(obj))) {
        char namebuf[MAXPDSTRING];
        t_object* ob = obj;
        int ac = binbuf_getnatom(ob->te_binbuf);
        t_atom* av = binbuf_getvec(ob->te_binbuf);
        if (ac < 1)
            return File();

        atom_string(av, namebuf, MAXPDSTRING);
        helpName = String::fromUTF8(namebuf).fromLastOccurrenceOf("/", false, false);
    } else {
        helpDir = class_gethelpdir(pdclass);
        helpName = class_gethelpname(pdclass);
    }

    auto patchHelpPaths = helpPaths;

    // Add abstraction dir to search paths
    if (pd_class(reinterpret_cast<t_pd*>(obj)) == canvas_class && canvas_isabstraction(reinterpret_cast<t_canvas*>(obj))) {
        auto* cnv = reinterpret_cast<t_canvas*>(obj);
        patchHelpPaths.add(File(String::fromUTF8(canvas_getenv(cnv)->ce_dir->s_name)));
    }

    // Add parent patch dir to search paths
    if (parentPatchFile.existsAsFile()) {
        patchHelpPaths.add(parentPatchFile.getParentDirectory());
    }
    
    patchHelpPaths.add(helpDir);

    String firstName = helpName + "-help.pd";
    String secondName = "help-" + helpName + ".pd";

    auto findHelpPatch = [&firstName, &secondName](File const& searchDir, bool recursive) -> File {
        for (const auto& fileIter : RangedDirectoryIterator(searchDir, recursive)) {
            auto file = fileIter.getFile();
            if (file.getFileName() == firstName || file.getFileName() == secondName) {
                return file;
            }
        }
        return File();
    };

    for (auto& path : patchHelpPaths) {
        auto file = findHelpPatch(path, true);
        if (file.existsAsFile()) {
            return file;
        }
    }

    auto* helpdir = class_gethelpdir(pd_class(&reinterpret_cast<t_gobj*>(obj)->g_pd));

    // Search for files int the patch directory
    auto file = findHelpPatch(String::fromUTF8(helpdir), true);
    if (file.existsAsFile()) {
        return file;
    }

    return File();
}

ObjectMap Library::getObjectDescriptions()
{
    if (libraryLock.try_lock()) {
        auto descriptions = objectDescriptions;
        libraryLock.unlock();
        return descriptions;
    }
    return {};
}
KeywordMap Library::getObjectKeywords()
{
    if (libraryLock.try_lock()) {
        auto keywords = objectKeywords;
        libraryLock.unlock();
        return keywords;
    }
    return {};
}
CategoryMap Library::getObjectCategories()
{
    if (libraryLock.try_lock()) {
        auto categories = objectCategories;
        libraryLock.unlock();
        return categories;
    }
    return {};
}
IODescriptionMap Library::getIoletDescriptions()
{
    if (libraryLock.try_lock()) {
        auto descriptions = ioletDescriptions;
        libraryLock.unlock();
        return descriptions;
    }

    return {};
}

ArgumentMap Library::getArguments()
{
    if (libraryLock.try_lock()) {
        auto args = arguments;
        libraryLock.unlock();
        return args;
    }
    return {};
}

MethodMap Library::getMethods()
{
    if (libraryLock.try_lock()) {
        auto m = methods;
        libraryLock.unlock();
        return m;
    }
    return {};
}

} // namespace pd
