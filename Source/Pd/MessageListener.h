/*
 // Copyright (c) 2022-2023 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

namespace pd {

class MessageListener {
    
public:
    virtual void receiveMessage(String const& name, int argc, t_atom* argv) = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(MessageListener)
};

}
