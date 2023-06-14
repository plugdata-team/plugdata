/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Ofelia.h"

#if defined(_WIN32)
    #define DLL_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
    #define DLL_EXPORT __attribute__((visibility("default")))
#else
    #define DLL_EXPORT
#endif



DLL_EXPORT bool ofxOfeliaExecutableFound()
{
   return pd::Ofelia::ofeliaExecutable.existsAsFile();
}
