/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <functional>
#include <unordered_map>
#include <mutex>

#include <m_pd.h>
#include "x_libpd_weakreference.h"

std::mutex pdWeakRefLock;
std::unordered_map<t_pd*, std::vector<pd_weak_reference*>> pdWeakReferences;


void register_weak_reference(t_pd* ptr, pd_weak_reference* ref)
{
    pdWeakRefLock.lock();
    pdWeakReferences[ptr].push_back(ref);
    pdWeakRefLock.unlock();
}

void unregister_weak_reference(t_pd* ptr, pd_weak_reference* ref)
{
    pdWeakRefLock.lock();
    
    auto& refs = pdWeakReferences[ptr];
    
    auto it = std::find(refs.begin(), refs.end(), ref);

    if (it != refs.end()) {
        pdWeakReferences[ptr].erase(it);
    }
    
    pdWeakRefLock.unlock();
}


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
