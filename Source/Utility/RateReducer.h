/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

// Class that blocks events that are too close together, up to a certain rate
// We use this to reduce the rate at which MouseEvents come in, to improve performance (especially on Linux)
struct RateReducer {
    RateReducer()
        : lastEventTime(Time::getMillisecondCounter())
    {
    }

    bool tooFast()
    {
        auto now = Time::getMillisecondCounter();
        auto msSinceLastEvent = (lastEventTime >= now) ? now - lastEventTime
                                                           : (std::numeric_limits<uint32>::max() - lastEventTime) + now;
        
        constexpr uint32 minimumEventInterval = 1000 / 60; // 60fps
        
        if (msSinceLastEvent < minimumEventInterval) {
            return true;
        }
        
        lastEventTime = now;

        return false;
    }

private:
    uint32 lastEventTime;
};

template<typename T>
class MouseRateReducedComponent : public T {
public:
    using T::T;

    void mouseDrag(MouseEvent const& e) override
    {
        if (rateReducer.tooFast())
            return;

        T::mouseDrag(e);
    }

private:
    RateReducer rateReducer;
};
