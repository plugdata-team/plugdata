/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <functional>
#include <unordered_map>

#include <m_pd.h>

#ifdef __cplusplus

extern "C"
{
#endif

extern void increment_pd_refcount(t_pd* ptr);
extern int decrement_pd_refcount(t_pd* ptr);

#ifdef __cplusplus
}

namespace pd
{

struct ReferenceCountedPointer
{
    ReferenceCountedPointer(t_pd* pdPtr) : ptr(pdPtr)
    {
        increment_pd_refcount(ptr);
    }
    
    ReferenceCountedPointer(void* pdPtr) : ptr(static_cast<t_pd*>(pdPtr))
    {
        increment_pd_refcount(ptr);
    }
    
    ~ReferenceCountedPointer()
    {
        sys_lock();
        pd_free(ptr);
        sys_unlock();
    }
    
    template<typename T>
    T* get() const
    {
        return reinterpret_cast<T*>(ptr);
    }
    
    // TODO: instead, just stop checking this
    operator bool() const {
            return true;
        }
    
    operator void*() const {
            return (void*)ptr; // Implicitly convert MyType to int
        }
    
    operator t_pd*() const {
            return ptr; // Implicitly convert MyType to int
        }
    
    t_pd* operator->() {
            return ptr;
        }

    t_pd* ptr;
};

}

#endif


