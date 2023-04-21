/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

#include "Instance.h"
#include "OfeliaMessageManager.h"
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

void OfeliaMessageManager::timerCallback()
{
    callback();
}

void OfeliaMessageManager::setRunLoop(std::function<void()> fn)
{
    callback = fn;
}

OfeliaMessageManager* OfeliaMessageManager::getOrCreate()
{
    if(!instance)
    {
        instance = new OfeliaMessageManager();
        instance->startTimerHz(60);
    }
    
    return instance;
}

void OfeliaMessageManager::setAudioCallbackLock(CriticalSection const* lock)
{
    auto* instance = getOrCreate();
    instance->audioLock = lock;
}

OfeliaMessageManager* OfeliaMessageManager::instance = nullptr;
    
} // namespace pd



void ofelia_audio_lock_impl()
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    if(auto* lock = instance->audioLock) {
        lock->enter();
    }
}
void ofelia_audio_unlock_impl()
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    
    if(auto* lock = instance->audioLock) {
        lock->exit();
    }
}

void ofelia_call_async_impl(std::function<void()> fn)
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
void ofelia_set_run_loop_impl(std::function<void()> fn)
{
    auto* instance = pd::OfeliaMessageManager::getOrCreate();
    instance->setRunLoop(fn);
}

