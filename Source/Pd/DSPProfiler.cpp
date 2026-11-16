/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "DSPProfiler.h"

#include "Instance.h"
#include "Interface.h"

namespace pd {

DSPProfiler::DSPProfiler(Instance& instance)
    : pd(instance)
{
}

DSPProfiler::~DSPProfiler()
{
    if (running) {
        pd.lockAudioThread();
        dsp_profiler_setenabled(0);
        pd.unlockAudioThread();
    }
}

void DSPProfiler::addUser()
{
    if (numUsers++ == 0)
        setRunning(true);
}

void DSPProfiler::removeUser()
{
    jassert(numUsers > 0);
    if (--numUsers == 0)
        setRunning(false);
}

void DSPProfiler::setRunning(bool const shouldRun)
{
    if (shouldRun == running)
        return;

    if (shouldRun) {
        measured.clear();
        entries.clear();
        totalLoad = 0.0;
        profiledSeconds = 0.0;
        generation = 0;
        revision++;
    } else {
        update();
    }

    running = shouldRun;
    pd.lockAudioThread();
    dsp_profiler_setenabled(running);
    pd.unlockAudioThread();

    if (running)
        startTimerHz(4);
    else
        stopTimer();
}

void DSPProfiler::timerCallback()
{
    update();
}

void DSPProfiler::update()
{
    double audioSeconds = 0.0;
    unsigned int newGeneration = 0;

    pd.lockAudioThread();
    readEntries.resize(dsp_profiler_getsize());
    auto const count = dsp_profiler_read(readEntries.data(), readEntries.size(), &audioSeconds, &newGeneration);
    readEntries.resize(count);
    if (count > 0 && (newGeneration != generation || count != measured.size())) {
        generation = newGeneration;
        matchObjects();
    }
    pd.unlockAudioThread();

    // DSP is off: keep showing what was measured while it was on
    waitingForDSP = count == 0;
    if (count == 0 || audioSeconds <= 0.0)
        return;

    for (int i = 0; i < count; i++) {
        auto& entry = measured[i];
        entry.seconds += readEntries[i].e_time;
        entry.audioSeconds += audioSeconds;
    }
    profiledSeconds += audioSeconds;

    entries.clear();
    totalLoad = 0.0;
    for (auto const& entry : measured) {
        entries.add(entry);
        totalLoad += entry.getLoad();
    }
    std::ranges::stable_sort(entries, [](Entry const& a, Entry const& b) {
        return a.getLoad() > b.getLoad();
    });
    revision++;
}

// Pd rebuilt its DSP chain: objects that are still in it keep what was measured for them. Called with the lock held
void DSPProfiler::matchObjects()
{
    UnorderedMap<void*, int> previous;
    for (int i = 0; i < measured.size(); i++)
        previous[measured[i].object] = i;

    // Toplevel objects are in a root canvas
    UnorderedMap<void*, t_canvas*> rootCanvases;
    for (auto* canvas = pd_getcanvaslist(); canvas; canvas = canvas->gl_next) {
        for (auto* y = canvas->gl_list; y; y = y->g_next)
            rootCanvases[y] = canvas;
    }

    // In the order Pd reads them out
    SmallArray<Measured> matched;
    for (int i = 0; i < readEntries.size(); i++) {
        auto* object = readEntries[i].e_object;
        auto* objectClass = object ? static_cast<void*>(pd_class(&object->te_g.g_pd)) : nullptr;

        auto const it = previous.find(object);
        if (it != previous.end() && measured[it->second].objectClass == objectClass) {
            matched.add(measured[it->second]);
        } else {
            Measured entry;
            entry.object = object;
            entry.objectClass = objectClass;
            if (object) {
                entry.text = Interface::getObjectText(object).toString();
                entry.type = entry.text.upToFirstOccurrenceOf(" ", false, false);
                if (entry.type.isEmpty())
                    entry.type = String::fromUTF8(class_getname(pd_class(&object->te_g.g_pd)));
            } else {
                entry.text = entry.type = "Signal routing";
            }
            matched.add(std::move(entry));
        }
    }

    // Where each object is, now that they're all known
    for (int i = 0; i < readEntries.size(); i++) {
        auto& entry = matched[i];
        auto const parent = readEntries[i].e_parent;
        if (!entry.object) {
            entry.location = "Summing and copying signals";
        } else if (isPositiveAndBelow(parent, readEntries.size())) {
            entry.location = matched[parent].text;
        } else if (auto const root = rootCanvases.find(entry.object); root != rootCanvases.end()) {
            entry.location = String::fromUTF8(root->second->gl_name->s_name);
        }

        // Objects inside a clone can't be shown, so show the outermost clone they're in
        entry.highlightTarget = entry.object;
        for (auto p = parent; isPositiveAndBelow(p, readEntries.size()); p = readEntries[p].e_parent) {
            if (auto* ancestor = readEntries[p].e_object; ancestor && String(class_getname(pd_class(&ancestor->te_g.g_pd))) == "clone")
                entry.highlightTarget = ancestor;
        }
    }

    measured = std::move(matched);
}

}
