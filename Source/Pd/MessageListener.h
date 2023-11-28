/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"

namespace pd {


class MessageListener {
public:
    virtual void receiveMessage(String const& symbol, std::vector<pd::Atom> const& atoms) = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

// MessageDispatcher handles the organising of messages from Pd to the plugdata GUI
// It provides an optimised way to listen to messages within pd from the message thread,
// without performing and memory allocation on the audio thread, and which groups messages within the same audio block (or multiple audio blocks, depending on how long it takes to get a callback from the message thread) togethter
class MessageDispatcher : private AsyncUpdater
{
    template <typename T>
    class MemoryPool {
    private:
        std::vector<T> blocks;
        std::vector<bool> usedBlocks;

    public:
        MemoryPool(size_t initial_size) : blocks(initial_size), usedBlocks(initial_size, false) {}

        T* allocate(std::size_t n) {
            
            auto it = std::find(usedBlocks.begin(), usedBlocks.end(), false);
            while (it != usedBlocks.end()) {
                
                // Check if there are enough consecutive free blocks
                auto remaining = std::distance(it, usedBlocks.end());
                if (std::count(it, std::next(it, std::min<size_t>(n, remaining)), false) >= n) {
                    break;  // Found a suitable block
                }
                // Move to the next potential block
                it = std::find(std::next(it), usedBlocks.end(), false);
            }

            if (it == usedBlocks.end()) {
                // No free block found, resize both vectors
                size_t newSize = blocks.size() * 2;

                blocks.resize(newSize);
                usedBlocks.resize(newSize, false);

                it = std::find(usedBlocks.begin(), usedBlocks.end(), false);
            }

            std::fill(it, it + n, true);
            return &blocks[std::distance(usedBlocks.begin(), it)];
        }

        void deallocate(T* ptr, std::size_t n) {
            size_t index = std::distance(&blocks[0], ptr);
            std::fill(usedBlocks.begin() + index, usedBlocks.begin() + index + n, false);
        }
    };

    // Custom allocator for std::vector using MemoryPool
    template <typename T>
    class MemoryPoolAllocator {
    public:
        using value_type = T;

        MemoryPoolAllocator(MemoryPool<T>& memory_pool) noexcept
            : memory_pool_(memory_pool) {}

        template <typename U>
        MemoryPoolAllocator(const MemoryPoolAllocator<U>& other) noexcept
            : memory_pool_(other.memory_pool_) {}

        T* allocate(std::size_t n) {
            return memory_pool_.allocate(n);
        }

        void deallocate(T* p, std::size_t n) {
            if (n == 1) {
                memory_pool_.deallocate(p, n);
            }
        }

        MemoryPool<T>& memory_pool_;
    };
    
    template <typename U>
    friend bool operator==(const MemoryPoolAllocator<U>& a, const MemoryPoolAllocator<U>& b) {
        return &a.memory_pool_ == &b.memory_pool_;
    }

    template <typename U>
    friend bool operator!=(const MemoryPoolAllocator<U>& a, const MemoryPoolAllocator<U>& b) {
        return !(a == b);
    }

    template <typename T>
    using MemoryPoolVector = std::vector<T, MemoryPoolAllocator<T>>;

    // Alias for your Message type
    using Message = std::tuple<void*, t_symbol*, MemoryPoolVector<pd::Atom>>;
    
public:
    void enqueueMessage(void* target, t_symbol* symbol, int argc, t_atom* argv)
    {
        if (messageListeners.find(target) == messageListeners.end())
            return;
        
        auto array = MemoryPoolVector<pd::Atom>(memoryPool);
        array.reserve(argc);

        for (int i = 0; i < argc; i++) {
            if (argv[i].a_type == A_FLOAT) {
                array.emplace_back(atom_getfloat(argv + i));
            } else if (argv[i].a_type == A_SYMBOL) {
                array.emplace_back(atom_getsymbol(argv + i));
            } else {
                array.emplace_back();
            }
        }
                
        Message message(target, symbol, std::move(array));
        messageQueue.try_enqueue(std::move(message));
    }
    
    void addMessageListener(void* object, pd::MessageListener* messageListener)
    {
        ScopedLock lock(messageListenerLock);
        messageListeners[object].emplace_back(messageListener);
    }
    
    void removeMessageListener(void* object, MessageListener* messageListener)
    {
        ScopedLock lock(messageListenerLock);

        if (!messageListeners.count(object))
            return;

        auto& listeners = messageListeners[object];
        auto it = std::find(listeners.begin(), listeners.end(), messageListener);

        if (it != listeners.end())
            listeners.erase(it);

        if (listeners.empty())
            messageListeners.erase(object);
    }
    
    void dispatch()
    {
        if(messageQueue.size_approx() != 0) {
            triggerAsyncUpdate();
        }
    }
    
private:
    
    void handleAsyncUpdate() override
    {
        Message incomingMessage = {nullptr, nullptr, {MemoryPoolVector<pd::Atom>(memoryPool)}};

        std::vector<std::pair<void*, std::vector<juce::WeakReference<pd::MessageListener>>::iterator>> nullListeners;
        
        while(messageQueue.try_dequeue(incomingMessage))
        {
            auto& [target, symbol, atoms] = incomingMessage;
            for (auto it = messageListeners.at(target).begin(); it != messageListeners.at(target).end(); ++it) {
                auto listenerWeak = *it;
                auto listener = listenerWeak.get();
                
                auto heapAtoms = std::vector<pd::Atom, std::allocator<pd::Atom>>(atoms.begin(), atoms.end());;
                if (listener)
                    listener->receiveMessage(String::fromUTF8(symbol->s_name), heapAtoms);
                else
                    nullListeners.push_back({target, it});
            }
        }
        
        for (int i = nullListeners.size() - 1; i >= 0; i--) {
            auto& [target, iterator] = nullListeners[i];
            messageListeners[target].erase(iterator);
        }
    }
    
    MemoryPool<pd::Atom> memoryPool = MemoryPool<pd::Atom>(4096);
    CriticalSection messageListenerLock;
    std::unordered_map<void*, std::vector<juce::WeakReference<MessageListener>>> messageListeners;
    moodycamel::ConcurrentQueue<Message> messageQueue = moodycamel::ConcurrentQueue<Message>(4096);
};
        

}
