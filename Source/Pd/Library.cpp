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
        if (!(newName.startsWith("else/") || newName.startsWith("cyclone/") || newName.endsWith("_aliased") || newName.endsWith(":gfx"))) {
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
            if (file.hasFileExtension("pd") || file.hasFileExtension("pd_lua")) {
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

Library::ObjectReferenceTable Library::parseObjectEntry(ValueTree const& objectEntry)
{
    ObjectReferenceTable table;

    auto parseReferenceItem = [](ValueTree const& node) -> ObjectReferenceTable::ReferenceItem {
        return {
            node.getProperty("type").toString(),
            node.getProperty("description").toString()
        };
    };

    table.title = objectEntry.getProperty("name");
    table.description = objectEntry.getProperty("description");

    auto parseIolet = [&](ValueTree const& node) -> ObjectReferenceTable::IoletReference {
        ObjectReferenceTable::IoletReference iolet;
        iolet.tooltip  = node.getProperty("tooltip").toString();
        iolet.variable = static_cast<bool>(node.getProperty("variable"));

        for (int i = 0; i < node.getNumChildren(); ++i)
        {
            auto msg = node.getChild(i);
            if (msg.getType() == Identifier("message"))
                iolet.messages.add(parseReferenceItem(msg));
        }
        return iolet;
    };

    for (int i = 0; i < objectEntry.getNumChildren(); ++i)
    {
        auto section = objectEntry.getChild(i);
        auto sectionType = section.getType();

        if (sectionType == Identifier("iolets"))
        {
            for (int j = 0; j < section.getNumChildren(); ++j)
            {
                auto iolet = section.getChild(j);
                auto ioletType = iolet.getType();

                if (ioletType == Identifier("inlet"))  table.inlets.add(parseIolet(iolet));
                else if (ioletType == Identifier("outlet")) table.outlets.add(parseIolet(iolet));
            }
        }
        else if (sectionType == Identifier("categories"))
        {
            for (int j = 0; j < section.getNumChildren(); ++j)
            {
                auto category = section.getChild(j);
                table.categories.add(category.getProperty("name").toString());
            }
        }
        else if (sectionType == Identifier("arguments"))
        {
            for (int j = 0; j < section.getNumChildren(); ++j)
                table.arguments.add(parseReferenceItem(section.getChild(j)));
        }
        else if (sectionType == Identifier("methods"))
        {
            for (int j = 0; j < section.getNumChildren(); ++j)
                table.methods.add(parseReferenceItem(section.getChild(j)));
        }
        else if (sectionType == Identifier("flags"))
        {
            for (int j = 0; j < section.getNumChildren(); ++j)
            {
                auto flag = section.getChild(j);
                table.flags.add({
                    flag.getProperty("name").toString(),
                    flag.getProperty("description").toString()
                });
            }
        }
    }

    return table;
}

void Library::run()
{
    HeapArray<uint8_t> decodedDocs;
    decodedDocs.reserve(2 * 1024 * 1024);
    {
        auto documentation = BinaryData::getResource(BinaryData::Documentation_bin);
        Decompress::extractXz((uint8_t*)documentation.data(), documentation.size(), decodedDocs);
    }

    MemoryInputStream stream(decodedDocs.data(), decodedDocs.size(), false);

    while(!stream.isExhausted()) {
        ObjectReferenceTable table;
        table.title       = stream.readString();
        table.description = stream.readString();
        int numCategories = stream.readInt();
        for (int i = 0; i < numCategories; ++i)
            table.categories.add(stream.readString());

        for (auto* iolets : { &table.inlets, &table.outlets })
        {
            int count = stream.readInt();
            for (int i = 0; i < count; ++i) {
                ObjectReferenceTable::IoletReference iolet;
                iolet.tooltip  = stream.readString();
                iolet.variable = stream.readBool();
                int count = stream.readInt();
                for (int i = 0; i < count; ++i)
                    iolet.messages.add({ stream.readString(), stream.readString() });
                iolets->add(iolet);
            }
        }
        for (auto* items : { &table.arguments, &table.methods, &table.flags })
        {
            int count = stream.readInt();
            for (int i = 0; i < count; ++i)
                items->add({ stream.readString(), stream.readString() });
        }
        documentation.add(table);;
    }

    auto weights = HeapArray<float>(2);
    weights[0] = 6.0f; // More weight for name
    weights[1] = 3.0f; // More weight for description
    searchDatabase.setWeights(weights.vector());

    for(auto& doc : documentation)
    {
        HeapArray<std::string> fields;
        fields.add(doc.title.toStdString());
        fields.add(doc.description.toStdString());
        for(auto& str : doc.categories)
            fields.add(str.toStdString());
        for(auto& str : doc.inlets)
            fields.add(str.tooltip.toStdString());
        for(auto& str : doc.outlets)
            fields.add(str.tooltip.toStdString());
        for(auto& str : doc.arguments)
            fields.add(str.description.toStdString());
        for(auto& str : doc.flags)
            fields.add(str.description.toStdString());
        for(auto& str : doc.methods)
            fields.add(str.description.toStdString());

        String objectOrigin;
        for (auto origin : objectOrigins) {
            if(doc.categories.contains(origin))
                objectOrigin = origin;
        }

        if (objectOrigin == "Gem") {
#if !ENABLE_GEM
            continue;
#else
            gemObjects.add(doc.title);
#endif
        }

        searchDatabase.addEntry(&doc, fields.vector());
        searchDatabase.setThreshold(0.4f);

        if (objectOrigin.isEmpty()) {
            documentationIndex[hash(doc.title)] = &doc;
        } else if (objectOrigin == "Gem") {
            documentationIndex[hash(objectOrigin + "/" + doc.title)] = &doc;
        } else if (objectOrigin == "MERDA") {
            documentationIndex[hash("ELSE/" + doc.title)] = &doc;
        } else if (documentationIndex.count(hash(doc.title))) {
            documentationIndex[hash(objectOrigin + "/" + doc.title)] = &doc;
        } else {
            documentationIndex[hash(doc.title)] = &doc;
            documentationIndex[hash(objectOrigin + "/" + doc.title)] = &doc;
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
            if ((file.hasFileExtension("pd") || file.hasFileExtension("pd_lua")) && filename.startsWith(query) && !filename.startsWith("help-") && !filename.endsWith("-help")) {
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

        auto const& name = fuzzyMatch.key->title;
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
        auto const& name = fuzzyMatch.key->title;
        if (name.isNotEmpty()) {
            result.addIfNotAlreadyThere(name);
        }
    }

    return result;
}

Library::ObjectReferenceTable const& Library::getObjectInfo(String const& name)
{
    static Library::ObjectReferenceTable emptyObject = {};

    auto hashName = hash(name);
    if(!documentationIndex.contains(hashName))
        return emptyObject;

    return *documentationIndex[hashName];
}

StackArray<StringArray, 2> Library::parseIoletTooltips(ObjectReferenceTable::IoletsReference const& inlets, ObjectReferenceTable::IoletsReference const& outlets, String const& name, int const numIn, int const numOut)
{
    StackArray<StringArray, 2> result;

    auto const args = StringArray::fromTokens(name.fromFirstOccurrenceOf(" ", false, false), true);
    for (int type = 0; type < 2; type++) {
        int const total = type ? numOut : numIn;
        auto& descriptions = type ? outlets : inlets;
        
        // if the amount of inlets is not equal to the amount in the spec, look for repeating iolets
        if (descriptions.size() < total) {
            for (int i = 0; i < descriptions.size(); i++) {
                if (descriptions[i].variable) {
                    for (int j = 0; j < total - descriptions.size() + 1; j++) {
                        auto description = descriptions[i].tooltip;
                        description = description.replace("$mth", String(j));
                        description = description.replace("$nth", String(j + 1));

                        if (isPositiveAndBelow(j, args.size())) {
                            description = description.replace("$arg", args[j]);
                        }

                        result[type].add(description);
                    }
                } else {
                    result[type].add(descriptions[i].tooltip);
                }
            }
        } else {
            for (auto&& description : descriptions) {
                result[type].add(description.tooltip);
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
    String const firstName = helpName + "-help.pd";
    String const secondName = "help-" + helpName + ".pd";

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
