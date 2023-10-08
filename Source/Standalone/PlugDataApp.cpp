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

#include "PlugDataWindow.h"
#include "Canvas.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Dialogs/Dialogs.h"

extern "C" {
#include <x_libpd_multi.h>
}

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

struct t_namelist /* element in a linked list of stored strings */
{
    struct t_namelist* nl_next; /* next in list */
    char* nl_string;            /* the string */
};

static void namelist_free(t_namelist* listwas)
{
    t_namelist *nl, *nl2;
    for (nl = listwas; nl; nl = nl2) {
        nl2 = nl->nl_next;
        t_freebytes(nl->nl_string, strlen(nl->nl_string) + 1);
        t_freebytes(nl, sizeof(*nl));
    }
}
static char const* strtokcpy(char* to, size_t to_len, char const* from, char delim)
{
    unsigned int i = 0;

    for (; i < (to_len - 1) && from[i] && from[i] != delim; i++)
        to[i] = from[i];
    to[i] = '\0';

    if (i && from[i] != '\0')
        return from + i + 1;

    return nullptr;
}

class PlugDataApp : public JUCEApplication {

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
    }

    const String getApplicationName() override
    {
        return "plugdata";
    }
    const String getApplicationVersion() override
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
            auto* pd = dynamic_cast<PluginProcessor*>(pluginHolder->processor.get());;


            if (pd && file.existsAsFile()) {
                pd->loadPatch(file);
                SettingsFile::getInstance()->addToRecentlyOpened(file);
            }
        }
    }

    void initialise(String const& arguments) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colours::transparentBlack);

        pluginHolder = std::make_unique<StandalonePluginHolder>(appProperties.getUserSettings(), false, "");
        
        mainWindow = new PlugDataWindow(pluginHolder->processor->createEditorIfNeeded());
        
        mainWindow->setVisible(true);
        mainWindow->parseSystemArguments(arguments);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        pluginHolder->stopPlaying();
        pluginHolder = nullptr;
        appProperties.saveIfNeeded();
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
    auto* editor = dynamic_cast<PluginEditor*>(mainComponent->getEditor());
    auto& openedEditors = editor->pd->openedEditors;
    
    if(openedEditors.size() == 1) {
        delete this;
        editor->closeAllTabs(true);
    }
    else {
        editor->closeAllTabs(false);
        removeFromDesktop();
        openedEditors.removeFirstMatchingValue(editor);
        delete this;
    }
}

int PlugDataWindow::parseSystemArguments(String const& arguments)
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

    int retval = parse_startup_arguments(argv.data(), argc, &openlist, &messagelist);

    StringArray openedPatches;
    // open patches specifies with "-open" args
    for (auto* nl = openlist; nl; nl = nl->nl_next) {
        auto toOpen = File(String(nl->nl_string).unquoted());
        if (toOpen.existsAsFile() && toOpen.hasFileExtension("pd")) {
            
            if (auto* pd = dynamic_cast<PluginProcessor*>(pluginHolder->processor.get())) {
                pd->loadPatch(toOpen);
                SettingsFile::getInstance()->addToRecentlyOpened(toOpen);
                openedPatches.add(toOpen.getFullPathName());
            }
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
            if (auto* pd = dynamic_cast<PluginProcessor*>(getAudioProcessor())) {
                pd->loadPatch(toOpen);
                SettingsFile::getInstance()->addToRecentlyOpened(toOpen);
            }
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

    return retval;
}

inline StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone) {
        return dynamic_cast<PlugDataApp*>(JUCEApplicationBase::getInstance())->pluginHolder.get();
    }

    return nullptr;
}

START_JUCE_APPLICATION(PlugDataApp)
