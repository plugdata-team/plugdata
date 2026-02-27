/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/Config.h"
#include <juce_gui_basics/juce_gui_basics.h>

extern "C" {
#include <s_inter.h>
}

#include "WeakReference.h"
#include "Instance.h"

pd::WeakReference::WeakReference(void* p, Instance* instance)
    : ptr(p)
    , pd(instance)
{
    pd->registerWeakReference(ptr, &weakRef);
}

pd::WeakReference::WeakReference(Instance* instance)
    : ptr(nullptr)
    , pd(instance)
{
}

pd::WeakReference::WeakReference(WeakReference const& toCopy)
    : ptr(toCopy.ptr)
    , pd(toCopy.pd)
{
    weakRef = toCopy.weakRef.load();
    pd->registerWeakReference(ptr, &weakRef);
}

pd::WeakReference::~WeakReference()
{
    if (pd)
        pd->unregisterWeakReference(ptr, &weakRef);
}

pd::WeakReference& pd::WeakReference::operator=(pd::WeakReference const& other)
{
    bool const valid = other.ptr && other.pd;
    if (valid && this != &other) // Check for self-assignment
    {
        pd = other.pd;

        pd->weakReferenceLock.enter();
        pd->unregisterWeakReference(ptr, &other.weakRef);

        // Use atomic exchange to safely copy the weakRef value
        weakRef.store(other.weakRef.load());
        ptr = other.ptr;

        pd->registerWeakReference(ptr, &weakRef);
        pd->weakReferenceLock.exit();
    }

    return *this;
}

void pd::WeakReference::setThis() const
{
    if (pd)
        pd->setThis();
}
