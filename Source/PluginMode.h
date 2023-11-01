#pragma once

#include "PluginEditor.h"
#include "Canvas.h"
#include "Tabbar/Tabbar.h"
#include "Standalone/PlugDataWindow.h"

class PluginMode : public Component {
public:
    explicit PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , desktopWindow(editor->getPeer())
        , windowBounds(editor->getBounds().withPosition(editor->getTopLevelComponent()->getPosition()))
    {
        if (ProjectInfo::isStandalone) {
            // If the window is already maximised, unmaximise it to prevent problems
#if JUCE_LINUX || JUCE_BSD
            OSUtils::maximiseX11Window(desktopWindow->getNativeHandle(), false);
#else
            if (desktopWindow->isFullScreen()) {
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
        
        if (ProjectInfo::isStandalone) {
            auto frameSize = desktopWindow->getFrameSizeIfPresent();
            nativeTitleBarHeight = frameSize ? frameSize->getTop() : 0;
        } else {
            nativeTitleBarHeight = 0;
        }
        
        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);
        
        editorButton = std::make_unique<MainToolbarButton>(Icons::Edit);
        editorButton->setTooltip("Show editor");
        editorButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        editorButton->onClick = [this]() {
            closePluginMode();
        };
        
        titleBar.addAndMakeVisible(*editorButton);
        
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(true);
        setInterceptsMouseClicks(false, false);
        
        // Add this view to the editor
        editor->addAndMakeVisible(this);
        
        if (ProjectInfo::isStandalone) {
            fullscreenButton = std::make_unique<MainToolbarButton>(Icons::Fullscreen);
            fullscreenButton->setTooltip("Enter fullscreen kiosk mode");
            fullscreenButton->setBounds(0, 0, titlebarHeight, titlebarHeight);
            fullscreenButton->onClick = [this]() {
                setKioskMode(true);
            };
            titleBar.addAndMakeVisible(*fullscreenButton);
        }
        
        scaleComboBox.addItemList({"50%", "75%", "100%", "125%", "150%", "175%", "200%"}, 1);
        scaleComboBox.setTooltip("Change plugin scale");
        scaleComboBox.setText("100%");
        scaleComboBox.setBounds(fullscreenButton ? fullscreenButton->getWidth() + 4 : 4, 8, 70, titlebarHeight - 16);
        scaleComboBox.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        scaleComboBox.setColour(ComboBox::backgroundColourId, findColour(PlugDataColour::toolbarHoverColourId).withAlpha(0.8f));
        scaleComboBox.onChange = [this](){
            auto itemId = scaleComboBox.getSelectedId();
            float scale;
            switch (itemId) {
            case 1:     scale = 0.5f;  break;
            case 2:     scale = 0.75f; break;
            case 3:     scale = 1.0f;  break;
            case 4:     scale = 1.25f; break;
            case 5:     scale = 1.5f;  break;
            case 6:     scale = 1.75f; break;
            case 7:     scale = 2.0f;  break;
            default:
                return;
            }
            if (selectedItemId != itemId) {
                selectedItemId = itemId;
                auto newWidth = width * scale;
                auto newHeight = (height * scale) + titlebarHeight + nativeTitleBarHeight;
                // setting the min=max will disable resizing
                if (!ProjectInfo::isStandalone) editor->pluginConstrainer.setSizeLimits(newWidth, newHeight, newWidth, newHeight);
                editor->constrainer.setSizeLimits(newWidth, newHeight, newWidth, newHeight);
                editor->setSize(newWidth, newHeight);
                setBounds(0, 0, newWidth, newHeight);
            }
        };

        titleBar.addAndMakeVisible(scaleComboBox);
        
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

        auto componentHeight = height + titlebarHeight;
        
        // Set editor bounds
        editor->setSize(width, componentHeight);

        // Set local bounds
        setBounds(0, 0, width, componentHeight);
    }

    ~PluginMode() = default;

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

        MessageManager::callAsync([editor = this->editor, bounds = windowBounds]() {

            if(!ProjectInfo::isStandalone) editor->pluginConstrainer.setSizeLimits(850, 650, 99000, 99000);
            editor->constrainer.setSizeLimits(850, 650, 99000, 99000);

            if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
                mainWindow->setBoundsConstrained(bounds);
            } else {
                editor->setBounds(bounds);
            }

            editor->resized();
            if (auto* tabbar = editor->getActiveTabbar()) {
                tabbar->resized();
            }
        });

