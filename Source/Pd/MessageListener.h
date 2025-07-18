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
    virtual void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) = 0;

    void* object;
    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

template<typename T>
class MessageVector {
private:
    static constexpr size_t Capacity = 1 << 20;
    static constexpr size_t OverflowBlockSize = 1024;
    AtomicValue<size_t> size = 0;
    // Primary contiguous buffer
    T buffer[Capacity];

    // Small blocks for when we overflow our buffer
    SmallArray<std::unique_ptr<T[]>> overflowBlocks;

    void addOverflow(T const& value)
    {
        if (overflowBlocks.empty() || (size % OverflowBlockSize) == 0) {
            overflowBlocks.emplace_back(std::make_unique<T[]>(OverflowBlockSize));
        }
        size_t blockIndex = (size - Capacity) / OverflowBlockSize;
        size_t blockOffset = (size - Capacity) % OverflowBlockSize;
        overflowBlocks[blockIndex][blockOffset] = value;
    }

public:
    // Constructor
    MessageVector() { }

    bool empty() const
    {
        return size == 0;
    }

    void append(T const&& header, T const* values, int numValues)
    {
        if (EXPECT_LIKELY((size + numValues + 1) < Capacity)) {
            std::copy(values, values + numValues, buffer + size);
            buffer[size + numValues] = header;
            size += (numValues + 1);
        } else {
            auto spaceLeft = Capacity > size ? std::min<int>(Capacity - size, numValues) : 0;
            int numOverflow = numValues - spaceLeft;
            for (int i = 0; i < spaceLeft; i++) {
                buffer[size + i] = values[i];
            }
            size += spaceLeft;
            for (int i = 0; i < numOverflow; i++) {
                addOverflow(values[spaceLeft + i]);
                size++;
            }

            if (size < Capacity) {
                buffer[size] = header;
            } else {
                addOverflow(header);
            }
            size++;
        }
    }

    T& back()
    {
        if (size == 0) {
            throw std::out_of_range("No elements in vector");
        }
        if (EXPECT_LIKELY(size <= Capacity)) {
            return buffer[size - 1];
        }
        size_t blockIndex = (size - Capacity - 1) / OverflowBlockSize;
        size_t blockOffset = (size - Capacity - 1) & (OverflowBlockSize - 1);
        return overflowBlocks[blockIndex][blockOffset];
    }

    void pop()
    {
        --size;
    }

    void pop(int amount)
    {
        size -= amount;
    }

    void clear()
    {
        overflowBlocks.clear();
        size = 0;
    }
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
// it's guaranteed to be lock-free and wait-free, until memory allocation needs to happen
class MessageDispatcher : public AsyncUpdater {

    // Represents a single Pd message.
    // Holds target object, and symbol+size compressed into a single value
    // Atoms are stored in a separate buffer, and read out based on reported size

    struct MessageTargetSizeSymbol {
        PointerIntPair<void*, 2, uint8_t> targetAndSize;
        PointerIntPair<t_symbol*, 2, uint8_t> symbolAndSize;
    };

    struct Message {
        Message() { };

        // Both are 16 bytes, so we can squish them together into a single queue
        union {
            MessageTargetSizeSymbol header;
            t_atom atom;
        };
    };

    using MessageBuffer = MessageVector<Message>;

public:
    MessageDispatcher()
    {
        usedHashes.reserve(128);
        nullListeners.reserve(128);
    }

    static void enqueueMessage(void* instance, void* target, t_symbol* symbol, int const argc, t_atom* argv) noexcept
    {
        auto const* pd = static_cast<pd::Instance*>(instance);
        auto* dispatcher = pd->messageDispatcher.get();
        if (EXPECT_LIKELY(!dispatcher->block)) {
            auto const size = argc > 15 ? 15 : argc;

            auto& backBuffer = dispatcher->getBackBuffer();
            Message message;
            message.header = { PointerIntPair<void*, 2, uint8_t>(target, (size & 0b1100) >> 2), PointerIntPair<t_symbol*, 2, uint8_t>(symbol, size & 0b11) };

            backBuffer.append(std::move(message), reinterpret_cast<Message*>(argv), size);
        }
    }

