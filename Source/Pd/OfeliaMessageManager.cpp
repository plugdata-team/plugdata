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
// Most OS require that drawing and window management is done from the message thread

// With ofelia in pd-vanilla, it's okay because Pd only has one thread (the GUI runs in a separate process)
// In plugdata though, it's a problem because our main thread is the GUI thread,
// not the audio thread where we run Pd.

// So what we did, is wrap all the calls to Lua inside a MessageManager::callAsync
// Since these will be dequeued in order, there shouldn't be any problems with that
// But whenever ofelia wants to touch pd's variables, we also have to lock the audio thread so that we can safely interact with Pd
// On top of that, not every case can be done asynchronously, for example, if we need to return values.
// So if async is not an option, make sure to use the ofelia_lock/unlock function at least



extern void ofelia_set_audio_lock_impl(std::function<void()> fn);
extern void ofelia_set_audio_unlock_impl(std::function<void()> fn);
extern void ofelia_set_async_impl(std::function<void(std::function<void()>)> fn);
extern void ofelia_set_run_loop_impl(std::function<void(std::function<void()>)> fn);

namespace pd {

void OfeliaMessageManager::create()
{
    instance = new OfeliaMessageManager();
    
    ofelia_set_audio_lock_impl([]() {
            if(audioLock) audioLock->enter();
        });

    ofelia_set_audio_unlock_impl([]() {
            if(audioLock) audioLock->exit();
        });
    
    ofelia_set_async_impl([](std::function<void()> fn) {
        
        if (MessageManager::getInstance()->isThisTheMessageThread()) {
            fn();
        }
        else {
            MessageManager::callAsync([fn]() {
                fn();
            });
        }
    });
    ofelia_set_run_loop_impl([](std::function<void()> fn) {
        instance->runLoop = fn;
    });
}

// This function will run ofelia's event loop
// It returns an int that indicates how long the JUCE run loop can run to maintain 60fps
int OfeliaMessageManager::pollEvents()
{
    if(instance)
    {
        auto startTime = Time::currentTimeMillis();
        
        instance->runLoop();
        
        auto endTime = Time::currentTimeMillis();
        
        // 16 ms is approximately 60 fps, but to hit 60fps, we need to use 15
        // From that, we subtract how long the last drawing took to determine
        // for how long the JUCE message manager can run
        return std::max<int>(1, 16 - (endTime - startTime));
    }
    
    // Try to reduce overhead by letting the JUCE message manager make longer loops when ofelia is inactive
    return 800;
}

void OfeliaMessageManager::setAudioCallbackLock(CriticalSection const* lock)
{
    audioLock = lock;
}

CriticalSection const* OfeliaMessageManager::audioLock = nullptr;
OfeliaMessageManager* OfeliaMessageManager::instance = nullptr;

} // namespace pd




