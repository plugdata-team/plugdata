#pragma once

#include "PluginEditor.h"
#include "Standalone/PlugDataWindow.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , cnvParent(cnv->getParentComponent())
        , windowBounds(editor->getBounds())
        , windowConstrainer(editor->getConstrainer())
        , viewportBounds(cnv->viewport->getBounds())
    {
        if (ProjectInfo::isStandalone) {
            mainWindow = static_cast<PlugDataWindow*>(editor->getTopLevelComponent());
        }
        resizeLimits = { editor->getConstrainer()->getMinimumWidth(), editor->getConstrainer()->getMinimumHeight(), editor->getConstrainer()->getMaximumWidth(), editor->getConstrainer()->getMaximumHeight() };
        for (auto* child : editor->getChildren()) {
            if (child->isVisible()) {
                child->setVisible(false);
                children.emplace_back(child);
            }
        }

        editor->zoomScale = 1.0f;

        editor->setResizeLimits(width, height + titlebarHeight, width, height + titlebarHeight);
        editor->setSize(width, height + titlebarHeight);
        editor->setResizable(false, false);

        if (mainWindow) {
            mainWindow->setResizeLimits(width, height + titlebarHeight, width, height + titlebarHeight);
            mainWindow->setSize(width, height + titlebarHeight);
            mainWindow->setResizable(false, false);
        }

        setBounds(0, 0, width, height + titlebarHeight);
        editor->addAndMakeVisible(this);

         // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);
 
        // Close Button
        closeButton->setButtonText(Icons::Edit);
        closeButton->getProperties().set("Style", "LargeIcon");
        closeButton->setTooltip("Show Editor..");
        if (ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("macos_buttons")) {
            closeButton->setBounds(0, 0, titlebarHeight, titlebarHeight);
        } else {
            closeButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
        closeButton->addListener(this);
        titleBar.addAndMakeVisible(*closeButton);

        addAndMakeVisible(titleBar);

        // Viewed Content
        content.setBounds(0, titlebarHeight, width, height);
        content.addAndMakeVisible(cnv);
        // cnv->viewport->setViewPosition(cnv->canvasOrigin);
        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());
        cnv->locked = true;
        cnv->presentationMode = true;
        addAndMakeVisible(content);
    }

    ~PluginMode()
    {
    }

    void buttonClicked(Button* button) override
    {
        if (button == closeButton.get()) {
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
            editor->pluginMode.reset(nullptr);
        }
    }

    void paint(Graphics& g) override
    {

        auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);
        if (editor->wantsRoundedCorners()) {
            // TitleBar background
            g.setColour(baseColour);
            g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), titlebarHeight, Corners::windowCornerRadius);
        } else {
            // TitleBar background
            g.setColour(baseColour);
            g.fillRect(0, 0, getWidth(), titlebarHeight);
        }
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, titlebarHeight, static_cast<float>(getWidth()), titlebarHeight, 1.0f);

        // TitleBar text
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText(cnv->patch.getTitle(), titleBar.getBounds(), Justification::centred);
    }

    bool keyPressed(KeyPress const& key) override
    {
        // Block keypresses to editor
        return true;
    }

    void mouseDown(MouseEvent const& e) override
    {
        // no window dragging by titleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        if (e.getPosition().getY() < titlebarHeight) {
            if (mainWindow) {
                if (!mainWindow->isUsingNativeTitleBar())
                    windowDragger.startDraggingComponent(mainWindow, e.getEventRelativeTo(mainWindow));
            }
        }
    }
 
    void mouseDrag(MouseEvent const& e) override
    {
        if (!ProjectInfo::isStandalone)
            return;

        if (mainWindow) {
            if (!mainWindow->isUsingNativeTitleBar())
                windowDragger.dragComponent(mainWindow, e.getEventRelativeTo(mainWindow), nullptr);
        }
    }

private:
    Canvas* cnv;
    PluginEditor* editor;
    PlugDataWindow* mainWindow = nullptr;

    Component titleBar;
    Component content;

    // ResizableWindow* mainWindow;
    ComponentBoundsConstrainer* windowConstrainer;
    Component* cnvParent;
    struct ResizeLimits {
        int minWidth;
        int minHeight;
        int maxWidth;
        int maxHeight;
    };

    ResizeLimits resizeLimits;
    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    int const width = cnv->patchWidth.getValue();
    int const height = cnv->patchHeight.getValue();

    int const titlebarHeight = 40;

    std::vector<Component*> children;

    std::unique_ptr<TextButton> closeButton = std::make_unique<TextButton>();

    ComponentDragger windowDragger;
};