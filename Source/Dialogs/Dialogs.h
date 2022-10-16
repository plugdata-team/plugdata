/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

class PlugDataPluginEditor;

struct Dialog : public Component {

    Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int childWidth, int childHeight, int yPosition, bool showCloseButton)
        : parentComponent(editor)
        , height(childHeight)
        , width(childWidth)
        , y(yPosition)
        , owner(ownerPtr)
    {
        parentComponent->addAndMakeVisible(this);
        setBounds(0, 0, parentComponent->getWidth(), parentComponent->getHeight());

        setAlwaysOnTop(true);

        if (showCloseButton) {
            closeButton.reset(getLookAndFeel().createDocumentWindowButton(4));
            addAndMakeVisible(closeButton.get());
            closeButton->onClick = [this]() {
                closeDialog();
            };
            closeButton->setAlwaysOnTop(true);
        }
    }

    void setViewedComponent(Component* child)
    {
        viewedComponent.reset(child);
        addAndMakeVisible(child);
        resized();
    }

    Component* getViewedComponent()
    {
        return viewedComponent.get();
    }

    void paint(Graphics& g)
    {
        g.setColour(Colours::black.withAlpha(0.5f));

#if PLUGDATA_STANDALONE
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
#else
        g.fillRect(getLocalBounds());
#endif
        if (viewedComponent) {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(viewedComponent->getBounds().toFloat(), 5.0f);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(viewedComponent->getBounds().toFloat(), 5.0f, 1.0f);
        }
    }

    void resized()
    {
        if (viewedComponent) {
            viewedComponent->setSize(width, height);
            viewedComponent->setCentrePosition({ getBounds().getCentreX(), y - (height / 2) });
        }

        if (closeButton) {
            closeButton->setBounds(viewedComponent->getRight() - 35, viewedComponent->getY() + 8, 28, 28);
        }
    }

    void mouseDown(MouseEvent const& e)
    {
        closeDialog();
    }

    void closeDialog()
    {
        owner->reset(nullptr);
    }

    int height, width, y;

    Component* parentComponent;

    std::unique_ptr<Component> viewedComponent = nullptr;
    std::unique_ptr<Button> closeButton = nullptr;
    std::unique_ptr<Dialog>* owner;
};

struct Dialogs {
    static Component* showTextEditorDialog(String text, String filename, std::function<void(String, bool)> callback);

    static void showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String filename, std::function<void(int)> callback);
    static void showArrayDialog(std::unique_ptr<Dialog>* target, Component* centre, std::function<void(int, String, String)> callback);

    static void createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, Component* centre, ValueTree const& settingsTree);

    static void showObjectMenu(PlugDataPluginEditor* parent, Component* target);
};


struct DekenInterface
{
    static StringArray getExternalPaths();
};
