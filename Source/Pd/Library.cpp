/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"

#include <BinaryData.h>

#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"
#include "Utility/Decompress.h"

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <s_stuff.h>
#include <z_libpd.h>
}

#include <utility>
#include "Library.h"
#include "Instance.h"
#include "Pd/Interface.h"

struct _canvasenvironment {
    t_symbol* ce_dir;    /* directory patch lives in */
    int ce_argc;         /* number of "$" arguments */
    t_atom* ce_argv;     /* array of "$" arguments */
    int ce_dollarzero;   /* value of "$0" */
    t_namelist* ce_path; /* search path */
};

namespace pd {

Library::Library(pd::Instance* instance)
    : Thread("Library Index Thread")
    , pd(instance)
{
    watcher.addFolder(ProjectInfo::appDataDir);
    watcher.addListener(this);

    // Needs to be async, otherwise LV2 validation fails
    MessageManager::callAsync([this, pd = juce::WeakReference(pd)] {
        if (pd.get()) {
            pd->setThis();
            updateLibrary();
        }
    });

    startThread();
}

Library::~Library()
{
    appDirChanged = nullptr;
    waitForThreadToExit(-1);
}

void Library::updateLibrary()
{
    auto const settingsTree = ValueTree::fromXml(ProjectInfo::appDataDir.getChildFile(".settings").loadFileAsString());
    auto const pathTree = settingsTree.getChildWithName("Paths");

    pd->lockAudioThread();
    pd->setThis();

    // Get available objects directly from pd
    t_class* o = pd_objectmaker;

    auto* mlist = static_cast<t_methodentry*>(libpd_get_class_methods(o));
    t_methodentry* m;

    allObjects.clear();

    int i;
    for (i = o->c_nmethod, m = mlist; i--; m++) {
        if (!m || !m->me_name)
            continue;

        auto newName = String::fromUTF8(m->me_name->s_name);
        if (!(newName.startsWith("else/") || newName.startsWith("cyclone/") || newName.endsWith("_aliased"))) {
            allObjects.add(newName);
        }
    }

    // Find patches in our search tree
    for (auto path : pathTree) {
        auto filePath = path.getProperty("Path").toString();

        auto file = File(filePath);
        if (!file.exists() || !file.isDirectory())
            continue;

        for (auto const& file : OSUtils::iterateDirectory(file, false, true)) {
            if (file.hasFileExtension("pd")) {
                auto filename = file.getFileNameWithoutExtension();
                if (!filename.startsWith("help-") && !filename.endsWith("-help")) {
                    allObjects.add(filename);
                }
            }
        }
    }

    // These can't be created by name in Pd, but plugdata allows it
    allObjects.add("graph");
    allObjects.add("garray");

    // These aren't in there but should be
    allObjects.add("float");
    allObjects.add("symbol");
    allObjects.add("list");

    pd->unlockAudioThread();
}

void Library::run()
{
    HeapArray<uint8_t> decodedDocs;
    decodedDocs.reserve(2 * 1024 * 1024);
    
    Decompress::extractXz((uint8_t const*)BinaryData::Documentation_bin, BinaryData::Documentation_binSize, decodedDocs);
    
    MemoryInputStream instream(decodedDocs.data(), decodedDocs.size(), false);
    ValueTree documentationTree = ValueTree::readFromStream(instream);

    auto weights = HeapArray<float>(2);
    weights[0] = 6.0f; // More weight for name
    weights[1] = 3.0f; // More weight for description
    searchDatabase.setWeights(weights.vector());

    for (auto objectEntry : documentationTree) {
        auto categoriesTree = objectEntry.getChildWithName("categories");

        HeapArray<std::string> fields;
        int numProperties = objectEntry.getNumProperties();
        for (int i = 0; i < numProperties; i++) // Name and description
        {
            auto property = objectEntry.getProperty(objectEntry.getPropertyName(i)).toString();
            fields.add(property.toStdString());
        }
        for (auto subtree : objectEntry) // Parent tree for arguments, inlets, outlets
        {
            for (auto child : subtree) // tree for individual arguments, inlets, outlets, etc.
            {
                for (int i = 0; i < child.getNumProperties(); i++) {
                    auto property = child.getProperty(child.getPropertyName(i)).toString();
                    if (!property.containsOnly("0123456789.,-")) {
                        fields.add(property.toStdString());
                    }
                }
            }
        }

        String origin;
        for (auto category : categoriesTree) {
            auto cat = category.getProperty("name").toString();
            if (objectOrigins.contains(cat)) {
                origin = cat;
            }
        }

        auto name = objectEntry.getProperty("name").toString();

        if (origin == "Gem") {
#if !ENABLE_GEM
            continue;
#else
            gemObjects.add(name);
#endif
        }

        searchDatabase.addEntry(objectEntry, fields.vector());
        searchDatabase.setThreshold(0.4f);

        if (origin.isEmpty()) {
            documentationIndex[hash(name)] = objectEntry;
        } else if (origin == "Gem") {
            documentationIndex[hash(origin + "/" + name)] = objectEntry;
        } else if (origin == "MERDA") {
            documentationIndex[hash("ELSE/" + name)] = objectEntry;
        } else if (documentationIndex.count(hash(name))) {
            documentationIndex[hash(origin + "/" + name)] = objectEntry;
        } else {
            documentationIndex[hash(name)] = objectEntry;
            documentationIndex[hash(origin + "/" + name)] = objectEntry;
        }
    }

    initWait.signal();
}

void Library::waitForInitialisationToFinish()
{
    if (!isInitialised) {
        initWait.wait();
        isInitialised = true;
    }
}

bool Library::isGemObject(String const& query) const
{
    return gemObjects.contains(query);
}

StringArray Library::autocomplete(String const& query, File const& patchDirectory) const
{
    StringArray result;
    result.ensureStorageAllocated(20);

    // First, look for non-help patches in the current patch directory
    if (patchDirectory.isDirectory()) {
        for (auto const& file : OSUtils::iterateDirectory(patchDirectory, false, true, 20)) {
            auto filename = file.getFileNameWithoutExtension();
            if (file.hasFileExtension("pd") && filename.startsWith(query) && !filename.startsWith("help-") && !filename.endsWith("-help")) {
                result.add(filename);
            }
        }
    }

    // Then, go over all regular objects for direct autocompletion
    for (auto const& str : allObjects) {
        if (result.size() >= 20)
            break;

        if (str.startsWith(query)) {
            result.addIfNotAlreadyThere(str);
        }
    }

    result.sort(true);

    // Finally, do a fuzzy search of all object documentation
    auto const fuzzyResults = searchDatabase.search(query.toStdString());
    for (auto& fuzzyMatch : fuzzyResults) {
        if (result.size() >= 20)
            break;

        auto name = fuzzyMatch.key.getProperty("name").toString();
        if (name.isNotEmpty()) {
            result.addIfNotAlreadyThere(name);
        }
    }

    return result;
}

StringArray Library::searchObjectDocumentation(String const& query)
{
    StringArray result;
    result.ensureStorageAllocated(20);

    for (auto const& str : allObjects) {
        if (str.startsWith(query)) {
            result.addIfNotAlreadyThere(str);
        }
    }

    auto const fuzzyResults = searchDatabase.search(query.toStdString());
    result.ensureStorageAllocated(result.size() + fuzzyResults.size());

    for (auto& fuzzyMatch : fuzzyResults) {
        auto name = fuzzyMatch.key.getProperty("name").toString();
        if (name.isNotEmpty()) {
            result.addIfNotAlreadyThere(name);
        }
    }

    return result;
}

ValueTree Library::getObjectInfo(String const& name)
{
    return documentationIndex[hash(name)];
}

StackArray<StringArray, 2> Library::parseIoletTooltips(ValueTree const& iolets, String const& name, int const numIn, int const numOut)
{
    StackArray<StringArray, 2> result;
    SmallArray<std::pair<String, bool>, 8> inlets;
    SmallArray<std::pair<String, bool>, 8> outlets;

    auto const args = StringArray::fromTokens(name.fromFirstOccurrenceOf(" ", false, false), true);

    for (auto iolet : iolets) {
        auto isVariable = iolet.getProperty("variable").toString() == "1";
        auto tooltip = iolet.getProperty("tooltip");
        if (iolet.getType() == Identifier("inlet")) {
            inlets.add({ tooltip, isVariable });
        }

        if (iolet.getType() == Identifier("outlet")) {
            outlets.add({ tooltip, isVariable });
        }
    }

    for (int type = 0; type < 2; type++) {
        int const total = type ? numOut : numIn;
        auto& descriptions = type ? outlets : inlets;
        // if the amount of inlets is not equal to the amount in the spec, look for repeating iolets
        if (descriptions.size() < total) {
            for (int i = 0; i < descriptions.size(); i++) {
                if (descriptions[i].second) { // repeating inlet found
                    for (int j = 0; j < total - descriptions.size() + 1; j++) {

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
            for (auto&& description : descriptions) {
                result[type].add(description.first);
            }
        }
    }

    return result;
}

StringArray Library::getAllObjects()
{
    return allObjects;
}

void Library::filesystemChanged()
{
    updateLibrary();
}

File Library::findPatch(String const& patchToFind)
{
    return findFile(patchToFind + ".pd");
}

File Library::findFile(String const& fileToFind)
{
    auto const pathTree = SettingsFile::getInstance()->getValueTree().getChildWithName("Paths");
    for (auto path : pathTree) {
        auto searchPath = File(path.getProperty("Path").toString());
        auto childFile = searchPath.getChildFile(fileToFind);
        if (OSUtils::isFileFast(childFile.getFullPathName()))
            return childFile;
    }

    return {};
}

String Library::getObjectOrigin(t_gobj* obj)
{
    auto const* pdclass = pd_class(reinterpret_cast<t_pd*>(obj));

    if (pdclass == canvas_class && canvas_isabstraction(reinterpret_cast<t_canvas*>(obj))) {
        auto const* cnv = reinterpret_cast<t_canvas*>(obj);
        auto const parentPath = String::fromUTF8(canvas_getenv(cnv)->ce_dir->s_name);
        for (auto& origin : objectOrigins) {
            if (parentPath.containsIgnoreCase("/" + origin)) {
                return origin;
            }
        }
    }

    if (pdclass->c_externdir) {
        auto const externDir = String::fromUTF8(pdclass->c_externdir->s_name);
        for (auto& origin : objectOrigins) {
            if (externDir.containsIgnoreCase(origin)) {
                return origin;
            }
        }
    }

    return {};
}

File Library::findHelpfile(String const& helpName)
{
    String firstName = helpName + "-help.pd";
    String secondName = "help-" + helpName + ".pd";

    for (auto& path : helpPaths) {
        if (!path.exists())
            continue;

        for (auto const& file : OSUtils::iterateDirectory(path, false, true)) {
            auto pathName = file.getFullPathName().replace("\\", "/").trimCharactersAtEnd("/");
            // Hack to make it find else/cyclone/Gem helpfiles...
            pathName = pathName.replace("/9.else", "/else");
            pathName = pathName.replace("/10.cyclone", "/cyclone");
            pathName = pathName.replace("/14.gem", "/Gem");

            if (pathName.endsWith("/" + firstName) || pathName.endsWith("/" + secondName)) {
                return file;
            }
        }
    }
    
    return {};
}

File Library::findHelpfile(t_gobj* obj, File const& parentPatchFile)
{
    String helpName;
    String helpDir;

    auto const* pdclass = pd_class(reinterpret_cast<t_pd*>(obj));

    if (pdclass == canvas_class && canvas_isabstraction(reinterpret_cast<t_canvas*>(obj))) {
        char namebuf[MAXPDSTRING];
        t_object const* ob = pd::Interface::checkObject(obj);
        int const ac = binbuf_getnatom(ob->te_binbuf);
        t_atom const* av = binbuf_getvec(ob->te_binbuf);
        if (ac < 1)
            return {};

        atom_string(av, namebuf, MAXPDSTRING);
        helpName = String::fromUTF8(namebuf);
    } else {
        helpDir = class_gethelpdir(pdclass);
        helpName = class_gethelpname(pdclass);
        helpName = helpName.upToLastOccurrenceOf(".pd", false, false);
    }

    auto patchHelpPaths = SmallArray<File, 16>();

    // Add abstraction dir to search paths
    if (pdclass == canvas_class && canvas_isabstraction(reinterpret_cast<t_canvas*>(obj))) {
        auto const* cnv = reinterpret_cast<t_canvas*>(obj);
        patchHelpPaths.add(File(String::fromUTF8(canvas_getenv(cnv)->ce_dir->s_name)));
        if (helpDir.isNotEmpty()) {
            patchHelpPaths.add(File(String::fromUTF8(canvas_getenv(cnv)->ce_dir->s_name)).getChildFile(helpDir));
        }
    }

    // Add parent patch dir to search paths
    if (parentPatchFile.existsAsFile()) {
        patchHelpPaths.add(parentPatchFile.getParentDirectory());
        if (helpDir.isNotEmpty()) {
            patchHelpPaths.add(parentPatchFile.getParentDirectory().getChildFile(helpDir));
        }
    }

    for (auto path : helpPaths) {
        patchHelpPaths.add(helpDir.isNotEmpty() ? path.getChildFile(helpDir) : path);
    }

    String firstName = helpName + "-help.pd";
    String secondName = "help-" + helpName + ".pd";

    auto findHelpPatch = [&firstName, &secondName](File const& searchDir) -> File {
        for (auto const& file : OSUtils::iterateDirectory(searchDir, false, true)) {
            auto pathName = file.getFullPathName().replace("\\", "/").trimCharactersAtEnd("/");
            // Hack to make it find else/cyclone/Gem helpfiles...
            pathName = pathName.replace("/9.else", "/else");
            pathName = pathName.replace("/10.cyclone", "/cyclone");
            pathName = pathName.replace("/14.gem", "/Gem");

            if (pathName.endsWith("/" + firstName) || pathName.endsWith("/" + secondName)) {
                return file;
            }
        }
        return {};
    };

    for (auto& path : patchHelpPaths) {
        if (!path.exists())
            continue;

        auto file = findHelpPatch(path);
        if (file.existsAsFile()) {
            return file;
        }
    }

    auto* rawHelpDir = class_gethelpdir(pd_class(&obj->g_pd));
    helpDir = String::fromUTF8(rawHelpDir);

    if (helpDir.isNotEmpty() && File(helpDir).exists()) {
        // Search for files in the patch directory
        auto file = findHelpPatch(helpDir);
        if (file.existsAsFile()) {
            return file;
        }
    }

    return {};
}

} // namespace pd
