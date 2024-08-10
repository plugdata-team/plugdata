/*
 // Copyright (c) 2022-2023 Nejrup, Alex Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "PluginEditor.h"
#include "Canvas.h"
#include "Standalone/PlugDataWindow.h"

class PluginMode : public Component
    , public NVGComponent {
public:
    explicit PluginMode(PluginEditor* editor, pd::Patch::Ptr patch)
        : NVGComponent(this)
        , patchPtr(patch)
        , cnv(std::make_unique<Canvas>(editor, patch, this))
        , editor(editor)
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
        if (ProjectInfo::isStandalone) {
            auto frameSize = desktopWindow->getFrameSizeIfPresent();
            nativeTitleBarHeight = frameSize ? frameSize->getTop() : 0;
        }

        if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
            mainWindow->setUsingNativeTitleBar(false);
#if JUCE_WINDOWS
            mainWindow->setOpaque(true);
#else
            mainWindow->setOpaque(false);
#endif
        }
        
        auto& pluginModeTheme = editor->pd->pluginModeTheme;
        if(pluginModeTheme.isValid())
        {
            pluginModeLnf = std::make_unique<PlugDataLook>();
            pluginModeLnf->setTheme(pluginModeTheme);
            editor->setLookAndFeel(pluginModeLnf.get());
            editor->getTopLevelComponent()->sendLookAndFeelChange();
        }
        
        editor->nvgSurface.detachContext();

        desktopWindow = editor->getPeer();

        editor->nvgSurface.invalidateAll();
        cnv->setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, cnv.get()));
        patch->openInPluginMode = true;

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
        setInterceptsMouseClicks(true, true);

        // Add this view to the editor
        editor->addAndMakeVisible(this);

        StringArray itemList;
        for (auto scale : pluginScales) {
            itemList.add(String(scale.intScale) + "%");
        }

        scaleComboBox.addItemList(itemList, 1);
        if (ProjectInfo::isStandalone) {
            scaleComboBox.addSeparator();
            scaleComboBox.addItem("Fullscreen", 8);
        }
        scaleComboBox.setTooltip("Change plugin scale");
        scaleComboBox.setText("100%");
        scaleComboBox.setBounds(8, 8, 70, titlebarHeight - 16);
        scaleComboBox.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        scaleComboBox.setColour(ComboBox::backgroundColourId, findColour(PlugDataColour::toolbarHoverColourId).withAlpha(0.8f));
        scaleComboBox.onChange = [this]() {
            auto itemId = scaleComboBox.getSelectedId();
            if (itemId == 8) {
                setKioskMode(true);
                return;
            }
            if (selectedItemId != itemId) {
                selectedItemId = itemId;
                setWidthAndHeight(pluginScales[itemId - 1].floatScale);
                pluginPreviousScale = pluginScales[itemId - 1].intScale;
            }
        };

        titleBar.addAndMakeVisible(scaleComboBox);

        addAndMakeVisible(titleBar);

        // set scale to the last scale that was set for this patches plugin mode
        // if none was set, use 100% scale
        if (pluginModeScaleMap.contains(patchPtr->getPointer().get())) {
            int previousScale = pluginModeScaleMap[patchPtr->getPointer().get()];
            scaleComboBox.setText(String(previousScale) + String("%"), dontSendNotification);
            setWidthAndHeight(previousScale * 0.01f);
        } else {
            setWidthAndHeight(1.0f);
        }

        cnv->connectionLayer.setVisible(false);
    }

    ~PluginMode() override
    {
        if(pluginModeLnf)
        {
            editor->setLookAndFeel(editor->pd->lnf);
            editor->getTopLevelComponent()->sendLookAndFeelChange();
        }
    }

    void setWidthAndHeight(float scale)
    {
        auto newWidth = static_cast<int>(width * scale);
        auto newHeight = static_cast<int>(height * scale) + titlebarHeight;

        if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
#if JUCE_LINUX || JUCE_BSD
            // We need to add the window margin for the shadow on Linux, or else X11 will try to make the window smaller than it should be when the window moves
            auto margin = 36;
#else
            auto margin = 0;
#endif
            // Setting the min=max will disable resizing
            editor->constrainer.setSizeLimits(newWidth + margin, newHeight + margin, newWidth + margin, newHeight + margin);
            mainWindow->getConstrainer()->setSizeLimits(newWidth + margin, newHeight + margin, newWidth + margin, newHeight + margin);
        } else {
            editor->pluginConstrainer.setSizeLimits(newWidth, newHeight, newWidth, newHeight);
        }

#if JUCE_LINUX || JUCE_BSD

        if (ProjectInfo::isStandalone) {
            OSUtils::updateX11Constraints(getPeer()->getNativeHandle());
        }
#endif
        editor->setSize(newWidth, newHeight);
        setBounds(0, 0, newWidth, newHeight);

        editor->nvgSurface.invalidateAll();
    }

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, findNVGColour(PlugDataColour::canvasBackgroundColourId));
        nvgFillRect(nvg, 0, 0, getWidth(), getHeight());

        NVGScopedState scopedState(nvg);
        nvgScale(nvg, pluginModeScale, pluginModeScale);
        nvgTranslate(nvg, cnv->getX(), cnv->getY() - ((isWindowFullscreen() ? 0 : 40) / pluginModeScale));

        auto bounds = getLocalBounds();
        bounds /= pluginModeScale;
        bounds = bounds.translated(cnv->canvasOrigin.x, cnv->canvasOrigin.y);

        cnv->performRender(nvg, bounds);
    }

    void closePluginMode()
    {
        // save the current scale in map for retrieval, so plugin mode remembers the last set scale
        pluginModeScaleMap[patchPtr->getPointer().get()] = pluginPreviousScale;

        auto constrainedNewBounds = windowBounds.withWidth(std::max(windowBounds.getWidth(), 850)).withHeight(std::max(windowBounds.getHeight(), 650));
        if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
            bool isUsingNativeTitlebar = SettingsFile::getInstance()->getProperty<bool>("native_window");
            if (isUsingNativeTitlebar) {
                mainWindow->setResizeLimits(850, 650, 99000, 99000);
                mainWindow->setOpaque(true);
                mainWindow->setUsingNativeTitleBar(true);
            }
            editor->constrainer.setSizeLimits(850, 650, 99000, 99000);
#if JUCE_LINUX || JUCE_BSD
            OSUtils::updateX11Constraints(getPeer()->getNativeHandle());
#endif
            auto correctedPosition = constrainedNewBounds.getTopLeft() - Point<int>(0, nativeTitleBarHeight);
            mainWindow->setBoundsConstrained(constrainedNewBounds.withPosition(correctedPosition));
        } else {
            editor->pluginConstrainer.setSizeLimits(850, 650, 99000, 99000);
            editor->setBounds(constrainedNewBounds);
        }

        editor->nvgSurface.detachContext();
        cnv->patch.openInPluginMode = false;
        editor->getTabComponent().triggerAsyncUpdate();

        editor->parentSizeChanged();
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
        // Detect if the user exited fullscreen with the macOS's fullscreen button
#if JUCE_MAC
        if (ProjectInfo::isStandalone && isWindowFullscreen() && !desktopWindow->isFullScreen()) {
            setKioskMode(false);
        }
#endif

        float scale = getWidth() / width;
        if (ProjectInfo::isStandalone && isWindowFullscreen()) {

            // Calculate the scale factor required to fit the editor in the screen
            float const scaleX = static_cast<float>(getWidth()) / width;
            float const scaleY = static_cast<float>(getHeight()) / height;
            scale = jmin(scaleX, scaleY);

            // Calculate the position of the editor after scaling
            int const scaledWidth = static_cast<int>(width * scale);
            int const scaledHeight = static_cast<int>(height * scale);
            int const x = (getWidth() - scaledWidth) / 2;
            int const y = (getHeight() - scaledHeight) / 2;

            pluginModeScale = scale;

            // Hide titlebar
            titleBar.setBounds(0, 0, 0, 0);
            scaleComboBox.setVisible(false);
            editorButton->setVisible(false);

            auto b = getLocalBounds() + cnv->canvasOrigin;
            cnv->setTransform(cnv->getTransform().scale(scale));
            cnv->setBounds(-b.getX() + (x / scale), -b.getY() + (y / scale), b.getWidth() + b.getX(), b.getHeight() + b.getY());
        } else {
            pluginModeScale = scale;
            scaleComboBox.setVisible(true);
            editorButton->setVisible(true);

            titleBar.setBounds(0, 0, getWidth(), titlebarHeight);
            scaleComboBox.setBounds(8, 8, 74, titlebarHeight - 16);
            editorButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);

            auto b = getLocalBounds() + cnv->canvasOrigin;
            cnv->setTransform(cnv->getTransform().scale(scale));
            cnv->setBounds(-b.getX(), -b.getY() + (titlebarHeight / scale), (b.getWidth() / scale) + b.getX(), (b.getHeight() / scale) + b.getY());
        }
        repaint();
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
        if (scaleComboBox.contains(e.getEventRelativeTo(&scaleComboBox).getPosition()))
            return;

        // Offset the start of the drag when dragging the window by Titlebar
        if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
            if (e.getPosition().getY() < titlebarHeight) {
                isDraggingWindow = true;
                windowDragger.startDraggingWindow(mainWindow, e.getEventRelativeTo(mainWindow));
            }
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!isDraggingWindow)
            return;

        if (auto* mainWindow = dynamic_cast<PlugDataWindow*>(editor->getTopLevelComponent())) {
            windowDragger.dragWindow(mainWindow, e.getEventRelativeTo(mainWindow), nullptr);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDraggingWindow = false;
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

    pd::Patch::Ptr getPatch()
    {
        return patchPtr;
    }
    void setKioskMode(bool shouldBeKiosk)
    {
        auto* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent());

        if (!window)
            return;

        isFullScreenKioskMode = shouldBeKiosk;

        if (shouldBeKiosk) {
            editor->constrainer.setSizeLimits(1, 1, 99000, 99000);
            originalPluginWindowBounds = window->getBounds();
            desktopWindow = window->getPeer();
            setFullScreen(window, true);
        } else {
            setFullScreen(window, false);
            selectedItemId = 3;
            scaleComboBox.setText("100%");
            setWidthAndHeight(1.0f);
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

    Canvas* getCanvas()
    {
        return cnv.get();
    }

private:
    pd::Patch::Ptr patchPtr;
    std::unique_ptr<Canvas> cnv;
    PluginEditor* editor;
    ComponentPeer* desktopWindow;

    Component titleBar;
    int const titlebarHeight = 40;
    int nativeTitleBarHeight = 0;
    ComboBox scaleComboBox;
    std::unique_ptr<MainToolbarButton> editorButton;

    int selectedItemId = 3; // default is 100% for now

    WindowDragger windowDragger;
    bool isDraggingWindow = false;

    bool isFullScreenKioskMode = false;

    Rectangle<int> originalPluginWindowBounds;

    Rectangle<int> windowBounds;
    float const width = float(cnv->patchWidth.getValue()) + 1.0f;
    float const height = float(cnv->patchHeight.getValue()) + 1.0f;
    float pluginModeScale = 1.0f;
    int pluginPreviousScale = 100;
        
    std::unique_ptr<PlugDataLook> pluginModeLnf;

    static inline std::map<t_canvas*, int> pluginModeScaleMap;

    struct Scale {
        float floatScale;
        int intScale;
    };
    std::vector<Scale> pluginScales { Scale { 0.5f, 50 }, Scale { 0.75f, 75 }, Scale { 1.0f, 100 }, Scale { 1.25f, 125 }, Scale { 1.5f, 150 }, Scale { 1.75f, 175 }, Scale { 2.0f, 200 } };
};
