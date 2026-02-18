/*
 // Copyright (c) 2021-2025 Timothy Schoen and Pierre Guillot
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

ImplementationBase::ImplementationBase(t_gobj* obj, t_canvas const* parent, PluginProcessor* processor)
    : pd(processor)
    , ptr(obj, processor)
    , cnv(parent)
{
}

ImplementationBase::~ImplementationBase() = default;

Canvas* ImplementationBase::getMainCanvas(t_canvas const* patchPtr, bool const alsoSearchRoot) const
{
    auto editors = pd->getEditors();

    for (auto* editor : editors) {
        if (editor->pluginMode) {
            if (auto* cnv = editor->pluginMode->getCanvas()) {
                return cnv;
            }
        }
        for (auto* cnv : editor->getCanvases()) {
            if (cnv->patch.getUncheckedPointer() == patchPtr) {
                return cnv;
            }
        }
    }

    if (alsoSearchRoot) {
        pd->lockAudioThread();
        while (patchPtr->gl_owner)
            patchPtr = patchPtr->gl_owner;
        pd->unlockAudioThread();
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
    case hash("canvas.mouse"):
    case hash("canvas.vis"):
    case hash("canvas.edit"):
    case hash("canvas.zoom"):
    case hash("key"):
    case hash("keyname"):
    case hash("keyup"):
    case hash("keycode"):
    case hash("mouse"):
    case hash("mousestate"):
    case hash("mousefilter"):
        return true;
    default:
        return false;
    }
}

ImplementationBase* ImplementationBase::createImplementation(String const& type, t_gobj* ptr, t_canvas const* cnv, PluginProcessor* pd)
{
    switch (hash(type)) {
    case hash("canvas.mouse"):
        return new CanvasMouseObject(ptr, cnv, pd);
    case hash("canvas.vis"):
        return new CanvasVisibleObject(ptr, cnv, pd);
    case hash("canvas.zoom"):
        return new CanvasZoomObject(ptr, cnv, pd);
    case hash("canvas.edit"):
        return new CanvasEditObject(ptr, cnv, pd);
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
        return nullptr;
    }
}

ObjectImplementationManager::ObjectImplementationManager(pd::Instance* processor)
    : pd(static_cast<PluginProcessor*>(processor))
{
}

void ObjectImplementationManager::handleAsyncUpdate()
{
    SmallArray<std::pair<t_canvas const*, t_canvas const*>> allCanvases;
    SmallArray<std::pair<t_canvas const*, t_gobj*>> allImplementations;
    UnorderedSet<t_gobj const*> allObjects;

    pd->setThis();

    pd->lockAudioThread();
    for (auto* topLevelCnv = pd_getcanvaslist(); topLevelCnv; topLevelCnv = topLevelCnv->gl_next) {
        allCanvases.add({ topLevelCnv, topLevelCnv });
        getSubCanvases(topLevelCnv, topLevelCnv, allCanvases);
    }

    for (auto& [top, glist] : allCanvases) {
        for (t_gobj* y = glist->gl_list; y; y = y->g_next) {
            auto const* name = pd::Interface::getObjectClassName(&y->g_pd);
            if (ImplementationBase::hasImplementation(name)) {
                allImplementations.add({ top, y });
                allObjects.insert(y);
            }
        }
    }
    pd->unlockAudioThread();

    for (auto it = objectImplementations.cbegin(); it != objectImplementations.cend();) {
        auto& [ptr, implementation] = *it;

        if (!allObjects.contains(ptr)) {
            it = objectImplementations.erase(it); // Erase and move iterator forward.
        } else {
            ++it;
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

void ObjectImplementationManager::getSubCanvases(t_canvas const* top, t_canvas const* canvas, SmallArray<std::pair<t_canvas const*, t_canvas const*>>& allCanvases)
{
    for (t_gobj* y = canvas->gl_list; y; y = y->g_next) {
        if (pd_class(&y->g_pd) == canvas_class) {
            allCanvases.add({ top, reinterpret_cast<t_glist const*>(y) });
            getSubCanvases(top, reinterpret_cast<t_glist*>(y), allCanvases);
        } else if (pd_class(&y->g_pd) == clone_class) {
            for (int i = 0; i < clone_get_n(y); i++) {
                allCanvases.add({ top, reinterpret_cast<t_glist const*>(y) });
                getSubCanvases(top, clone_get_instance(y, i), allCanvases);
            }
        }
    }
}

void ObjectImplementationManager::clearObjectImplementationsForPatch(t_canvas const* patch)
{
    for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
        if (pd_class(&y->g_pd) == canvas_class) {
            clearObjectImplementationsForPatch(reinterpret_cast<t_canvas*>(y));
        }
        if (pd_class(&y->g_pd) == clone_class) {
            for (int i = 0; i < clone_get_n(y); i++) {
                auto const* clone = clone_get_instance(y, i);
                clearObjectImplementationsForPatch(clone);
            }
        }
        objectImplementations.erase(y);
    }
}
