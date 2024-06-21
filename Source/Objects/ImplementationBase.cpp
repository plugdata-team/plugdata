/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

extern "C" {

#include <m_pd.h>

t_glist* clone_get_instance(t_gobj*, int);
int clone_get_n(t_gobj*);
}

#include "AllGuis.h"
#include "Pd/Instance.h"
#include "Pd/Patch.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Object.h"
#include "Sidebar/Palettes.h"
#include "ObjectBase.h"

#include "ImplementationBase.h"
#include "ObjectImplementations.h"
#include "Gem.h"

ImplementationBase::ImplementationBase(t_gobj* obj, t_canvas* parent, PluginProcessor* processor)
    : pd(processor)
    , ptr(obj, processor)
    , cnv(parent)
{
    update();
}

ImplementationBase::~ImplementationBase() = default;

Canvas* ImplementationBase::getMainCanvas(t_canvas* patchPtr, bool alsoSearchRoot) const
{
    auto editors = pd->getEditors();

    for (auto* editor : editors) {
        for (auto* cnv : editor->getCanvases()) {
            auto glist = cnv->patch.getPointer();
            if (glist && glist.get() == patchPtr) {
                return cnv;
            }
        }
    }

    if (alsoSearchRoot) {
        patchPtr = glist_getcanvas(patchPtr);
        for (auto* editor : editors) {
            for (auto* cnv : editor->getCanvases()) {
                auto glist = cnv->patch.getPointer();
                if (glist && glist.get() == patchPtr) {
                    return cnv;
                }
            }
        }
    }

    return nullptr;
}

ImplementationBase* ImplementationBase::createImplementation(String const& type, t_gobj* ptr, t_canvas* cnv, PluginProcessor* pd)
{
    switch (hash(type)) {
    case hash("canvas"):
    case hash("graph"):
        return new SubpatchImpl(ptr, cnv, pd);
    case hash("canvas.mouse"):
        return new CanvasMouseObject(ptr, cnv, pd);
    case hash("canvas.vis"):
        return new CanvasVisibleObject(ptr, cnv, pd);
    case hash("canvas.zoom"):
        return new CanvasZoomObject(ptr, cnv, pd);
    case hash("key"):
        return new KeyObject(ptr, cnv, pd, KeyObject::Key);
    case hash("keyname"):
        return new KeyObject(ptr, cnv, pd, KeyObject::KeyName);
    case hash("keyup"):
        return new KeyObject(ptr, cnv, pd, KeyObject::KeyUp);
    case hash("keycode"):
        return new KeycodeObject(ptr, cnv, pd);
    case hash("mouse"):
        return new MouseObject(ptr, cnv, pd);
    case hash("mousestate"):
        return new MouseStateObject(ptr, cnv, pd);
    case hash("mousefilter"):
        return new MouseFilterObject(ptr, cnv, pd);
    case hash("receive"):
    case hash("send"):
        return new ActivityListener(ptr, cnv, pd, ActivityListener::Recursive);
    case hash("inlet"):
    case hash("outlet"):
        return new ActivityListener(ptr, cnv, pd, ActivityListener::Parent);
    default:
        return new ActivityListener(ptr, cnv, pd);
    }
}

