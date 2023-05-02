
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

#include "Constants.h"
#include "Utility/StackShadow.h"
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"

// For each OS, we have a different approach to rendering the window shadow
// macOS:
// - Use the native shadow, it works fine
// Windows:
//  - Native shadows don't work with rounded corners
//  - Putting a transparent margin around the window makes everything very slow
//  - We use a modified dropshadower class instead
// Linux:
// - Native shadow is inconsistent across window managers and distros (sometimes there is no shadow, even though other windows have it...)
// - Dropshadower is slow and glitchy
// - We use a transparent margin around the window to draw the shadow in

static bool drawWindowShadow = true;

namespace pd {
class Patch;
};

class StandalonePluginHolder : private AudioIODeviceCallback
    , private Value::Listener {
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
    StandalonePluginHolder(PropertySet* settingsToUse, bool takeOwnershipOfSettings = true, String const& preferredDefaultDeviceName = String(), AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions = nullptr, Array<PluginInOuts> const& channels = Array<PluginInOuts>())

        : settings(settingsToUse, takeOwnershipOfSettings)
        , channelConfiguration(channels)
    {
        shouldMuteInput.addListener(this);
        shouldMuteInput = !isInterAppAudioConnected();

        createPlugin();

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr)
            options = std::make_unique<AudioDeviceManager::AudioDeviceSetup>(*preferredSetupOptions);

        auto audioInputRequired = (inChannels > 0);

#if TESTING
        // init(audioInputRequired, preferredDefaultDeviceName);
#else
        if (audioInputRequired && RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
            RuntimePermissions::request(RuntimePermissions::recordAudio, [this, preferredDefaultDeviceName](bool granted) { init(granted, preferredDefaultDeviceName); });
        else
            init(audioInputRequired, preferredDefaultDeviceName);
#endif
    }

    void init(bool enableAudioInput, String const& preferredDefaultDeviceName)
    {
        setupAudioDevices(enableAudioInput, preferredDefaultDeviceName, options.get());

        startPlaying();
    }

    ~StandalonePluginHolder() override
    {

        deletePlugin();
        shutDownAudioDevices();
    }

    virtual void createPlugin()
    {
        processor = createPluginFilterOfType(AudioProcessor::wrapperType_Standalone);

        processor->disableNonMainBuses();
        processor->setRateAndBufferSizeDetails(44100, 512);

        processorHasPotentialFeedbackLoop = (getNumInputChannels() > 0 && getNumOutputChannels() > 0);
    }

    virtual void deletePlugin()
    {
        stopPlaying();
        processor = nullptr;
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

    static String getFilePatterns(String const& fileSuffix)
    {
        if (fileSuffix.isEmpty())
            return {};

        return (fileSuffix.startsWithChar('.') ? "*" : "*.") + fileSuffix;
    }

    Value& getMuteInputValue()
    {
        return shouldMuteInput;
    }
    bool getProcessorHasPotentialFeedbackLoop() const
    {
        return processorHasPotentialFeedbackLoop;
    }
    void valueChanged(Value& value) override
    {
        muteInput = getValue<bool>(value);
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

#if !(JUCE_IOS || JUCE_ANDROID)
            settings->setValue("shouldMuteInput", getValue<bool>(shouldMuteInput));
#endif
        }
    }

    void reloadAudioDeviceState(bool enableAudioInput, String const& preferredDefaultDeviceName, AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions)
    {
        std::unique_ptr<XmlElement> savedState;

        if (settings != nullptr) {
            savedState = settings->getXmlValue("audioSetup");

#if !(JUCE_IOS || JUCE_ANDROID)
            shouldMuteInput.setValue(false);
#endif
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
            MessageManager::callAsync([this](){
                MemoryOutputStream data;
                Base64::convertFromBase64(data, settings->getValue("filterState"));
                if (data.getDataSize() > 0)
                    processor->setStateInformation(data.getData(), static_cast<int>(data.getDataSize()));
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
    AudioProcessorPlayer player;
    Array<PluginInOuts> channelConfiguration;

    // avoid feedback loop by default
    bool processorHasPotentialFeedbackLoop = true;
    std::atomic<bool> muteInput { true };
    Value shouldMuteInput;
    AudioBuffer<float> emptyBuffer;

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
        if (muteInput) {
            emptyBuffer.clear();
            inputChannelData = emptyBuffer.getArrayOfReadPointers();
        }

        player.audioDeviceIOCallbackWithContext(inputChannelData,
            numInputChannels,
            outputChannelData,
            numOutputChannels,
            numSamples,
            context);
    }

    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        emptyBuffer.setSize(device->getActiveInputChannels().countNumberOfSetBits(), device->getCurrentBufferSizeSamples());
        emptyBuffer.clear();

        player.audioDeviceAboutToStart(device);
    }

    void audioDeviceStopped() override
    {
        player.audioDeviceStopped();
        emptyBuffer.setSize(0, 0);
    }

    void setupAudioDevices(bool enableAudioInput, String const& preferredDefaultDeviceName, AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions)
    {
        deviceManager.addAudioCallback(&maxSizeEnforcer);
        deviceManager.addMidiInputDeviceCallback({}, &player);

#if !JUCE_WINDOWS
        if (auto* newIn = MidiInput::createNewDevice("to plugdata", &player).release()) {
            customMidiInputs.add(newIn);
        }
#endif
        reloadAudioDeviceState(enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputDeviceCallback({}, &player);
        deviceManager.removeAudioCallback(&maxSizeEnforcer);
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
    std::unique_ptr<StackDropShadower> dropShadower;

public:
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    /** Creates a window with a given title and colour.
     The settings object can be a PropertySet that the class should use to
     store its settings (it can also be null). If takeOwnershipOfSettings is
     true, then the settings object will be owned and deleted by this object.
     */
    PlugDataWindow(String const& systemArguments, String const& title, Colour backgroundColour, PropertySet* settingsToUse, bool takeOwnershipOfSettings, String const& preferredDefaultDeviceName = String(), AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions = nullptr,
        Array<PluginInOuts> const& constrainToConfiguration = {})
        : DocumentWindow(title, backgroundColour, DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton)
    {

        setTitleBarHeight(0);

        drawWindowShadow = Desktop::canUseSemiTransparentWindows();

        setTitleBarButtonsRequired(DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton, false);

        pluginHolder = std::make_unique<StandalonePluginHolder>(settingsToUse, takeOwnershipOfSettings, preferredDefaultDeviceName, preferredSetupOptions, constrainToConfiguration);

        parseSystemArguments(systemArguments);

        mainComponent = new MainContentComponent(*this);
        auto* editor = mainComponent->getEditor();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        bool hasReloadStateProperty = settingsTree.hasProperty("reload_last_state");

        // When starting with any sysargs, assume we don't want the last patch to open
        // Prevents a possible crash and generally kinda makes sense
        if (systemArguments.isEmpty() && hasReloadStateProperty && static_cast<bool>(settingsTree.getProperty("reload_last_state"))) {
            pluginHolder->reloadPluginState();
        }

        setContentOwned(mainComponent, true);

        // Make sure it gets updated on init
        propertyChanged("native_window", settingsTree.getProperty("native_window"));

        auto const getWindowScreenBounds = [this]() -> Rectangle<int> {
            const auto width = getWidth();
            const auto height = getHeight();

            const auto& displays = Desktop::getInstance().getDisplays();

            if (auto* props = pluginHolder->settings.get()) {
                constexpr int defaultValue = -100;

                const auto x = props->getIntValue("windowX", defaultValue);
                const auto y = props->getIntValue("windowY", defaultValue);

                if (x != defaultValue && y != defaultValue) {
                    const auto screenLimits = displays.getDisplayForRect({ x, y, width, height })->userArea;

                    return { jlimit(screenLimits.getX(), jmax(screenLimits.getX(), screenLimits.getRight() - width), x), jlimit(screenLimits.getY(), jmax(screenLimits.getY(), screenLimits.getBottom() - height), y), width, height };
                }
            }

            const auto displayArea = displays.getPrimaryDisplay()->userArea;

            return { displayArea.getCentreX() - width / 2, displayArea.getCentreY() - height / 2, width, height };
        };

        setBoundsConstrained(getWindowScreenBounds());
    }

    void propertyChanged(String name, var value) override
    {
        if (name == "native_window") {

            bool nativeWindow = static_cast<bool>(value);

            auto* editor = getAudioProcessor()->getActiveEditor();
            
            setUsingNativeTitleBar(nativeWindow);

            if (!nativeWindow) {

                setOpaque(false);

                setResizable(false, false);

                if (drawWindowShadow) {

#if JUCE_MAC
                    setDropShadowEnabled(true);
#else
                    setDropShadowEnabled(false);
#endif

#if JUCE_WINDOWS
                    dropShadower = std::make_unique<StackDropShadower>(DropShadow(Colour(0, 0, 0).withAlpha(0.8f), 22, { 0, 3 }));
                    dropShadower->setOwner(this);
#endif
                } else {
                    setDropShadowEnabled(false);
                }
            } else {
                setOpaque(true);
                dropShadower.reset(nullptr);
                setDropShadowEnabled(true);
                setResizable(true, false);
            }

            editor->resized();
            resized();
            repaint();
        }
    }

    int parseSystemArguments(String const& arguments);

    ~PlugDataWindow() override
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder = nullptr;
    }

    BorderSize<int> getBorderThickness() override
    {
        return BorderSize<int>(0);
    }

    AudioProcessor* getAudioProcessor() const noexcept
    {
        return pluginHolder->processor.get();
    }

    /*
    AudioDeviceManager& getDeviceManager() const noexcept
    {
        return pluginHolder->deviceManager;
    } */

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder->deletePlugin();

        if (auto* props = pluginHolder->settings.get())
            props->removeValue("filterState");

        pluginHolder->createPlugin();
        setContentOwned(new MainContentComponent(*this), true);
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
        // Save plugin state to allow reloading
        pluginHolder->savePluginState();

        pluginHolder->processor->suspendProcessing(true);

        // Close all patches, allowing them to save first
        closeAllPatches();
    }

    void closeAllPatches(); // implemented in PlugDataApp.cpp

    void maximiseButtonPressed() override
    {
#if JUCE_LINUX
        if (auto* b = getMaximiseButton()) {
            if (auto* peer = getPeer()) {
                b->setToggleState(!OSUtils::isMaximised(peer->getNativeHandle()), dontSendNotification);
            } else {
                b->setToggleState(false, dontSendNotification);
            }
        }

        OSUtils::maximiseLinuxWindow(getPeer()->getNativeHandle());

#else
        setFullScreen(!isFullScreen());
#endif
        resized();
    }

#if JUCE_LINUX
    void paint(Graphics& g) override
    {
        if (drawWindowShadow && !isUsingNativeTitleBar() && !isFullScreen()) {
            auto b = getLocalBounds();
            Path localPath;
            localPath.addRoundedRectangle(b.toFloat().reduced(22.0f), Corners::windowCornerRadius);

            int radius = isActiveWindow() ? 21 : 16;
            StackShadow::renderDropShadow(g, localPath, Colour(0, 0, 0).withAlpha(0.6f), radius, { 0, 3 });
        }
    }
    void activeWindowStatusChanged() override
    {
        repaint();
    }
#elif JUCE_WINDOWS
    void activeWindowStatusChanged() override
    {
        if (drawWindowShadow && !isUsingNativeTitleBar() && dropShadower)
            dropShadower->repaint();
    }
#endif

    void resized() override
    {
        ResizableWindow::resized();

        
        Rectangle<int> titleBarArea(0, 7, getWidth() - 6, 23);

#if JUCE_LINUX
        if (!isFullScreen() && !isUsingNativeTitleBar() && drawWindowShadow) {
            auto margin = mainComponent ? mainComponent->getMargin() : 18;
            titleBarArea = Rectangle<int>(0, 7 + margin, getWidth() - (6 + margin), 23);
        }
#endif
        
        getLookAndFeel().positionDocumentWindowButtons(*this, titleBarArea.getX(), titleBarArea.getY(), titleBarArea.getWidth(), titleBarArea.getHeight(), getMinimiseButton(), getMaximiseButton(), getCloseButton(), false);

        if (auto* content = getContentComponent()) {
            content->resized();
            content->repaint();
            MessageManager::callAsync([this, content] {
                if (content->isShowing())
                    content->grabKeyboardFocus();
            });
        }
    }

    virtual StandalonePluginHolder* getPluginHolder()
    {
        return pluginHolder.get();
    }

    bool hasOpenedDialog();

    std::unique_ptr<StandalonePluginHolder> pluginHolder;

private:
    class MainContentComponent : public Component
        , private ComponentListener
        , public MenuBarModel {

    public:
        MainContentComponent(PlugDataWindow& filterWindow)
            : owner(filterWindow)
            , editor(owner.getAudioProcessor()->hasEditor() ? owner.getAudioProcessor()->createEditorIfNeeded() : new GenericAudioProcessorEditor(*owner.getAudioProcessor()))
        {
            inputMutedValue.referTo(owner.pluginHolder->getMuteInputValue());

            if (editor != nullptr) {

                auto* commandManager = dynamic_cast<ApplicationCommandManager*>(editor.get());

                // Menubar, only for standalone on mac
                // Doesn't add any new features, but was easy to implement because we already have a command manager
                setApplicationCommandManagerToWatch(commandManager);
#if JUCE_MAC && !TESTING
                MenuBarModel::setMacMainMenu(this);
#endif

                editor->addComponentListener(this);
                componentMovedOrResized(*editor, false, true);

                addAndMakeVisible(editor.get());
                editor->setAlwaysOnTop(true);
            }
        }

        void paintOverChildren(Graphics& g) override
        {
#if JUCE_LINUX
            if (!owner.isUsingNativeTitleBar() && !owner.hasOpenedDialog()) {
                g.setColour(findColour(PlugDataColour::outlineColourId));

                if (!Desktop::canUseSemiTransparentWindows()) {
                    g.drawRect(getLocalBounds().toFloat().reduced(getMargin()), 1.0f);
                } else {
                    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(getMargin()), Corners::windowCornerRadius, 1.0f);
                }
            }
#elif JUCE_WINDOWS

            g.setColour(findColour(PlugDataColour::outlineColourId));

            g.drawRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius, 1.0f);
#endif
        }

        AudioProcessorEditor* getEditor()
        {
            return editor.get();
        }

        StringArray getMenuBarNames() override
        {
            return { "File", "Edit" };
        }

        int getMargin() const
        {
            if (owner.isUsingNativeTitleBar() || owner.isFullScreen()) {
                return 0;
            }

#if JUCE_LINUX
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

            auto* commandManager = dynamic_cast<ApplicationCommandManager*>(editor.get());

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
#if JUCE_MAC && !TESTING
            MenuBarModel::setMacMainMenu(nullptr);
#endif
            if (editor != nullptr) {
                editor->removeComponentListener(this);
                owner.pluginHolder->processor->editorBeingDeleted(editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced(getMargin());

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
        Rectangle<int> oldBounds;

    private:
        void componentMovedOrResized(Component&, bool, bool) override
        {
            ScopedValueSetter<bool> const scope(preventResizingEditor, true);

            if (editor != nullptr) {
                auto rect = getSizeToContainEditor();

                setSize(rect.getWidth(), rect.getHeight());
            }
        }

        Rectangle<int> getSizeToContainEditor() const
        {
            if (editor != nullptr)
                return getLocalArea(editor.get(), editor->getLocalBounds()).expanded(getMargin());

            return {};
        }

        PlugDataWindow& owner;
        std::unique_ptr<AudioProcessorEditor> editor;
        Value inputMutedValue;
        bool preventResizingEditor = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
    };

    MainContentComponent* mainComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataWindow)
};

inline StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone) {
        auto& desktop = Desktop::getInstance();
        int const numTopLevelWindows = desktop.getNumComponents();

        for (int i = 0; i < numTopLevelWindows; ++i)
            if (auto window = dynamic_cast<PlugDataWindow*>(desktop.getComponent(i)))
                return window->getPluginHolder();
    }

    return nullptr;
}
