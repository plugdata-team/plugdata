/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"

template<typename T>
class CircularBuffer {
public:
    /**
     * Creates a circular buffer of size n.
     * @param n The buffer size. Must be a power of 2.
     * @param value The value to fill the buffer with.
     */
    explicit CircularBuffer<T>(size_t n, T value = T()) :
            size(n), data(n, value), i(0) {
        // ensure the buffer size is a power of 2
        jassert(n == nextPowerOfTwo(n));
    }

    /**
     * Returns a reference to the buffer element at the given index.
     * @param x the index
     * @return a reference to a buffer element
     */
    T &operator()(size_t x) {
        jassert(x >= 0 && x < size);
        return data[mask(i + x)];
    }

    /**
     * Returns a copy of the buffer element at the given index.
     * @param x the index
     * @return a copy of a buffer element
     */
    T operator()(size_t x) const {
        jassert(x >= 0 && x < size);
        return data[mask(i + x)];
    }

    /**
     * Returns a reference to the buffer element at the given index,
     * starting from the end of the buffer.
     * @param x the index
     * @return a reference to a buffer element
     */
    T &operator[](size_t x) {
        return operator()(size - x - 1);
    }

    /**
     * Returns a copy of the buffer element at the given index,
     * starting from the end of the buffer.
     * @param x the index
     * @return a copy of a buffer element
     */
    T operator[](size_t x) const {
        return operator()(size - x - 1);
    }

    
    T last() const
    {
        return data[mask(i)];
    }

    /**
     * Inserts an element at the front of the buffer,
     * shifting out the last element.
     * @param element the element to push into the buffer
     * @return The element that was shifted out of the buffer.
     */
    T shift(T element) {
        auto pushed = operator()(0);
        push(element);
        return pushed;
    }

    /**
     * Inserts an element at the front of the buffer,
     * shifting out the last element.
     * @param element the element to push into the buffer
     */
    void push(T element) {
        data[mask(i++)] = element;
    }

    /**
     * Replaces every element in the buffer with the type's default value.
     */
    void clear() {
        fill(T());
    }

    /**
     * Replaces every element in the buffer with the given value.
     * @param value The value to fill.
     */
    void fill(T value) {
        std::fill(data.begin(), data.end(), value);
    }

    /**
     * The size of the buffer.
     */
    
    size_t length()
    {
        return size;
    }


private:
    
    const size_t size;
    
    /**
     * Vector containing the underlying data.
     */
    std::vector<T> data;

    /**
     * index of the current first element of the circular buffer.
     */
    size_t i;

    size_t mask(size_t val) const {
        return val & (size - 1);
    }
};
