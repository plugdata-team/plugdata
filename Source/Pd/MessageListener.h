/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"
#include <readerwriterqueue.h>

namespace pd {

class MessageListener {
public:
    virtual ~MessageListener() = default;
    virtual void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) = 0;

    void* object;
    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
class MessageDispatcher final : public AsyncUpdater {

    struct MessageHeader {
        PointerIntPair<void*, 2, uint8_t> targetAndSize;
        PointerIntPair<t_symbol*, 2, uint8_t> symbolAndSize;
    };

    // Holds either a message header, an atom, or the atom count of a message with at least EscapeSize atoms
    // Messages are stored as [atoms...][count, only if escaped][header], so they can be read back newest-first
    struct Slot {
        Slot() { }

        union {
            MessageHeader header;
            t_atom atom;
            size_t count;
        };
    };
    static_assert(sizeof(Slot) == sizeof(t_atom));
    static_assert(std::is_trivially_copyable_v<Slot>);

    // The header has 4 bits for the atom count, this value means "real count is in the slot below the header"
    static constexpr size_t EscapeSize = 15;

    struct Chunk {
        static constexpr size_t NumSlots = 4096; // 64 KiB
        Slot* end = nullptr;                     // Write position when the chunk was handed to the consumer
        Slot slots[NumSlots];
    };

    static constexpr size_t InitialChunks = 64;
    static constexpr size_t MaxFreeChunks = 64;

    struct PendingMessage {
        void* target;
        t_symbol* symbol;
        Slot const* atoms;
        int size;
    };

public:
    explicit MessageDispatcher(pd::Instance& instance)
        : pdInstance(instance)
    {
        // Reserve up front so the producer doesn't allocate when sealing chunks. The arrays get swapped around in takeFilledChunks(), so all of them need the same capacity
        writer.filledChunks.reserve(256);
        writer.freeChunks.reserve(256);
        drainChunks.reserve(256);
        retiredChunks.reserve(256);
        for (size_t i = 0; i < InitialChunks; i++)
            writer.freeChunks.add(new Chunk);
        startChunk(writer.freeChunks.pop_back_val());

        usedHeaders.reserve(128);
        listenerScratch.reserve(16);
    }

    ~MessageDispatcher() override
    {
        for (auto* chunk : writer.freeChunks)
            delete chunk;
        for (auto* chunk : writer.filledChunks)
            delete chunk;
        for (auto* chunk : drainChunks)
            delete chunk;
        delete writer.chunk;
    }

    // Called by Pd, with the Pd lock held. Type 1 messages (connection debugging/activity) are dropped instead of allocating when the chunk pool is exhausted.
    static void enqueueMessage(void* instance, int type, void* target, t_symbol* symbol, int const argc, t_atom* argv) noexcept
    {
        auto* dispatcher = static_cast<pd::Instance*>(instance)->messageDispatcher.get();
        auto& writer = dispatcher->writer;
        if (EXPECT_UNLIKELY(writer.block.load()))
            return;

        // Fast path for messages with up to 2 atoms (bang, float, symbol, most lists): the copies are inlined, so this stays a leaf function without any calls, stack frame or memory barriers
        auto* const ptr = writer.ptr;
        auto const numAtoms = static_cast<size_t>(argc);
        if (EXPECT_LIKELY(numAtoms <= 2 && writer.end - ptr > 2)) {
            if (numAtoms != 0) {
                std::memcpy(static_cast<void*>(ptr), argv, sizeof(Slot));
                if (numAtoms == 2)
                    std::memcpy(static_cast<void*>(ptr + 1), argv + 1, sizeof(Slot));
            }
            ptr[numAtoms].header = makeHeader(target, symbol, numAtoms);
            writer.ptr = ptr + numAtoms + 1;
            return;
        }

        dispatcher->enqueueLargeMessage(type, target, symbol, numAtoms, argv);
    }

