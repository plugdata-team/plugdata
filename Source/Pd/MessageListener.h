/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"
#include "Utility/ThreadSafeStack.h"
#include <readerwriterqueue.h>

namespace pd {

class MessageListener {
public:
    virtual void receiveMessage(t_symbol* symbol, StackArray<pd::Atom, 8> const& atoms, int numAtoms) = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
// without performing and memory allocation on the audio thread, and which groups messages within the same audio block (or multiple audio blocks, depending on how long it takes to get a callback from the message thread) togethter
class MessageDispatcher {
    // Wrapper to store 8 atoms in stack memory
    // We never read more than 8 args in the whole source code, so this prevents unnecessary memory copying
    // We also don't want this list to be dynamic since we want to stack allocate it
    class Message {
    public:
        void* target;
        t_symbol* symbol;
        StackArray<t_atom, 8> data;
        int size;

        Message() = default;

        Message(void* ptr, t_symbol* sym, int argc, t_atom* argv)
            : target(ptr)
            , symbol(sym)
        {
            size = std::min(argc, 8);
            std::copy(argv, argv + size, data.data());
        }

        Message(Message const& other) noexcept
        {
            target = other.target;
            symbol = other.symbol;
            size = other.size;
            data = other.data;
        }

        Message& operator=(Message const& other) noexcept
        {
            // Check for self-assignment
            if (this != &other) {
                target = other.target;
                symbol = other.symbol;
                size = other.size;
                data = other.data;
            }

            return *this;
        }
    };

public:
    MessageDispatcher()
    {
        usedHashes.reserve(stackSize);
        nullListeners.reserve(stackSize);
    }

    void enqueueMessage(void* target, t_symbol* symbol, int argc, t_atom* argv)
    {
        if (block)
            return;

        messageStack.push({ target, symbol, argc, argv });
    }

    // used when no plugineditor is active, so we can just ignore messages
    void setBlockMessages(bool blockMessages)
    {
        block = blockMessages;

        // If we're blocking messages from now on, also clear out the queue
        if (blockMessages) {
            Message message;
            while (messageStack.pop(message)) { }
            messageStack.swapBuffers();
            while (messageStack.pop(message)) { }
        }
    }

    void addMessageListener(void* object, pd::MessageListener* messageListener)
    {
        ScopedLock lock(messageListenerLock);
        messageListeners[object].insert(juce::WeakReference(messageListener));
    }

    void removeMessageListener(void* object, MessageListener* messageListener)
    {
        ScopedLock lock(messageListenerLock);

        auto objectListenerIterator = messageListeners.find(object);
        if (objectListenerIterator == messageListeners.end())
            return;

        auto& listeners = objectListenerIterator->second;

        listeners.erase(messageListener);

        if (listeners.empty())
            messageListeners.erase(object);
    }

    void dequeueMessages() // Note: make sure correct pd instance is active when calling this
    {
        usedHashes.clear();
        nullListeners.clear();

        messageStack.swapBuffers();
        Message message;
        while (messageStack.pop(message)) {
            auto target = messageListeners.find(message.target);
            if (EXPECT_LIKELY(target == messageListeners.end()))
                continue;
            
            auto hash = reinterpret_cast<intptr_t>(message.target) ^ reinterpret_cast<intptr_t>(message.symbol);
            if (EXPECT_UNLIKELY(usedHashes.contains(hash))) {
                continue;
            }
            usedHashes.insert(hash);

            StackArray<pd::Atom, 8> atoms;
            for (auto it = target->second.begin(); it != target->second.end(); ++it) {
                if (it->wasObjectDeleted())
                    continue;

                auto listener = it->get();

                for (int at = 0; at < message.size; at++) {
                    atoms[at] = pd::Atom(&message.data[at]);
                }
                auto symbol = message.symbol ? message.symbol : gensym("");

                if (listener)
                    listener->receiveMessage(symbol, atoms, message.size);
                else
                    nullListeners.add({ message.target, it });
            }
        }
 
        nullListeners.erase(
            std::remove_if(nullListeners.begin(), nullListeners.end(),
                           [&](const auto& entry) {
                               auto& [target, iterator] = entry;
                               return messageListeners[target].erase(iterator) != messageListeners[target].end();
                           }),
            nullListeners.end()
        );
    }

private:
    static constexpr int stackSize = 65536;
    using MessageStack = ThreadSafeStack<Message, stackSize>;

    SmallArray<std::pair<void*, UnorderedSet<juce::WeakReference<pd::MessageListener>>::iterator>, 16> nullListeners;
    UnorderedSet<intptr_t> usedHashes;
    MessageStack messageStack;

    UnorderedMap<void*, UnorderedSet<juce::WeakReference<MessageListener>>> messageListeners;
    CriticalSection messageListenerLock;

    // Block messages unless an editor has been constructed
    // Otherwise the message queue will not be cleared by the editors v-blank
    std::atomic<bool> block = true;
};

}
