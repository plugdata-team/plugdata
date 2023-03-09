/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

struct OSUtils
{
    enum KeyboardLayout
    {
        QWERTY,
        AZERTY
        /* QWERTZ */
    };
    
#if JUCE_WINDOWS
    
    static void createJunction(std::string from, std::string to);
    static void createHardLink(std::string from, std::string to);
    static bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);
    
#elif JUCE_MAC
    static void enableInsetTitlebarButtons(void* nativeHandle, bool enabled);
    
#else // Linux or BSD
    static void maximiseLinuxWindow(void* handle);
    static bool isMaximised(void* handle);
#endif
    
    static KeyboardLayout getKeyboardLayout();
};

