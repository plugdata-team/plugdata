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

// A custom allocator that doesn't deallocate memory, but reuses it.
template <typename T>
class ReuseAllocator {
public:
    using value_type = T;

    ReuseAllocator() noexcept {}

    template <typename U>
    ReuseAllocator(const ReuseAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        size_t bytes = n * sizeof(T);

        // Check free list for a suitable block
        for (auto it = freeList.begin(); it != freeList.end(); ++it) {
            if (it->size >= bytes) {
                T* result = static_cast<T*>(it->ptr);
    
                it->size -= bytes;
                if(it->size <= 0) {
                    freeList.erase(it);
                }
                else {
                    it->ptr = static_cast<char*>(it->ptr) + bytes;
                }
                return result;
            }
        }

        // Allocate new memory if no suitable block is found
        T* newBlock = static_cast<T*>(std::malloc(bytes));
        if (!newBlock) {
            throw std::bad_alloc();
        }

        return newBlock;
    }
    
    void reserve(size_t n)
    {
        deallocate(allocate(n), n);
    }

    void deallocate(T* p, size_t n) noexcept {
        size_t bytes = n * sizeof(T);

        // Push the memory back into the free list
        freeList.push_back({p, bytes});
        freedSize += bytes;
        
        // TODO: we need to actually free sometimes it, if we've allocated a lot
    }

    template <typename U>
    bool operator==(const ReuseAllocator<U>&) const noexcept {
        return true;
    }

    template <typename U>
    bool operator!=(const ReuseAllocator<U>&) const noexcept {
        return false;
    }

private:
    // A simple free list to hold reusable memory blocks
    static inline std::vector<MemoryBlock> freeList;
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
    
    using MessageStack = plf::stack<Message, ReuseAllocator<Message>>;
    using AtomStack = plf::stack<t_atom, ReuseAllocator<t_atom>>;

    static inline ReuseAllocator<Message> messageAllocator = ReuseAllocator<Message>();
    static inline ReuseAllocator<t_atom> atomAllocator = ReuseAllocator<t_atom>();
    
    struct MessageBuffer
    {
        MessageStack messages = MessageStack(messageAllocator);
        AtomStack atoms = AtomStack(atomAllocator);
    };
    
public:
    MessageDispatcher()
    {
        usedHashes.reserve(128);
        nullListeners.reserve(128);
        
        messageAllocator.reserve(1<<19);
        atomAllocator.reserve(1<<19);
        
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
        if (ProjectInfo::isStandalone || EXPECT_LIKELY(!dispatcher->block)) {
            auto size = std::min(argc, 15);
            auto& backBuffer = dispatcher->getBackBuffer();
            for (int i = 0; i < size; i++)
                backBuffer.atoms.push(argv[i]);
            
            backBuffer.messages.push({ PointerIntPair<void*, 2, uint8_t>(target, (size >> 2) & 0b11), PointerIntPair<t_symbol*, 2, uint8_t>(symbol, size & 0b11) });
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
