// Copyright (c) 2024 Timothy Schoen
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

// Lock-free multithread (single consumer/single producer) stack implementation
// Before you start popping values, you need to call swapBuffers(). Other than that, push/pop like a regular stack implementation


#pragma once
#include <atomic>
#include <iostream>
#include <string>
#include <mutex>

template <typename T, int stackSize>
class ThreadSafeStack {
private:
    struct Buffer {
        T data[stackSize];
        int index = 0;
    };
    
    Buffer buffers[2];

    Buffer* front_;
    Buffer* back_;
    std::atomic<bool> empty = true;
    std::mutex swapLock;

public:
    ThreadSafeStack() {
        front_ = &buffers[0];
        back_ = &buffers[1];
    }

    bool isEmpty()
    {
        return empty.load(std::memory_order_relaxed);
    }
    
    // Swap front and back buffers
    void swapBuffers()
    {
        swapLock.lock();
        back_ = std::exchange(front_, back_);
        swapLock.unlock();
        empty = true;
    }

    void push(const T& value) {
        swapLock.lock();
        Buffer* current_back = back_;
        int current_index = current_back->index;

        // Check if the current back buffer is full
        while(current_index >= stackSize) {
            current_index -= stackSize;
        }

        // Perform the push
        current_back->data[current_index] = value;
        current_back->index = current_index + 1;
        swapLock.unlock();
        empty.store(false, std::memory_order_relaxed);
    }

     bool pop(T& result) {
         if(front_->index == 0) {
             return false;
         }

        // Perform the pop
        int current_index = (front_->index - 1) % stackSize;
        T* element = &front_->data[current_index];
        // Decrement the index
        front_->index = current_index;

        result = *element;
        return true;
    }
};
