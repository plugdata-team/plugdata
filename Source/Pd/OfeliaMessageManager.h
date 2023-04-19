/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Instance.h"
#include <concurrentqueue.h>


// This file contains a set of helper functions that ofelia can use to make sure it draws on the message thread
// macOS (and possibly other OS?) require that drawing is done from the message thread
// It's also generally good to send all your window management function from the same thread
// With ofelia in pd-vanilla, it's okay because Pd only has one thread (the GUI runs in a separate process)
// In plugdata though, it's a problem because our main thread is the GUI thread,
// not the audio thread where we run Pd.

// So what we did, is wrap all the calls to Lua inside a MessageManager::callAsync
// Since these will be dequeued in order, there shouldn't be any problems with that
// But whenever ofelia wants to touch pd's variables, we also have to lock the audio thread so that we can safely interact with Pd
// On top of that, not every case can be done asynchronously, for example, if we need to return values.
// So if async is not an option, make sure to use the ofelia_lock/unlock function at least

namespace pd {

struct OfeliaMessageManager : public Timer, public DeletedAtShutdown
{
    void timerCallback() override
    {
        callback();
    }
    
    void setRunLoop(std::function<void()> fn)
    {
        callback = fn;
    }
    
    static OfeliaMessageManager* getOrCreate()
    {
        if(!instance)
        {
            instance = new OfeliaMessageManager();
            instance->startTimerHz(60);
        }
        
        return instance;
    }
    
    static void setAudioCallbackLock(CriticalSection const* lock)
    {
        auto* instance = getOrCreate();
        instance->audioLock = lock;
    }
    
    static inline OfeliaMessageManager* instance = nullptr;
    
    CriticalSection const* audioLock = nullptr;
    std::function<void()> callback = [](){};
    
    moodycamel::ConcurrentQueue<std::function<void()>> asyncQueue;
};

} // namespace pd


void ofelia_audio_lock()
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    
    if(auto* lock = instance->audioLock) {
        lock->enter();
    }
}
void ofelia_audio_unlock()
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    
    if(auto* lock = instance->audioLock) {
        lock->exit();
    }
}

void ofelia_call_async(std::function<void()> fn)
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    
    if(MessageManager::getInstance()->isThisTheMessageThread()) {
        fn();
    }
    else {
        MessageManager::callAsync([fn](){
            fn();
        });
    }
}

// This is a hook that ofelia can use to set up its message loop
void ofelia_set_run_loop(std::function<void()> fn)
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    instance->setRunLoop(fn);
}
