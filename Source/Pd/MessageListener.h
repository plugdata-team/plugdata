/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"
#include <readerwriterqueue.h>
#include "Utility/Stack.h"

namespace pd {

class MessageListener {
public:
    virtual void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) = 0;

    void* object;
    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

// A simple memory block structure to manage free list.
struct MemoryBlock {
    void* ptr;
    size_t size;
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
// without performing and memory allocation or locking
class MessageDispatcher {

    // Represents a single Pd message.
    // Holds target object, and symbol+size compressed into a single value
    // Atoms are stored in a separate buffer, and read out based on reported size
    struct Message {
        PointerIntPair<void*, 2, uint8_t> targetAndSize;
        PointerIntPair<t_symbol*, 2, uint8_t> symbolAndSize;
    };
    
    using MessageStack = plf::stack<Message>;
    using AtomStack = plf::stack<t_atom>;

    struct MessageBuffer
    {
        MessageStack messages;
        AtomStack atoms;
    };
    
public:
    MessageDispatcher()
    {
        usedHashes.reserve(128);
        nullListeners.reserve(128);
        
        for(auto& buffer : buffers)
        {
            buffer.messages.reserve(1<<17);
            buffer.atoms.reserve(1<<17);
        }
        
    }

    static void enqueueMessage(void* instance, void* target, t_symbol* symbol, int argc, t_atom* argv) noexcept
    {
        auto* pd = reinterpret_cast<pd::Instance*>(instance);
        auto* dispatcher = pd->messageDispatcher.get();
        if (EXPECT_LIKELY(!dispatcher->block)) {
            auto size = argc > 15 ? 15 : argc;
            auto& backBuffer = dispatcher->getBackBuffer();
            
            backBuffer.messages.push({PointerIntPair<void*, 2, uint8_t>(target, (size & 0b1100) >> 2), PointerIntPair<t_symbol*, 2, uint8_t>(symbol, size & 0b11)});
            for (int i = 0; i < size; i++)
                backBuffer.atoms.push(argv[i]);
        }
    }

    // used when no plugineditor is active, so we can just ignore messages
    void setBlockMessages(bool blockMessages)
    {
        block = blockMessages;

        // If we're blocking messages from now on, also clear out the queue
        if (blockMessages) {
            sys_lock();
            for(auto& buffer : buffers)
            {
                buffer.messages.clear();
                buffer.atoms.clear();
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
        auto objectListenerIterator = messageListeners.find(object);
        if (objectListenerIterator == messageListeners.end())
            return;

        auto& listeners = objectListenerIterator->second;

        listeners.erase(messageListener);

        if (listeners.empty())
            messageListeners.erase(object);
    }

    bool popMessage(MessageStack& buffer, Message& result)
    {
        if (buffer.empty())
            return false;

        result = buffer.top();
        buffer.pop();
        return true;
    }
    
    void dequeueMessages() // Note: make sure correct pd instance is active when calling this
    {
        currentBuffer.store((currentBuffer.load() + 1) % 3);
        
        auto& frontBuffer = getFrontBuffer();
        
        usedHashes.clear();
        nullListeners.clear();

        Message message;
        while (popMessage(frontBuffer.messages, message)) {
            auto targetPtr = message.targetAndSize.getPointer();
            auto target = messageListeners.find(targetPtr);
            auto [symbol, size] = message.symbolAndSize;
            size = (message.targetAndSize.getInt() << 2) | size;

            if (EXPECT_LIKELY(target == messageListeners.end())) {
                for (int at = 0; at < size; at++) {
                    frontBuffer.atoms.pop();
                }
                continue;
            }

            auto hash = reinterpret_cast<intptr_t>(message.targetAndSize.getOpaqueValue()) ^ reinterpret_cast<intptr_t>(message.symbolAndSize.getOpaqueValue());
            if (EXPECT_UNLIKELY(usedHashes.contains(hash))) {
                for (int at = 0; at < size; at++) {
                    frontBuffer.atoms.pop();
                }
                continue;
            }
            usedHashes.insert(hash);

            SmallArray<pd::Atom> atoms;
            atoms.resize(size);
            t_atom atom;

            for (int at = size - 1; at >= 0; at--) {
                atom = frontBuffer.atoms.top();
                frontBuffer.atoms.pop();
                atoms[at] = &atom;
            }

            if (!symbol)
                continue;

            for (auto it = target->second.begin(); it != target->second.end(); ++it) {
                if (it->wasObjectDeleted())
                    continue;
                auto listener = it->get();

                if (listener)
                    listener->receiveMessage(symbol, atoms);
                else
                    nullListeners.add({ targetPtr, it });
            }
        }

        nullListeners.erase(
            std::remove_if(nullListeners.begin(), nullListeners.end(),
                [&](auto const& entry) {
                    auto& [target, iterator] = entry;
                    return messageListeners[target].erase(iterator) != messageListeners[target].end();
                }),
            nullListeners.end());
        
        frontBuffer.messages.trim();
        frontBuffer.atoms.trim();
    }
    
    MessageBuffer& getBackBuffer()
    {
        return buffers[currentBuffer.load()];
    }

    MessageBuffer& getFrontBuffer()
    {
        return buffers[(currentBuffer.load() + 1) % 3];
    }

private:
    AtomicValue<bool, Relaxed> block = true; // Block messages if message queue cannot be cleared
    StackArray<MessageBuffer, 3> buffers;
    AtomicValue<int> currentBuffer;

    SmallArray<std::pair<void*, UnorderedSet<juce::WeakReference<pd::MessageListener>>::iterator>, 16> nullListeners;
    UnorderedSet<intptr_t> usedHashes;
    UnorderedMap<void*, UnorderedSet<juce::WeakReference<MessageListener>>> messageListeners;
};

}
