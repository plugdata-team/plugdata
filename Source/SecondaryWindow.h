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
        , closeButton("Show Editor..")
    {

        auto width = cnv->patchWidth.getValue();
        auto height = cnv->patchHeight.getValue();
        //editor->setVisible(false);
        //mainWindow->getContentComponent()->addAndMakeVisible(this);
        editor->addAndMakeVisible(this);
        //mainWindow->setResizable(false, false);
        editor->setResizeLimits(width, height, 99999, 99999);
        editor->setResizable(false, false);
        // mainWindow->setResizeLimits(width, height, 99999, 99999);
        editor->setSize(width, height);

        closeButton.addListener(this);
        addAndMakeVisible(cnv);
        addAndMakeVisible(closeButton);
        setBounds(0, 0, width, height);
        

        editor->zoomScale = 1.0f;
        cnv->viewport->setViewPosition(cnv->canvasOrigin);

        cnv->viewport->setBounds(getBounds().withTrimmedRight(-cnv->viewport->getScrollBarThickness()).withTrimmedBottom(-cnv->viewport->getScrollBarThickness()));
        cnv->locked = true;
        cnv->presentationMode = true;

        closeButton.setBounds(getWidth()  - 75, 5, 70, 20);

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