void ImplementationBase::openSubpatch(pd::Patch::Ptr subpatch)
{
    if (auto glist = ptr.get<t_glist>()) {
        if (!subpatch) {
            subpatch = new pd::Patch(ptr, pd, false);
        }

        if (canvas_isabstraction(glist.get())) {
            auto path = File(String::fromUTF8(canvas_getdir(glist.get())->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
            subpatch->setCurrentFile(URL(path));
        }
    } else {
        return;
    }

    for (auto* editor : pd->getEditors()) {
        if (!editor->isActiveWindow())
            continue;

        editor->getTabComponent().openPatch(subpatch);
        break;
    }
}

void ImplementationBase::closeOpenedSubpatchers()
{
    auto glist = ptr.get<t_glist>();
    if (!glist)
        return;

    for (auto* editor : pd->getEditors()) {
        for (auto* canvas : editor->getCanvases()) {
            auto canvasPtr = canvas->patch.getPointer();
            if (canvasPtr && canvasPtr.get() == glist.get()) {
                canvas->editor->getTabComponent().closeTab(canvas);
                break;
            }
        }
    }
}

ObjectImplementationManager::ObjectImplementationManager(pd::Instance* processor)
    : pd(static_cast<PluginProcessor*>(processor))
{
}

void ObjectImplementationManager::handleAsyncUpdate()
{
    Array<ObjectImplementationManager::ObjectCanvas> allImplementations;
    Array<t_canvas*> parentPatches;

    pd->setThis();

    visitedCanvases.clear();

    pd->lockAudioThread();
    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
        parentPatches.add(cnv);
        allImplementations.addArray(getImplementationsForPatch(parentPatches));
    }
    pd->unlockAudioThread();

    // Remove unused object implementations
    for (auto it = objectImplementations.cbegin(); it != objectImplementations.cend();) {
        auto& [ptr, implementation] = *it;

        auto found = std::find_if(allImplementations.begin(), allImplementations.end(), [ptr](ObjectCanvas const& toCompare) {
            return toCompare.obj == ptr;
        });

        if (found == allImplementations.end()) {
            objectImplementations.erase(it++);
        } else {
            it++;
        }
    }

    for (auto& objCnvParents : allImplementations) {
        // Ensure the objectImplementation is created if it does not exist
        if (!objectImplementations.count(objCnvParents.obj)) {
            auto const name = String::fromUTF8(pd::Interface::getObjectClassName(&objCnvParents.obj->g_pd));
            objectImplementations[objCnvParents.obj] = std::unique_ptr<ImplementationBase>(ImplementationBase::createImplementation(name, objCnvParents.obj, objCnvParents.parentPatches.getLast(), pd));
        }

        objectImplementations[objCnvParents.obj]->update(objCnvParents.parentPatches);

    }
}

void ObjectImplementationManager::updateObjectImplementations()
{
    triggerAsyncUpdate();
}

Array<ObjectImplementationManager::ObjectCanvas> ObjectImplementationManager::getImplementationsForPatch(Array<t_canvas*>& parentPatches)
{
    Array<ObjectImplementationManager::ObjectCanvas> implementations;

    auto* glist = static_cast<t_glist*>(parentPatches.getLast());
    for (t_gobj* y = glist->gl_list; y; y = y->g_next) {

        if (pd_class(&y->g_pd) == canvas_class) {
            auto canvas = reinterpret_cast<t_canvas *>(y);
            // make sure we haven't visited this canvas before
            if (visitedCanvases.find(canvas) == visitedCanvases.end()) {
                visitedCanvases.insert(canvas);
                auto canvasPatch = Array<t_canvas *>();
                canvasPatch.addArray(parentPatches);
                canvasPatch.add(canvas);
                implementations.addArray(getImplementationsForPatch(canvasPatch));
            }
        }
        if (pd_class(&y->g_pd) == clone_class) {
            for (int i = 0; i < clone_get_n(y); i++) {
                auto clone = Array<t_canvas*>();
                clone.add(clone_get_instance(y, i));
                implementations.addArray(getImplementationsForPatch(clone));
                implementations.add({ &clone.getLast()->gl_obj.te_g, parentPatches });
            }
        }
        implementations.add({ y, parentPatches });
    }

    return implementations;
}

void ObjectImplementationManager::clearObjectImplementationsForPatch(t_canvas* patch)
{
    auto* glist = static_cast<t_glist*>(patch);

    for (t_gobj* y = glist->gl_list; y; y = y->g_next) {
        if (pd_class(&y->g_pd) == canvas_class) {
            clearObjectImplementationsForPatch(reinterpret_cast<t_canvas*>(y));
        }
        if (pd_class(&y->g_pd) == clone_class) {
            for (int i = 0; i < clone_get_n(y); i++) {
                auto* clone = clone_get_instance(y, i);
                clearObjectImplementationsForPatch(clone);
            }
        }
        objectImplementations.erase(y);
    }
}
