/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "SettingsFile.h"

SettingsFileListener::SettingsFileListener() {
    SettingsFile::getInstance()->listeners.add(this);
}

SettingsFileListener::~SettingsFileListener() {
    SettingsFile::getInstance()->listeners.removeFirstMatchingValue(this);
}


JUCE_IMPLEMENT_SINGLETON(SettingsFile)

