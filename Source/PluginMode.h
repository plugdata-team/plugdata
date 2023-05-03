#pragma once

#include "PluginEditor.h"
#include "Standalone/PlugDataWindow.h"

class PluginMode : public Component
{
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , desktopWindow(editor->getPeer())
        , windowBounds(editor->getBounds().withPosition(editor->getTopLevelComponent()->getPosition()))
    {
        if(ProjectInfo::isStandalone) {
            // If the window is already maximised, unmaximise it to prevent problems
#if JUCE_LINUX
            if(OSUtils::isMaximised(desktopWindow->getNativeHandle()))
            {
                OSUtils::maximiseLinuxWindow(desktopWindow->getNativeHandle());
            }
#else
            if(desktopWindow->isFullScreen())
            {
                desktopWindow->setFullScreen(false);
            }
#endif
        }
        // Save original canvas properties
        originalCanvasScale = getValue<float>(cnv->zoomScale);
        originalCanvasPos = cnv->getPosition();
        originalLockedMode = getValue<bool>(cnv->locked);
        originalPresentationMode = getValue<bool>(cnv->presentationMode);

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
        editorButton->onClick = [this]()
        {
            closePluginMode();
        };

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
            fullscreenButton->onClick = [this](){
                setKioskMode(true);
            };
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

        cnv->setTopLeftPosition(-cnv->canvasOrigin);

        pluginModeConstrainer.setSizeLimits(width / 2, height / 2 + titlebarHeight, width * 10, height * 10 + titlebarHeight);

        editor->setConstrainer(&pluginModeConstrainer);

        // Set editor bounds
        editor->setSize(width, height + titlebarHeight);

        // Set local bounds
        setBounds(0, 0, width, height + titlebarHeight);
    }

    ~PluginMode()
    {
    }

    void closePluginMode()
    {
        if (cnv) {
            content.removeChildComponent(cnv);
            // Reset the canvas properties to before plugin mode was entered
            cnv->viewport->setViewedComponent(cnv, false);
            cnv->patch.openInPluginMode = false;
            cnv->zoomScale.setValue(originalCanvasScale);
            cnv->zoomScale.getValueSource().sendChangeMessage(true);
            cnv->setTopLeftPosition(originalCanvasPos);
            cnv->locked = originalLockedMode;
            cnv->presentationMode = originalPresentationMode;
        }

        MessageManager::callAsync([
            editor = this->editor,
            bounds = windowBounds
            ](){
            editor->setConstrainer(editor->defaultConstrainer);
            editor->setBoundsConstrained(bounds);
            editor->getParentComponent()->resized();
            editor->getActiveTabbar()->resized();
        });

        // Destroy this view
        editor->pluginMode.reset(nullptr);
    }

    bool isWindowFullscreen()
    {
        if(ProjectInfo::isStandalone) {
            return isFullScreenKioskMode;
        }
        
        return false;
    }

    void paint(Graphics& g) override
    {
        if (!cnv)
            return;

        if (ProjectInfo::isStandalone && isWindowFullscreen()) {
            // Fill background for Fullscreen / Kiosk Mode
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillRect(editor->getTopLevelComponent()->getLocalBounds());
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
        g.drawText(cnv->patch.getTitle().upToLastOccurrenceOf(".pd", false, true), titleBar.getBounds(), Justification::centred);
    }

    void resized() override
    {
        float const controlsHeight = isWindowFullscreen() ? 0 : titlebarHeight;
        float const scale = getWidth() / width;
        float const resizeRatio = width / (height + (controlsHeight / scale));

        pluginModeConstrainer.setFixedAspectRatio(resizeRatio);
        
        if (ProjectInfo::isStandalone && isWindowFullscreen()) {

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
            if (ProjectInfo::isStandalone) {
                borderResizer->setBounds(getLocalBounds());
            }
            else {
                int const resizerSize = 18;
                cornerResizer->setBounds(getWidth() - resizerSize,
                    getHeight() - resizerSize,
                    resizerSize, resizerSize);
            }

            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(0, titlebarHeight / scale);

            titleBar.setBounds(0, 0, getWidth(), titlebarHeight);

            editorButton->setBounds(titleBar.getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
    }

    void parentSizeChanged() override
    {
        // Fullscreen / Kiosk Mode
        if (ProjectInfo::isStandalone && isWindowFullscreen()) {
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

    void setKioskMode(bool shouldBeBiosk)
    {
        auto* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent());
        
        if(!window) return;
        
        isFullScreenKioskMode = shouldBeBiosk;
        
        if(shouldBeBiosk)
        {
            originalPluginWindowBounds = window->getBounds();
            editor->setConstrainer(nullptr);
            window->setUsingNativeTitleBar(false);
            desktopWindow = window->getPeer();
            
#if JUCE_LINUX
            auto bounds = Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea;
            window->setBounds(bounds);
            editor->setBounds(bounds);
#else
            window->setFullScreen(true);
#endif
            
            borderResizer->setVisible(false);
        }
        else {
#if !JUCE_LINUX
            window->setFullScreen(false);
#endif
            editor->setConstrainer(&pluginModeConstrainer);
            setBounds(originalPluginWindowBounds);
            editor->setBounds(originalPluginWindowBounds);
            window->setBounds(originalPluginWindowBounds.translated(0, -nativeTitleBarHeight));
            window->resized();
            window->getContentComponent()->resized();

            window->setUsingNativeTitleBar(SettingsFile::getInstance()->getProperty<bool>("native_window"));
            desktopWindow = window->getPeer();
            
            borderResizer->setVisible(true);
        }
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (isWindowFullscreen() && key == KeyPress::escapeKey) {
            setKioskMode(false);
            return true;
        } else {
            grabKeyboardFocus();
            if (key.getModifiers().isAnyModifierKeyDown()) {
                // Block All Modifiers
                return true;
            }
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

    Point<int> originalCanvasPos;
    float originalCanvasScale;
    bool originalLockedMode;
    bool originalPresentationMode;
    bool isFullScreenKioskMode = false;

    Rectangle<int> originalPluginWindowBounds;

    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    Rectangle<int> windowBounds;
    float const width = float(cnv->patchWidth.getValue()) + 1.0f;
    float const height = float(cnv->patchHeight.getValue()) + 1.0f;
};
