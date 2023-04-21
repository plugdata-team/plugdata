/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Utility/HashUtils.h"

namespace pd {

struct MessageListener {
    virtual void receiveMessage(t_symbol* symbol, int argc, t_atom* argv) {};

    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener);
};

}
