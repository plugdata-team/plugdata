/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <algorithm>
#include <m_pd.h>
#include <readerwriterqueue.h>

namespace pd {

// A message that Pd sent through a connection
struct ConnectionTraceEvent {
    ConnectionTraceEvent(t_outconnect* connection, t_symbol* selector, uint64_t const sequenceNumber, int const argc, t_atom const* argv)
        : connection(connection)
        , selector(selector)
        , sequenceNumber(sequenceNumber)
        , numAtoms(std::min<int>(argc, maxAtoms))
        , truncated(argc > maxAtoms)
    {
        std::copy_n(argv, numAtoms, atoms);
    }

    static constexpr int maxAtoms = 32;

    t_outconnect* connection;
    t_symbol* selector;
    uint64_t sequenceNumber;
    int numAtoms;
    bool truncated; // the message had more atoms than fit in atoms[]
    t_atom atoms[maxAtoms];
};

// Records every message that Pd sends through a connection, in the order Pd sends them, for Detective Mode.
// Pd calls enqueue() with its lock held, from whichever thread is running Pd; only the message thread dequeues.
// When the message thread falls behind, the newest messages are dropped instead of blocking Pd
class ConnectionTraceCollector final {
public:
    // Activity notifications are sent like connection messages, but with a canvas as the target
    explicit ConnectionTraceCollector(t_symbol* activitySymbol)
        : activitySymbol(activitySymbol)
    {
    }

    // Registered with register_trace_trigger()
    static void enqueue(void* collector, int const type, void* target, t_symbol* selector, int const argc, t_atom* argv)
    {
        auto* self = static_cast<ConnectionTraceCollector*>(collector);
        if (type != 1 || selector == self->activitySymbol)
            return;

        // Built in place, and only if there's room
        self->queue.try_emplace(static_cast<t_outconnect*>(target), selector, ++self->sequenceCounter, argc, argv);
    }

    // Passes the recorded messages to the callback, oldest first, at most as many as fit in the queue per call
    template<typename Callback>
    void dequeue(Callback&& callback)
    {
        for (size_t i = 0; i < capacity; i++) {
            auto const* event = queue.peek();
            if (!event)
                break;

            callback(*event);
            queue.pop();
        }
    }

private:
    static constexpr size_t capacity = 4096;

    moodycamel::ReaderWriterQueue<ConnectionTraceEvent> queue { capacity };
    t_symbol* activitySymbol;
    uint64_t sequenceCounter = 0; // Only touched with Pd's lock held
};

}
