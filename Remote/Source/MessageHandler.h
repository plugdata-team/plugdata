#pragma once

#include <JuceHeader.h>
#include "../ipc/boost/interprocess/ipc/message_queue.hpp"

class MessageHandler
{
    
public:
    
    enum MessageType
    {
        tGlobal,
        tGUI,
        tObject
    };
    
    MessageHandler(String ID)
    {
        send_queue = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, (ID + "_send").toRawUTF8());
        
        receive_queue = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, (ID + "_receive").toRawUTF8());
        
    }
    
    void sendMessageToObject(String ID, String type, String message) {
        MemoryOutputStream ostream;
        ostream.writeInt(tObject);
        ostream.writeString(ID);
        ostream.writeString(type);
        ostream.writeString(message);
        
        sendMessage(ostream.getMemoryBlock());
    }
    
    void sendMessageToObject(String ID, String type, float value) {
        MemoryOutputStream ostream;
        ostream.writeInt(tObject);
        ostream.writeString(ID);
        ostream.writeString(type);
        ostream.writeFloat(value);
        
        sendMessage(ostream.getMemoryBlock());
    }
    
    void sendMessageToObject(String ID, String type, int value) {
        MemoryOutputStream ostream;
        ostream.writeInt(tObject);
        ostream.writeString(ID);
        ostream.writeString(type);
        ostream.writeInt(value);
        
        sendMessage(ostream.getMemoryBlock());
    }
    
    
    void sendMessageToObject(String ID, String type, std::vector<float> values) {
        MemoryOutputStream ostream;
        ostream.writeInt(tObject);
        ostream.writeString(ID);
        ostream.writeString(type);
        ostream.writeInt(values.size());
        for(auto& value : values) {
            ostream.writeFloat(value);
        }
       
        sendMessage(ostream.getMemoryBlock());
    }
    
    void sendMessage(MemoryBlock block)
    {
        send_queue->send(block.getData(), block.getSize(), 1);
    }
    
    bool receiveMessages(MemoryBlock& message) {
        int bufsize = 10000;
        void* buffer = malloc(bufsize);
        size_t size;
        unsigned int priority;
        bool status = receive_queue->try_receive(buffer, bufsize, size, priority);
        
        if(status) message = MemoryBlock(buffer, bufsize);
        
        return status;
    }

    std::unique_ptr<boost::interprocess::message_queue> send_queue;
    std::unique_ptr<boost::interprocess::message_queue> receive_queue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MessageHandler)
};
