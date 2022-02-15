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

#include <JuceHeader.h>

#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#include "PlugDataWindow.h"

class PlugDataApp : public JUCEApplication
{
   public:
    PlugDataApp()
    {
        PluginHostType::jucePlugInClientCurrentWrapperType = AudioProcessor::wrapperType_Standalone;

        PropertiesFile::Options options;
        

        options.applicationName = "PlugData";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
#else
        options.folderName = "";
#endif

        appProperties.setStorageParameters(options);
    }

    const String getApplicationName() override
    {
        return "PlugData";
    }
    const String getApplicationVersion() override
    {
        return JucePlugin_VersionString;
    }
    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    // For opening files with PlugData standalone
    void anotherInstanceStarted(const String& commandLine) override
    {
        auto file = File(commandLine.upToFirstOccurrenceOf(" ", false, false));
        if (file.existsAsFile())
        {
            auto* pd = dynamic_cast<PatchLoader*>(mainWindow->getAudioProcessor());

            if (pd)
            {
                if (file.existsAsFile())
                {
                    pd->loadPatch(file);
                }
            }
        }
    }

    virtual PlugDataWindow* createWindow()
    {
#ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = {JucePlugin_PreferredChannelConfigurations};
#endif

        return new PlugDataWindow(getApplicationName(), LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), appProperties.getUserSettings(), false, {}, nullptr
#ifdef JucePlugin_PreferredChannelConfigurations
                                  ,
                                  Array<StandalonePluginHolder::PluginInOuts>(channels, numElementsInArray(channels))
#else
                                  ,
                                  {}
#endif
#if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                                      ,
                                  false
#endif
        );
    }

    void initialise(const String&) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colour(20, 20, 20));
        mainWindow.reset(createWindow());

        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow) mainWindow->pluginHolder->savePluginState();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            Timer::callAfterDelay(100,
                                  []()
                                  {
                                      if (auto app = JUCEApplicationBase::getInstance()) app->systemRequestedQuit();
                                  });
        }
        else
        {
            quit();
        }
    }

   protected:
    ApplicationProperties appProperties;
    std::unique_ptr<PlugDataWindow> mainWindow;
};

JUCE_CREATE_APPLICATION_DEFINE(PlugDataApp);

#endif
