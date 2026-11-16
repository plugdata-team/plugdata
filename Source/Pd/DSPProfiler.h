/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <d_profiler.h>
#include "Utility/Config.h"

namespace pd {

class Instance;

// Measures how much CPU time the perform routines of each DSP object take, while it runs.
// Pd times them in its DSP tick (see dsp_profiler_read), this adds up what it measured
class DSPProfiler final : private Timer {
public:
    struct Entry {
        void* object = nullptr;          // nullptr for the time between objects, like summing signals
        void* highlightTarget = nullptr; // the object, or the clone it's in, since those can't be opened
        String text;
        String type;     // the object's name, or the abstraction's
        String location; // the subpatch, abstraction or clone it's in, or its patch
        double seconds = 0.0;
        double audioSeconds = 0.0; // of audio computed while it was profiled

        // As a share of the time that's available to compute the audio, like the CPU meter
        double getLoad() const { return audioSeconds > 0.0 ? seconds / audioSeconds : 0.0; }
    };

    explicit DSPProfiler(Instance& instance);
    ~DSPProfiler() override;

    // It runs while something uses it, like a Debugger panel that shows it. Starting again forgets what it measured before
    void addUser();
    void removeUser();
    bool isRunning() const { return running; }

    // Sorted by load, highest first
    SmallArray<Entry> const& getEntries() const { return entries; }
    double getTotalLoad() const { return totalLoad; }
    double getProfiledSeconds() const { return profiledSeconds; }
    bool isWaitingForDSP() const { return running && waitingForDSP; }

    // Changes whenever the entries do
    uint32 getRevision() const { return revision; }

private:
    struct Measured : Entry {
        void* objectClass = nullptr; // to tell a new object at the address of a deleted one apart
    };

    void setRunning(bool shouldRun);
    void timerCallback() override;
    void update();
    void matchObjects();

    Instance& pd;
    bool running = false;
    int numUsers = 0;
    bool waitingForDSP = false;
    unsigned int generation = 0;

    SmallArray<t_dspprofilerentry> readEntries;
    SmallArray<Measured> measured; // in the order Pd reads them out

    SmallArray<Entry> entries;
    double totalLoad = 0.0;
    double profiledSeconds = 0.0;
    uint32 revision = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPProfiler)
};

}
