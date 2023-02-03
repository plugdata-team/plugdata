/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

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
