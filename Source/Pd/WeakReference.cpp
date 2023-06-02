/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <m_pd.h>
#include "WeakReference.h"

extern "C"
{
    // Called from pd_free to invalidate all the weak references
    void clear_weak_references(t_pd* ptr)
    {
        auto* instance = static_cast<pd::Instance*>(get_plugdata_instance());
        
        instance->pdWeakRefLock.lock();
        for(auto* ref : instance->pdWeakReferences[ptr])
        {
            *ref = false;
        }
        instance->pdWeakReferences.erase(ptr);
        instance->pdWeakRefLock.unlock();
    }
}
extern "C"
{
    // Called from pd_free to invalidate all the weak references
    void clear_weak_references(t_pd* ptr)
    {
        
        pdWeakRefLock.lock();
        for(auto* ref : pdWeakReferences[ptr])
        {
            *ref = false;
        }
        pdWeakReferences.erase(ptr);
        pdWeakRefLock.unlock();
    }
}


WeakReference::WeakReference(void* p, Instance* instance) : ptr(static_cast<t_pd*>(p)), pd(instance)
{
    pd->registerWeakReference(&weakRef);
}

WeakReference::~WeakReference()
{
    pd->unregisterWeakReference(&weakRef);
}

void WeakReference::setThis() const
{
    pd->setThis();
}
