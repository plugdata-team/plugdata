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
    // Wrapper to store 8 atoms in stack memory
    // We never read more than 8 args in the whole source code, so this prevents unnecessary memory copying
    // We also don't want this list to be dynamic since we want to stack allocate it
    class Atoms {
    public:
        t_atom data[8];
        short size = 0;
        
        Atoms() = default;
        
        Atoms(int argc, t_atom* argv)
        {
            size = std::min(argc, 8);
            std::copy(argv, argv + size, data);
        }

        // Provide a copy constructor
        Atoms(const Atoms& other) {
            size = other.size;
            // Perform a deep copy
            for (int i = 0; i < size; ++i) {
                data[i] = other.data[i];
            }
        }

        // Provide a copy assignment operator
        Atoms& operator=(const Atoms& other) {
            size = other.size;
            // Check for self-assignment
            if (this != &other) {
                // Perform a deep copy
                for (int i = 0; i < size; ++i) {
                    data[i] = other.data[i];
                }
            }
            return *this;
        }
    };

    
    // Alias for your Message type
    using Message = std::tuple<void*, t_symbol*, Atoms>;
    
public:
    void enqueueMessage(void* target, t_symbol* symbol, int argc, t_atom* argv)
    {
        if (messageListeners.find(target) == messageListeners.end())
            return;
        
        messageQueue.try_enqueue({target, symbol, Atoms(argc, argv)});
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
        ScopedLock lock(messageListenerLock);

        std::vector<std::pair<void*, std::vector<juce::WeakReference<pd::MessageListener>>::iterator>> nullListeners;
        
        auto size = messageQueue.size_approx();
        
        std::vector<Message> messages(size);

        auto numDequeued = messageQueue.try_dequeue_bulk(messages.begin(), size);
        messages.resize(numDequeued);
        
        std::vector<Message> uniqueMessages;
        std::set<std::pair<void*, t_symbol*>> uniquePairs;
        
        for (int i = messages.size() - 1; i >= 0; --i) {
            const auto& pair = std::make_pair(std::get<0>(messages[i]), std::get<1>(messages[i]));
            
            if (messageListeners.find(pair.first) == messageListeners.end()) continue;
            
            if (uniquePairs.find(pair) != uniquePairs.end()) {
                uniqueMessages.push_back(messages[i]);
            } else {
                uniquePairs.insert(pair);
            }
        }
        
        for (auto& [target, symbol, atoms] : uniqueMessages) {
            for (auto it = messageListeners.at(target).begin(); it != messageListeners.at(target).end(); ++it) {
                auto listenerWeak = *it;
                auto listener = listenerWeak.get();
                
                auto heapAtoms = pd::Atom::fromAtoms(atoms.size, atoms.data);
                
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
    
    CriticalSection messageListenerLock;
    std::unordered_map<void*, std::vector<juce::WeakReference<MessageListener>>> messageListeners;
    moodycamel::ConcurrentQueue<Message> messageQueue = moodycamel::ConcurrentQueue<Message>(4096);
};
        

}
