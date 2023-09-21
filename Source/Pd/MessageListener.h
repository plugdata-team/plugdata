/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Instance.h"

namespace pd {

struct MessageListener {
    virtual void receiveMessage(String const& symbol, std::vector<pd::Atom> const& atoms) {};

    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener);
};

}
