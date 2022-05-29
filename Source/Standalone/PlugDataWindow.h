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

#include <JuceHeader.h>

#include "../PluginEditor.h"

#if !JUCE_MAC
#define CUSTOM_SHADOW 1
#else
#define CUSTOM_SHADOW 0
#endif

namespace pd
{
class Patch;
};

class StandalonePluginHolder : private AudioIODeviceCallback, private Timer, private Value::Listener
{
   public:
    /** Structure used for the number of inputs and outputs. */
    struct PluginInOuts
    {
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
    StandalonePluginHolder(PropertySet* settingsToUse, bool takeOwnershipOfSettings = true, const String& preferredDefaultDeviceName = String(), const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr, const Array<PluginInOuts>& channels = Array<PluginInOuts>(),
#if JUCE_ANDROID || JUCE_IOS
                           bool shouldAutoOpenMidiDevices = true
#else
                           bool shouldAutoOpenMidiDevices = false
#endif
                           )

        : settings(settingsToUse, takeOwnershipOfSettings),
          channelConfiguration(channels),
          autoOpenMidiDevices(shouldAutoOpenMidiDevices)
    {
        shouldMuteInput.addListener(this);
        shouldMuteInput = !isInterAppAudioConnected();

        createPlugin();

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr) options = std::make_unique<AudioDeviceManager::AudioDeviceSetup>(*preferredSetupOptions);

        auto audioInputRequired = (inChannels > 0);

        if (audioInputRequired && RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
            RuntimePermissions::request(RuntimePermissions::recordAudio, [this, preferredDefaultDeviceName](bool granted) { init(granted, preferredDefaultDeviceName); });
        else
            init(audioInputRequired, preferredDefaultDeviceName);
    }

    void init(bool enableAudioInput, const String& preferredDefaultDeviceName)
    {
        setupAudioDevices(enableAudioInput, preferredDefaultDeviceName, options.get());
#if JUCE_DEBUG
        // reloadPluginState();
#endif
        startPlaying();

        if (autoOpenMidiDevices) startTimer(500);
    }

    ~StandalonePluginHolder() override
    {
        stopTimer();

        deletePlugin();
        shutDownAudioDevices();
    }

    virtual void createPlugin()
    {
        processor.reset(createPluginFilterOfType(AudioProcessor::wrapperType_Standalone));
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
        if (processor == nullptr) return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns : processor->getMainBusNumInputChannels());
    }

    int getNumOutputChannels() const
    {
        if (processor == nullptr) return 0;

        return (channelConfiguration.size() > 0 ? channelConfiguration[0].numOuts : processor->getMainBusNumOutputChannels());
    }

    static String getFilePatterns(const String& fileSuffix)
    {
        if (fileSuffix.isEmpty()) return {};

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
        muteInput = static_cast<bool>(value.getValue());
    }

    File getLastFile() const
    {
        File f;

        if (settings != nullptr) f = File(settings->getValue("lastStateFile"));

        if (f == File()) f = File::getSpecialLocation(File::userDocumentsDirectory);

        return f;
    }

    void setLastFile(const FileChooser& fc)
    {
        if (settings != nullptr) settings->setValue("lastStateFile", fc.getResult().getFullPathName());
    }

