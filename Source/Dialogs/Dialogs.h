/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

class PlugDataPluginEditor;

struct BlackoutComponent : public Component
{
    Component* parent;
    Component* dialog;

    std::function<void()> onClose;

    BlackoutComponent(
        Component* p, Component* d, std::function<void()> closeCallback = []() {})
        : parent(p->getTopLevelComponent()),
          dialog(d),
          onClose(closeCallback)
    {
        parent->addAndMakeVisible(this);
        // toBehind(dialog);
        setAlwaysOnTop(true);
        dialog->setAlwaysOnTop(true);

        resized();
    }

    void paint(Graphics& g)
    {
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    }

    void resized()
    {
        setBounds(parent->getLocalBounds().reduced(4));
    }

    void mouseDown(const MouseEvent& e)
    {
        onClose();
    }
};

struct Dialogs : public Component
{
    Dialogs();

    static void showSaveDialog(Component* centre, String filename, std::function<void(int)> callback);
    static void showArrayDialog(Component* centre, std::function<void(int, String, String)> callback);

    static Component::SafePointer<Component> createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree);

    static void showObjectMenu(PlugDataPluginEditor* parent, Component* target);
};
