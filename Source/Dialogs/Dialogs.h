/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Constants.h"
#include "Utility/SettingsFile.h"
#include "Utility/WindowDragger.h"
#include "PluginEditor.h"

class Canvas;

class Dialog : public Component
    , public ComponentListener {

public:
    Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int childWidth, int childHeight, bool showCloseButton, int margin = 0);

    ~Dialog() override
    {
        parentComponent->removeComponentListener(this);

        if (auto* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent())) {
            if (ProjectInfo::isStandalone) {
                if (auto* closeButton = window->getCloseButton())
                    closeButton->setEnabled(true);
                if (auto* minimiseButton = window->getMinimiseButton())
                    minimiseButton->setEnabled(true);
                if (auto* maximiseButton = window->getMaximiseButton())
                    maximiseButton->setEnabled(true);
            }
        }
    }

    void componentMovedOrResized(Component& comp, bool wasMoved, bool wasResized) override
    {
        setBounds(parentComponent->getScreenX(), parentComponent->getScreenY(), parentComponent->getWidth(), parentComponent->getHeight());
    }

    void setViewedComponent(Component* child)
    {
        viewedComponent.reset(child);
        viewedComponent->addMouseListener(this, false);
        addAndMakeVisible(child);
        resized();
    }

    bool wantsRoundedCorners() const;

    void paint(Graphics& g) override
    {
        if (!ProjectInfo::canUseSemiTransparentWindows()) {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        } else {
            g.setColour(Colours::black.withAlpha(0.5f));
        }

        auto bounds = getLocalBounds().toFloat().reduced(backgroundMargin);

        if (wantsRoundedCorners()) {
            g.fillRoundedRectangle(bounds.toFloat(), Corners::windowCornerRadius);
        } else {
            g.fillRect(bounds);
        }

        if (viewedComponent) {
            g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
            g.fillRoundedRectangle(viewedComponent->getBounds().toFloat(), Corners::windowCornerRadius);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(viewedComponent->getBounds().toFloat(), Corners::windowCornerRadius, 1.0f);
        }
    }

    void parentSizeChanged() override
    {
        if (auto* parent = getParentComponent()) {
            setBounds(parent->getLocalBounds());
        }
    }

    void resized() override
    {
        if (viewedComponent) {
#if JUCE_IOS
            viewedComponent->setBounds(0, 0, getWidth(), getHeight());
#else
            viewedComponent->setSize(width, height);
            viewedComponent->setCentrePosition({ getLocalBounds().getCentreX(), getLocalBounds().getCentreY() });
#endif
        }

        if (closeButton) {
            auto closeButtonBounds = Rectangle<int>(viewedComponent->getRight() - 35, viewedComponent->getY() + 8, 28, 28);
            closeButton->setBounds(closeButtonBounds);
        }
    }

#if !JUCE_IOS
    void mouseDown(MouseEvent const& e) override
    {
        if (!hasKeyboardFocus(false)) {
            parentComponent->toFront(false);
            toFront(true);
        }

        if (isPositiveAndBelow(e.getEventRelativeTo(viewedComponent.get()).getMouseDownY(), 40) && ProjectInfo::isStandalone) {
            dragger.startDraggingWindow(parentComponent->getTopLevelComponent(), e);
            dragging = true;
        } else if (!viewedComponent->getBounds().contains(e.getEventRelativeTo(this).getPosition()) && !blockCloseAction) {
            parentComponent->toFront(true);
            closeDialog();
        }
    }

    void mouseDrag(MouseEvent const& e) override;

    void mouseUp(MouseEvent const& e) override
    {
        dragging = false;
    }
#endif

    void setBlockFromClosing(bool block)
    {
        blockCloseAction = block;
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress::escapeKey) {
            closeDialog();
            return true;
        }

        return false;
    }

    void closeDialog()
    {
        owner->reset(nullptr);
    }

    int height, width;

    Component* parentComponent;
    WindowDragger dragger;

    std::unique_ptr<Component> viewedComponent = nullptr;
    std::unique_ptr<Button> closeButton = nullptr;
    std::unique_ptr<Dialog>* owner;

    bool blockCloseAction = false;
    bool dragging = false;
    int backgroundMargin = 0;
};

struct Dialogs {
    static Component* showTextEditorDialog(String const& text, String filename, std::function<void(String, bool)> callback);
    static void appendTextToTextEditorDialog(Component* dialog, String const& text);

    static void showAskToSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String const& filename, std::function<void(int)> callback, int margin = 0, bool withLogo = true);

    static void showSettingsDialog(PluginEditor* editor);

    static void showMainMenu(PluginEditor* editor, Component* centre);

    static void showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> const& callback, StringArray const& options = { "Okay", "Cancel " });

    static void showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent);

    static void showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent);
    static void showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& objectName);

    static void showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position);

    static void showObjectMenu(PluginEditor* parent, Component* target);

    static void showDeken(PluginEditor* editor);

    static void dismissFileDialog();

    static void showOpenDialog(std::function<void(URL)> const& callback, bool canSelectFiles, bool canSelectDirectories, String const& lastFileId, String const& extension, Component* parentComponent);

    static void showSaveDialog(std::function<void(URL)> const& callback, String const& extension, String const& lastFileId, Component* parentComponent = nullptr, bool directoryMode = false);

    static inline std::unique_ptr<FileChooser> fileChooser = nullptr;
};

struct DekenInterface {
    static StringArray getExternalPaths();
};
