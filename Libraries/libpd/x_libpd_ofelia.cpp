/*
 // Copyright (c) 2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <functional>

extern void ofelia_audio_lock_impl();
extern void ofelia_audio_unlock_impl();
extern void ofelia_call_async_impl(std::function<void()> fn);
extern void ofelia_set_run_loop_impl(std::function<void()> fn);


void ofelia_audio_lock();
void ofelia_audio_unlock();
void ofelia_call_async(std::function<void()> fn);
void ofelia_set_run_loop(std::function<void()> fn);

void ofelia_audio_lock()
{
    ofelia_audio_lock_impl();
}
void ofelia_audio_unlock()
{
    ofelia_audio_unlock_impl();
}

void ofelia_call_async(std::function<void()> fn)
{
    ofelia_call_async_impl(fn);
}

// This is a hook that ofelia can use to set up its message loop
void ofelia_set_run_loop(std::function<void()> fn)
{
    ofelia_set_run_loop_impl(fn);
}
