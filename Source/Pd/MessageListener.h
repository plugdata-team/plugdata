/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"
#include <concurrentqueue.h>

namespace pd {


class MessageListener {
public:
    virtual void receiveMessage(t_symbol* symbol, const pd::Atom atoms[8], int numAtoms) = 0;

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
    class Message {
    public:
        void* target;
        t_symbol* symbol;
        t_atom data[8];
        int size;
        
        Message() {};
        
        Message(void* ptr, t_symbol* sym, int argc, t_atom* argv) : target(ptr), symbol(sym)
        {
            size = std::min(argc, 8);
            std::copy(argv, argv + size, data);
        }
        
        Message(const Message& other) noexcept {
            target = other.target;
            symbol = other.symbol;
            size = other.size;
            std::copy(other.data, other.data + other.size, data);
        }
    
        Message& operator=(const Message& other) noexcept {
            // Check for self-assignment
            if (this != &other) {
                target = other.target;
                symbol = other.symbol;
                size = other.size;
                std::copy(other.data, other.data + other.size, data);
            }
            
            return *this;
        }
    };

public:
    void enqueueMessage(void* target, t_symbol* symbol, int argc, t_atom* argv)
    {
        messageQueue.enqueue({target, symbol, argc, argv});
    }
    
    void addMessageListener(void* object, pd::MessageListener* messageListener)
    {
        ScopedLock lock(messageListenerLock);
        messageListeners[object].insert(juce::WeakReference(messageListener));
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
        Message incomingMessage;
        std::map<size_t, Message> uniqueMessages;

        while(messageQueue.try_dequeue(incomingMessage))
        {
            auto hash = reinterpret_cast<size_t>(incomingMessage.target) ^ reinterpret_cast<size_t>(incomingMessage.symbol);
            uniqueMessages[hash] = std::move(incomingMessage);
        }
        
        // Collect MessageListeners that have been deallocated for later removal
        std::vector<std::pair<void*, std::set<juce::WeakReference<pd::MessageListener>>::iterator>> nullListeners;
        
        for (auto& [hash, message] : uniqueMessages) {
            
            if (messageListeners.find(message.target) == messageListeners.end()) continue;
            
            for (auto it = messageListeners.at(message.target).begin(); it != messageListeners.at(message.target).end(); ++it) {
                if(it->wasObjectDeleted()) continue;
                
                auto listener = it->get();
                
                pd::Atom atoms[8];
                for(int at = 0; at < message.size; at++)
                {
                    atoms[at] = pd::Atom(message.data + at);
                }
                auto symbol = message.symbol ? message.symbol : gensym(""); // TODO: fix instance issues!
                
                if (listener)
                    listener->receiveMessage(symbol, atoms, message.size);
                else
                    nullListeners.push_back({message.target, it});
            }
        }
                
        for (int i = nullListeners.size() - 1; i >= 0; i--) {
            auto& [target, iterator] = nullListeners[i];
            messageListeners[target].erase(iterator);
        }
    }
    
    moodycamel::ConcurrentQueue<Message> messageQueue = moodycamel::ConcurrentQueue<Message>(32768);
    std::map<void*, std::set<juce::WeakReference<MessageListener>>> messageListeners;
    CriticalSection messageListenerLock;
};
        

}
