/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdStorage.h"
#include "PdPatch.h"
#include "PdInstance.h"
#include "Canvas.h"

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>

#include "g_undo.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_multi.h"
}

namespace pd {

// Function to load state tree from existing patch, only called on init
void Storage::setContent(t_glist* patch, String content)
{
    try {
        auto tree = ValueTree::fromXml(content);
        
        if (tree.isValid()) {
            auto [info, undo] = getInfoForPatch(patch);
            info.copyPropertiesAndChildrenFrom(tree, nullptr);
            
            auto subpatches = std::vector<t_canvas*>();
            for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
                const String name = libpd_get_object_class_name(y);
                if (name == "canvas" || name == "graph") {
                    auto* glist = reinterpret_cast<t_canvas*>(y);
                    if (glist) {
                        // Check if not array
                        t_class* c = glist->gl_list->g_pd;
                        if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                            continue;
                        }
                    }
                    subpatches.push_back(glist);
                }
            }
            
            int subIdx = 0;
            for(int i = 0; i < info.getNumChildren(); i++)
            {
                auto childTree = info.getChild(i);
                if(childTree.getType() == Identifier("Info")) {
                    
                    if(subIdx >= subpatches.size()) {
                        std::cerr << "Connection: consistency check failed" << std::endl;
                        break;
                    }
                    
                    auto [info, undo] = getInfoForPatch(subpatches[subIdx]);
                    info.copyPropertiesAndChildrenFrom(childTree, nullptr);
                    subIdx++;
                }
            }
        }
    } catch (...) {
        std::cerr << "error loading state" << std::endl;
    }
}

ValueTree Storage::getContent(t_glist* patch)
{
    auto allContent = ValueTree("AllInfo");
    auto [info, undo] = getInfoForPatch(patch);
    allContent.copyPropertiesAndChildrenFrom(info, nullptr);
    
    auto subpatches = std::vector<t_canvas*>();
    for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
        const String name = libpd_get_object_class_name(y);
        if (name != "canvas" && name != "graph") continue;
        
        auto* glist = reinterpret_cast<t_canvas*>(y);
        if (glist) {
            // Check if not array
            t_class* c = glist->gl_list->g_pd;
            if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                continue;
            }
        }
        
        auto [subinfo, subundo] = getInfoForPatch(glist);
        allContent.appendChild(subinfo, nullptr);
    }
    
    return allContent;
}

// Get info from local state
String Storage::getInfo(Canvas* cnv, String const& id, String const& property)
{
    auto [info, undo] = getInfoForPatch(cnv->patch.getPointer());
    return info.getChildWithProperty("ID", id).getProperty(property).toString();
}

// Set info to local state
// Also pushes the change into pd patch
void Storage::setInfo(Canvas* cnv, String const& id, String const& property, String const& content, bool undoable)
{
    jassert(property != "Updated" && property != "ID");

    auto tree = ValueTree("InfoObj");
    auto [info, undo] = getInfoForPatch(cnv->patch.getPointer());
    
    auto existingInfo = info.getChildWithProperty("ID", id);
    if (existingInfo.isValid()) {
        tree = existingInfo;
    }

    if (!existingInfo.isValid()) {
        info.appendChild(tree, nullptr);
    }

    if (undoable)
        createUndoAction(cnv);

    tree.setProperty("ID", id, nullptr);
    tree.setProperty("Updated", false, nullptr);
    tree.setProperty(property, content, undo);
}

// Checks if we're at a storage undo event, and applies undo if needed
void Storage::undoIfNeeded(Canvas* cnv)
{
    cnv->pd->getCallbackLock()->enter();

    auto* patch = cnv->patch.getPointer();
    auto [info, undo] = getInfoForPatch(patch);
    
    t_undo* udo = canvas_undo_get(patch);

    if (udo && udo->u_last && !strcmp(udo->u_last->name, "plugdata_undo")) {
        undo->undo();
    }

    cnv->pd->getCallbackLock()->exit();
}

// Checks if we're at a storage redo event, and applies redo if needed
void Storage::redoIfNeeded(Canvas* cnv)
{
    cnv->pd->getCallbackLock()->enter();

    auto* patch = cnv->patch.getPointer();
    auto [info, undo] = getInfoForPatch(patch);
    
    t_undo* udo = canvas_undo_get(patch);

    if (udo && udo->u_last && !strcmp(udo->u_last->next->name, "plugdata_undo")) {
        undo->redo();
    }

    cnv->pd->getCallbackLock()->exit();
}

// Creates a dummy undoable action in pd and begins a new transaction in out own undo manager
// Called from setInfo
void Storage::createUndoAction(Canvas* cnv)
{

    auto* patch = cnv->patch.getPointer();
    
    if (!patch) return;
    
    auto [info, undo] = getInfoForPatch(patch);

    undo->beginNewTransaction();

    cnv->pd->getCallbackLock()->enter();
    // Create dummy undoable action that we can detect by name when calling undo
    canvas_undo_add(patch, UNDO_MOTION, "plugdata_undo", canvas_undo_set_move(patch, 1));

    cnv->pd->getCallbackLock()->exit();
}

std::pair<ValueTree, UndoManager*> Storage::getInfoForPatch(void* patch)
{    
    if(canvasInfo.count(patch)) {
        
        return {std::get<0>(canvasInfo[patch]), std::get<1>(canvasInfo[patch]).get()};
    }
    else {
        canvasInfo[patch] = {ValueTree("Info"), std::make_unique<UndoManager>()};
        return {std::get<0>(canvasInfo[patch]), std::get<1>(canvasInfo[patch]).get()};
    }
}

} // namespace pd
