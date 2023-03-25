#pragma once

#include "PluginEditor.h"
// #include "Standalone/PlugDataWindow.h"

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
        , infiniteCanvas(SettingsFile::getInstance()->getProperty<int>("infinite_canvas"))
    {
        if (ProjectInfo::isStandalone) {
            mainWindow = static_cast<DocumentWindow*>(editor->getTopLevelComponent());
        }

        // Hide all of the editor's content
        for (auto const& child : editor->getChildren()) {
            if (child->isVisible()) {
                child->setVisible(false);
                children.emplace_back(child);
            }
        }

        // Reset zoom level
        editor->zoomScale = 1.0f;

        // Set window bounds
        if (mainWindow) {
            mainWindow->setResizeLimits(width, height + titlebarHeight, width, height + titlebarHeight);
            mainWindow->setSize(width, height + titlebarHeight);
            mainWindow->setResizable(false, false);
        } else {
            editor->setResizeLimits(width, height + titlebarHeight, width, height + titlebarHeight);
            editor->setSize(width, height + titlebarHeight);
            editor->setResizable(false, false);
        }

        // Add this view to the editor
        setBounds(0, 0, width, height + titlebarHeight);
        editor->addAndMakeVisible(this);

        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);

        closeButton = std::make_unique<TextButton>();
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

        // Viewed Content (canvas)
        content.setBounds(0, titlebarHeight, width, height);

        if (infiniteCanvas)
            SettingsFile::getInstance()->setProperty("infinite_canvas", false); // Temporarily disable infinte canvas

        cnv->updatingBounds = true;
        cnv->viewport->setViewPosition(cnv->canvasOrigin);
        cnv->updatingBounds = false;

        content.addAndMakeVisible(cnv);
        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());
        cnv->locked = true;
        cnv->presentationMode = true;

        MessageManager::callAsync([this, cnv] {
            // Called async to make sure viewport pos has updated
            cnv->updatingBounds = true;
            cnv->setBounds(0, 0, width, height);
        });

        addAndMakeVisible(content);
    }

    ~PluginMode()
    {
        if (infiniteCanvas)
            SettingsFile::getInstance()->setProperty("infinite_canvas", true);
    }

    void buttonClicked(Button* button) override
    {
        if (button == closeButton.get()) {
            // Restore the original editor content
            for (auto const& child : children) {
                child->setVisible(true);
            }

            // Reset the canvas properties
            cnv->viewport->setBounds(viewportBounds);
            cnv->locked = false;
            cnv->presentationMode = false;

            // Add the canvas to it's original parent component
            cnvParent->addAndMakeVisible(cnv);

            // Restore the editor's resize limits
            if (mainWindow) {
                mainWindow->setResizeLimits(windowConstrainer->getMinimumWidth(), windowConstrainer->getMinimumHeight(), windowConstrainer->getMaximumWidth(), windowConstrainer->getMaximumHeight());
                mainWindow->setSize(windowBounds.getWidth(), windowBounds.getHeight());
                mainWindow->setResizable(true, false);
            } else {
                editor->setResizeLimits(windowConstrainer->getMinimumWidth(), windowConstrainer->getMinimumHeight(), windowConstrainer->getMaximumWidth(), windowConstrainer->getMaximumHeight());
                editor->setSize(windowBounds.getWidth(), windowBounds.getHeight());
                editor->setResizable(true, false);
            }

            // Restore canvas bounds
            cnv->updatingBounds = false;

            cnv->viewport->resized();

            SettingsFile::getInstance()->setProperty("plugin_mode", false);

            // Destroy this view
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

        // TitleBar outline
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, titlebarHeight, static_cast<float>(getWidth()), titlebarHeight, 1.0f);

        // TitleBar text
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText(cnv->patch.getTitle().trimCharactersAtEnd(".pd"), titleBar.getBounds(), Justification::centred);
    }

    void parentSizeChanged() override
    {
        /*
        float editorWidth = editor->getWidth();
        int editorHeight = (editorWidth / resizeRatio) + titlebarHeight;
        editor->setSize(editorWidth, editorHeight);

        // Apply and limit zoom
        auto scale = std::clamp(editorWidth / width, 0.5f, 2.0f);
        cnv->setTransform(AffineTransform().scaled(scale));
        */
    }

    bool keyPressed(KeyPress const& key) override
    {
        // Block keypresses to editor
        return true;
    }

    void mouseDown(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Drag window by TitleBar
        if (e.getPosition().getY() < titlebarHeight) {
            if (mainWindow) {
                if (!mainWindow->isUsingNativeTitleBar())
                    windowDragger.startDraggingComponent(mainWindow, e.getEventRelativeTo(mainWindow));
            }
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Drag window by TitleBar
        if (mainWindow) {
            if (!mainWindow->isUsingNativeTitleBar())
                windowDragger.dragComponent(mainWindow, e.getEventRelativeTo(mainWindow), nullptr);
        }
    }

private:
    Canvas* cnv;
    PluginEditor* editor;
    DocumentWindow* mainWindow = nullptr;

    Component titleBar;
    int const titlebarHeight = 40;
    std::unique_ptr<TextButton> closeButton;
    ComponentDragger windowDragger;

    Component content;

    ComponentBoundsConstrainer* windowConstrainer;
    Component* cnvParent;

    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    int const width = cnv->patchWidth.getValue();
    int const height = cnv->patchHeight.getValue();
    float resizeRatio = float(width) / float(height);

    bool infiniteCanvas;

    std::vector<Component*> children;
};