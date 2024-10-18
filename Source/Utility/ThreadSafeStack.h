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
#include <plf_stack/plf_stack.h>

template<typename T, int stackSize>
class ThreadSafeStack {

    using StackBuffer = plf::stack<T>;
    StackBuffer buffers[2];
    StackBuffer* frontBuffer;
    StackBuffer* backBuffer;
    std::mutex swapLock;

public:
    ThreadSafeStack()
    {
        frontBuffer = &buffers[0];
        backBuffer = &buffers[1];
        frontBuffer->reserve(stackSize);
        backBuffer->reserve(stackSize);
    }

    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(swapLock);
        return backBuffer->empty();
    }

    // Swap front and back buffers
    void swapBuffers()
    {
        std::lock_guard<std::mutex> lock(swapLock);
        backBuffer = std::exchange(frontBuffer, backBuffer);
#if JUCE_DEBUG
        jassert(backBuffer->empty());
#endif
    }

    void push(T const& value)
    {
        std::lock_guard<std::mutex> lock(swapLock);
        backBuffer->push(value);
    }

    bool pop(T& result)
    {
        if (frontBuffer->empty())
            return false;

        // no need for mutex because this is called from the same thread as swapBuffers()
        result = frontBuffer->top();
        frontBuffer->pop();
        return true;
    }
};
