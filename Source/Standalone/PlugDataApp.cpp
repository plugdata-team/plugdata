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
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Pd/Setup.h"

#include "PlugDataWindow.h"
#include "Canvas.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Dialogs/Dialogs.h"

#if JUCE_WINDOWS
#    include <filesystem>
#endif

#ifdef _WIN32
#    include <io.h>
#    include <windows.h>
#    include <winbase.h>
#endif
#ifdef _MSC_VER /* This is only for Microsoft's compiler, not cygwin, e.g. */
#    define snprintf _snprintf
#endif

//#ifdef JUCE_DEBUG // Just to make sure this never accidentally gets in our CI builds
#define RUN_HELPFILE_TESTS 1

#if RUN_HELPFILE_TESTS
#include "Sidebar/Sidebar.h" // So we can read and clear the console
#endif
//#endif

class PlugDataApp : public JUCEApplication {

    Image logo = ImageFileFormat::loadFrom(BinaryData::plugdata_logo_png, BinaryData::plugdata_logo_pngSize);

public:
    PlugDataApp()
    {
        PluginHostType::jucePlugInClientCurrentWrapperType = AudioProcessor::wrapperType_Standalone;

        PropertiesFile::Options options;

        options.applicationName = "plugdata";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX || JUCE_BSD
        options.folderName = "~/.config";
#else
        options.folderName = "";
#endif

        appProperties.setStorageParameters(options);
        
#if RUN_HELPFILE_TESTS
        // Call after window is ready
        Timer::callAfterDelay(200, [this](){
            runTests();
        });
#endif
    }
    
#if RUN_HELPFILE_TESTS

    int numProcessed = 0;
    String loggedErrors;
    
    void openHelpfilesRecursively(TabComponent& tabbar, std::vector<File>& helpFiles)
    {
        if(helpFiles.empty())
        {
            std::cout << "TEST COMPLETED SUCCESFULLY" << std::endl;

            File("~/Projecten/plugdata/testdata.md").replaceWithText(loggedErrors);
            return;
        }
        
        auto helpFile = helpFiles.back();
        helpFiles.pop_back();
        
        std::cout << "STARTED PROCESSING: " << numProcessed++ << " " << helpFile.getFullPathName() << std::endl;
        
        tabbar.openPatch(URL(helpFile));

        auto* pd = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor())->pd;
        auto* editor = pd->getEditors()[0];
        
        // Evil test that deletes the patch instantly after being created, leaving dangling pointers everywhere
        // plugdata should be able to handle that!
#define TEST_PATCH_DETACHED 0
#if TEST_PATCH_DETACHED
       
        std::vector<pd::WeakReference> openedPatches;
        // Close all patches
        for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
            openedPatches.push_back(pd::WeakReference(cnv, pd));
        }
        
        pd->patches.clear();
        for(auto patch : openedPatches)
        {
            if(auto cnv = patch.get<t_glist*>()) {
                libpd_closefile(cnv.get());
            }
        }
#endif
        
        StringArray errors;
        auto messages = pd->getConsoleMessages();
        for(auto& [ptr, message, type, length, repeats] : messages)
        {
            if(type == 1)
            {
                errors.add(message);
            }
        }
        
        if(!errors.isEmpty())
        {
            loggedErrors += "\n\n\n" + helpFile.getFullPathName() + "\n--------------------------------------------------------------------------\n";
            for(auto& error : errors)
            {
                loggedErrors += error + "\n";
            }
        }
        
        editor->sidebar->clearConsole();
        
        Timer::callAfterDelay(50, [this, &helpFiles, tabbar = &tabbar]() mutable {
            while(auto* cnv = tabbar->getCurrentCanvas()) { // TODO: why is this faster than closeAllTabs?()
                tabbar->closeTab(cnv);
            }
            openHelpfilesRecursively(*tabbar, helpFiles);
        });
    }
    
    void runTests()
    {
        static std::vector<File> allHelpfiles = {};
        // Open every helpfile, this will make sure it initialises and closes every object at least once (but probasbly a whole bunch of times in different contexts)
        // Run with AddressSanitizer, UBSanitizer or ThreadSanitizer to find all memory, UB and threading problems
        for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation"), true, true))
        {
            if(file.hasFileExtension(".pd"))
            {
                allHelpfiles.push_back(file);
            }
        }

        auto& tabbar = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor())->getTabComponent();
        
        openHelpfilesRecursively(tabbar, allHelpfiles);
    }
#endif
    
    String const getApplicationName() override
    {
        return "plugdata";
    }
    String const getApplicationVersion() override
    {
        return PLUGDATA_VERSION;
    }
    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    // For opening files with plugdata standalone and parsing commandline arguments
    void anotherInstanceStarted(String const& commandLine) override
    {
        auto tokens = StringArray::fromTokens(commandLine, " ", "\"");
        auto file = File(tokens[0].unquoted());
        if (file.existsAsFile()) {
            auto* pd = dynamic_cast<PluginProcessor*>(pluginHolder->processor.get());
            auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
            if (pd && editor && file.existsAsFile()) {
                auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                editor->getTabComponent().openPatch(URL(file));
                SettingsFile::getInstance()->addToRecentlyOpened(file);
            }
        }
    }

    // Open file callback on iOS
    /*
    bool urlOpened(URL& url) override {
        anotherInstanceStarted(url.toString(false));
        return true;
    } */

    void initialise(String const& arguments) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colours::transparentBlack);

        pluginHolder = std::make_unique<StandalonePluginHolder>(appProperties.getUserSettings(), false, "");

        mainWindow = new PlugDataWindow(pluginHolder->processor->createEditorIfNeeded());

        mainWindow->setVisible(true);
        parseSystemArguments(arguments);

