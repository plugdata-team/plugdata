/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Utility/Config.h"
#include <juce_gui_basics/juce_gui_basics.h>

extern "C"
{
    #include <s_inter.h>
}

#include "WeakReference.h"
#include "Instance.h"

extern "C"
{
    // Called from pd_free to invalidate all the weak references
    void clear_weak_references(t_pd* ptr)
    {
        auto* instance = static_cast<pd::Instance*>(get_plugdata_instance());
        if(instance) instance->clearWeakReferences(ptr);
    }
}


pd::WeakReference::WeakReference(void* p, Instance* instance) : ptr(static_cast<t_pd*>(p)), pd(instance)
{
    pd->registerWeakReference(ptr, &weakRef);
}

pd::WeakReference::~WeakReference()
{
    pd->unregisterWeakReference(ptr, &weakRef);
}

void pd::WeakReference::setThis() const
{
    pd->setThis();
}