    // used when no plugineditor is active, so we can just ignore messages
    void setBlockMessages(bool const blockMessages)
    {
        block = blockMessages;

        // If we're blocking messages from now on, also clear out the queue
        if (blockMessages) {
            sys_lock();
            for (auto& buffer : buffers) {
                buffer.clear();
            }
            sys_unlock();
        }
    }

    void addMessageListener(void* object, pd::MessageListener* messageListener)
    {
        messageListeners[object].insert(juce::WeakReference(messageListener));
        messageListener->object = object;
    }

    void removeMessageListener(void* object, MessageListener* messageListener)
    {
        auto const objectListenerIterator = messageListeners.find(object);
        if (objectListenerIterator == messageListeners.end())
            return;

        auto& listeners = objectListenerIterator->second;

        listeners.erase(messageListener);

        if (listeners.empty())
            messageListeners.erase(object);
    }

    static bool popMessage(MessageBuffer& buffer, Message& result)
    {
        if (buffer.empty())
            return false;

        result = buffer.back();
        buffer.pop();
        return true;
    }

    void dequeueMessages() // Note: make sure correct pd instance is active when calling this
    {
        auto& frontBuffer = getFrontBuffer();

        usedHashes.clear();
        nullListeners.clear();

        Message message;
        while (popMessage(frontBuffer, message)) {
            auto targetPtr = message.header.targetAndSize.getPointer();
            auto target = messageListeners.find(targetPtr);
            auto [symbol, size] = message.header.symbolAndSize;
            size = message.header.targetAndSize.getInt() << 2 | size;

            if (EXPECT_LIKELY(target == messageListeners.end())) {
                frontBuffer.pop(size);
                continue;
            }

            auto hash = reinterpret_cast<intptr_t>(message.header.targetAndSize.getOpaqueValue()) ^ reinterpret_cast<intptr_t>(message.header.symbolAndSize.getOpaqueValue());
            if (EXPECT_UNLIKELY(usedHashes.contains(hash))) {
                frontBuffer.pop(size);
                continue;
            }
            usedHashes.insert(hash);

            SmallArray<pd::Atom> atoms;
            atoms.resize(size);
            t_atom atom;

            for (int at = size - 1; at >= 0; at--) {
                atom = frontBuffer.back().atom;
                frontBuffer.pop();
                atoms[at] = &atom;
            }

            for (auto it = target->second.begin(); it < target->second.end(); ++it) {
                if (auto* listener = it->get())
                    listener->receiveMessage(symbol, atoms);
                else
                    nullListeners.add({ targetPtr, it });
            }
        }

        nullListeners.erase(
            std::ranges::remove_if(nullListeners,
                [&](auto const& entry) {
                    auto& [target, iterator] = entry;
                    return messageListeners[target].erase(iterator) != messageListeners[target].end();
                })
                .begin(),
            nullListeners.end());

        frontBuffer.clear();

        currentBuffer.store((currentBuffer.load() + 1) % 3);
    }

    void handleAsyncUpdate()
    {
        dequeueMessages();
    }

    MessageBuffer& getBackBuffer()
    {
        return buffers[currentBuffer.load()];
    }

    MessageBuffer& getFrontBuffer()
    {
        return buffers[(currentBuffer.load() + 2) % 3];
    }

private:
    AtomicValue<bool, Relaxed> block = true; // Block messages if message queue cannot be cleared
    StackArray<MessageBuffer, 3> buffers;
    AtomicValue<int, Sequential> currentBuffer;

    SmallArray<std::pair<void*, UnorderedSet<juce::WeakReference<pd::MessageListener>>::iterator>, 16> nullListeners;
    UnorderedSet<intptr_t> usedHashes;
    UnorderedMap<void*, UnorderedSet<juce::WeakReference<MessageListener>>> messageListeners;
};

}
