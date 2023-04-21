#pragma once

typedef unsigned int hash32;

constexpr hash32 hash(char const* str)
{
    unsigned int result = 5381;

    if (!str)
        return result;

    while (*str) {
        result = ((result << 5) + result) + *str;
        str++;
    }

    return result;
}

inline hash32 hash(String const& str)
{
    return hash(str.toUTF8().getAddress());
}
