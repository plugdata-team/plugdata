/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"
#include <readerwriterqueue.h>
#include <plf_stack/plf_stack.h>

namespace pd {

class MessageListener {
public:
    virtual void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) = 0;

    void* object;
    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
// without performing and memory allocation on the audio thread, and which groups messages within the same audio block (or multiple audio blocks, depending on how long it takes to get a callback from the message thread) togethter
class MessageDispatcher {

    // Represents a single Pd message.
    // Holds target object, and symbol+size compressed into a single value
    // Atoms are stored in a separate buffer, and read out based on reported size
    struct Message {
        PointerIntPair<void*, 2, uint8_t> targetAndSize;
        PointerIntPair<t_symbol*, 2, uint8_t> symbolAndSize;
    };

public:
    MessageDispatcher()
    {
        usedHashes.reserve(StackSize);
        nullListeners.reserve(StackSize);
        frontAtomBuffer = &atomBuffers[0];
        backAtomBuffer = &atomBuffers[1];
        frontMessageBuffer = &messageBuffers[0];
        backMessageBuffer = &messageBuffers[1];
        frontAtomBuffer->reserve(StackSize);
        backAtomBuffer->reserve(StackSize);
        frontMessageBuffer->reserve(StackSize);
        backMessageBuffer->reserve(StackSize);
    }

    static void enqueueMessage(void* instance, void* target, t_symbol* symbol, int argc, t_atom* argv) noexcept
    {
        auto* pd = reinterpret_cast<pd::Instance*>(instance);
        auto* dispatcher = pd->messageDispatcher.get();
        if (ProjectInfo::isStandalone || EXPECT_LIKELY(!dispatcher->block)) {
            auto size = std::min(argc, 15);
            for(int i = 0; i < size; i++)
                dispatcher->backAtomBuffer->push(argv[i]);
     
            dispatcher->backMessageBuffer->emplace(PointerIntPair<void*, 2, uint8_t>(target, (size >> 2) & 0b11), PointerIntPair<t_symbol*, 2, uint8_t>(symbol, size & 0b11));
        }
    }

    // used when no plugineditor is active, so we can just ignore messages
    void setBlockMessages(bool blockMessages)
    {
        block = blockMessages;

        // If we're blocking messages from now on, also clear out the queue
        if (blockMessages) {
            sys_lock();
            frontMessageBuffer->clear();
            backMessageBuffer->clear();
            frontAtomBuffer->clear();
            backAtomBuffer->clear();
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
    
    bool popMessage(Message& result)
    {
        if (frontMessageBuffer->empty())
            return false;

        result = frontMessageBuffer->top();
        frontMessageBuffer->pop();
        return true;
    }

    void dequeueMessages() // Note: make sure correct pd instance is active when calling this
    {
        // Not thread safe, but worst thing that could happen is reading the wrong value, which is okay here
        // It's good to at least be able to skip the sys_lock() if the queue is probably empty (and otherwise, it'll get dequeued on the next frame)
        if(backMessageBuffer->empty()) return;
        
        sys_lock(); // Better to lock around all of pd so that enqueueMessage doesn't context switch
        backMessageBuffer = std::exchange(frontMessageBuffer, backMessageBuffer);
        backAtomBuffer = std::exchange(frontAtomBuffer, backAtomBuffer);
        backMessageBuffer->clear();
        backAtomBuffer->clear();
        sys_unlock();
            
        usedHashes.clear();
        nullListeners.clear();
        
        Message message;
        while (popMessage(message)) {
            auto targetPtr = message.targetAndSize.getPointer();
            auto target = messageListeners.find(targetPtr);
            auto [symbol, size] = message.symbolAndSize;
            size = (message.targetAndSize.getInt() << 2) | size;
            
            if (EXPECT_LIKELY(target == messageListeners.end())) {
                for (int at = 0; at < size; at++) {
                    frontAtomBuffer->pop();
                }
                continue;
            }
            
            auto hash = reinterpret_cast<intptr_t>(message.targetAndSize.getOpaqueValue()) ^ reinterpret_cast<intptr_t>(message.symbolAndSize.getOpaqueValue());
            if (EXPECT_UNLIKELY(usedHashes.contains(hash))) {
                for (int at = 0; at < size; at++) {
                    frontAtomBuffer->pop();
                }
                continue;
            }
            usedHashes.insert(hash);
            
            SmallArray<pd::Atom> atoms;
            atoms.resize(size);
            t_atom atom;
            
            for (int at = size-1; at >= 0; at--) {
                atom = frontAtomBuffer->top();
                frontAtomBuffer->pop();
                atoms[at] = &atom;
            }
            
            if(!symbol) continue;
            
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
                           [&](const auto& entry) {
                               auto& [target, iterator] = entry;
                               return messageListeners[target].erase(iterator) != messageListeners[target].end();
                           }),
            nullListeners.end()
        );
    }

private:
    static constexpr int StackSize = 1<<19;
    using MessageStack = plf::stack<Message>;
    using AtomStack = plf::stack<t_atom>;

    std::atomic<bool> block = true; // Block messages if message queue cannot be cleared
    
    StackArray<MessageStack, 2> messageBuffers;
    MessageStack* frontMessageBuffer;
    MessageStack* backMessageBuffer;
    
    StackArray<AtomStack, 2> atomBuffers;
    AtomStack* frontAtomBuffer;
    AtomStack* backAtomBuffer;
    
    SmallArray<std::pair<void*, UnorderedSet<juce::WeakReference<pd::MessageListener>>::iterator>, 16> nullListeners;
    UnorderedSet<intptr_t> usedHashes;
    UnorderedMap<void*, UnorderedSet<juce::WeakReference<MessageListener>>> messageListeners;
};

}
