#pragma once

#include "PluginEditor.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , desktopWindow(editor->getPeer())
        , settings(SettingsFile::getInstance())
        , cnvParent(cnv->getParentComponent())
        , windowBounds(editor->getBounds())
        , viewportBounds(cnv->viewport->getBounds())
        , infiniteCanvas(settings->getProperty<int>("infinite_canvas"))
    {
        auto c = editor->getConstrainer();
        windowConstrainer = { c->getMinimumWidth(), c->getMinimumHeight(), c->getMaximumWidth(), c->getMaximumHeight() };

        // Hide all of the editor's content
        for (auto* child : editor->getChildren()) {
            if (child->isVisible()) {
                child->setVisible(false);
                children.emplace_back(child);
            }
        }

        // Reset zoom level
        editor->zoomScale = 1.0f;

        // Set window constrainers
        if (ProjectInfo::isStandalone) {
            nativeTitleBarHeight = desktopWindow->getFrameSize().getTop();
            desktopWindow->getConstrainer()->setSizeLimits(width / 2, height / 2 + titlebarHeight + nativeTitleBarHeight, width * 2, height * 2 + titlebarHeight + nativeTitleBarHeight);
        }
        editor->setResizeLimits(width / 2, height / 2 + titlebarHeight, width * 2, height * 2 + titlebarHeight);

        // Set editor bounds
        editor->setSize(width, height + titlebarHeight);

        // Set local bounds
        setBounds(0, 0, width, height + titlebarHeight);

        // Add this view to the editor
        editor->addAndMakeVisible(this);

        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);

        closeButton = std::make_unique<TextButton>(Icons::Edit);
        closeButton->getProperties().set("Style", "LargeIcon");
        closeButton->setTooltip("Show Editor..");
        if (ProjectInfo::isStandalone && !settings->getProperty<bool>("macos_buttons")) {
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
            settings->setProperty("infinite_canvas", false); // Temporarily disable infinte canvas

        cnv->updatingBounds = true;
        cnv->viewport->setViewPosition(cnv->canvasOrigin);
        cnv->updatingBounds = false;

        content.addAndMakeVisible(cnv);
        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());
        cnv->locked = true;
        cnv->presentationMode = true;

        MessageManager::callAsync([_this = SafePointer(this), this, cnv] {
            if (!_this)
                return;
            // Called async to make sure viewport pos has updated
            cnv->updatingBounds = true;
            cnv->setBounds(0, 0, width, height);
        });

        addAndMakeVisible(content);
    }

    ~PluginMode()
    {
        if (!cnv)
            return;

        if (infiniteCanvas)
            settings->setProperty("infinite_canvas", true);

        cnv->updatingBounds = false;
        cnv->locked = false;
        cnv->presentationMode = false;
    }

    void buttonClicked(Button* button) override
    {
        if (button == closeButton.get()) {

            editor->pd->pluginMode = var(false);

            // Restore the original editor content
            for (auto* child : children) {
                child->setVisible(true);
            }

            // Reset the canvas properties
            cnv->viewport->setBounds(viewportBounds);

            // Add the canvas to it's original parent component
            cnvParent->addAndMakeVisible(cnv);

            // Restore Bounds & Resize Limits with the current position
            auto* _desktopWindow = desktopWindow;
            auto* _editor = editor;
            auto _windowConstrainer = windowConstrainer;
            auto _bounds = windowBounds.withPosition(getTopLevelComponent()->getPosition());
            MessageManager::callAsync([this, _desktopWindow, _editor, _windowConstrainer, _bounds]() {
                if (ProjectInfo::isStandalone) {
                    _desktopWindow->getConstrainer()->setFixedAspectRatio(0);
                    _desktopWindow->getConstrainer()->setSizeLimits(_windowConstrainer[0], _windowConstrainer[1], _windowConstrainer[2], _windowConstrainer[3]);
                    _desktopWindow->setBounds(_bounds, false);
                }
                _editor->getConstrainer()->setFixedAspectRatio(0);
                _editor->setResizeLimits(_windowConstrainer[0], _windowConstrainer[1], _windowConstrainer[2], _windowConstrainer[3]);
                _editor->setBounds(_bounds);
                _editor->getParentComponent()->resized();
            });

            // Destroy this view
            editor->pluginMode.reset(nullptr);
        }
    }

    void paint(Graphics& g) override
    {
        if (!cnv)
            return;

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

        int const editorWidth = editor->getWidth();
        int const editorHeight = editor->getHeight();

        float const scale = editorWidth / width;

        if (ProjectInfo::isStandalone) {
            desktopWindow->getConstrainer()->setFixedAspectRatio(width / (height + ((titlebarHeight + nativeTitleBarHeight) / scale)));
        } else {
            editor->getConstrainer()->setFixedAspectRatio(width / (height + ((titlebarHeight + nativeTitleBarHeight) / scale)));
        }

        setSize(editorWidth, editorHeight);

        content.setTransform(content.getTransform().scale(scale));
        content.setTopLeftPosition(0, titlebarHeight / scale);

        titleBar.setBounds(0, 0, editorWidth, titlebarHeight);

        if (ProjectInfo::isStandalone && !settings->getProperty<bool>("macos_buttons")) {
            closeButton->setBounds(0, 0, titlebarHeight, titlebarHeight);
        } else {
            closeButton->setBounds(titleBar.getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Offset the start of the drag when dragging the window by Titlebar
        if (!nativeTitleBarHeight) {
            if (e.getPosition().getY() < titlebarHeight)
                windowDragger.startDraggingComponent(&desktopWindow->getComponent(), e.getEventRelativeTo(&desktopWindow->getComponent()));
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Drag window by TitleBar
        if (!nativeTitleBarHeight)
            windowDragger.dragComponent(&desktopWindow->getComponent(), e.getEventRelativeTo(&desktopWindow->getComponent()), nullptr);
    }

    bool keyPressed(KeyPress const& key) override
    {
        // Block keypresses to editor
        return true;
    }

private:
    SafePointer<Canvas> cnv;
    PluginEditor* editor;
    ComponentPeer* desktopWindow;
    SettingsFile* settings;

    Component titleBar;
    int const titlebarHeight = 40;
    int nativeTitleBarHeight;
    std::unique_ptr<TextButton> closeButton;

    Component content;

    ComponentDragger windowDragger;
    std::vector<int> windowConstrainer;

    Component* cnvParent;

    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    float const width = cnv->patchWidth.getValue();
    float const height = cnv->patchHeight.getValue();
    float resizeRatio;

    bool infiniteCanvas;

    std::vector<Component*> children;
};
