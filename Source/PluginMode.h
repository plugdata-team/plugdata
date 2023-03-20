/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "PluginEditor.h"

class SecondaryWindow : public Component
    , public Button::Listener {
public:
    SecondaryWindow(Canvas* cnv)
        // : DocumentWindow("Test", findColour(PlugDataColour::textObjectBackgroundColourId), DocumentWindow::allButtons, false)
        : cnv(cnv)
        , editor(cnv->editor)
        , mainWindow(static_cast<ResizableWindow*>(editor->getTopLevelComponent()))
        , cnvParent(cnv->getParentComponent())
        , windowBounds(editor->getBounds())
        , windowConstrainer(editor->getConstrainer())
        , viewportBounds(cnv->viewport->getBounds())
    {

        auto width = cnv->patchWidth.getValue();
        auto height = cnv->patchHeight.getValue();
        editor->addAndMakeVisible(this);

        editor->setResizeLimits(width, height, 99999, 99999);
        editor->setResizable(false, false);
        editor->setSize(width, height);

        if (ProjectInfo::isStandalone) {
            mainWindow->setResizeLimits(width, height, 99999, 99999);
            mainWindow->setResizable(false, false);
            mainWindow->setSize(width, height);
        }

        closeButton.addListener(this);
        addAndMakeVisible(cnv);
        addAndMakeVisible(closeButton);
        setBounds(0, 0, width, height);

        editor->zoomScale = 1.0f;

        // cnv->viewport->setViewPosition(cnv->canvasOrigin);
        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());

        cnv->locked = true;
        cnv->presentationMode = true;

        closeButton.setButtonText(Icons::Edit);
        closeButton.setTooltip("Show Editor..");
        if (ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("macos_buttons")) {
            closeButton.setBounds(5, 5, 30, 30);
        } else {
            closeButton.setBounds(getWidth() - 75, 5, 30, 30);
        }
    }

    void buttonClicked(Button* button) override
    {
        if (button == &closeButton) {
            editor->setVisible(true);
            cnv->viewport->setBounds(viewportBounds);
            cnv->locked = false;
            cnv->presentationMode = false;
            cnvParent->addAndMakeVisible(cnv);
            cnv->resized();

            editor->setResizeLimits(windowConstrainer->getMinimumWidth(), windowConstrainer->getMinimumHeight(), windowConstrainer->getMaximumWidth(), windowConstrainer->getMaximumHeight());
            editor->setSize(windowBounds.getWidth(), windowBounds.getHeight());
            editor->setResizable(true, false);

            if (ProjectInfo::isStandalone) {
                mainWindow->setResizeLimits(windowConstrainer->getMinimumWidth(), windowConstrainer->getMinimumHeight(), windowConstrainer->getMaximumWidth(), windowConstrainer->getMaximumHeight());
                mainWindow->setSize(windowBounds.getWidth(), windowBounds.getHeight());
                mainWindow->setResizable(true, false);
            }

            delete this;
        }
    }

private:
    Canvas* cnv;
    PluginEditor* editor;
    TextButton closeButton;
    ResizableWindow* mainWindow;
    Component* cnvParent;
    ComponentBoundsConstrainer* windowConstrainer;
    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
};