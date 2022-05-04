/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

class PlugDataPluginEditor;

struct Dialog : public Component
{
    Dialog(Component* editor, int childWidth, int childHeight, int yPosition, bool showCloseButton) : parentComponent(editor->getParentComponent()), height(childHeight), width(childWidth), y(yPosition)
    {
        parentComponent->addAndMakeVisible(this);
        setBounds(0, 0, parentComponent->getWidth(), parentComponent->getHeight());

        setAlwaysOnTop(true);

        if (showCloseButton)
        {
            closeButton.reset(getLookAndFeel().createDocumentWindowButton(4));
            addAndMakeVisible(closeButton.get());
            closeButton->onClick = [this]() { onClose(); };
            closeButton->setAlwaysOnTop(true);
        }
    }

    void setViewedComponent(Component* child)
    {
        viewedComponent.reset(child);
        addAndMakeVisible(child);
        resized();
    }

    void paint(Graphics& g)
    {
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

        if (viewedComponent)
        {
            g.setColour(findColour(PlugDataColour::toolbarColourId));
            g.fillRoundedRectangle(viewedComponent->getBounds().toFloat(), 5.0f);

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.drawRoundedRectangle(viewedComponent->getBounds().toFloat(), 5.0f, 1.0f);
        }
    }

    void resized()
    {
        if (viewedComponent)
        {
            viewedComponent->setSize(width, height);
            viewedComponent->setCentrePosition({getBounds().getCentreX(), y - (height / 2)});
        }

        if (closeButton)
        {
            closeButton->setBounds(viewedComponent->getRight() - 35, viewedComponent->getY() + 8, 28, 28);
        }
    }

    void mouseDown(const MouseEvent& e)
    {
        onClose();
    }

    int height, width, y;

    Component* parentComponent;

    std::function<void()> onClose = [this]() { delete this; };

    std::unique_ptr<Component> viewedComponent = nullptr;
    std::unique_ptr<Button> closeButton = nullptr;
};

struct Dialogs
{
    Dialogs();

    static void showSaveDialog(Component* centre, String filename, std::function<void(int)> callback);
    static void showArrayDialog(Component* centre, std::function<void(int, String, String)> callback);

    static Component* createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree);

    static void showObjectMenu(PlugDataPluginEditor* parent, Component* target);
};
