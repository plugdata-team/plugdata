
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

#include "Constants.h"
#include "Utility/StackShadow.h"
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"
#include "Utility/MidiDeviceManager.h"
#include "../PluginEditor.h"
#include "../CanvasViewport.h"
#include "Dialogs/Dialogs.h"

// For each OS, we have a different approach to rendering the window shadow
// macOS:
// - Use the native shadow, it works fine
// Windows:
//  - We instruct Windows 11 to make the window rounded. On Windows 10, the window will not be rounded, which follows the default OS window style anyway
// Linux:
// - Native shadow is inconsistent across window managers and distros (sometimes there is no shadow, even though other windows have it...)
// - We use a transparent margin around the window to draw the shadow in

static bool drawWindowShadow = true;

namespace pd {
class Patch;
}

class PlugDataProcessorPlayer : public AudioProcessorPlayer {
public:
    PlugDataProcessorPlayer()
        : midiDeviceManager(this)
    {
    }

    void handleIncomingMidiMessage(MidiInput* input, MidiMessage const& message) override
    {
        auto deviceIndex = midiDeviceManager.getMidiInputDeviceIndex(input->getIdentifier());
        if (deviceIndex >= 0) {
            getMidiMessageCollector().addMessageToQueue(MidiDeviceManager::convertToSysExFormat(message, deviceIndex));
        }
    }

    MidiDeviceManager midiDeviceManager;
};