#if JUCE_LINUX || JUCE_BSD
        mainWindow->getPeer()->setIcon(logo);
#endif

        auto getWindowScreenBounds = [this]() -> juce::Rectangle<int> {
            const auto width = mainWindow->getWidth();
            const auto height = mainWindow->getHeight();

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
        mainWindow->setBoundsConstrained(getWindowScreenBounds());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        pluginHolder->stopPlaying();
        pluginHolder = nullptr;
        appProperties.saveIfNeeded();
    }

    int parseSystemArguments(String const& arguments)
    {
        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        bool hasReloadStateProperty = settingsTree.hasProperty("reload_last_state");

        // When starting with any sysargs, assume we don't want the last patch to open
        // Prevents a possible crash and generally kinda makes sense
        if (arguments.isEmpty() && hasReloadStateProperty && static_cast<bool>(settingsTree.getProperty("reload_last_state"))) {
            pluginHolder->reloadPluginState();
        }

        auto args = StringArray::fromTokens(arguments, true);
        size_t argc = args.size();

        auto argv = std::vector<char const*>(argc);

        for (int i = 0; i < args.size(); i++) {
            argv[i] = args.getReference(i).toRawUTF8();
        }

        t_namelist* openlist = nullptr;
        t_namelist* messagelist = nullptr;

        int retval = 0; // parse_startup_arguments(argv.data(), argc, &openlist, &messagelist);

        StringArray openedPatches;
        // open patches specifies with "-open" args
        for (auto* nl = openlist; nl; nl = nl->nl_next) {
            auto toOpen = File(String(nl->nl_string).unquoted());
            if (toOpen.existsAsFile() && toOpen.hasFileExtension("pd")) {

                auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                editor->getTabComponent().openPatch(URL(toOpen));
                SettingsFile::getInstance()->addToRecentlyOpened(toOpen);
                openedPatches.add(toOpen.getFullPathName());
            }
        }

#if JUCE_LINUX || JUCE_WINDOWS
        for (auto arg : args) {
            arg = arg.trim().unquoted().trim();

            // Would be best to enable this on Linux, but some distros use ancient gcc which doesn't have std::filesystem
#    if JUCE_WINDOWS
            if (!std::filesystem::exists(arg.toStdString()))
                continue;
#    endif
            auto toOpen = File(arg);
            if (toOpen.existsAsFile() && toOpen.hasFileExtension("pd") && !openedPatches.contains(toOpen.getFullPathName())) {
                auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                editor->getTabComponent().openPatch(URL(toOpen));
                SettingsFile::getInstance()->addToRecentlyOpened(toOpen);
            }
        }
#endif

        /* send messages specified with "-send" args */
        for (auto* nl = messagelist; nl; nl = nl->nl_next) {
            t_binbuf* b = binbuf_new();
            binbuf_text(b, nl->nl_string, strlen(nl->nl_string));
            binbuf_eval(b, nullptr, 0, nullptr);
            binbuf_free(b);
        }

        namelist_free(openlist);
        namelist_free(messagelist);
        messagelist = nullptr;

        return retval;
    }

    void systemRequestedQuit() override
    {
        if (ModalComponentManager::getInstance()->cancelAllModalComponents()) {
            Timer::callAfterDelay(100,
                []() {
                    if (auto app = JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
        } else if (mainWindow) {
            mainWindow->closeButtonPressed();
        } else {
            quit();
        }
    }

    std::unique_ptr<StandalonePluginHolder> pluginHolder;

protected:
    ApplicationProperties appProperties;
    PlugDataWindow* mainWindow;
};

void PlugDataWindow::closeAllPatches()
{
    // Show an ask to save dialog for each patch that is dirty
    // Because save dialog uses an asynchronous callback, we can't loop over them (so have to chain them)
    if (auto* editor = dynamic_cast<PluginEditor*>(mainComponent->getEditor())) {
        auto* processor = ProjectInfo::getStandalonePluginHolder()->processor.get();
        auto* mainEditor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());
        auto& openedEditors = editor->pd->openedEditors;

        if (editor == mainEditor) {
            processor->editorBeingDeleted(editor);
        }

        if (openedEditors.size() == 1) {
            editor->getTabComponent().closeAllTabs(true, nullptr, [this, editor, &openedEditors]() {
                editor->nvgSurface.detachContext();
                removeFromDesktop();
                openedEditors.removeObject(editor);
            });
        } else {
            editor->getTabComponent().closeAllTabs(false, nullptr, [this, editor, &openedEditors]() {
                editor->nvgSurface.detachContext();
                removeFromDesktop();
                openedEditors.removeObject(editor);
            });
        }
    }
}

StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone) {
        return dynamic_cast<PlugDataApp*>(JUCEApplicationBase::getInstance())->pluginHolder.get();
    }

    return nullptr;
}

START_JUCE_APPLICATION(PlugDataApp)
