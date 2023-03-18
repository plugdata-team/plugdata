/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

// #include "Canvas.h"
#include "PluginEditor.h"
//#include "Standalone/PlugDataWindow.h"

class SecondaryWindow : public DocumentWindow
    , public Button::Listener {
public:
    SecondaryWindow(Canvas* cnv)
        : DocumentWindow (cnv->getTitle(),
                    findColour(PlugDataColour::toolbarBackgroundColourId),
                    0,
                    false)
        , cnv(cnv)
        , originalOwner(cnv->getParentComponent())
        , editor(cnv->editor)
        , window(dynamic_cast<ResizableWindow*>(editor->getParentComponent()->getParentComponent()))
    {
#if PLUGDATA_STANDALONE


#endif

        //
        // setTitleBarHeight(0);
        // 
        
        //cnv->setBounds(getBounds());

        closeButton.setButtonText("Show Editor"); // set the button text
        
        closeButton.addListener(this);      // add listener for button clicks
        
       /*  closeButton.setColour(closeButton.buttonColourId, findColour(PlugDataColour::toolbarBackgroundColourId));
        closeButton.setColour(closeButton.buttonOnColourId, findColour(PlugDataColour::toolbarHoverColourId));
        closeButton.setColour(closeButton.textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
        closeButton.setColour(closeButton.textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
        closeButton.setAlpha(0.5f); */

        editor->removeKeyListener(dynamic_cast<KeyListener*>(cnv));
        editor->setVisible(false);
        window->setResizeLimits(11, 11, 10000, 10000);

            window->setSize(cnv->patchWidth.getValue(), cnv->patchHeight.getValue());
        window->setResizable(false, false);
        editor->zoomScale = 1.0f;

        //window->addAndMakeVisible(closeButton);     // add the button to the window

        
        setSize(cnv->patchWidth.getValue(), cnv->patchHeight.getValue());
         setContentOwned(new Component(), false);
         getContentComponent()->addAndMakeVisible(cnv);
        //addAndMakeVisible(closeButton);
        cnv->setBounds(editor->getX(), editor->getY(), getWidth(), getHeight());

        editor->getParentComponent()->addAndMakeVisible(this);
        editor->getParentComponent()->setBounds(0, 0, cnv->patchWidth.getValue(), cnv->patchHeight.getValue());
        cnv->locked = true;
        cnv->presentationMode = true;

       // cnv->setBounds(window->getBounds());
        cnv->resized();
        window->repaint();
    }

    void buttonClicked(Button* button) override
    {
        if (button == &closeButton) {
            editor->setVisible(true);
            originalOwner->addAndMakeVisible(cnv);
            window->setResizable(true, false);
          //  editor->addModifierKeyListener(editor);
            // setVisible(false);
            delete this;
        }
    }

    void resized() override
    {
        // position the close button in the top-right corner of the window
        closeButton.setBounds(10, 10, 70, 25);
        // cnv->setBounds(0, 0, getWidth(), getHeight() - 40); // set the canvas size
    }

private:
    Component* originalOwner;
    Canvas* cnv;
    PluginEditor* editor;
    TextButton closeButton;
    ResizableWindow* window;
};