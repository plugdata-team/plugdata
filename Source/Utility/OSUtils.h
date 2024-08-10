/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <string>
#include "Hash.h"

namespace juce
{
class ComponentPeer;
}

struct OSUtils {
    enum KeyboardLayout {
        QWERTY,
        AZERTY
        /* QWERTZ */
    };

    static unsigned int keycodeToHID(unsigned int scancode);

#if defined(_WIN32) || defined(_WIN64)
    static void createJunction(std::string from, std::string to);
    static void createHardLink(std::string from, std::string to);
    static bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);
    static void useWindowsNativeDecorations(void* windowHandle, bool rounded);
#elif defined(__unix__) && !defined(__APPLE__)
    static void maximiseX11Window(void* handle, bool shouldBeMaximised);
    static bool isX11WindowMaximised(void* handle);
    static void updateX11Constraints(void* handle);
#elif JUCE_MAC
    static void enableInsetTitlebarButtons(void* nativeHandle, bool enabled);
    static void HideTitlebarButtons(void* view, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton);
#endif

    static juce::Array<juce::File> iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles, int maximum = -1);
    static bool isDirectoryFast(juce::String const& path);
    static hash32 getUniqueFileHash(juce::String const& path);

    static KeyboardLayout getKeyboardLayout();

#if JUCE_MAC || JUCE_IOS
    static float MTLGetPixelScale(void* view);
    static void* MTLCreateView(void* parent, int x, int y, int width, int height);
    static void MTLDeleteView(void* view);
#endif
#if JUCE_MAC
    class ScrollTracker {
    public:
        ScrollTracker();

        ~ScrollTracker();

        static ScrollTracker* create()
        {
            return new ScrollTracker();
        }

        static bool isScrolling()
        {
            return instance->scrolling;
        }

    private:
        bool scrolling = false;
        void* observer;
        static inline ScrollTracker* instance = create();
    };
#elif JUCE_IOS
    class ScrollTracker {
    public:
        ScrollTracker(juce::ComponentPeer* peer);

        ~ScrollTracker();

        static ScrollTracker* create(juce::ComponentPeer* peer)
        {
            if (instance)
                return instance;

            return instance = new ScrollTracker(peer);
        }

        static bool isScrolling()
        {
            return instance->scrolling;
        }

    private:
        bool scrolling = false;
        void* observer;
        static inline ScrollTracker* instance = nullptr;
    };

    static juce::BorderSize<int> getSafeAreaInsets();
    static bool isIPad();
    static void showMobileMainMenu(juce::ComponentPeer* peer, std::function<void(int)> callback);
    static void showMobileCanvasMenu(juce::ComponentPeer* peer, std::function<void(int)> callback);

#endif
};
