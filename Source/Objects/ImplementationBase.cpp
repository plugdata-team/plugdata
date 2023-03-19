/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

extern "C" {

#include <m_pd.h>
#include <x_libpd_extra_utils.h>
}

#include "Utility/HashUtils.h"
#include "Pd/Instance.h"
#include "Pd/Patch.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "Object.h"
#include "ObjectBase.h"

#include "ImplementationBase.h"
#include "ObjectImplementations.h"

// TODO: create function to check if ptr was deleted, by passing the parent patch in the contructor?

ImplementationBase::ImplementationBase(void* obj, PluginProcessor* processor) : pd(processor), ptr(obj)
{
    pd->registerMessageListener(ptr, this);
}

ImplementationBase::~ImplementationBase()
{
    pd->unregisterMessageListener(ptr, this);
}

void ImplementationBase::receiveMessage(String const& symbol, int argc, t_atom* argv)
{
    auto atoms = pd::Atom::fromAtoms(argc, argv);

    MessageManager::callAsync([_this = WeakReference(this), symbol, atoms]() mutable {
        if (!_this)
            return;
        
        _this->receiveObjectMessage(symbol, atoms);
    });
}

Canvas* ImplementationBase::getMainCanvas(void* patchPtr)
{
    if(auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor()))
    {
        for(auto* cnv : editor->canvases)
        {
            if(cnv->patch.getPointer() == patchPtr)
            {
                return cnv;
            }
        }
    }
    
    return nullptr;
}

bool ImplementationBase::hasImplementation(const String& type)
{
    switch (hash(type)) {
        case hash("canvas"):
        case hash("graph"):
        case hash("key"):
        case hash("keyname"):
        case hash("keyup"):
        case hash("canvas.active"):
        case hash("canvas.mouse"):
        case hash("canvas.vis"):
        case hash("canvas.zoom"):
        case hash("canvas.edit"):
            return true;
            

        default:
            return false;
    }
}
ImplementationBase* ImplementationBase::createImplementation(const String& type, void* ptr, PluginProcessor* pd)
{
    switch (hash(type)) {
        case hash("canvas"):
        case hash("graph"):
            return new SubpatchImpl(ptr, pd);
        case hash("canvas.active"):
            return new CanvasActiveObject(ptr, pd);
        case hash("canvas.mouse"):
            return new CanvasMouseObject(ptr, pd);
        case hash("canvas.vis"):
            return new CanvasVisibleObject(ptr, pd);
        case hash("canvas.zoom"):
            return new CanvasZoomObject(ptr, pd);
        case hash("canvas.edit"):
            return new CanvasEditObject(ptr, pd);
        case hash("key"):
            return new KeyObject(ptr, pd, KeyObject::Key);
        case hash("keyname"):
            return new KeyObject(ptr, pd, KeyObject::KeyName);
        case hash("keyup"):
            return new KeyObject(ptr, pd, KeyObject::KeyUp);

        default: break;
    }
    
    return nullptr;
}

void ImplementationBase::openSubpatch(std::unique_ptr<pd::Patch>& subpatch)
{
    if(!subpatch)
    {
        subpatch = std::make_unique<pd::Patch>(ptr, pd, false);
    }

    auto* glist = subpatch->getPointer();

    if (!glist)
        return;

    auto abstraction = canvas_isabstraction(glist);
    File path;

    if (abstraction) {
        path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
    }
    
    auto* newPatch = pd->patches.add(subpatch.get());
    newPatch->setCurrentFile(path);
    
    if(auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor()))
    {
        // Check if subpatch is already opened
        for (auto* cnv : editor->canvases) {
            if (cnv->patch == *subpatch) {
                auto* tabbar = cnv->getTabbar();
                tabbar->setCurrentTabIndex(cnv->getTabIndex());
                return;
            }
        }
        
        auto* newCanvas = editor->canvases.add(new Canvas(editor, *newPatch, nullptr));
        editor->addTab(newCanvas);
    }
}

void ImplementationBase::closeOpenedSubpatchers()
{
    if(auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor())) {
        
        for (auto* canvas : editor->canvases) {
            if (canvas && canvas->patch.getPointer() == ptr) {
                
                canvas->editor->closeTab(canvas);
                break;
            }
        }
    }
}


ObjectImplementationManager::ObjectImplementationManager(pd::Instance* processor) : pd(static_cast<PluginProcessor*>(processor)) {
    
}

void ObjectImplementationManager::updateObjectImplementations()
{
    Array<void*> allImplementations;
    
    pd->setThis();
    
    t_glist* x;
    for (x = pd_getcanvaslist(); x; x = x->gl_next)
    {
        allImplementations.addArray(getImplementationsForPatch(x));
    }
    
    // Remove unused object implementations
    for (auto it = objectImplementations.cbegin(); it != objectImplementations.cend();)
    {
      auto& [ptr, implementation] = *it;
      auto found = std::find(allImplementations.begin(), allImplementations.end(), ptr);
    
      if (found == allImplementations.end())
      {
        objectImplementations.erase(it++);
      }
      else
      {
        it++;
      }
    }
    
    for(auto* ptr : allImplementations)
    {
        if(!objectImplementations.count(ptr)) {
            
            auto const name = String::fromUTF8(libpd_get_object_class_name(ptr));
            
            objectImplementations[ptr] = std::unique_ptr<ImplementationBase>(ImplementationBase::createImplementation(name, ptr, pd));
        }
    }
}

Array<void*> ObjectImplementationManager::getImplementationsForPatch(void* patch)
{
    Array<void*> implementations;
    
    auto* glist = static_cast<t_glist*>(patch);

    for (t_gobj* y = glist->gl_list; y; y = y->g_next) {
        
        auto const name = String::fromUTF8(libpd_get_object_class_name(y));
        
        if (name == "canvas" || name == "graph") {
            implementations.addArray(getImplementationsForPatch(y));
        }
        if(ImplementationBase::hasImplementation(name)) {
            implementations.add(y);
        }
    }
    
    return implementations;
}
