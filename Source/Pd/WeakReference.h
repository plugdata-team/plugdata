/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <functional>
#include <mutex>
#include <m_pd.h>

#include "Utility/Containers.h"

using pd_weak_reference = std::atomic<bool>;

namespace pd {

class Instance;
struct WeakReference {
    WeakReference(void* p, Instance* instance);

    WeakReference(Instance* instance);

    WeakReference(WeakReference const& toCopy);

    ~WeakReference();

    WeakReference& operator=(WeakReference const& other);

    bool operator==(WeakReference const& other) const
    {
        return ptr == other.ptr;
    }

    bool operator==(void const* other) const
    {
        return ptr == other;
    }

    void setThis() const;

    template<typename T>
    struct Ptr {

        Ptr(T* pointer, pd_weak_reference const& ref)
            : weakRef(ref)
            , ptr(pointer)
        {
            sys_lock();
        }

        ~Ptr()
        {
            sys_unlock();
        }

        operator bool() const
        {
            return weakRef && ptr != nullptr;
        }

        T* get()
        {
            return weakRef ? ptr : nullptr;
        }

        template<typename C>
        C* cast()
        {
            return weakRef ? reinterpret_cast<C*>(ptr) : nullptr;
        }

        T* operator->()
        {
            return ptr;
        }

        pd_weak_reference const& weakRef;
        T* ptr;

        JUCE_DECLARE_NON_COPYABLE(Ptr)
    };

    template<typename T>
    Ptr<T> get() const
    {
        setThis();
        return Ptr<T>(reinterpret_cast<T*>(ptr), weakRef);
    }

    template<typename T>
    T* getRaw() const
    {
        return weakRef ? reinterpret_cast<T*>(ptr) : nullptr;
    }

    template<typename T>
    T* getRawUnchecked() const
    {
        return reinterpret_cast<T*>(ptr);
    }

    bool isValid() const noexcept
    {
        return weakRef && ptr != nullptr;
    }
    
    bool isDeleted() const noexcept
    {
        return !weakRef;
    }

private:
    void* ptr;
    Instance* pd;
    pd_weak_reference weakRef = true;
};

}
