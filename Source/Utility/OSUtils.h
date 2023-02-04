/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#if JUCE_WINDOWS

void createJunction(std::string from, std::string to);
void createHardLink(std::string from, std::string to);
bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);

#elif JUCE_MAC

void enableInsetTitlebarButtons(void* nativeHandle, bool enabled);

#elif JUCE_LINUX

void maximiseLinuxWindow(void* handle);
bool isMaximised(void* handle);

#endif
