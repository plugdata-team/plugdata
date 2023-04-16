#pragma once

#include "PluginEditor.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , desktopWindow(editor->getPeer())
        , windowBounds(editor->getBounds().withPosition(editor->getTopLevelComponent()->getPosition()))
    {
        // Set zoom value and update synchronously
        cnv->zoomScale.setValue(1.0f);
        cnv->zoomScale.getValueSource().sendChangeMessage(true);
        
        nativeTitleBarHeight = ProjectInfo::isStandalone ? desktopWindow->getFrameSize().getTop() : 0;

        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);

        editorButton = std::make_unique<TextButton>(Icons::Edit);
        editorButton->getProperties().set("Style", "LargeIcon");
        editorButton->setTooltip("Show Editor..");
        editorButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        editorButton->addListener(this);
        titleBar.addAndMakeVisible(*editorButton);

        setAlwaysOnTop(true);
        setWantsKeyboardFocus(true);
        setInterceptsMouseClicks(false, false);
        
        // Add this view to the editor
        editor->addAndMakeVisible(this);
        
        if (ProjectInfo::isStandalone) {
            borderResizer = std::make_unique<MouseRateReducedComponent<ResizableBorderComponent>>(editor->getTopLevelComponent(), &pluginModeConstrainer);
            borderResizer->setAlwaysOnTop(true);
            addAndMakeVisible(borderResizer.get());
        }
        else {
            cornerResizer = std::make_unique<MouseRateReducedComponent<ResizableCornerComponent>>(editor, &pluginModeConstrainer);
            cornerResizer->setAlwaysOnTop(true);
            addAndMakeVisible(cornerResizer.get());
        }
        
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

        // Store old constrainers so we can restore them later
        oldEditorConstrainer = editor->getConstrainer();
        
        pluginModeConstrainer.setSizeLimits(width / 2, height / 2 + titlebarHeight, width * 10, height * 10 + titlebarHeight + nativeTitleBarHeight);
        
        editor->setConstrainer(&pluginModeConstrainer);

        // Set editor bounds
        editor->setSize(width, height + titlebarHeight);

        // Set local bounds
        setBounds(0, 0, width, height + titlebarHeight);
    }

    ~PluginMode()
    {
        if (!cnv)
            return;
    }

    void closePluginMode()
    {
        if (cnv) {
            content.removeChildComponent(cnv);
            // Reset the canvas properties
            cnv->viewport->setViewedComponent(cnv, false);
            cnv->patch.openInPluginMode = false;
            cnv->jumpToOrigin();
            cnv->setSize(Canvas::infiniteCanvasSize, Canvas::infiniteCanvasSize);
            cnv->locked = false;
            cnv->presentationMode = false;
        }
        
        MessageManager::callAsync([
            editor = this->editor,
            bounds = windowBounds,
            editorConstrainer = oldEditorConstrainer
            ](){
            editor->setConstrainer(editorConstrainer);
            editor->setBoundsConstrained(bounds);
            editor->getParentComponent()->resized();
            editor->getActiveTabbar()->resized();
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
    
    void resized() override
    {
        if (ProjectInfo::isStandalone && desktopWindow->isFullScreen()) {
            // Calculate the scale factor required to fit the editor in the screen
            float const scaleX = static_cast<float>(getWidth()) / width;
            float const scaleY = static_cast<float>(getHeight()) / height;
            float const scale = jmin(scaleX, scaleY);
            
            // Calculate the position of the editor after scaling
            int const scaledWidth = static_cast<int>(width * scale);
            int const scaledHeight = static_cast<int>(height * scale);
            int const x = (getWidth() - scaledWidth) / 2;
            int const y = (getHeight() - scaledHeight) / 2;
            
            // Apply the scale and position to the editor
            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(x / scale, y / scale);
            
            // Hide titlebar
            titleBar.setBounds(0, 0, 0, 0);
        }
        else {
            float const scale = getWidth() / width;
            float const resizeRatio = width / (height + ((titlebarHeight + nativeTitleBarHeight) / scale));

            if (ProjectInfo::isStandalone) {
                borderResizer->setBounds(getLocalBounds());
            }
            else {
                int const resizerSize = 18;
                cornerResizer->setBounds(getWidth() - resizerSize,
                    getHeight() - resizerSize,
                    resizerSize, resizerSize);
            }
            
            pluginModeConstrainer.setFixedAspectRatio(resizeRatio);
            
            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(0, titlebarHeight / scale);

            titleBar.setBounds(0, 0, getWidth(), titlebarHeight);

            editorButton->setBounds(titleBar.getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
    }

    void parentSizeChanged() override
    {
        // Fullscreen / Kiosk Mode
        if (ProjectInfo::isStandalone && desktopWindow->isFullScreen()) {

            // Determine the screen size
            auto const screenBounds = desktopWindow->getBounds();

            // Fill the screen
            setBounds(0, 0, screenBounds.getWidth(), screenBounds.getHeight());
        } else {
            setBounds(editor->getLocalBounds());
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
        
    ComponentBoundsConstrainer pluginModeConstrainer;
    ComponentBoundsConstrainer* oldEditorConstrainer;
        
    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;
    
    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    Rectangle<int> windowBounds;
    float const width = float(cnv->patchWidth.getValue()) + 1.0f;
    float const height = float(cnv->patchHeight.getValue()) + 1.0f;
};
