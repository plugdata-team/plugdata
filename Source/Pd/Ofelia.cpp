/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Ofelia.h"

extern bool ofxOfeliaExecutableFound()
{
   return pd::Ofelia::ofeliaExecutable.existsAsFile();
}