    // Used when no plugineditor is active, so we can just ignore messages
    void setBlockMessages(bool const blockMessages)
    {
        pdInstance.lockAudioThread();
        writer.block = blockMessages;

        // If we're blocking messages from now on, also discard everything that's queued
        if (blockMessages) {
            for (auto* chunk : writer.filledChunks)
                (writer.freeChunks.size() < MaxFreeChunks ? writer.freeChunks : retiredChunks).add(chunk);
            writer.filledChunks.clear();
            writer.ptr = writer.chunk->slots;
        }
        pdInstance.unlockAudioThread();

        freeRetiredChunks();
    }

    void addMessageListener(void* object, pd::MessageListener* messageListener)
    {
        messageListener->object = object;

        auto& listeners = messageListeners[object];
        for (auto const& listener : listeners) {
            if (listener.get() == messageListener)
                return;
        }

        listeners.add(juce::WeakReference(messageListener));
    }

    void removeMessageListener(void* object, MessageListener* messageListener)
    {
        auto const entry = messageListeners.find(object);
        if (entry == messageListeners.end())
            return;

        auto& listeners = entry->second;

        listeners.erase(
            std::ranges::remove_if(listeners,
                [messageListener](auto const& listener) {
                    auto const* referent = listener.get();
                    return referent == nullptr || referent == messageListener;
                })
                .begin(),
            listeners.end());

        if (listeners.empty())
            messageListeners.erase(object);
    }

    void dequeueMessages() // Note: only call this from the message thread
    {
        if (isDequeueing)
            return;

        ScopedValueSetter<bool> const dequeueGuard(isDequeueing, true);

        takeFilledChunks();

        // Walk the messages newest-first, keeping only the newest one for each (target, symbol, size)
        usedHeaders.clear();
        pendingMessages.clear();
        for (int i = static_cast<int>(drainChunks.size()) - 1; i >= 0; i--) {
            auto const* chunk = drainChunks[i];
            auto const* slot = chunk->end;
            while (slot != chunk->slots) {
                auto const& header = (--slot)->header;
                size_t size = header.targetAndSize.getInt() << 2 | header.symbolAndSize.getInt();
                if (EXPECT_UNLIKELY(size == EscapeSize))
                    size = (--slot)->count;
                slot -= size;

                auto const key = std::pair(reinterpret_cast<uintptr_t>(header.targetAndSize.getOpaqueValue()), reinterpret_cast<uintptr_t>(header.symbolAndSize.getOpaqueValue()));
                if (usedHeaders.insert(key).second)
                    pendingMessages.add({ header.targetAndSize.getPointer(), header.symbolAndSize.getPointer(), slot, static_cast<int>(size) });
            }
        }

        // Replay messages in original order
        // The drained chunks stay ours until the next dequeueMessages() call, so the atoms remain valid while we dispatch
        for (int i = static_cast<int>(pendingMessages.size()) - 1; i >= 0; i--) {
            auto const& message = pendingMessages[i];
            auto const target = messageListeners.find(message.target);
            if (EXPECT_LIKELY(target == messageListeners.end()))
                continue;

            atomScratch.clear();
            for (int at = 0; at < message.size; at++)
                atomScratch.emplace_back(&message.atoms[at].atom);

            // Dispatch from a copy, since we're entering into arbitrary plugdata code here that can possible deallocate things we need
            listenerScratch.clear();
            for (auto const& listener : target->second)
                listenerScratch.add(listener);

            bool sawDeadListener = false;
            for (auto const& listener : listenerScratch) {
                if (auto* referent = listener.get())
                    referent->receiveMessage(message.symbol, atomScratch);
                else
                    sawDeadListener = true;
            }

            if (EXPECT_UNLIKELY(sawDeadListener))
                removeDeadListeners(message.target);
        }
    }

    void removeDeadListeners(void* targetPtr)
    {
        auto const entry = messageListeners.find(targetPtr);
        if (entry == messageListeners.end())
            return;

        auto& listeners = entry->second;
        listeners.erase(
            std::ranges::remove_if(listeners,
                [](auto const& listener) { return listener.get() == nullptr; })
                .begin(),
            listeners.end());

        if (listeners.empty())
            messageListeners.erase(targetPtr);
    }

