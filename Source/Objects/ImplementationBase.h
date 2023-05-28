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
#include <x_libpd_refcounter.h>

class PluginProcessor;
class Canvas;

class ImplementationBase {

public:
    ImplementationBase(void* obj, PluginProcessor* pd);

    virtual ~ImplementationBase();

    static ImplementationBase* createImplementation(String const& type, void* ptr, PluginProcessor* pd);
    static bool hasImplementation(char const* type);

    virtual void update() {};

    void openSubpatch(pd::Patch* subpatch);
    void closeOpenedSubpatchers();

    Canvas* getMainCanvas(void* patchPtr) const;

    PluginProcessor* pd;
    pd::ReferenceCountedPointer ptr;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ImplementationBase);
};

class ObjectImplementationManager {
public:
    explicit ObjectImplementationManager(pd::Instance* pd);

    void updateObjectImplementations();
    void clearObjectImplementationsForPatch(void* patch);

private:
    Array<void*> getImplementationsForPatch(void* patch);

    PluginProcessor* pd;

    std::map<void*, std::unique_ptr<ImplementationBase>> objectImplementations;
};
