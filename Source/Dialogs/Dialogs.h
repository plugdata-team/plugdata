/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Constants.h"
#include "Utility/SettingsFile.h"
#include "Utility/WindowDragger.h"
#include "PluginEditor.h"

class Canvas;

class Dialog final : public Component {

public:
    Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int childWidth, int childHeight, bool showCloseButton, int margin = 0);

    ~Dialog() override
    {
        if (auto* editor = dynamic_cast<PluginEditor*>(parentComponent)) {
            editor->nvgSurface.setRenderThroughImage(false);
        }

        if (auto const* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent())) {
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
        g.setColour(Colours::black.withAlpha(0.5f));

        auto const bounds = getLocalBounds().toFloat().reduced(backgroundMargin);

        if (wantsRoundedCorners()) {
            g.fillRoundedRectangle(bounds.toFloat(), Corners::windowCornerRadius);
        } else {
            g.fillRect(bounds);
        }

        if (viewedComponent) {
            g.setColour(PlugDataColours::dialogBackgroundColour);
            g.fillRoundedRectangle(viewedComponent->getBounds().toFloat(), isIphone() ? 0 : Corners::windowCornerRadius);

            g.setColour(PlugDataColours::outlineColour);
            g.drawRoundedRectangle(viewedComponent->getBounds().toFloat(), isIphone() ? 0 : Corners::windowCornerRadius, 1.0f);
        }
    }

    static bool isIphone()
    {
#if JUCE_IOS
        return !OSUtils::isIPad();
#else
        return false;
#endif
    }

    void parentSizeChanged() override
    {
        if (auto const* parent = getParentComponent()) {
            setBounds(parent->getLocalBounds());
        }
    }

    void resized() override
    {
        if (viewedComponent) {
            if (isIphone()) {
                // Only on iPhone, fullscreen every dialog becauwe we don't have much space
                viewedComponent->setBounds(0, 0, getWidth(), getHeight());
            } else {

                viewedComponent->setSize(std::min(width, getWidth()), std::min(height, getHeight()));
                viewedComponent->setCentrePosition({ getLocalBounds().getCentreX(), getLocalBounds().getCentreY() });
            }
        }

        if (closeButton) {
            auto const closeButtonBounds = Rectangle<int>(viewedComponent->getRight() - 35, viewedComponent->getY() + 6, 28, 28);
            closeButton->setBounds(closeButtonBounds);
        }
    }

#if !JUCE_IOS
    void mouseDown(MouseEvent const& e) override
    {
        if (isPositiveAndBelow(e.getEventRelativeTo(viewedComponent.get()).getMouseDownY(), 40) && ProjectInfo::isStandalone) {
            dragger.startDraggingWindow(parentComponent->getTopLevelComponent(), e);
            dragging = true;
        } else if (!Process::isForegroundProcess()) {
            Process::makeForegroundProcess();
            if (auto* topLevel = getTopLevelComponent()) {
                topLevel->toFront(true);
            }
        } else if (!viewedComponent->getBounds().contains(e.getPosition())) {
            closeDialog();
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (dragging) {
            dragger.dragWindow(parentComponent->getTopLevelComponent(), e);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        dragging = false;
    }
#endif

    void setBlockFromClosing(bool const block)
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
    static Component* showTextEditorDialog(String const& text, String filename, std::function<void(String, bool)> closeCallback, std::function<void(String)> saveCallback, float desktopScale = 1.0f, bool enableSyntaxHighlighting = false);
    static void appendTextToTextEditorDialog(Component* dialog, String const& text);
    static void clearTextEditorDialog(Component* dialog);

    static void showAskToSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String const& filename, std::function<void(int)> callback, int margin = 0, bool withLogo = true);

    static void showSettingsDialog(PluginEditor* editor);

    static void showMainMenu(PluginEditor* editor, Component* centre);

    static void showMultiChoiceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(int)> const& callback, StringArray const& options = { "Okay", "Cancel " }, String const& icon = Icons::Warning);

    static void showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent);

    static void showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent);
    static void showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& objectName);

    static void showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position);

    static void showObjectMenu(PluginEditor* parent, Component const* target);

    static void showDeken(PluginEditor* editor);
    static void showStore(PluginEditor* editor);

    static void dismissFileDialog();

    static void showOpenDialog(std::function<void(URL)> const& callback, bool canSelectFiles, bool canSelectDirectories, String const& lastFileId, String const& extension, Component* parentComponent);

    static void showSaveDialog(std::function<void(URL)> const& callback, String const& extension, String const& lastFileId, Component* parentComponent = nullptr, bool directoryMode = false);

    static inline std::unique_ptr<FileChooser> fileChooser = nullptr;
};

struct DekenInterface {
    static StringArray getExternalPaths();
};
