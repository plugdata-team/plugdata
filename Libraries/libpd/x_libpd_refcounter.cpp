/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <functional>
#include <unordered_map>
#include <mutex>

#include <m_pd.h>
#include "x_libpd_refcounter.h"

std::mutex pdRefCountLock;
std::unordered_map<t_pd*, int> pdReferenceCount;


void increment_pd_refcount(t_pd* ptr)
{
    pdRefCountLock.lock();
    pdReferenceCount[ptr]++;
    pdRefCountLock.unlock();
}

int decrement_pd_refcount(t_pd* ptr)
{
    pdRefCountLock.lock();
    int shouldBeDeleted = !--pdReferenceCount[ptr];
    pdRefCountLock.unlock();
    
    if(shouldBeDeleted)
    {
        pdReferenceCount.erase(ptr);
    }
    
    return shouldBeDeleted;
}
