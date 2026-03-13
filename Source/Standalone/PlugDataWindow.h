
/*

 This file is part of the JUCE library.
 Copyright (c) 2020 - Raw Material Software Limited

 JUCE is an open source library subject to commercial or open-source
 licensing.

 By using JUCE, you agree to the terms of both the JUCE 6 End-User License
 Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

 End User License Agreement: www.juce.com/juce-6-licence
 Privacy Policy: www.juce.com/juce-privacy-policy

 Or: You may also use this code under the terms of the GPL v3 (see
 www.gnu.org/licenses).

 JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
 EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
 DISCLAIMED.

 */

#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_plugin_client/detail/juce_CreatePluginFilter.h>

#include "Constants.h"
#include "Utility/StackShadow.h"
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"
#include "../PluginEditor.h"
#include "../CanvasViewport.h"
#include "Dialogs/Dialogs.h"

static bool drawWindowShadow = true;

namespace pd {
class Patch;
}

class StandalonePluginHolder final : private AudioIODeviceCallback
    , public Component {
public:
    /** Structure used for the number of inputs and outputs. */
    struct PluginInOuts {
        short numIns, numOuts;
    };

    StandalonePluginHolder()
    {
        createPlugin();
        MessageManager::callAsync([this]() {
            if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
                RuntimePermissions::request(RuntimePermissions::recordAudio, [this](bool const granted) {
                    setupAudioDevices();
                    startPlaying();
                });
            else {
                setupAudioDevices();
                startPlaying();
            }
        });
    }

    ~StandalonePluginHolder() override
    {
        processor->suspendProcessing(true);
        stopPlaying();
        processor = nullptr;
        shutDownAudioDevices();
    }

    void createPlugin()
    {
        processor = createPluginFilterOfType(AudioProcessor::wrapperType_Standalone);
        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails(44100, 512);
    }

    int getNumInputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return processor->getMainBusNumInputChannels();
    }

    int getNumOutputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return processor->getMainBusNumOutputChannels();
    }

    void startPlaying()
    {
        player.setProcessor(processor.get());
    }

    void stopPlaying()
    {
        player.setProcessor(nullptr);
    }

    void saveAudioDeviceState()
    {
        auto camelToSnake = [](String const& input){
            String result;
            for (int i = 0; i < input.length(); ++i)
            {
                auto c = input[i];
                if (CharacterFunctions::isUpperCase(c))
                {
                    if (i != 0)
                        result << '_';
                    result << CharacterFunctions::toLowerCase(c);
                }
                else
                {
                    result << c;
                }
            }
            return result;
        };

        if(auto const xml = deviceManager.createStateXml()) {
            DynamicObject::Ptr audioSetup = new DynamicObject();
            for (int i = 0; i < xml->getNumAttributes(); i++) {
                auto const name = camelToSnake(xml->getAttributeName(i));
                audioSetup->setProperty(name, xml->getAttributeValue(i));
            }
            SettingsFile::getInstance()->setProperty("audio_setup", var(audioSetup.get()));
        }
    }

    void reloadAudioDeviceState()
    {
        auto snakeToCamel = [](String const& input)
        {
            String result;
            bool capitalizeNext = false;
            for (int i = 0; i < input.length(); ++i)
            {
                auto c = input[i];
                if (c == '_')
                {
                    capitalizeNext = true;
                }
                else
                {
                    if (capitalizeNext)
                    {
                        result << CharacterFunctions::toUpperCase(c);
                        capitalizeNext = false;
                    }
                    else
                    {
                        result << c;
                    }
                }
            }
            return result;
        };

        std::unique_ptr<XmlElement> savedState;
        auto audioSetup = SettingsFile::getInstance()->getProperty<DynamicObject>("audio_setup");
        if(audioSetup && audioSetup->getProperties().size()) {
            savedState = std::make_unique<XmlElement>("DEVICESETUP");
            for(auto& property : audioSetup->getProperties())
            {
                savedState->setAttribute(snakeToCamel(property.name.toString()), property.value.toString());
            }
        }

        auto const inputChannels = getNumInputChannels();
        auto outputChannels = getNumOutputChannels();

        if (inputChannels == 0 && outputChannels == 0 && processor->isMidiEffect()) {
            // add a dummy output channel for MIDI effect plug-ins so they can receive audio callbacks
            outputChannels = 1;
        }

        deviceManager.initialise(inputChannels, outputChannels, savedState.get(), true, "", nullptr);
    }

    bool isInterAppAudioConnected()
    {
#if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*>(deviceManager.getCurrentAudioDevice()))
            return device->isInterAppAudioConnected();
#endif

        return false;
    }

    Image getIAAHostIcon([[maybe_unused]] int size)
    {
#if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*>(deviceManager.getCurrentAudioDevice()))
            return device->getIcon(size);
#endif

        return { };
    }

    static StandalonePluginHolder* getInstance();

    std::unique_ptr<AudioProcessor> processor;
    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;

    std::unique_ptr<AudioDeviceManager::AudioDeviceSetup> options;

