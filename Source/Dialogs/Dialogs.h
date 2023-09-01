/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Constants.h"
#include "Utility/SettingsFile.h"

class PluginEditor;
class Canvas;

using ArrayDialogCallback = std::function<void(int, String, int, int, bool, std::pair<float, float>)>;

class Dialog : public Component {

public:
    Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int childWidth, int childHeight, bool showCloseButton, int margin = 0)
        : parentComponent(editor)
        , height(childHeight)
        , width(childWidth)
        , owner(ownerPtr)
        , backgroundMargin(margin)
    {
        parentComponent->addAndMakeVisible(this);
        setBounds(0, 0, parentComponent->getWidth(), parentComponent->getHeight());

        setAlwaysOnTop(true);
        setWantsKeyboardFocus(true);

        grabKeyboardFocus();

        if (showCloseButton) {
            closeButton.reset(getLookAndFeel().createDocumentWindowButton(DocumentWindow::closeButton));
            addAndMakeVisible(closeButton.get());
            closeButton->onClick = [this]() {
                closeDialog();
            };
            closeButton->setAlwaysOnTop(true);
        }

        // Some parts of the code check for an active dialog to decide if it needs to paint an outline
        getTopLevelComponent()->repaint();
    }

    void setViewedComponent(Component* child)
    {
        viewedComponent.reset(child);
        addAndMakeVisible(child);
        resized();
    }

    Component* getViewedComponent() const
    {
        return viewedComponent.get();
    }

    bool wantsRoundedCorners() const;

    void paint(Graphics& g) override
    {
        g.setColour(Colours::black.withAlpha(0.5f));

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
        setBounds(getParentComponent()->getLocalBounds());
    }

    void resized() override
    {
        if (viewedComponent) {
            viewedComponent->setSize(width, height);
            viewedComponent->setCentrePosition({ getBounds().getCentreX(), getBounds().getCentreY() });
        }

        if (closeButton) {
            auto macOSStyle = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");
            auto closeButtonBounds = Rectangle<int>(viewedComponent->getRight() - 35, viewedComponent->getY() + 8, 28, 28);
            closeButton->setBounds(closeButtonBounds.reduced(macOSStyle ? 5 : 0));
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

    int height, width;

    Component* parentComponent;

    std::unique_ptr<Component> viewedComponent = nullptr;
    std::unique_ptr<Button> closeButton = nullptr;
    std::unique_ptr<Dialog>* owner;

    int backgroundMargin = 0;
};

struct Dialogs {
    static Component* showTextEditorDialog(String const& text, String filename, std::function<void(String, bool)> callback);
    static void appendTextToTextEditorDialog(Component* dialog, String const& text);

    static void showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String const& filename, std::function<void(int)> callback, int margin = 0);
    static void showArrayDialog(std::unique_ptr<Dialog>* target, Component* centre, ArrayDialogCallback callback);

    static void showSettingsDialog(PluginEditor* editor);

    static void showMainMenu(PluginEditor* editor, Component* centre);

    static void askToLocatePatch(PluginEditor* editor, String const& backupState, std::function<void(File)> callback);

    static void showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> const& callback);

    static void showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent);

    static void showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent);
    static void showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& objectName);

    static void showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position);

    static void showObjectMenu(PluginEditor* parent, Component* target);

    static void showDeken(PluginEditor* editor);
    static void showPatchStorage(PluginEditor* editor);

    static PopupMenu createObjectMenu(PluginEditor* parent);
};

struct DekenInterface {
    static StringArray getExternalPaths();
};
