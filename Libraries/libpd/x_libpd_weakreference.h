/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once


#include <m_pd.h>

#ifdef __cplusplus

#include <functional>
#include <vector>
#include <atomic>

extern "C"
{
#endif

using pd_weak_reference = std::atomic<bool>;

extern void clear_weak_references(t_pd* ptr);
extern void register_weak_reference(t_pd* ptr, pd_weak_reference* ref);
extern void unregister_weak_reference(t_pd* ptr, pd_weak_reference* ref);

#ifdef __cplusplus
}

namespace pd
{

struct WeakReference
{
  
    WeakReference(void* p) : WeakReference(static_cast<t_pd*>(p))
    {
    }
    
    WeakReference(t_pd* p) : ptr(p)
    {
        register_weak_reference(ptr, &weakRef);
    }
    
    ~WeakReference()
    {
        unregister_weak_reference(ptr, &weakRef);
    }
    
    template<typename T>
    struct Ptr
    {
        
        Ptr(T* pointer, const pd_weak_reference& ref) : weakRef(ref), ptr(pointer)
        {
            sys_lock();
        }
        
        ~Ptr()
        {
            sys_unlock();
        }
        
        operator bool() const {
            return weakRef;
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
    
        T* operator->() {
            return ptr;
        }
        
        const pd_weak_reference& weakRef;
        T* ptr;
    };
    
    template<typename T>
    Ptr<T> get() const
    {
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
    
private:
    t_pd* ptr;
    pd_weak_reference weakRef = true;
};

}

#endif