private:
    /*  This class can be used to ensure that audio callbacks use buffers with a
     predictable maximum size.

     On some platforms (such as iOS 10), the expected buffer size reported in
     audioDeviceAboutToStart may be smaller than the blocks passed to
     audioDeviceIOCallback. This can lead to out-of-bounds reads if the render
     callback depends on additional buffers which were initialised using the
     smaller size.

     As a workaround, this class will ensure that the render callback will
     only ever be called with a block with a length less than or equal to the
     expected block size.
     */
    class CallbackMaxSizeEnforcer final : public AudioIODeviceCallback {
    public:
        explicit CallbackMaxSizeEnforcer(AudioIODeviceCallback& callbackIn)
            : inner(callbackIn)
        {
        }

        void audioDeviceAboutToStart(AudioIODevice* device) override
        {
            maximumSize = device->getCurrentBufferSizeSamples();
            storedInputChannels.resize(static_cast<size_t>(device->getActiveInputChannels().countNumberOfSetBits()));
            storedOutputChannels.resize(static_cast<size_t>(device->getActiveOutputChannels().countNumberOfSetBits()));

            inner.audioDeviceAboutToStart(device);
        }

        void audioDeviceIOCallbackWithContext(float const* const* inputChannelData,
            int const numInputChannels,
            float* const* outputChannelData,
            int const numOutputChannels,
            int const numSamples,
            AudioIODeviceCallbackContext const& context) override
        {
            jassertquiet(static_cast<int>(storedInputChannels.size()) == numInputChannels);
            jassertquiet(static_cast<int>(storedOutputChannels.size()) == numOutputChannels);

            int position = 0;

            while (position < numSamples) {
                auto const blockLength = jmin(maximumSize, numSamples - position);

                initChannelPointers(inputChannelData, storedInputChannels, position);
                initChannelPointers(outputChannelData, storedOutputChannels, position);

                inner.audioDeviceIOCallbackWithContext(storedInputChannels.data(),
                    static_cast<int>(storedInputChannels.size()),
                    storedOutputChannels.data(),
                    static_cast<int>(storedOutputChannels.size()),
                    blockLength,
                    context);

                position += blockLength;
            }
        }

        void audioDeviceStopped() override
        {
            inner.audioDeviceStopped();
        }

    private:
        struct GetChannelWithOffset {
            int offset;

            template<typename Ptr>
            auto operator()(Ptr ptr) const noexcept -> Ptr { return ptr + offset; }
        };

        template<typename Ptr, typename Vector>
        void initChannelPointers(Ptr&& source, Vector&& target, int const offset)
        {
            std::transform(source, source + target.size(), target.begin(), GetChannelWithOffset { offset });
        }

        AudioIODeviceCallback& inner;
        int maximumSize = 0;
        std::vector<float const*> storedInputChannels;
        std::vector<float*> storedOutputChannels;
    };

    CallbackMaxSizeEnforcer maxSizeEnforcer { *this };

    void audioDeviceIOCallbackWithContext(float const* const* inputChannelData,
        int const numInputChannels,
        float* const* outputChannelData,
        int const numOutputChannels,
        int const numSamples,
        AudioIODeviceCallbackContext const& context) override
    {
        player.audioDeviceIOCallbackWithContext(inputChannelData,
            numInputChannels,
            outputChannelData,
            numOutputChannels,
            numSamples,
            context);
    }

    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        player.audioDeviceAboutToStart(device);
    }

    void audioDeviceStopped() override
    {
        player.audioDeviceStopped();
    }

    void setupAudioDevices();

    void shutDownAudioDevices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandalonePluginHolder)
};

/**
 A class that can be used to run a simple standalone application containing your filter.

 Just create one of these objects in your JUCEApplicationBase::initialise() method, and
 let it do its work. It will create your filter object using the same createPluginFilter() function
 that the other plugin wrappers use.

 @tags{Audio}
 */

