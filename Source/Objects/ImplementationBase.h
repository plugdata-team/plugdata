/*
 // Copyright (c) 2021-2025 Timothy Schoen and Pierre Guillot
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

    Canvas* getMainCanvas(t_canvas* patchPtr, bool alsoSearchRoot = false) const;

    PluginProcessor* pd;
    pd::WeakReference ptr;
    t_canvas* cnv;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ImplementationBase)
};

class ObjectImplementationManager final : public AsyncUpdater {
public:
    explicit ObjectImplementationManager(pd::Instance* pd);

    void updateObjectImplementations();
    void clearObjectImplementationsForPatch(t_canvas* patch);

    void handleAsyncUpdate() override;

private:
    static void getSubCanvases(t_canvas* top, t_canvas* patch, SmallArray<std::pair<t_canvas*, t_canvas*>>& allCanvases);

    PluginProcessor* pd;

    UnorderedSegmentedMap<t_gobj*, std::unique_ptr<ImplementationBase>> objectImplementations;
    SmallArray<std::pair<t_canvas*, t_gobj*>> targetImplementations;
    UnorderedSegmentedSet<t_gobj*> targetObjects;
    CriticalSection objectImplementationLock;
};
