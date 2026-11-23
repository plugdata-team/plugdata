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
    if (pd)
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
    if (auto* instance = pd.get()) {
        ScopedLock const lock(instance->weakReferenceLock);
        weakRef = toCopy.weakRef.load();
        instance->registerWeakReference(ptr, &weakRef);
    } else {
        weakRef = false;
    }
}

pd::WeakReference::WeakReference(WeakReference&& toMove) noexcept
    : ptr(toMove.ptr)
    , pd(std::move(toMove.pd))
{
    if (auto* instance = pd.get()) {
        ScopedLock const lock(instance->weakReferenceLock);
        weakRef = toMove.weakRef.load();
        instance->moveWeakReference(ptr, &toMove.weakRef, &weakRef);
    } else {
        weakRef = false;
    }
}

pd::WeakReference::~WeakReference()
{
    if (pd)
        pd->unregisterWeakReference(ptr, &weakRef);
}

pd::WeakReference& pd::WeakReference::operator=(pd::WeakReference const& other)
{
    if (this == &other)
        return *this;

    // Unregister this flag from its original owner before changing instances.
    if (pd)
        pd->unregisterWeakReference(ptr, &weakRef);

    pd = other.pd;
    ptr = other.ptr;
    if (auto* instance = pd.get()) {
        ScopedLock const lock(instance->weakReferenceLock);
        weakRef = other.weakRef.load();
        instance->registerWeakReference(ptr, &weakRef);
    } else {
        weakRef = false;
    }

    return *this;
}

pd::WeakReference& pd::WeakReference::operator=(pd::WeakReference&& other) noexcept
{
    if (this == &other)
        return *this;

    if (pd)
        pd->unregisterWeakReference(ptr, &weakRef);

    pd = std::move(other.pd);
    other.pd = nullptr; // juce::WeakReference's move assignment swaps
    ptr = other.ptr;
    if (auto* instance = pd.get()) {
        ScopedLock const lock(instance->weakReferenceLock);
        weakRef = other.weakRef.load();
        instance->moveWeakReference(ptr, &other.weakRef, &weakRef);
    } else {
        weakRef = false;
    }

    return *this;
}

bool pd::WeakReference::setThis() const
{
    if (auto* instance = pd.get()) {
        instance->setThis();
        return true;
    }
    return false;
}