class PlugDataWindow final : public DocumentWindow
    , public SettingsFileListener {
    AudioProcessorEditor* editor;
    StandalonePluginHolder* pluginHolder;

public:
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    SafePointer<Dialog> dialog;

    /** Creates a window with a given title and colour.
     The settings object can be a PropertySet that the class should use to
     store its settings (it can also be null). If takeOwnershipOfSettings is
     true, then the settings object will be owned and deleted by this object.
     */
    explicit PlugDataWindow(AudioProcessorEditor* pluginEditor)
        : DocumentWindow("plugdata", LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton, false)
        , editor(pluginEditor)
    {
        setTitleBarHeight(0);
        pluginHolder = ProjectInfo::getStandalonePluginHolder();

        drawWindowShadow = Desktop::canUseSemiTransparentWindows();

        mainComponent = new MainContentComponent(*this, editor);

        setContentOwned(mainComponent, true);

        updateWindowStyle();
    }

    void parentHierarchyChanged() override
    {
        activeWindowStatusChanged();

#if JUCE_MAC
        auto nativeWindow = SettingsFile::getInstance()->getProperty<bool>("native_window");
        if (auto peer = getPeer()) {
            if (!isFullScreen()) {
                OSUtils::enableInsetTitlebarButtons(peer, !nativeWindow);
            }
        }
#elif JUCE_WINDOWS
        if (auto peer = getPeer())
            OSUtils::useWindowsNativeDecorations(peer, !isFullScreen());
#endif
    }

    void updateWindowStyle()
    {
        auto nativeWindow = SettingsFile::getInstance()->getProperty<bool>("native_window");
#if JUCE_IOS
        nativeWindow = true;
#endif
        if (!mainComponent)
            return;

        auto* editor = mainComponent->getEditor();
        auto* pdEditor = dynamic_cast<PluginEditor*>(editor);

        if (isOnDesktop()) // Ensures we only recreate the desktop window once
        {
            removeFromDesktop();
        }

        if (!nativeWindow) {
#if JUCE_WINDOWS
            setOpaque(true);
#else
            setOpaque(false);
#endif
            setResizable(false, false);
            // we also need to set the constrainer of THIS window so it's set for the peer
            setConstrainer(&pdEditor->constrainer);
            pdEditor->setUseBorderResizer(true);
        } else {
            setOpaque(true);
            setConstrainer(nullptr);
            setResizable(true, false);
            setResizeLimits(850, 650, 99000, 99000);
            pdEditor->setUseBorderResizer(false);
        }

        setDropShadowEnabled(false);
        addToDesktop(getDesktopWindowStyleFlags());
        toFront(true);

        editor->resized();
        resized();
        lookAndFeelChanged();
    }

    void settingsChanged(String const& name, var const& value) override
    {
        if (name == "native_window") {
            auto* editor = mainComponent->getEditor();
            auto* pdEditor = dynamic_cast<PluginEditor*>(editor);
            pdEditor->nvgSurface.detachContext();
            updateWindowStyle();
        }
    }

    ~PlugDataWindow() override
    {
        clearContentComponent();
    }

    BorderSize<int> getBorderThickness() const override
    {
        return BorderSize<int>(0);
    }

    int getDesktopWindowStyleFlags() const override
    {
        auto flags = ComponentPeer::windowHasMinimiseButton | ComponentPeer::windowHasMaximiseButton | ComponentPeer::windowHasCloseButton | ComponentPeer::windowAppearsOnTaskbar | ComponentPeer::windowIsResizable;
#if !JUCE_MAC
        flags |= ComponentPeer::windowIsSemiTransparent;
#endif

#if !JUCE_IOS
        if (SettingsFile::getInstance()->getProperty<bool>("native_window")) {
            flags |= ComponentPeer::windowHasTitleBar;
            flags |= ComponentPeer::windowHasDropShadow;
        } else
#endif
        {
#if JUCE_WINDOWS
            // On Windows 11 we handle dropshadow differently
            if (SystemStats::getOperatingSystemType() != SystemStats::Windows11) {
                flags |= ComponentPeer::windowHasDropShadow;
            }
#else
            flags |= ComponentPeer::windowHasDropShadow;
#endif
        }

        return flags;
    }

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();

        SettingsFile::getInstance()->setProperty("audio_setup", var());

        pluginHolder->createPlugin();
        setContentOwned(new MainContentComponent(*this, pluginHolder->processor->createEditorIfNeeded()), true);
        pluginHolder->startPlaying();
    }

    // Prevents CMD+W from terminating app
    bool keyStateChanged(bool isKeyDown) override
    {
        if (KeyPress(87, ModifierKeys::commandModifier, 0).isCurrentlyDown())
            return true;
        return false;
    }

    void closeButtonPressed() override
    {
        // Close all patches, allowing them to save first
        closeAllPatches();
    }

    // implemented in PlugDataApp.cpp
    void closeAllPatches();

    bool isMaximised() const
    {
#if JUCE_LINUX
        if (auto* peer = getPeer()) {
            return OSUtils::isLinuxWindowMaximised(peer);
        } else {
            return isFullScreen();
        }
#else
        return isFullScreen();
#endif
    }

    static bool useNativeTitlebar()
    {
        return SettingsFile::getInstance()->getProperty<bool>("native_window");
    }

    void maximiseButtonPressed() override
    {
#if JUCE_LINUX || JUCE_BSD
        if (auto* b = getMaximiseButton()) {
            if (auto* peer = getPeer()) {
                bool shouldBeMaximised = OSUtils::isLinuxWindowMaximised(peer);
                b->setToggleState(!shouldBeMaximised, dontSendNotification);

                if (!useNativeTitlebar()) {
                    OSUtils::maximiseLinuxWindow(peer, !shouldBeMaximised);
                }
            } else {
                b->setToggleState(false, dontSendNotification);
            }
        }
#else
        setFullScreen(!isFullScreen());
        parentHierarchyChanged();
#endif
        resized();
    }

