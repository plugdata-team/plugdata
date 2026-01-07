/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <string>
#include "Containers.h"
#include "Hash.h"

namespace juce {
class ComponentPeer;
}

struct OSUtils {
    enum KeyboardLayout {
        QWERTY,
        AZERTY
        /* QWERTZ */
    };

    static unsigned int keycodeToHID(unsigned int scancode);
    static void* getDesktopParentPeer(juce::Component* component);

#if defined(_WIN32) || defined(_WIN64)
    static bool createJunction(std::string from, std::string to);
    static bool createHardLink(std::string from, std::string to);
    static bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);
    static void useWindowsNativeDecorations(void* windowHandle, bool rounded);
#elif defined(__unix__) && !defined(__APPLE__)
    static void maximiseX11Window(void* handle, bool shouldBeMaximised);
    static bool isX11WindowMaximised(void* handle);
    static void updateX11Constraints(void* handle);
#elif JUCE_MAC
    static void setWindowMovable(void* nativeHandle, bool canMove);
    static void enableInsetTitlebarButtons(void* nativeHandle, bool enabled);
    static void hideTitlebarButtons(void* view, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton);
#endif

    static SmallArray<juce::File> iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles, int maximum = -1);
    static bool isDirectoryFast(juce::String const& path);
    static bool isFileFast(juce::String const& path);
    static hash32 getUniqueFileHash(juce::String const& path);

    static KeyboardLayout getKeyboardLayout();

    static bool is24HourTimeFormat();

#if JUCE_MAC || JUCE_IOS
    static float MTLGetPixelScale(void* view);
    static void* MTLCreateView(void* parent, int x, int y, int width, int height);
    static void MTLDeleteView(void* view);
    static void MTLSetVisible(void* view, bool shouldBeVisible);
#endif
#if JUCE_MAC
    class ScrollTracker {
    public:
        ScrollTracker();

        ~ScrollTracker();

        static std::unique_ptr<ScrollTracker> create()
        {
            return std::make_unique<ScrollTracker>();
        }

        static bool isScrolling()
        {
            return instance->scrolling;
        }

    private:
        bool scrolling = false;
        void* observer;
        static inline std::unique_ptr<ScrollTracker> instance = create();
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
            
            if(!peer->getComponent().isVisible())
                return nullptr;
            
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
    static float getScreenCornerRadius();
    static void showMobileMainMenu(juce::ComponentPeer* peer, std::function<void(int)> callback);
    static void showMobileCanvasMenu(juce::ComponentPeer* peer, std::function<void(int)> callback);
    static bool addOpenURLMethodToDelegate();
#endif
};
