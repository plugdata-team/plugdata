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

    WeakReference(WeakReference&& toMove) noexcept;

    ~WeakReference();

    WeakReference& operator=(WeakReference const& other);

    WeakReference& operator=(WeakReference&& other) noexcept;

    bool operator==(WeakReference const& other) const
    {
        return ptr == other.ptr;
    }

    bool operator==(void const* other) const
    {
        return ptr == other;
    }

    bool setThis() const;

    template<typename T>
    struct Ptr {

        Ptr(T* pointer, pd_weak_reference const& ref, bool instanceAlive)
            : weakRef(ref)
            , ptr(instanceAlive ? pointer : nullptr)
            , locked(instanceAlive)
        {
            if (locked)
                sys_lock();
        }

        ~Ptr()
        {
            if (locked)
                sys_unlock();
        }

        explicit operator bool() const
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
        bool locked;

        JUCE_DECLARE_NON_COPYABLE(Ptr)
    };

    template<typename T>
    Ptr<T> get() const
    {
        return Ptr<T>(static_cast<T*>(ptr), weakRef, setThis());
    }

    template<typename T>
    T* getRaw() const
    {
        return isValid() ? static_cast<T*>(ptr) : nullptr;
    }

    template<typename T>
    T* getRawUnchecked() const
    {
        return static_cast<T*>(ptr);
    }

    bool isValid() const noexcept
    {
        return pd != nullptr && weakRef && ptr != nullptr;
    }

    bool isDeleted() const noexcept
    {
        return pd == nullptr || !weakRef;
    }

private:
    void* ptr;
    juce::WeakReference<Instance> pd;
    pd_weak_reference weakRef = true;
};

}
