/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Pd/Instance.h"
#include "Pd/MessageListener.h"
#include "Constants.h"

class PluginProcessor;
class Canvas;

class ImplementationBase {

public:
    ImplementationBase(t_gobj* obj, t_canvas* parent, PluginProcessor* pd);

    virtual ~ImplementationBase();

    static ImplementationBase* createImplementation(String const& type, t_gobj* ptr, t_canvas* cnv, PluginProcessor* pd);
    static bool hasImplementation(char const* type);

    virtual void update() { }

    void openSubpatch(pd::Patch::Ptr subpatch);
    void closeOpenedSubpatchers();

    Canvas* getMainCanvas(t_canvas* patchPtr, bool alsoSearchRoot = false) const;

    PluginProcessor* pd;
    pd::WeakReference ptr;
    t_canvas* cnv;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ImplementationBase)
};

class ObjectImplementationManager : public AsyncUpdater {
public:
    explicit ObjectImplementationManager(pd::Instance* pd);

    void updateObjectImplementations();
    void clearObjectImplementationsForPatch(t_canvas* patch);

    void handleAsyncUpdate() override;

private:
    Array<t_gobj*> getImplementationsForPatch(t_canvas* patch);

    PluginProcessor* pd;

    std::map<t_gobj*, std::unique_ptr<ImplementationBase>> objectImplementations;
};
