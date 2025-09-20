/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/OSUtils.h"

template<typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(size_t n, T value = T())
        : size(n)
        , data(n, value)
        , i(0)
    {
        jassert(n == nextPowerOfTwo(n));
    }

    T last() const
    {
        return data[mask(i - 1)];
    }

    // Get the last x elements since the last pushed value
    SmallArray<T> last(size_t const x) const
    {
        SmallArray<T> result;

        int const end = static_cast<int>(i) - 1;
        int const start = end - static_cast<int>(x);
        for (int j = start; j < end; j++) {
            result.add(data[mask(j)]);
        }

        return result;
    }

    void push(T element)
    {
        data[mask(i++)] = element;
    }

    size_t length() const
    {
        return size;
    }

private:
    size_t const size;
    HeapArray<T> data;
    size_t i;

    size_t mask(size_t const val) const
    {
        // val = static_cast<size_t>(val >= 0 ? val : val + static_cast<long long>(size));
        return val & (size - 1);
    }
};
