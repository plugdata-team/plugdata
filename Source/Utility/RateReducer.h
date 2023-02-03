/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */



#pragma once
#include <JuceHeader.h>

// Class that blocks events that are too close together, up to a certain rate
// We use this to reduce the rate at which MouseEvents come in, to improve performance (especially on Linux)
struct RateReducer : public Timer
{
    RateReducer(int rate) : timerHz(rate) {
    }
    
    bool tooFast() {
        if(allowEvent) {
            allowEvent = false;
            startTimerHz(timerHz);
            return false;
        }
    
        return true;
    }
    
    void timerCallback() override {
        allowEvent = true;
    }
    
    void stop() {
        stopTimer();
        allowEvent = true;
    }
    
private:
     int timerHz;
     bool allowEvent = true;
};
