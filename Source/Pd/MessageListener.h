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
        
        Message(Message&& other) noexcept {
            target = std::move(other.target);
            symbol = std::move(other.symbol);
            size = other.size;
            // Move the data
            for (int i = 0; i < size; ++i) {
                data[i] = std::move(other.data[i]);
            }
            // Reset the other object
            other.size = 0;
        }
    
        // Move assignment operator
        Message& operator=(Message&& other) noexcept {
            // Check for self-assignment
            if (this != &other) {
                target = std::move(other.target);
                symbol = std::move(other.symbol);
                size = other.size;
                // Move the data
                for (int i = 0; i < size; ++i) {
                    data[i] = std::move(other.data[i]);
                }
                // Reset the other object
                other.size = 0;
            }
            return *this;
        }
    };

public:
    void enqueueMessage(void* target, t_symbol* symbol, int argc, t_atom* argv)
    {
        if (messageListeners.find(target) == messageListeners.end())
            return;
    
        messageQueue.try_enqueue({target, symbol, argc, argv});
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
        ScopedLock lock(messageListenerLock);

        // Collect MessageListeners that have been deallocated for later removal
        std::vector<std::pair<void*, std::set<juce::WeakReference<pd::MessageListener>>::iterator>> nullListeners;
        
        Message incomingMessage;
        std::map<size_t, Message> uniqueMessages;

        while(messageQueue.try_dequeue(incomingMessage))
        {
            auto hash = reinterpret_cast<size_t>(incomingMessage.target) + reinterpret_cast<size_t>(incomingMessage.symbol);
            //TODO: make a simple hash from symbol and target!
            uniqueMessages[hash] = std::move(incomingMessage);
        }
        
        for (auto& [hash, message] : uniqueMessages) {
            
            if (messageListeners.find(message.target) == messageListeners.end()) continue;
            
            for (auto it = messageListeners.at(message.target).begin(); it != messageListeners.at(message.target).end(); ++it) {
                if(it->wasObjectDeleted()) continue;
                
                auto listener = it->get();
                
                auto heapAtoms = pd::Atom::fromAtoms(message.size, message.data);
                auto symbol = message.symbol ? String::fromUTF8(message.symbol->s_name) : String();
                
                if (listener)
                    listener->receiveMessage(symbol, heapAtoms);
                else
                    nullListeners.push_back({message.target, it});
            }
        }
        
        for (int i = nullListeners.size() - 1; i >= 0; i--) {
            auto& [target, iterator] = nullListeners[i];
            messageListeners[target].erase(iterator);
        }
    }
    
    CriticalSection messageListenerLock;
    std::map<void*, std::set<juce::WeakReference<MessageListener>>> messageListeners;
    moodycamel::ReaderWriterQueue<Message> messageQueue = moodycamel::ReaderWriterQueue<Message>(32768);
};
        

}
