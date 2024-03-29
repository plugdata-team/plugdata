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

#if __x86_64
    #define cpu_relax() asm volatile ("pause" ::: "memory");
#elif  __arm__
    #define cpu_relax() asm volatile ("yield" ::: "memory");
#elif  __aarch64__
    #define cpu_relax() asm volatile ("wfe" ::: "memory");
#else
    #warning No known relax instruction for your CPU!
    #define cpu_relax()
#endif

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


template <typename T, int stackSize>
class LockFreeStack {
private:
    struct Buffer {
        std::atomic<int> index;
        T data[stackSize];
    };

    std::atomic<Buffer*> front_;
    std::atomic<Buffer*> back_;

public:
    LockFreeStack() {
        front_ = new Buffer();
        front_.load()->index.store(0);

        back_ = new Buffer();
        back_.load()->index.store(0);
    }

    ~LockFreeStack() {
        delete front_.load();
        delete back_.load();
    }

    int numElements()
    {
        return back_.load(std::memory_order_relaxed)->index;
    }
    
    // Swap front and back buffers
    void swapBuffers()
    {
        
        /* TODO: I think this is a better way to do it, but it doesn't work yet
         Buffer* current_front = front_.load(std::memory_order_seq_cst);
         Buffer* current_back = back_.load(std::memory_order_seq_cst);

        while(!front_.compare_exchange_strong(current_front, current_back, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
            // wait a little before a retry -- preserving CPU resources
            jassertfalse;
            cpu_relax();
        } */
        
        back_.store(front_.exchange(back_.load(std::memory_order_relaxed), std::memory_order_relaxed));
    }

    void push(const T& value) {
        Buffer* current_back = back_.load(std::memory_order_relaxed);
        int current_index = current_back->index.load(std::memory_order_relaxed);

        // Check if the current back buffer is full
        if (current_index >= stackSize) {
            current_back->index.store(0, std::memory_order_release);
            current_index = 0;
        }

        // Perform the push
        current_back->data[current_index] = value;
        current_back->index.store(current_index + 1, std::memory_order_release);
    }

     bool pop(T& result) {
        Buffer* current_front = front_.load(std::memory_order_relaxed);

        if(current_front->index.load(std::memory_order_relaxed) == 0) return false;

        // Perform the pop
        int current_index = current_front->index.load(std::memory_order_relaxed) - 1;
        T* element = &current_front->data[current_index];
        // Decrement the index
        current_front->index.store(current_index, std::memory_order_release);

        result = *element;
        return true;
    }
};
