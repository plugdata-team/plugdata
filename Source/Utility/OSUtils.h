/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <string>

struct OSUtils {
    enum KeyboardLayout {
        QWERTY,
        AZERTY
        /* QWERTZ */
    };

#if defined(_WIN32) || defined(_WIN64)
    static void createJunction(std::string from, std::string to);
    static void createHardLink(std::string from, std::string to);
    static bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);
#elif defined(__unix__) && !defined(__APPLE__)
    static void maximiseLinuxWindow(void* handle);
    static bool isMaximised(void* handle);
#elif JUCE_MAC
    static void enableInsetTitlebarButtons(void* nativeHandle, bool enabled);
    static void HideTitlebarButtons(void* view, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton);
#endif

    static juce::Array<juce::File> iterateDirectory(const juce::File& directory, bool recursive, bool onlyFiles);
    
    static KeyboardLayout getKeyboardLayout();
};
