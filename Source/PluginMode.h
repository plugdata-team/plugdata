/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "PluginEditor.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , mainWindow(static_cast<ResizableWindow*>(editor->getTopLevelComponent()))
        , cnvParent(cnv->getParentComponent())
        , windowBounds(editor->getBounds())
        , windowConstrainer(editor->getConstrainer())
        , viewportBounds(cnv->viewport->getBounds())
    {
        for (auto* child : editor->getChildren()) {
            if (child->isVisible()) {
                child->setVisible(false);
                children.emplace_back(child);
            }
        }

        editor->addAndMakeVisible(this);

        editor->setResizeLimits(width, height + titlebarHeight, 99999, 99999);
        editor->setResizable(false, false);
        editor->setSize(width, height + titlebarHeight);

        if (ProjectInfo::isStandalone) {
            mainWindow->setResizeLimits(width, height + titlebarHeight, 99999, 99999);
            mainWindow->setResizable(false, false);
            mainWindow->setSize(width, height + titlebarHeight);
        }

        closeButton.addListener(this);
        addAndMakeVisible(cnv);
        addAndMakeVisible(closeButton);
        setBounds(0, 0, width, height + titlebarHeight);
        cnv->toBack();

        editor->zoomScale = 1.0f;

        // cnv->viewport->setViewPosition(cnv->canvasOrigin);
        // cnv->setBounds(50, 50, 100, 100);
        cnv->setBounds(0, 50, width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness() + titlebarHeight);
        cnv->locked = true;
        cnv->presentationMode = true;

        closeButton.setButtonText(Icons::Edit);
        closeButton.setTooltip("Show Editor..");
        if (ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("macos_buttons")) {
            closeButton.setBounds(5, 5, 30, 30);
        } else {
            closeButton.setBounds(getWidth() - 75, 5, 30, 30);
        }

        repaint();
    }

    void buttonClicked(Button* button) override
    {
        if (button == &closeButton) {
            for (auto* child : children) {
                child->setVisible(true);
            }

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

    void paint(Graphics& g) override
    {
        auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);
        if (editor->wantsRoundedCorners()) {
            // Toolbar background
            g.setColour(baseColour);
            g.fillRect(0, 10, getWidth(), titlebarHeight - 9);
            g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), titlebarHeight, Corners::windowCornerRadius);
        } else {
            // Toolbar background
            g.setColour(baseColour);
            g.fillRect(0, 0, getWidth(), titlebarHeight);
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
    int width = cnv->patchWidth.getValue();
    int height = cnv->patchHeight.getValue();
    int titlebarHeight = 30;

    std::vector<Component*> children;
};