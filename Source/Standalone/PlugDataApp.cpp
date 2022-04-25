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
#include "PlugDataWindow.h"
#include "../Canvas.h"
#include "../PluginProcessor.h"

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
            auto* pd = dynamic_cast<PlugDataAudioProcessor*>(mainWindow->getAudioProcessor());

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
        return new PlugDataWindow(getApplicationName(), LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), appProperties.getUserSettings(), false, {}, nullptr, {});
    }

    void initialise(const String&) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colours::transparentBlack);
        
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



void PlugDataWindow::closeButtonPressed()
{
    pluginHolder->savePluginState();
    
    // Show an ask to save dialog for each patch that is dirty
    // Because save dialog uses an asynchronous callback, we can't loop over them (so have to chain them)
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pluginHolder->processor->getActiveEditor())) {
        checkCanvas = [this, editor](int i) mutable {
            auto* cnv = editor->canvases[i];
            bool isLast = i == editor->canvases.size() - 1;
            editor->tabbar.setCurrentTabIndex(i);
            
            i++;
            
            if(cnv->patch.isDirty()) {
                Dialogs::showSaveDialog(editor, cnv->patch.getTitle(),
                                        [this, editor, cnv, i, isLast](int result) mutable
            {
                if (result == 2)
                {
                    editor->saveProject([this, cnv, editor, i, isLast]() mutable {
                        if(isLast) {
                            JUCEApplication::quit();
                        }
                        else {
                            checkCanvas(i);
                        }
                    });
                }
                else if (result == 1) {
                    if(isLast) {
                        JUCEApplication::quit();
                    }
                    else {
                        checkCanvas(i);
                    }
                }
                    // last option: cancel, where we end the chain
                });

            }
            else if (!isLast) {
                checkCanvas(i);
            }
            else {
                JUCEApplication::quit();
            }
        };
        
        checkCanvas(0);
    }
};

JUCE_CREATE_APPLICATION_DEFINE(PlugDataApp);
