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
#include "PluginMode.h"

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
        if(editor->pluginMode)
        {
            if(auto* cnv = editor->pluginMode->getCanvas())
            {
                return cnv;
            }
        }
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

bool ImplementationBase::hasImplementation(char const* type)
{
    switch (hash(type)) {
    case hash("canvas"):
    case hash("graph"):
    case hash("key"):
    case hash("keyname"):
    case hash("keyup"):
    case hash("keycode"):
    case hash("canvas.mouse"):
    case hash("canvas.vis"):
    case hash("canvas.zoom"):
    case hash("mouse"):
    case hash("mousestate"):
    case hash("mousefilter"):
        return true;

    default:
        return false;
    }
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

    default:
        break;
    }

    return nullptr;
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
    Array<std::pair<t_canvas*, t_gobj*>> allImplementations;

    pd->setThis();

    pd->lockAudioThread();
    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
        for (auto* object : getImplementationsForPatch(cnv)) {
            allImplementations.add({ cnv, object });
        }
    }
    pd->unlockAudioThread();

    // Remove unused object implementations
    for (auto it = objectImplementations.cbegin(); it != objectImplementations.cend();) {
        auto& [ptr, implementation] = *it;

        auto found = std::find_if(allImplementations.begin(), allImplementations.end(), [ptr = ptr](auto const& toCompare) {
            return std::get<1>(toCompare) == ptr;
        });

        if (found == allImplementations.end()) {
            objectImplementations.erase(it++);
        } else {
            it++;
        }
    }

    for (auto& [cnv, obj] : allImplementations) {
        if (!objectImplementations.count(obj)) {

            auto const name = String::fromUTF8(pd::Interface::getObjectClassName(&obj->g_pd));

            objectImplementations[obj] = std::unique_ptr<ImplementationBase>(ImplementationBase::createImplementation(name, obj, cnv, pd));
        }

        objectImplementations[obj]->update();
    }
}

void ObjectImplementationManager::updateObjectImplementations()
{
    triggerAsyncUpdate();
}

Array<t_gobj*> ObjectImplementationManager::getImplementationsForPatch(t_canvas* patch)
{
    Array<t_gobj*> implementations;

    auto* glist = static_cast<t_glist*>(patch);
    for (t_gobj* y = glist->gl_list; y; y = y->g_next) {

        auto const* name = pd::Interface::getObjectClassName(&y->g_pd);

        if (pd_class(&y->g_pd) == canvas_class) {
            implementations.addArray(getImplementationsForPatch(reinterpret_cast<t_canvas*>(y)));
        }
        if (pd_class(&y->g_pd) == clone_class) {
            for (int i = 0; i < clone_get_n(y); i++) {
                auto* clone = clone_get_instance(y, i);
                implementations.addArray(getImplementationsForPatch(clone));
                implementations.add(&clone->gl_obj.te_g);
            }
        }
        if (ImplementationBase::hasImplementation(name)) {
            implementations.add(y);
        }
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