#if JUCE_LINUX || JUCE_BSD
    void paint(Graphics& g) override
    {
        if (drawWindowShadow && !useNativeTitlebar() && !isMaximised()) {
            StackShadow::drawShadowForRect(g, getLocalBounds().reduced(20, 20), 18, Corners::windowCornerRadius, isActiveWindow() ? 0.42f : 0.2f);
        }
    }
#endif

#if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
    void paintOverChildren(Graphics& g) override
    {
#    if JUCE_WINDOWS
        if (SystemStats::getOperatingSystemType() != SystemStats::Windows11) {
            g.setColour(PlugDataColours::outlineColour);
            g.drawRect(0, 0, getWidth(), getHeight());
        }
#    else
        if (drawWindowShadow && !useNativeTitlebar() && !isMaximised()) {
            g.setColour(PlugDataColours::outlineColour.withAlpha(isActiveWindow() ? 1.0f : 0.5f));
            g.drawRoundedRectangle(18, 18, getWidth() - 36, getHeight() - 36, Corners::windowCornerRadius, 1.0f);
        } else if (drawWindowShadow && !useNativeTitlebar()) {
            g.setColour(PlugDataColours::outlineColour.withAlpha(isActiveWindow() ? 1.0f : 0.5f));
            g.drawRect(0.5f, 0.5f, getWidth() - 1.0f, getHeight() - 1.0f, 1.0f);
        }
#    endif
    }
#endif

    void activeWindowStatusChanged() override
    {
#if JUCE_WINDOWS
        // Windows looses the opengl buffers when minimised,
        // regenerate here when restored from minimised
        if (isActiveWindow()) {
            if (auto* pluginEditor = dynamic_cast<PluginEditor*>(editor))
                pluginEditor->nvgSurface.invalidateAll();
        }
#endif
        repaint();
    }

    void resized() override
    {
        ResizableWindow::resized();

        Rectangle<int> titleBarArea(0, 7, getWidth() - 6, 23);

#if JUCE_LINUX || JUCE_BSD
        if (!isFullScreen() && !useNativeTitlebar() && drawWindowShadow) {
            auto margin = mainComponent ? mainComponent->getMargin() : 18;
            titleBarArea = Rectangle<int>(0, 7 + margin, getWidth() - (6 + margin), 23);
        }
#elif JUCE_MAC
        auto const nativeWindow = SettingsFile::getInstance()->getProperty<bool>("native_window");
        if (!nativeWindow) {
            Timer::callAfterDelay(150, [_this = SafePointer(this)] {
                if (_this) {
                    _this->parentHierarchyChanged();
                }
            });
        }
#endif

#if !JUCE_IOS
        getLookAndFeel().positionDocumentWindowButtons(*this, titleBarArea.getX(), titleBarArea.getY(), titleBarArea.getWidth(), titleBarArea.getHeight(), getMinimiseButton(), getMaximiseButton(), getCloseButton(), false);
#endif

        if (auto* content = getContentComponent()) {
            content->resized();
            content->repaint();
            MessageManager::callAsync([content = SafePointer(content)] {
                if (content && content->isShowing())
                    content->grabKeyboardFocus();
            });
        }
    }

