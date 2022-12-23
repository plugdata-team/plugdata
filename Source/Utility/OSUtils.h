/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

void createJunction(std::string from, std::string to);
void createHardLink(std::string from, std::string to);

#if JUCE_WINDOWS
bool runAsAdmin(std::string file, std::string lpParameters, void* hWnd);
#else

#endif
