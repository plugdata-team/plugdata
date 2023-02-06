#pragma once

#include <JuceHeader.h>

typedef uint32_t hash32;
typedef uint8_t u8;
#define EMPTY_HASH ((hash32)0x811c9dc5)

/** 
 *  FNV-1a hash function, compile and run time: https://gist.github.com/ctmatthews/c1cbd1b2e68c29a88b236475a0b26cd0
 */ 
constexpr hash32 hash(const char* str) 
{
    hash32 result = EMPTY_HASH;

    while (*str) {
        result ^= (hash32) *str; // NOTE: make this toupper(*s) or tolower(*s) if you want case-insensitive hashes
        result *= (hash32) 0x01000193; // 32 bit magic FNV-1a prime
        str++;
    }

    return result;
}

/**
 * FNV-1a hash function, for juce::String, only at run time
 */
inline hash32 hash(const juce::String& str) 
{
    return hash(str.toUTF8().getAddress());
}

enum objectMessage {
    msg_float = hash("float"),
    msg_bang = hash("bang"),
    msg_list = hash("list"),
    msg_set = hash("set"),
    msg_append = hash("append"),
    msg_symbol = hash("symbol"),
    msg_flashtime = hash("flashtime"),
    msg_bgcolor = hash("bgcolor"),
    msg_fgcolor = hash("fgcolor"),
    msg_color = hash("color"),
    msg_vis = hash("vis"),
    msg_send = hash("send"),
    msg_receive = hash("receive"),
    msg_min = hash("min"),
    msg_max = hash("max"),
    msg_coords = hash("coords"),
    msg_lowc = hash("lowc"),
    msg_oct = hash("oct"),
    msg_8ves = hash("8ves"),
    msg_open = hash("open"),
    msg_orientation = hash("orientation"),
    msg_number = hash("number"),
    msg_lin = hash("lin"),
    msg_log = hash("log"),
    msg_range = hash("range"),
    msg_steady = hash("steady"),
    msg_nonzero = hash("nonzero"),
    msg_label = hash("label"),
    msg_label_pos = hash("label_pos"),
    msg_label_font = hash("label_font"),
    msg_vis_size = hash("vis_size"),
    msg_init = hash("init"),
    msg_allpass = hash("allpass"),
    msg_lowpass = hash("lowpass"),
    msg_highpass = hash("highpass"),
    msg_bandpass = hash("bandpass"),
    msg_resonant = hash("resonant"),
    msg_bandstop = hash("bandstop"),
    msg_eq = hash("eq"),
    msg_lowshelf = hash("lowshelf"),
    msg_highshelf = hash("highshelf")
};