private:
    class MainContentComponent final : public Component
        , private ComponentListener
        , public MenuBarModel {

    public:
        MainContentComponent(PlugDataWindow& filterWindow, AudioProcessorEditor* pluginEditor)
            : owner(filterWindow)
            , editor(pluginEditor)
        {
            if (editor != nullptr) {

                auto* commandManager = &dynamic_cast<PluginEditor*>(editor.getComponent())->commandManager;

                // Menubar, only for standalone on mac
                // Doesn't add any new features, but was easy to implement because we already have a command manager
                setApplicationCommandManagerToWatch(commandManager);

                editor->addComponentListener(this);
                componentMovedOrResized(*editor, false, true);

                addAndMakeVisible(editor.getComponent());
                editor->setAlwaysOnTop(true);
            }
        }

        AudioProcessorEditor* getEditor()
        {
            return editor;
        }

        StringArray getMenuBarNames() override
        {
            return { "File", "Edit" };
        }

        int getMargin() const
        {
            if (owner.useNativeTitlebar() || owner.isFullScreen()) {
                return 0;
            }

#if JUCE_LINUX || JUCE_BSD
            if (drawWindowShadow) {
                return owner.isMaximised() ? 0 : 18;
            } else {
                return 0;
            }
#else
            return 0;
#endif
        }

        PopupMenu getMenuForIndex(int const topLevelMenuIndex, String const& menuName) override
        {
            PopupMenu menu;

            auto* commandManager = &dynamic_cast<PluginEditor*>(editor.getComponent())->commandManager;

            if (topLevelMenuIndex == 0) {
                menu.addCommandItem(commandManager, CommandIDs::NewProject);
                menu.addCommandItem(commandManager, CommandIDs::OpenProject);
                menu.addCommandItem(commandManager, CommandIDs::SaveProject);
                menu.addCommandItem(commandManager, CommandIDs::SaveProjectAs);
            } else {
                menu.addCommandItem(commandManager, CommandIDs::Copy);
                menu.addCommandItem(commandManager, CommandIDs::Paste);
                menu.addCommandItem(commandManager, CommandIDs::Duplicate);
                menu.addCommandItem(commandManager, CommandIDs::Delete);
                menu.addCommandItem(commandManager, CommandIDs::SelectAll);
                menu.addSeparator();
                menu.addCommandItem(commandManager, CommandIDs::Undo);
                menu.addCommandItem(commandManager, CommandIDs::Redo);
            }

            return menu;
        }

        void menuItemSelected(int menuItemID, int topLevelMenuIndex) override
        {
        }

        ~MainContentComponent() override
        {
            setApplicationCommandManagerToWatch(nullptr);
            if (editor != nullptr) {
                editor->removeComponentListener(this);
#if JUCE_IOS
                // This is safe on iOS because we can't have multiple plugdata windows
                // We also hit an assertion on shutdown without this line
                editor->processor.editorBeingDeleted(editor.getComponent());
#endif
            }
        }

#if JUCE_IOS
        void paint(Graphics& g) override
        {
            if (editor) {
                g.fillAll(PlugDataColours::toolbarBackgroundColour);
            } else {
                g.fillAll(PlugDataColours::toolbarBackgroundColour);
            }
        }
#endif

        void resized() override
        {
            auto r = getLocalBounds().reduced(getMargin());
#if JUCE_IOS
            if (getPeer()) {
                r = OSUtils::getSafeAreaInsets().subtractedFrom(r);
            }
#endif

            if (editor != nullptr) {
                auto const newPos = r.getTopLeft().toFloat().transformedBy(editor->getTransform().inverted());

                if (preventResizingEditor)
                    editor->setTopLeftPosition(newPos.roundToInt());
                else
                    editor->setBoundsConstrained(r);

                getTopLevelComponent()->repaint();
            }

            repaint();
        }

        void componentMovedOrResized(Component&, bool, bool) override
        {
            ScopedValueSetter<bool> const scope(preventResizingEditor, true);

            if (editor != nullptr) {
                auto const rect = getSizeToContainEditor();

                setSize(rect.getWidth(), rect.getHeight());
            }
        }

    private:
        Rectangle<int> getSizeToContainEditor() const
        {
#if JUCE_IOS
            if (editor != nullptr) {
                auto totalArea = Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea;
                totalArea = OSUtils::getSafeAreaInsets().addedTo(totalArea);
                return totalArea;
            }
#endif
            if (editor != nullptr)
                return getLocalArea(editor.getComponent(), editor->getLocalBounds()).expanded(getMargin());

            return { };
        }

        bool preventResizingEditor = false;
        PlugDataWindow& owner;
        SafePointer<AudioProcessorEditor> editor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
    };

public:
    MainContentComponent* mainComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataWindow)
};
