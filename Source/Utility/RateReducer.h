/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

struct RateReducer
{
    static bool tooFast(float hz) {

        auto ms = 1000.0f / hz;

        auto currentTime = Time::getCurrentTime().getMillisecondCounter();
        if(currentTime - lastTime > ms) {
            lastTime = currentTime;
            return false;
        }

        return true;
    }

    static inline uint64_t lastTime = 0;
};

