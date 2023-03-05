/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

class PluginEditor;
class Canvas;

class Dialog : public Component {

public:
    Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int childWidth, int childHeight, int yPosition, bool showCloseButton, int margin = 0)
        : parentComponent(editor)
        , height(childHeight)
        , width(childWidth)
        , y(yPosition)
        , owner(ownerPtr)
        , backgroundMargin(margin)
    {
        parentComponent->addAndMakeVisible(this);
        setBounds(0, 0, parentComponent->getWidth(), parentComponent->getHeight());

        setAlwaysOnTop(true);
        setWantsKeyboardFocus(true);

        grabKeyboardFocus();

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

    bool wantsRoundedCorners();

    void paint(Graphics& g) override
    {
        g.setColour(Colours::black.withAlpha(0.5f));

        auto bounds = getLocalBounds().toFloat().reduced(backgroundMargin);

        if (wantsRoundedCorners()) {
            g.fillRoundedRectangle(bounds.toFloat(), PlugDataLook::windowCornerRadius);
        } else {
            g.fillRect(bounds);
        }

        if (viewedComponent) {
            g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
            g.fillRoundedRectangle(viewedComponent->getBounds().toFloat(), PlugDataLook::windowCornerRadius);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(viewedComponent->getBounds().toFloat(), PlugDataLook::windowCornerRadius, 1.0f);
        }
    }

    void parentSizeChanged() override
    {
        setBounds(getParentComponent()->getLocalBounds());
    }

    void resized() override
    {
        if (viewedComponent) {
            viewedComponent->setSize(width, height);
            viewedComponent->setCentrePosition({ getBounds().getCentreX(), y - (height / 2) });
        }

        if (closeButton) {
            closeButton->setBounds(viewedComponent->getRight() - 35, viewedComponent->getY() + 8, 28, 28);
        }
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress::escapeKey) {
            closeDialog();

            return true;
        }

        return false;
    }

    void mouseDown(MouseEvent const& e) override
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

    int backgroundMargin = 0;
};

struct Dialogs {
    static Component* showTextEditorDialog(String text, String filename, std::function<void(String, bool)> callback);

    static void showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String filename, std::function<void(int)> callback, int margin = 0);
    static void showArrayDialog(std::unique_ptr<Dialog>* target, Component* centre, std::function<void(int, String, String)> callback);

    static void showSettingsDialog(PluginEditor* editor);

    static void showMainMenu(PluginEditor* editor, Component* centre);

    static void showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> callback);

    static void showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent);

    static void showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent);
    static void showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String objectName);

    static void showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position);

    static void showObjectMenu(PluginEditor* parent, Component* target);

    static PopupMenu createObjectMenu(PluginEditor* parent);
};

struct DekenInterface {
    static StringArray getExternalPaths();
};
