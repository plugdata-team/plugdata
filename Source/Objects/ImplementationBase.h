/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/HashUtils.h"
#include "Pd/Instance.h"
#include "Pd/MessageListener.h"
#include "Constants.h"

class PluginProcessor;
class Canvas;

class ImplementationBase : public pd::MessageListener
{

public:
    ImplementationBase(void* obj, PluginProcessor* pd);
    
    virtual ~ImplementationBase();
    
    static ImplementationBase* createImplementation(const String& type, void* ptr, PluginProcessor* pd);
    static bool hasImplementation(const String& type);
    
    virtual void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) {};
    
    void receiveMessage(String const& symbol, int argc, t_atom* argv) override;
    
    void openSubpatch(std::unique_ptr<pd::Patch>& subpatch);
    void closeOpenedSubpatchers();
    
    Canvas* getMainCanvas(void* patchPtr);
    
    PluginProcessor* pd;
    void* ptr;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(ImplementationBase);
};

class ObjectImplementationManager
{
public:
    ObjectImplementationManager(pd::Instance* pd);
    
    void updateObjectImplementations();
    
private:

    Array<void*> getImplementationsForPatch(void* patch);
    
    PluginProcessor* pd;

    std::map<void*, std::unique_ptr<ImplementationBase>> objectImplementations;
};