    /** Pops up a dialog letting the user save the processor's state to a file. */
    void askUserToSaveState(const String& fileSuffix = String())
    {
        stateFileChooser = std::make_unique<FileChooser>(TRANS("Save current state"), getLastFile(), getFilePatterns(fileSuffix));
        auto flags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;

        stateFileChooser->launchAsync(flags,
                                      [this](const FileChooser& fc)
                                      {
                                          if (fc.getResult() == File{}) return;

                                          setLastFile(fc);

                                          MemoryBlock data;
                                          processor->getStateInformation(data);

                                          if (!fc.getResult().replaceWithData(data.getData(), data.getSize())) AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, TRANS("Error whilst saving"), TRANS("Couldn't write to the specified file!"));
                                      });
    }

    /** Pops up a dialog letting the user re-load the processor's state from a file. */
    void askUserToLoadState(const String& fileSuffix = String())
    {
        stateFileChooser = std::make_unique<FileChooser>(TRANS("Load a saved state"), getLastFile(), getFilePatterns(fileSuffix));
        auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

        stateFileChooser->launchAsync(flags,
                                      [this](const FileChooser& fc)
                                      {
                                          if (fc.getResult() == File{}) return;

                                          setLastFile(fc);

                                          MemoryBlock data;

                                          if (fc.getResult().loadFileAsData(data))
                                              processor->setStateInformation(data.getData(), static_cast<int>(data.getSize()));
                                          else
                                              AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, TRANS("Error whilst loading"), TRANS("Couldn't read from the specified file!"));
                                      });
    }

    void startPlaying()
    {
        player.setProcessor(processor.get());

#if JucePlugin_Enable_IAA && JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*>(deviceManager.getCurrentAudioDevice()))
        {
            processor->setPlayHead(device->getAudioPlayHead());
            device->setMidiMessageCollector(&player.getMidiMessageCollector());
        }
#endif
    }

    void stopPlaying()
    {
        player.setProcessor(nullptr);
    }

    void saveAudioDeviceState()
    {
        if (settings != nullptr)
        {
            auto xml = deviceManager.createStateXml();

            settings->setValue("audioSetup", xml.get());

#if !(JUCE_IOS || JUCE_ANDROID)
            settings->setValue("shouldMuteInput", static_cast<bool>(shouldMuteInput.getValue()));
#endif
        }
    }

    void reloadAudioDeviceState(bool enableAudioInput, const String& preferredDefaultDeviceName, const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        std::unique_ptr<XmlElement> savedState;

        if (settings != nullptr)
        {
            savedState = settings->getXmlValue("audioSetup");

#if !(JUCE_IOS || JUCE_ANDROID)
            shouldMuteInput.setValue(false);
#endif
        }

        auto inputChannels = getNumInputChannels();
        auto outputChannels = getNumOutputChannels();

        if (inputChannels == 0 && outputChannels == 0 && processor->isMidiEffect())
        {
            // add a dummy output channel for MIDI effect plug-ins so they can receive audio callbacks
            outputChannels = 1;
        }

        deviceManager.initialise(enableAudioInput ? inputChannels : 0, outputChannels, savedState.get(), true, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void savePluginState()
    {
        if (settings != nullptr && processor != nullptr)
        {
            MemoryBlock data;
            processor->getStateInformation(data);

            settings->setValue("filterState", data.toBase64Encoding());
        }
    }

    void reloadPluginState()
    {
        if (settings != nullptr)
        {
            MemoryBlock data;

            if (data.fromBase64Encoding(settings->getValue("filterState")) && data.getSize() > 0) processor->setStateInformation(data.getData(), static_cast<int>(data.getSize()));
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
    std::atomic<bool> muteInput{true};
    Value shouldMuteInput;
    AudioBuffer<float> emptyBuffer;
    bool autoOpenMidiDevices;

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
    class CallbackMaxSizeEnforcer : public AudioIODeviceCallback
    {
       public:
        explicit CallbackMaxSizeEnforcer(AudioIODeviceCallback& callbackIn) : inner(callbackIn)
        {
        }

        void audioDeviceAboutToStart(AudioIODevice* device) override
        {
            maximumSize = device->getCurrentBufferSizeSamples();
            storedInputChannels.resize(static_cast<size_t>(device->getActiveInputChannels().countNumberOfSetBits()));
            storedOutputChannels.resize(static_cast<size_t>(device->getActiveOutputChannels().countNumberOfSetBits()));

            inner.audioDeviceAboutToStart(device);
        }

        void audioDeviceIOCallbackWithContext (const float** inputChannelData,
                                               int numInputChannels,
                                               float** outputChannelData,
                                               int numOutputChannels,
                                               int numSamples,
                                               const AudioIODeviceCallbackContext& context) override
        {
            jassertquiet ((int) storedInputChannels.size()  == numInputChannels);
            jassertquiet ((int) storedOutputChannels.size() == numOutputChannels);

            int position = 0;

            while (position < numSamples)
            {
                const auto blockLength = jmin (maximumSize, numSamples - position);

                initChannelPointers (inputChannelData,  storedInputChannels,  position);
                initChannelPointers (outputChannelData, storedOutputChannels, position);

                inner.audioDeviceIOCallbackWithContext (storedInputChannels.data(),
                                                        (int) storedInputChannels.size(),
                                                        storedOutputChannels.data(),
                                                        (int) storedOutputChannels.size(),
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
        struct GetChannelWithOffset
        {
            int offset;

            template <typename Ptr>
            auto operator()(Ptr ptr) const noexcept -> Ptr
            {
                return ptr + offset;
            }
        };

        template <typename Ptr, typename Vector>
        void initChannelPointers(Ptr&& source, Vector&& target, int offset)
        {
            std::transform(source, source + target.size(), target.begin(), GetChannelWithOffset{offset});
        }

        AudioIODeviceCallback& inner;
        int maximumSize = 0;
        std::vector<const float*> storedInputChannels;
        std::vector<float*> storedOutputChannels;
    };

    CallbackMaxSizeEnforcer maxSizeEnforcer{*this};

    void audioDeviceIOCallbackWithContext (const float** inputChannelData,
                                           int numInputChannels,
                                           float** outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        if (muteInput)
        {
            emptyBuffer.clear();
            inputChannelData = emptyBuffer.getArrayOfReadPointers();
        }

        player.audioDeviceIOCallbackWithContext (inputChannelData,
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
        player.setMidiOutput(deviceManager.getDefaultMidiOutput());
    }

    void audioDeviceStopped() override
    {
        player.setMidiOutput(nullptr);
        player.audioDeviceStopped();
        emptyBuffer.setSize(0, 0);
    }

    void setupAudioDevices(bool enableAudioInput, const String& preferredDefaultDeviceName, const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        deviceManager.addAudioCallback(&maxSizeEnforcer);
        deviceManager.addMidiInputDeviceCallback({}, &player);

        reloadAudioDeviceState(enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputDeviceCallback({}, &player);
        deviceManager.removeAudioCallback(&maxSizeEnforcer);
    }

    void timerCallback() override
    {
        auto newMidiDevices = MidiInput::getAvailableDevices();

        if (newMidiDevices != lastMidiDevices)
        {
            for (auto& oldDevice : lastMidiDevices)
                if (!newMidiDevices.contains(oldDevice)) deviceManager.setMidiInputDeviceEnabled(oldDevice.identifier, false);

            for (auto& newDevice : newMidiDevices)
                if (!lastMidiDevices.contains(newDevice)) deviceManager.setMidiInputDeviceEnabled(newDevice.identifier, true);

            lastMidiDevices = newMidiDevices;
        }
    }

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
{
    // Replacement for native Windows shadow, to allow rounded corners
#if CUSTOM_SHADOW
    Image shadowImageT, shadowImageB, shadowImageL, shadowImageR;
    DropShadow shadow = DropShadow(Colour(20, 20, 20).withAlpha(0.3f), 6, Point<int>(0, 0));
#endif

   public:
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    /** Creates a window with a given title and colour.
        The settings object can be a PropertySet that the class should use to
        store its settings (it can also be null). If takeOwnershipOfSettings is
        true, then the settings object will be owned and deleted by this object.
    */
    PlugDataWindow(const String& title, Colour backgroundColour, PropertySet* settingsToUse, bool takeOwnershipOfSettings, const String& preferredDefaultDeviceName = String(), const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                   const Array<PluginInOuts>& constrainToConfiguration = {},
#if JUCE_ANDROID || JUCE_IOS
                   bool autoOpenMidiDevices = true
#else
                   bool autoOpenMidiDevices = false
#endif
                   )
        : DocumentWindow(title, backgroundColour, DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton)
    {
#if CUSTOM_SHADOW
        setDropShadowEnabled(false);
#endif
        setTitleBarHeight(0);
        setTitleBarButtonsRequired(DocumentWindow::minimiseButton | DocumentWindow::maximiseButton | DocumentWindow::closeButton, false);

        pluginHolder = std::make_unique<StandalonePluginHolder>(settingsToUse, takeOwnershipOfSettings, preferredDefaultDeviceName, preferredSetupOptions, constrainToConfiguration, autoOpenMidiDevices);

        setOpaque(false);
        setContentOwned(new MainContentComponent(*this), true);

        const auto windowScreenBounds = [this]() -> Rectangle<int>
        {
            const auto width = getWidth();
            const auto height = getHeight();

            const auto& displays = Desktop::getInstance().getDisplays();

            if (auto* props = pluginHolder->settings.get())
            {
                constexpr int defaultValue = -100;

                const auto x = props->getIntValue("windowX", defaultValue);
                const auto y = props->getIntValue("windowY", defaultValue);

                if (x != defaultValue && y != defaultValue)
                {
                    const auto screenLimits = displays.getDisplayForRect({x, y, width, height})->userArea;

                    return {jlimit(screenLimits.getX(), jmax(screenLimits.getX(), screenLimits.getRight() - width), x), jlimit(screenLimits.getY(), jmax(screenLimits.getY(), screenLimits.getBottom() - height), y), width, height};
                }
            }

            const auto displayArea = displays.getPrimaryDisplay()->userArea;

            return {displayArea.getCentreX() - width / 2, displayArea.getCentreY() - height / 2, width, height};
        }();

        setBoundsConstrained(windowScreenBounds);

        if (auto* processor = getAudioProcessor())
            if (auto* editor = processor->getActiveEditor()) setResizable(editor->isResizable(), false);
    }

    ~PlugDataWindow() override
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder = nullptr;
    }

    AudioProcessor* getAudioProcessor() const noexcept
    {
        return pluginHolder->processor.get();
    }
    AudioDeviceManager& getDeviceManager() const noexcept
    {
        return pluginHolder->deviceManager;
    }

    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder->deletePlugin();

        if (auto* props = pluginHolder->settings.get()) props->removeValue("filterState");

        pluginHolder->createPlugin();
        setContentOwned(new MainContentComponent(*this), true);
        pluginHolder->startPlaying();
    }

    void closeButtonPressed() override;  // implemented in PlugDataApp.cpp

    void maximiseButtonPressed() override
    {
        setFullScreen(!isFullScreen());
    }

    // Fixes shadow with rounded edges on windows
#if CUSTOM_SHADOW
    void paint(Graphics& g) override
    {
        g.drawImageAt(shadowImageT, 0, 0);
        g.drawImageAt(shadowImageB, 0, getHeight() - 6);
        g.drawImageAt(shadowImageL, 0, 6);
        g.drawImageAt(shadowImageR, getWidth() - 6, 6);
    }
#endif

    void resized() override
    {
        ResizableWindow::resized();

        if (auto* b = getMaximiseButton()) b->setToggleState(isFullScreen(), dontSendNotification);

        auto titleBarArea = Rectangle<int>(0, 12, getWidth() - 8, 25);

        getLookAndFeel().positionDocumentWindowButtons(*this, titleBarArea.getX(), titleBarArea.getY(), titleBarArea.getWidth(), titleBarArea.getHeight(), getMinimiseButton(), getMaximiseButton(), getCloseButton(), false);

#if CUSTOM_SHADOW
        auto shadowImage = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);
        Graphics g(shadowImage);
        auto b = getLocalBounds();
        Path localPath;
        localPath.addRoundedRectangle(b.toFloat().reduced(4), 6.0f);
        shadow.drawForPath(g, localPath);

        g.setColour(Colour(186, 186, 186));
        g.drawRoundedRectangle(b.toFloat().reduced(4), 6.0f, 1.0f);

        auto top = b.removeFromTop(6);
        shadowImageT = shadowImage.getClippedImage(top);

        auto bottom = b.removeFromBottom(6);
        shadowImageB = shadowImage.getClippedImage(bottom);

        auto left = b.removeFromLeft(6);
        shadowImageL = shadowImage.getClippedImage(left);

        auto right = b.removeFromRight(6);
        shadowImageR = shadowImage.getClippedImage(right);

#endif
    }

    virtual StandalonePluginHolder* getPluginHolder()
    {
        return pluginHolder.get();
    }

    std::unique_ptr<StandalonePluginHolder> pluginHolder;

   private:
    class MainContentComponent : public Component, private ComponentListener, public MenuBarModel
    {
       public:
        MainContentComponent(PlugDataWindow& filterWindow) : owner(filterWindow), editor(owner.getAudioProcessor()->hasEditor() ? owner.getAudioProcessor()->createEditorIfNeeded() : new GenericAudioProcessorEditor(*owner.getAudioProcessor()))
        {
            inputMutedValue.referTo(owner.pluginHolder->getMuteInputValue());

            if (editor != nullptr)
            {
                auto* commandManager = dynamic_cast<ApplicationCommandManager*>(editor.get());

                // Menubar, only for standalone on mac
                // Doesn't add any new features, but was easy to implement because we already have a command manager
                setApplicationCommandManagerToWatch(commandManager);
#if JUCE_MAC
                MenuBarModel::setMacMainMenu(this);
#endif

                editor->addComponentListener(this);
                componentMovedOrResized(*editor, false, true);

                addAndMakeVisible(editor.get());
                editor->setAlwaysOnTop(true);
            }
        }

        StringArray getMenuBarNames() override
        {
            return {"File", "Edit"};
        }

        PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName) override
        {
            PopupMenu menu;

            auto* commandManager = dynamic_cast<ApplicationCommandManager*>(editor.get());

            if (topLevelMenuIndex == 0)
            {
                menu.addCommandItem(commandManager, CommandIDs::NewProject);
                menu.addCommandItem(commandManager, CommandIDs::OpenProject);
                menu.addCommandItem(commandManager, CommandIDs::SaveProject);
                menu.addCommandItem(commandManager, CommandIDs::SaveProjectAs);
            }
            else
            {
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
#if JUCE_MAC
            MenuBarModel::setMacMainMenu(nullptr);
#endif
            if (editor != nullptr)
            {
                editor->removeComponentListener(this);
                owner.pluginHolder->processor->editorBeingDeleted(editor.get());
                editor = nullptr;
            }
        }

        void resized() override
        {
            auto r = getLocalBounds();

            if (editor != nullptr)
            {
                const auto newPos = r.getTopLeft().toFloat().transformedBy(editor->getTransform().inverted());

                if (preventResizingEditor)
                    editor->setTopLeftPosition(newPos.roundToInt());
                else
                    editor->setBoundsConstrained(editor->getLocalArea(this, r.toFloat()).withPosition(newPos).toNearestInt());
            }
        }

       private:
        void componentMovedOrResized(Component&, bool, bool) override
        {
            const ScopedValueSetter<bool> scope(preventResizingEditor, true);

            if (editor != nullptr)
            {
                auto rect = getSizeToContainEditor();

                setSize(rect.getWidth(), rect.getHeight());
            }
        }

        Rectangle<int> getSizeToContainEditor() const
        {
            if (editor != nullptr) return getLocalArea(editor.get(), editor->getLocalBounds());

            return {};
        }

        PlugDataWindow& owner;
        std::unique_ptr<AudioProcessorEditor> editor;
        Value inputMutedValue;
        bool preventResizingEditor = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
    };

    std::function<void(int)> checkCanvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataWindow)
};

inline StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
#if JucePlugin_Enable_IAA || JucePlugin_Build_Standalone
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone)
    {
        auto& desktop = Desktop::getInstance();
        const int numTopLevelWindows = desktop.getNumComponents();

        for (int i = 0; i < numTopLevelWindows; ++i)
            if (auto window = dynamic_cast<PlugDataWindow*>(desktop.getComponent(i))) return window->getPluginHolder();
    }
#endif

    return nullptr;
}
