/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

// Class that blocks events that are too close together, up to a certain rate
// We use this to reduce the rate at which MouseEvents come in, to improve performance (especially on Linux)
struct RateReducer : public Timer {
    explicit RateReducer(int rate)
        : timerHz(rate)
    {
        ignoreUnused(timerHz);
    }

    bool tooFast()
    {
#if JUCE_LINUX
        if (allowEvent) {
            allowEvent = false;
            startTimerHz(timerHz);
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    void timerCallback() override
    {
        allowEvent = true;
        stopTimer();
    }

private:
    int timerHz;
    bool allowEvent = true;
};

template<typename T, int hz = 90>
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
    RateReducer rateReducer = RateReducer(hz);
};
