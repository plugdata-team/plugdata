#pragma once

typedef uint32_t hash32;
typedef uint8_t u8;
#define EMPTY_HASH ((hash32)0x811c9dc5)

/**
 *  FNV-1a hash function, compile and run time: https://gist.github.com/ctmatthews/c1cbd1b2e68c29a88b236475a0b26cd0
 */
constexpr hash32 hash(char const* str)
{
    hash32 result = EMPTY_HASH;

    while (*str) {
        result ^= (hash32)*str;       // NOTE: make this toupper(*s) or tolower(*s) if you want case-insensitive hashes
        result *= (hash32)0x01000193; // 32 bit magic FNV-1a prime
        str++;
    }
    return  result;
}

/**
 * FNV-1a hash function, for juce::String, only at run time
 */
inline hash32 hash(juce::String const& str)
{
    return hash(str.toUTF8().getAddress());
}