class StandalonePluginHolder : private AudioIODeviceCallback
    , public Component {
public:
    /** Structure used for the number of inputs and outputs. */
    struct PluginInOuts {
        short numIns, numOuts;
    };

    /** Creates an instance of the default plugin.

     The settings object can be a PropertySet that the class should use to store its
     settings - the takeOwnershipOfSettings indicates whether this object will delete
     the settings automatically when no longer needed. The settings can also be nullptr.

     A default device name can be passed in.

     Preferably a complete setup options object can be used, which takes precedence over
     the preferredDefaultDeviceName and allows you to select the input & output device names,
     sample rate, buffer size etc.

     In all instances, the settingsToUse will take precedence over the "preferred" options if not null.
     */
    explicit StandalonePluginHolder(PropertySet* settingsToUse, bool takeOwnershipOfSettings = true, String const& preferredDefaultDeviceName = String(), AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions = nullptr, Array<PluginInOuts> const& channels = Array<PluginInOuts>())

        : settings(settingsToUse, takeOwnershipOfSettings)
        , channelConfiguration(channels)
    {

        createPlugin();

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr)
            options = std::make_unique<AudioDeviceManager::AudioDeviceSetup>(*preferredSetupOptions);

        auto audioInputRequired = (inChannels > 0);

        if (audioInputRequired && RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
            RuntimePermissions::request(RuntimePermissions::recordAudio, [this, preferredDefaultDeviceName](bool granted) { init(granted, preferredDefaultDeviceName); });
        else
            init(audioInputRequired, preferredDefaultDeviceName);
    }

    void init(bool enableAudioInput, String const& preferredDefaultDeviceName)
    {
        setupAudioDevices(enableAudioInput, preferredDefaultDeviceName, options.get());
        startPlaying();
    }

    ~StandalonePluginHolder() override
    {
        savePluginState();
        processor->suspendProcessing(true);
        stopPlaying();
        processor = nullptr;
        shutDownAudioDevices();
    }

    virtual void createPlugin()
    {
        processor = createPluginFilterOfType(AudioProcessor::wrapperType_Standalone);
        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails(44100, 512);
    }

    int getNumInputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns : processor->getMainBusNumInputChannels());
    }

    int getNumOutputChannels() const
    {
        if (processor == nullptr)
            return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numOuts : processor->getMainBusNumOutputChannels());
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
        if (settings != nullptr) {
            auto xml = deviceManager.createStateXml();

            settings->setValue("audioSetup", xml.get());
        }
    }

    void reloadAudioDeviceState(bool enableAudioInput, String const& preferredDefaultDeviceName, AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions)
    {
        std::unique_ptr<XmlElement> savedState;

        if (settings != nullptr) {
            savedState = settings->getXmlValue("audioSetup");
        }

        auto inputChannels = getNumInputChannels();
        auto outputChannels = getNumOutputChannels();

        if (inputChannels == 0 && outputChannels == 0 && processor->isMidiEffect()) {
            // add a dummy output channel for MIDI effect plug-ins so they can receive audio callbacks
            outputChannels = 1;
        }

        deviceManager.initialise(enableAudioInput ? inputChannels : 0, outputChannels, savedState.get(), true, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void savePluginState()
    {
        if (settings != nullptr && processor != nullptr) {
            MemoryBlock data;
            processor->getStateInformation(data);

            MemoryOutputStream ostream;
            Base64::convertToBase64(ostream, data.getData(), data.getSize());
            settings->setValue("filterState", ostream.toString());
        }
    }

    void reloadPluginState()
    {
        if (settings != nullptr) {
            // Async to give the app a chance to start up before loading the patch
            MessageManager::callAsync([this, _this = SafePointer(this)]() {
                if (_this) {
                    MemoryOutputStream data;
                    Base64::convertFromBase64(data, settings->getValue("filterState"));
                    if (data.getDataSize() > 0)
                        processor->setStateInformation(data.getData(), static_cast<int>(data.getDataSize()));
                }
            });
        }
    }

    bool isInterAppAudioConnected()
    {
        return false;
    }

    Image getIAAHostIcon(int size)
    {
        return {};
    }

    static StandalonePluginHolder* getInstance();

    OptionalScopedPointer<PropertySet> settings;
    std::unique_ptr<AudioProcessor> processor;
    AudioDeviceManager deviceManager;
    PlugDataProcessorPlayer player;
    Array<PluginInOuts> channelConfiguration;

    std::unique_ptr<AudioDeviceManager::AudioDeviceSetup> options;
    Array<MidiDeviceInfo> lastMidiDevices;

    std::unique_ptr<FileChooser> stateFileChooser;

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
    class CallbackMaxSizeEnforcer : public AudioIODeviceCallback {
    public:
        explicit CallbackMaxSizeEnforcer(AudioIODeviceCallback& callbackIn)
            : inner(callbackIn)
        {
        }

        void audioDeviceAboutToStart(AudioIODevice* device) override
        {
            maximumSize = device->getCurrentBufferSizeSamples();
            storedInputChannels.resize((size_t)device->getActiveInputChannels().countNumberOfSetBits());
            storedOutputChannels.resize((size_t)device->getActiveOutputChannels().countNumberOfSetBits());

            inner.audioDeviceAboutToStart(device);
        }

        void audioDeviceIOCallbackWithContext(float const* const* inputChannelData,
            int numInputChannels,
            float* const* outputChannelData,
            int numOutputChannels,
            int numSamples,
            AudioIODeviceCallbackContext const& context) override
        {
            jassertquiet((int)storedInputChannels.size() == numInputChannels);
            jassertquiet((int)storedOutputChannels.size() == numOutputChannels);

            int position = 0;

            while (position < numSamples) {
                auto const blockLength = jmin(maximumSize, numSamples - position);

                initChannelPointers(inputChannelData, storedInputChannels, position);
                initChannelPointers(outputChannelData, storedOutputChannels, position);

                inner.audioDeviceIOCallbackWithContext(storedInputChannels.data(),
                    (int)storedInputChannels.size(),
                    storedOutputChannels.data(),
                    (int)storedOutputChannels.size(),
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
        void initChannelPointers(Ptr&& source, Vector&& target, int offset)
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
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
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

    void setupAudioDevices(bool enableAudioInput, String const& preferredDefaultDeviceName, AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions)
    {
#if JUCE_IOS
        deviceManager.addAudioCallback(&maxSizeEnforcer);
#else
        deviceManager.addAudioCallback(this);
#endif
        deviceManager.addMidiInputDeviceCallback({}, &player);

        reloadAudioDeviceState(enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputDeviceCallback({}, &player);

#if JUCE_IOS
        deviceManager.removeAudioCallback(&maxSizeEnforcer);
#else
        deviceManager.removeAudioCallback(this);
#endif
    }

    OwnedArray<MidiInput> customMidiInputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandalonePluginHolder)
};

/**
 A class that can be used to run a simple standalone application containing your filter.

 Just create one of these objects in your JUCEApplicationBase::initialise() method, and
 let it do its work. It will create your filter object using the same createPluginFilter() function
 that the other plugin wrappers use.

 @tags{Audio}
 */

class PlugDataWindow : public DocumentWindow
    , public SettingsFileListener {

    Image shadowImage;
    AudioProcessorEditor* editor;
    StandalonePluginHolder* pluginHolder;

public:
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    bool movedFromDialog = false;
    SafePointer<Dialog> dialog;

    /** Creates a window with a given title and colour.
     The settings object can be a PropertySet that the class should use to
     store its settings (it can also be null). If takeOwnershipOfSettings is
     true, then the settings object will be owned and deleted by this object.
     */
    PlugDataWindow(AudioProcessorEditor* pluginEditor)
        : DocumentWindow("plugdata", LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton)
        , editor(pluginEditor)
    {
        setTitleBarHeight(0);
        pluginHolder = ProjectInfo::getStandalonePluginHolder();

        drawWindowShadow = Desktop::canUseSemiTransparentWindows();
        setOpaque(false);

        mainComponent = new MainContentComponent(*this, editor);

        setContentOwned(mainComponent, true);
        parentHierarchyChanged();
    }

    void parentHierarchyChanged() override
    {
        DocumentWindow::parentHierarchyChanged();

        auto nativeWindow = SettingsFile::getInstance()->getProperty<bool>("native_window");
#if JUCE_IOS
        nativeWindow = true;
#endif
        if(!mainComponent) return;
        
        auto* editor = mainComponent->getEditor();
        auto* pdEditor = dynamic_cast<PluginEditor*>(editor);
        
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
        
#if JUCE_WINDOWS
        if (auto peer = getPeer())
            OSUtils::useWindowsNativeDecorations(peer->getNativeHandle(), !isFullScreen());
#endif
        
        editor->resized();
        resized();
        lookAndFeelChanged();
    }


    void propertyChanged(String const& name, var const& value) override
    {
        if (name == "native_window") {
            auto* editor = mainComponent->getEditor();
            auto* pdEditor = dynamic_cast<PluginEditor*>(editor);
            pdEditor->nvgSurface.detachContext();
            recreateDesktopWindow();
        }
        if (name == "macos_buttons") {
            bool isEnabled = true;
            if (auto* closeButton = getCloseButton())
                isEnabled = closeButton->isEnabled();
            lookAndFeelChanged();
            if (auto* closeButton = getCloseButton())
                closeButton->setEnabled(isEnabled);
            if (auto* minimiseButton = getMinimiseButton())
                minimiseButton->setEnabled(isEnabled);
            if (auto* maximiseButton = getMaximiseButton())
                maximiseButton->setEnabled(isEnabled);
        }
    }

    ~PlugDataWindow() override
    {
        clearContentComponent();
    }

    BorderSize<int> getBorderThickness() override
    {
        return BorderSize<int>(0);
    }

    int getDesktopWindowStyleFlags() const override
    {
        auto flags = ComponentPeer::windowHasMinimiseButton | ComponentPeer::windowHasMaximiseButton | ComponentPeer::windowHasCloseButton | ComponentPeer::windowAppearsOnTaskbar | ComponentPeer::windowIsSemiTransparent | ComponentPeer::windowIsResizable;

#if !JUCE_IOS
        if (SettingsFile::getInstance()->getProperty<bool>("native_window"))
        {
            flags |= ComponentPeer::windowHasTitleBar;
            flags |= ComponentPeer::windowHasDropShadow;
        } else
#endif
        {

            
#if JUCE_MAC
            flags |= ComponentPeer::windowHasDropShadow;
#elif JUCE_WINDOWS
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

        if (auto* props = pluginHolder->settings.get())
            props->removeValue("filterState");

        pluginHolder->createPlugin();
        setContentOwned(new MainContentComponent(*this, pluginHolder->processor->createEditorIfNeeded()), true);
        pluginHolder->startPlaying();
    }

    // Prevents CMD+W from terminating app
    bool keyStateChanged(bool isKeyDown) override
    {
        if (KeyPress(87, ModifierKeys::commandModifier, 0).isCurrentlyDown())
            return true;
        else
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
        if (auto* b = getMaximiseButton()) {
            return b->getToggleState();
        } else {
            return isFullScreen();
        }
#else
        return isFullScreen();
#endif
    }
        
    bool useNativeTitlebar()
    {
        return SettingsFile::getInstance()->getProperty<bool>("native_window");
    }

    void maximiseButtonPressed() override
    {
#if JUCE_LINUX || JUCE_BSD
        if (auto* b = getMaximiseButton()) {
            if (auto* peer = getPeer()) {
                bool shouldBeMaximised = OSUtils::isX11WindowMaximised(peer->getNativeHandle());
                b->setToggleState(!shouldBeMaximised, dontSendNotification);

                if (!useNativeTitlebar()) {
                    OSUtils::maximiseX11Window(peer->getNativeHandle(), !shouldBeMaximised);
                }
            } else {
                b->setToggleState(false, dontSendNotification);
            }
        }
#else
        setFullScreen(!isFullScreen());
#endif
        resized();
    }

#if JUCE_LINUX || JUCE_BSD
    void paint(Graphics& g) override
    {
        if (drawWindowShadow && !useNativeTitlebar() && !isFullScreen()) {
            auto b = getLocalBounds();
            Path localPath;
            localPath.addRoundedRectangle(b.toFloat().reduced(22.0f), Corners::windowCornerRadius);

            int radius = isActiveWindow() ? 22 : 17;
            StackShadow::renderDropShadow(g, localPath, Colour(0, 0, 0).withAlpha(0.6f), radius, { 0, 2 });
        }
    }
#elif JUCE_WINDOWS
    void paintOverChildren(Graphics& g) override
    {
        if (SystemStats::getOperatingSystemType() != SystemStats::Windows11)
        {
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRect(0, 0, getWidth(), getHeight());
        }
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

    void moved() override
    {
        if (movedFromDialog) {
            movedFromDialog = false;
        } else if (dialog) {
            dialog.getComponent()->closeDialog();
        }
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
    class MainContentComponent : public Component
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
                if (auto* maximiseButton = owner.getMaximiseButton()) {
                    bool maximised = maximiseButton->getToggleState();
                    return maximised ? 0 : 18;
                }

                return 18;
            } else {
                return 0;
            }
#else
            return 0;
#endif
        }

        PopupMenu getMenuForIndex(int topLevelMenuIndex, String const& menuName) override
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
            }
        }

#if JUCE_IOS
        void paint(Graphics& g) override
        {
            g.fillAll(findColour(PlugDataColour::toolbarBackgroundColourId));
        }
#endif

        void resized() override
        {
            auto r = getLocalBounds().reduced(getMargin());

#if JUCE_IOS
            if (auto* peer = getPeer()) {
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

    public:
        void componentMovedOrResized(Component&, bool, bool) override
        {
            ScopedValueSetter<bool> const scope(preventResizingEditor, true);

            if (editor != nullptr) {
                auto rect = getSizeToContainEditor();

                setSize(rect.getWidth(), rect.getHeight());
            }
        }

    private:
        Rectangle<int> getSizeToContainEditor() const
        {
            if (editor != nullptr)
                return getLocalArea(editor.getComponent(), editor->getLocalBounds()).expanded(getMargin());

            return {};
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
