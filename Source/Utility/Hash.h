/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

using hash32 = uint32_t;
#define EMPTY_HASH ((hash32)0x811c9dc5)

/**
 *  FNV-1a hash function, compile and run time: https://gist.github.com/ctmatthews/c1cbd1b2e68c29a88b236475a0b26cd0
 */
constexpr hash32 hash(char const* str)
{
    hash32 result = EMPTY_HASH;

    if (!str)
        return result;

    while (*str) {
        result ^= (hash32)*str;       // NOTE: make this toupper(*s) or tolower(*s) if you want case-insensitive hashes
        result *= (hash32)0x01000193; // 32 bit magic FNV-1a prime
        str++;
    }

    return result;
}

/**
 * FNV-1a hash function, for juce::String, only at run time
 */
inline hash32 hash(juce::String const& str)
{
    return hash(str.toRawUTF8());
}
