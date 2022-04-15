/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

class PlugDataPluginEditor;

struct Dialogs
{
    static void showSaveDialog(Component* centre, std::function<void(int)> callback);
    static void showArrayDialog(Component* centre, std::function<void(int, String, String)> callback);

    static Component* createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree);

    static void showObjectMenu(PlugDataPluginEditor* parent, Component* target, const std::function<void(String)>& cb);
};
