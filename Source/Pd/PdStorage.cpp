/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdStorage.h"
#include "PdInstance.h"
#include "PdPatch.h"

extern "C"
{
#include <g_canvas.h>
#include <m_imp.h>
#include <m_pd.h>

#include "g_undo.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_multi.h"

    extern t_class* text_class;

    void canvas_map(t_canvas* x, t_floatarg f);
}

namespace pd
{

Storage::Storage(t_glist* patch, Instance* inst) : parentPatch(patch), instance(inst)
{
    instance->getCallbackLock()->enter();

    for(t_gobj* y = patch->gl_list; y; y = y->g_next)
    {
        const String name = libpd_get_object_class_name(y);

        if(name == "graph" || name == "canvas")
        {
            auto* glist = pd_checkglist(&y->g_pd);
            auto* obj = glist->gl_list;

            if(obj != nullptr && obj->g_next == nullptr)
            {
                // Skip non-text object to prevent crash on libpd_get_object_text
                if(pd_class(&glist->gl_list->g_pd) != text_class)
                    continue;

                // Get object text to return the content of the comment
                char* text;
                int size;

                libpd_get_object_text(glist->gl_list, &text, &size);

                String name = String::fromUTF8(text, size);
                freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

                // Found an existing storage object!
                if(name.startsWith("plugdatainfo"))
                {
                    infoObject = glist->gl_list;
                    infoParent = glist;
                    instance->getCallbackLock()->exit();
                    loadInfoFromPatch(); // load info from existsing object
                    return;
                }
            }
        }
    }

    // If we're here, no object was found, so we create a new one

    // Ensures no undoable action is created, so there's no way to accidentally delete this object
    canvas_undo_get(glist_getcanvas(parentPatch))->u_doing = 1;

    infoParent = pd_checkglist(libpd_creategraphonparent(patch, 0, 0));

    canvas_undo_get(glist_getcanvas(parentPatch))->u_doing = 0;

    // Makes it nearly invisible in pd-vanilla, in PlugData it's hidden
    infoParent->gl_pixwidth = 1;
    infoParent->gl_pixheight = 1;

    // Create storage object
    int argc = 3;
    auto argv = std::vector<t_atom>(argc);
    SETFLOAT(argv.data(), 0);
    SETSYMBOL(argv.data() + 1, 0);
    SETSYMBOL(argv.data() + 2, gensym("plugdatainfo"));

    infoObject = &pd_checkobject(libpd_createobj(infoParent, gensym("text"), argc, argv.data()))->te_g;

    instance->getCallbackLock()->exit();
}

// Function to load state tree from existing patch, only called on init
void Storage::loadInfoFromPatch()
{
    if(! infoObject)
        return;

    // Make sure the canvas has a window to ensure correct behaviour
    canvas_setcurrent(infoParent);
    canvas_vis(infoParent, 1);
    canvas_map(infoParent, 1);

    char* text;
    int size = 0;
    libpd_get_object_text(infoObject, &text, &size);

    // copy state tree from patch
    String content = String::fromUTF8(text, size).fromFirstOccurrenceOf("plugdatainfo ", false, false);

    // Set parent to be current again
    canvas_setcurrent(parentPatch);
    canvas_map(infoParent, 0);
    canvas_vis(infoParent, 0);

    try
    {
        auto tree = ValueTree::fromXml(content);

        if(tree.isValid())
        {
            extraInfo = tree;
            return;
        }
    }
    catch(...)
    {
        std::cerr << "error loading state" << std::endl;
    }
}

// Function to store state tree in pd patch
void Storage::storeInfo()
{
    if(! infoObject)
        return;

    String newname = "plugdatainfo " + extraInfo.toXmlString(XmlElement::TextFormat().singleLine());

    // This is likely thread safe because nothing else should access this object
    binbuf_text((reinterpret_cast<t_text*>(infoObject))->te_binbuf, newname.toRawUTF8(), newname.getNumBytesAsUTF8());
}

// Function to change the id of an entry
// This will be called when the position of an object or connection changes,
// to ensure correct indexing
void Storage::setInfoId(const String& oldId, const String& newId)
{
    if(! infoObject)
        return;

    for(auto s : extraInfo)
    {
        if(s.getProperty("ID") == oldId && s.getProperty("Updated") == var(false))
        {
            s.setProperty("ID", newId, nullptr);
            s.setProperty("Updated", true, nullptr); // Updated flag in case we temporarily need conflicting IDs
            return;
        }
    }
}

void Storage::confirmIds()
{
    for(auto s : extraInfo)
        s.setProperty("Updated", false, nullptr);
    storeInfo();
}

// Check if info exists
bool Storage::hasInfo(const String& id) const
{
    auto child = extraInfo.getChildWithProperty("ID", id);
    return child.isValid();
}

// Get info from local state
String Storage::getInfo(const String& id, const String& property) const
{
    return extraInfo.getChildWithProperty("ID", id).getProperty(property).toString();
}

// Set info to local state
// Also pushes the change into pd patch
void Storage::setInfo(const String& id, const String& property, const String& info, bool undoable)
{
    jassert(property != "Updated" && property != "ID");

    auto tree = ValueTree("InfoObj");

    auto existingInfo = extraInfo.getChildWithProperty("ID", id);
    if(existingInfo.isValid())
    {
        tree = existingInfo;
    }

    if(! existingInfo.isValid())
    {
        extraInfo.appendChild(tree, nullptr);
    }

    if(undoable)
        createUndoAction();

    tree.setProperty("ID", id, nullptr);
    tree.setProperty("Updated", false, nullptr);
    tree.setProperty(property, info, &undoManager);

    storeInfo();
}

// Checks if we're at a storage undo event, and applies undo if needed
void Storage::undoIfNeeded()
{
    instance->getCallbackLock()->enter();

    t_undo* udo = canvas_undo_get(parentPatch);

    if(udo && udo->u_last && ! strcmp(udo->u_last->name, "plugdata_undo"))
    {
        undoManager.undo();
    }

    instance->getCallbackLock()->exit();

    storeInfo();
}

// Checks if we're at a storage redo event, and applies redo if needed
void Storage::redoIfNeeded()
{
    instance->getCallbackLock()->enter();

    t_undo* udo = canvas_undo_get(parentPatch);

    if(udo && udo->u_last && ! strcmp(udo->u_last->next->name, "plugdata_undo"))
    {
        undoManager.redo();
    }

    instance->getCallbackLock()->exit();

    storeInfo();
}

// Creates a dummy undoable action in pd and begins a new transaction in out own undo manager
// Called from setInfo
void Storage::createUndoAction()
{
    if(! parentPatch || ! infoParent)
        return;

    undoManager.beginNewTransaction();

    instance->getCallbackLock()->enter();
    // Create dummy undoable action that we can detect by name when calling undo
    canvas_undo_add(parentPatch, UNDO_MOTION, "plugdata_undo", canvas_undo_set_move(parentPatch, 1));

    instance->getCallbackLock()->exit();
}

// Checks if the provided object is the GraphOnParent container for our state (passed as t_gobj)
// We use this to make ignore this object in the GUI
bool Storage::isInfoParent(t_gobj* obj)
{
    const String name = libpd_get_object_class_name(obj);
    if(name == "graph" || name == "canvas")
    {
        auto* glist = pd_checkglist(&obj->g_pd);
        return isInfoParent(glist);
    }

    return false;
}

// Checks if the provided object is the GraphOnParent container for our state (passed as t_glist)
bool Storage::isInfoParent(t_glist* glist)
{
    auto* obj = glist->gl_list;
    // Check if the glist has one object (and not more or less)
    if(obj != nullptr && obj->g_next == nullptr)
    {
        char* text;
        int size;

        if(pd_class(&glist->gl_list->g_pd) != text_class)
            return false;

        libpd_get_object_text(glist->gl_list, &text, &size);

        String name = String(CharPointer_UTF8(text), size);
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

        if(name.startsWith("plugdatainfo"))
        {
            return true;
        }
    }
    return false;
}

} // namespace pd
