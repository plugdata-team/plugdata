/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <functional>

// This is a workaround for the issue that you cannot call functions from the main program from inside a DLL on Windows
struct ofxOfeliaCallbacks
{
    std::function<void()> lockFn = [](){};
    std::function<void()> unlockFn = [](){};
    std::function<void(std::function<void()>)> asyncFn = [](std::function<void()>){};
    std::function<void(std::function<void()>)> runloopFn = [](std::function<void()>){};
};

ofxOfeliaCallbacks ofxCallbacks;

void ofelia_set_audio_lock_impl(std::function<void()> fn)
{
    ofxCallbacks.lockFn = fn;
}

void ofelia_set_audio_unlock_impl(std::function<void()> fn)
{
    ofxCallbacks.unlockFn = fn;
}

void ofelia_set_async_impl(std::function<void(std::function<void()>)> fn)
{
    ofxCallbacks.asyncFn = fn;
}

void ofelia_set_run_loop_impl(std::function<void(std::function<void()>)> fn)
{
    ofxCallbacks.runloopFn = fn;
}


void ofelia_audio_lock()
{
    ofxCallbacks.lockFn();
}
void ofelia_audio_unlock()
{
    ofxCallbacks.unlockFn();
}

void ofelia_call_async(std::function<void()> fn)
{
    ofxCallbacks.asyncFn(fn);
}

// This is a hook that ofelia can use to set up its message loop
void ofelia_set_run_loop(std::function<void()> fn)
{
    ofxCallbacks.runloopFn(fn);
}