        // Destroy this view
        editor->pluginMode.reset(nullptr);
    }

    bool isWindowFullscreen() const
    {
        if (ProjectInfo::isStandalone) {
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
        float const scale = getWidth() / width;

        // Detect if the user exited fullscreen with the macOS's fullscreen button
#if JUCE_MAC
        if (ProjectInfo::isStandalone && isWindowFullscreen() && !desktopWindow->isFullScreen()) {
            setKioskMode(false);
        }
#endif

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
            scaleComboBox.setVisible(false);
            editorButton->setVisible(false);
        } else {
            content.setTransform(content.getTransform().scale(scale));
            content.setTopLeftPosition(0, titlebarHeight / scale);
            titleBar.setBounds(0, 0, getWidth(), titlebarHeight);

            scaleComboBox.setVisible(true);
            editorButton->setVisible(true);

            scaleComboBox.setBounds(fullscreenButton ? fullscreenButton->getWidth() + 4 : 4, 8, 70, titlebarHeight - 16);
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

    void setFullScreen(PlugDataWindow* window, bool shouldBeFullScreen)
    {
#if JUCE_LINUX || JUCE_BSD
        // linux can make the window take up the whole display by simply setting the bounds to that of the display
        auto bounds = shouldBeFullScreen ? Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea : originalPluginWindowBounds;
        desktopWindow->setBounds(bounds, shouldBeFullScreen);
#else
        window->setFullScreen(shouldBeFullScreen);
#endif
    }

    void setKioskMode(bool shouldBeKiosk)
    {
        auto* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent());

        if (!window)
            return;

        isFullScreenKioskMode = shouldBeKiosk;

        if (shouldBeKiosk) {
            editor->constrainer.setSizeLimits(1, 1, 99000, 99000);
            if (!ProjectInfo::isStandalone) editor->pluginConstrainer.setSizeLimits(1, 1, 99000, 99000);
            originalPluginWindowBounds = window->getBounds();
            window->setUsingNativeTitleBar(false);
            desktopWindow = window->getPeer();
            setFullScreen(window, true);
        } else {
            editor->constrainer.setSizeLimits(originalPluginWindowBounds.getWidth(), originalPluginWindowBounds.getHeight(), originalPluginWindowBounds.getWidth(), originalPluginWindowBounds.getHeight());
            setFullScreen(window, false);
            if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
                mainWindow->resized();
            }
            
            setBounds(originalPluginWindowBounds.withZeroOrigin());
            editor->setBounds(originalPluginWindowBounds);
            bool isUsingNativeTitlebar = SettingsFile::getInstance()->getProperty<bool>("native_window");
            window->setBounds(originalPluginWindowBounds.translated(0, isUsingNativeTitlebar ? -nativeTitleBarHeight : 0));

            window->getContentComponent()->resized();
            window->setUsingNativeTitleBar(isUsingNativeTitlebar);
            desktopWindow = window->getPeer();
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
    std::unique_ptr<MainToolbarButton> fullscreenButton;
    ComboBox scaleComboBox;
    std::unique_ptr<MainToolbarButton> editorButton;

    int selectedItemId = 3; // default is 100% for now

    Component content;

    ComponentDragger windowDragger;

    Point<int> originalCanvasPos;
    float originalCanvasScale;
    bool originalLockedMode;
    bool originalPresentationMode;
    bool isFullScreenKioskMode = false;

    Rectangle<int> originalPluginWindowBounds;

    Rectangle<int> windowBounds;
    float const width = float(cnv->patchWidth.getValue()) + 1.0f;
    float const height = float(cnv->patchHeight.getValue()) + 1.0f;
};
