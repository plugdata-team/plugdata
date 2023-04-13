#pragma once

#include "PluginEditor.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , desktopWindow(editor->getPeer())
        , windowBounds(editor->getBounds())
        , viewportBounds(cnv->viewport->getBounds())
    {
        auto c = editor->getConstrainer();
        windowConstrainer = { c->getMinimumWidth(), c->getMinimumHeight(), c->getMaximumWidth(), c->getMaximumHeight() };

        // Reset zoom level
        editor->zoomScale = 1.0f;
        editor->zoomScale.getValueSource().sendChangeMessage(true);

        // Set window constrainers
        if (ProjectInfo::isStandalone) {
            nativeTitleBarHeight = desktopWindow->getFrameSize().getTop();
            desktopWindow->getConstrainer()->setSizeLimits(width / 2, height / 2 + titlebarHeight + nativeTitleBarHeight, width * 10, height * 10 + titlebarHeight + nativeTitleBarHeight);
        }
        editor->setResizeLimits(width / 2, height / 2 + titlebarHeight, width * 10, height * 10 + titlebarHeight);

        // Set editor bounds
        editor->setSize(width, height + titlebarHeight);

        // Set local bounds
        setBounds(0, 0, width, height + titlebarHeight);

        // Add this view to the editor
        editor->addAndMakeVisible(this);

        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);

        editorButton = std::make_unique<TextButton>(Icons::Edit);
        editorButton->getProperties().set("Style", "LargeIcon");
        editorButton->setTooltip("Show Editor..");
        editorButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        editorButton->addListener(this);
        titleBar.addAndMakeVisible(*editorButton);

        if (ProjectInfo::isStandalone) {
            fullscreenButton = std::make_unique<TextButton>(Icons::Fullscreen);
            fullscreenButton->getProperties().set("Style", "LargeIcon");
            fullscreenButton->setTooltip("Kiosk Mode..");
            fullscreenButton->setBounds(0, 0, titlebarHeight, titlebarHeight);
            fullscreenButton->addListener(this);
            titleBar.addAndMakeVisible(*fullscreenButton);
        }

        addAndMakeVisible(titleBar);

        // Viewed Content (canvas)
        content.setBounds(0, titlebarHeight, width, height);

        content.addAndMakeVisible(cnv);

        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());
        cnv->locked = true;
        cnv->presentationMode = true;
        cnv->viewport->setViewedComponent(nullptr);

        addAndMakeVisible(content);

        auto const& origin = cnv->canvasOrigin;
        cnv->setBounds(-origin.x, -origin.y, width + origin.x, height + origin.y);

        setAlwaysOnTop(true);
        setWantsKeyboardFocus(true);
        setInterceptsMouseClicks(false, false);
    }

    ~PluginMode()
    {
        if (!cnv)
            return;

        cnv->locked = false;
        cnv->presentationMode = false;
    }

    void closePluginMode()
    {
        editor->pd->pluginMode = var(false);

        if (cnv) {
            content.removeChildComponent(cnv);
            // Reset the canvas properties
            cnv->viewport->setBounds(viewportBounds);
            cnv->viewport->setViewedComponent(cnv, false);
            cnv->viewport->resized();
            cnv->jumpToOrigin();
            cnv->setSize(Canvas::infiniteCanvasSize, Canvas::infiniteCanvasSize);
        }

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

    void buttonClicked(Button* button) override
    {
        if (button == fullscreenButton.get()) {
            auto* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent());
            window->setFullScreen(true);

        } else if (button == editorButton.get()) {
            closePluginMode();
        }
    }

    void paint(Graphics& g) override
    {
        if (!cnv)
            return;

        if (ProjectInfo::isStandalone && desktopWindow->isFullScreen()) {
            // Fill background for Fullscreen / Kiosk Mode
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillRect(editor->getTopLevelComponent()->getBounds());
            return;
        }

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
        if (ProjectInfo::isStandalone && desktopWindow->isFullScreen()) {

            // Fullscreen / Kiosk Mode

            // Determine the screen width and height
            int const screenWidth = desktopWindow->getBounds().getWidth();
            int const screenHeight = desktopWindow->getBounds().getHeight();

            // Fill the screen
            setBounds(0, 0, screenWidth, screenHeight);

            // Calculate the scale factor required to fit the editor in the screen
            float const scaleX = static_cast<float>(screenWidth) / width;
            float const scaleY = static_cast<float>(screenHeight) / height;
            float const scale = jmin(scaleX, scaleY);

            // Calculate the position of the editor after scaling
            int const scaledWidth = static_cast<int>(width * scale);
            int const scaledHeight = static_cast<int>(height * scale);
            int const x = (screenWidth - scaledWidth) / 2;
            int const y = (screenHeight - scaledHeight) / 2;

            // Apply the scale and position to the editor
            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(x / scale, y / scale);

            // Hide titlebar
            titleBar.setBounds(0, 0, 0, 0);
        } else {

            int const editorWidth = editor->getWidth();
            float const scale = editorWidth / width;
            float const resizeRatio = width / (height + ((titlebarHeight + nativeTitleBarHeight) / scale));

            int const editorHeight = editorWidth / resizeRatio - nativeTitleBarHeight;

            if (ProjectInfo::isStandalone) {
#if JUCE_LINUX
                editor->getConstrainer()->setFixedAspectRatio(resizeRatio);
#else
                desktopWindow->getConstrainer()->setFixedAspectRatio(resizeRatio);
#endif
            } else {
                editor->getConstrainer()->setFixedAspectRatio(resizeRatio);
            }

            setSize(editorWidth, editorHeight);

            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(0, titlebarHeight / scale);

            titleBar.setBounds(0, 0, editorWidth, titlebarHeight);

            editorButton->setBounds(titleBar.getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
    }

    bool hitTest(int x, int y) override
    {
        if (ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
            // Block modifier keys when mouseDown
            return false;
        } else {
            return true;
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
        grabKeyboardFocus();
        if (key.getModifiers().isAnyModifierKeyDown()) {
            // Block All Modifiers
            return true;
        } else {
            // Pass other keypresses on to the editor
            return false;
        }
    }

private:
    SafePointer<Canvas> cnv;
    PluginEditor* editor;
    ComponentPeer* desktopWindow;

    Component titleBar;
    int const titlebarHeight = 40;
    int nativeTitleBarHeight;
    std::unique_ptr<TextButton> editorButton;
    std::unique_ptr<TextButton> fullscreenButton;

    Component content;

    ComponentDragger windowDragger;
    std::vector<int> windowConstrainer;

    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    float const width = float(cnv->patchWidth.getValue()) + 1.0f;
    float const height = float(cnv->patchHeight.getValue()) + 1.0f;
};