    void handleAsyncUpdate() override
    {
        dequeueMessages();
    }

private:
    static MessageHeader makeHeader(void* target, t_symbol* symbol, size_t const size)
    {
        return { PointerIntPair<void*, 2, uint8_t>(target, static_cast<uint8_t>((size & 0b1100) >> 2)), PointerIntPair<t_symbol*, 2, uint8_t>(symbol, static_cast<uint8_t>(size & 0b11)) };
    }

    // Everything the fast path doesn't handle: more than 2 atoms, escaped atom counts, and starting a new chunk
#if JUCE_MSVC
    __declspec(noinline)
#else
    __attribute__((noinline))
#endif
    void enqueueLargeMessage(int const type, void* target, t_symbol* symbol, size_t numAtoms, t_atom* argv) noexcept
    {
        numAtoms = std::min(numAtoms, Chunk::NumSlots - 2); // A message can't span chunks
        auto const escaped = numAtoms >= EscapeSize;
        auto const numSlots = numAtoms + (escaped ? 2 : 1);

        if (static_cast<size_t>(writer.end - writer.ptr) < numSlots) {
            Chunk* chunk = nullptr;
            if (!writer.freeChunks.empty())
                chunk = writer.freeChunks.pop_back_val();
            else if (type != 1)
                chunk = new (std::nothrow) Chunk;

            if (!chunk)
                return;

            sealChunk();
            startChunk(chunk);
        }

        std::memcpy(static_cast<void*>(writer.ptr), argv, numAtoms * sizeof(Slot));
        writer.ptr += numAtoms;
        if (escaped) {
            writer.ptr->count = numAtoms;
            ++writer.ptr;
        }
        writer.ptr->header = makeHeader(target, symbol, escaped ? EscapeSize : numAtoms);
        ++writer.ptr;
    }

    // Swaps the chunks the producer filled since the last call into drainChunks
    void takeFilledChunks()
    {
        pdInstance.lockAudioThread();

        // Return the chunks we drained last time to the pool. Whatever doesn't fit gets freed after unlocking
        while (!drainChunks.empty() && writer.freeChunks.size() < MaxFreeChunks)
            writer.freeChunks.add(drainChunks.pop_back_val());
        std::swap(drainChunks, retiredChunks);

        // Hand over the chunk that's currently being written to as well
        if (writer.ptr != writer.chunk->slots) {
            sealChunk();
            startChunk(writer.freeChunks.empty() ? new Chunk : writer.freeChunks.pop_back_val());
        }
        std::swap(drainChunks, writer.filledChunks);

        pdInstance.unlockAudioThread();

        freeRetiredChunks();
    }

    // Frees chunks that didn't fit back into the pool, outside the Pd lock
    void freeRetiredChunks()
    {
        for (auto* chunk : retiredChunks)
            delete chunk;
        retiredChunks.clear();
    }

    void sealChunk()
    {
        writer.chunk->end = writer.ptr;
        writer.filledChunks.add(writer.chunk);
    }

    void startChunk(Chunk* chunk)
    {
        writer.chunk = chunk;
        writer.ptr = chunk->slots;
        writer.end = chunk->slots + Chunk::NumSlots;
    }

    pd::Instance& pdInstance;

    // Producer state: only touched while holding the Pd lock, so it doesn't need to be atomic
    struct Writer {
        Slot* ptr = nullptr;
        Chunk* chunk = nullptr;
        Slot* end = nullptr;
        AtomicValue<bool, Relaxed> block = true;
        SmallArray<Chunk*> filledChunks;
        SmallArray<Chunk*> freeChunks;
    } writer;

    // Padding to keep the consumer's writes to its members below off the producer's cache lines
    [[maybe_unused]] char writerPadding[128];

    // Consumer state (message thread only)
    SmallArray<Chunk*> drainChunks;
    SmallArray<Chunk*> retiredChunks;
    bool isDequeueing = false;
    UnorderedSet<std::pair<uintptr_t, uintptr_t>> usedHeaders;
    SmallArray<PendingMessage> pendingMessages;
    SmallArray<pd::Atom> atomScratch;
    SmallArray<juce::WeakReference<MessageListener>> listenerScratch;

    UnorderedMap<void*, SmallArray<juce::WeakReference<MessageListener>>> messageListeners;
};

}
