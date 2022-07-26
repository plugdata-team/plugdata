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

extern "C"
{
#include <x_libpd_multi.h>
}

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <winbase.h>
#endif
#ifdef _MSC_VER /* This is only for Microsoft's compiler, not cygwin, e.g. */
#define snprintf _snprintf
#endif

class PlugDataApp : public JUCEApplication
{
    struct t_namelist    /* element in a linked list of stored strings */
    {
        struct t_namelist *nl_next;  /* next in list */
        char *nl_string;            /* the string */
    };

    t_namelist *namelist_append_files(t_namelist *listwas, const char *s)
    {
        const char *npos;
        char temp[MAXPDSTRING];
        t_namelist *nl = listwas;

        npos = s;
        do
        {
            npos = strtokcpy(temp, sizeof(temp), npos, ':');
            if (! *temp) continue;
            nl = namelist_append(nl, temp, 0);
        }
            while (npos);
        return (nl);
    }

    t_namelist *namelist_append(t_namelist *listwas, const char *s, int allowdup)
    {
        t_namelist *nl, *nl2;
        nl2 = (t_namelist *)(getbytes(sizeof(*nl)));
        nl2->nl_next = 0;
        nl2->nl_string = (char *)getbytes(strlen(s) + 1);
        strcpy(nl2->nl_string, s);
        sys_unbashfilename(nl2->nl_string, nl2->nl_string);
        if (!listwas)
            return (nl2);
        else
        {
            for (nl = listwas; ;)
            {
                if (!allowdup && !strcmp(nl->nl_string, s))
                {
                    freebytes(nl2->nl_string, strlen(nl2->nl_string) + 1);
                    return (listwas);
                }
                if (!nl->nl_next)
                    break;
                nl = nl->nl_next;
            }
            nl->nl_next = nl2;
        }
        return (listwas);
    }
    
    void namelist_free(t_namelist *listwas)
    {
        t_namelist *nl, *nl2;
        for (nl = listwas; nl; nl = nl2)
        {
            nl2 = nl->nl_next;
            t_freebytes(nl->nl_string, strlen(nl->nl_string) + 1);
            t_freebytes(nl, sizeof(*nl));
        }
    }
    static const char *strtokcpy(char *to, size_t to_len, const char *from, char delim)
    {
        unsigned int i = 0;

            for (; i < (to_len - 1) && from[i] && from[i] != delim; i++)
                    to[i] = from[i];
            to[i] = '\0';

            if (i && from[i] != '\0')
                    return from + i + 1;

            return NULL;
    }
    
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

    // For opening files with PlugData standalone and parsing commandline arguments
    void anotherInstanceStarted(const String& commandLine) override
    {
        auto file = File(commandLine.upToFirstOccurrenceOf(" ", false, false));
        if (file.existsAsFile())
        {
            auto* pd = dynamic_cast<PlugDataAudioProcessor*>(mainWindow->getAudioProcessor());

            if (pd && file.existsAsFile())
            {
                pd->loadPatch(file);
            }
        }
    }

    virtual PlugDataWindow* createWindow()
    {
        return new PlugDataWindow(getApplicationName(), LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), appProperties.getUserSettings(), false, {}, nullptr, {});
    }

    int parseSystemArguments(const String& arguments);

    void initialise(const String& arguments) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colours::transparentBlack);

        mainWindow.reset(createWindow());

        mainWindow->setVisible(true);

        parseSystemArguments(arguments);

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
    
    PlugDataWindow* getWindow()  {
        return mainWindow.get();
    }

   protected:
    ApplicationProperties appProperties;
    std::unique_ptr<PlugDataWindow> mainWindow;
};

void PlugDataWindow::closeButtonPressed()
{
#if JUCE_MAC
    if(Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCommandDown()) {
        return;
        
    }
#endif
    
    // Show an ask to save dialog for each patch that is dirty
    // Because save dialog uses an asynchronous callback, we can't loop over them (so have to chain them)
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pluginHolder->processor->getActiveEditor()))
    {
        std::function<void(int)> checkCanvas;
        checkCanvas = [this, editor, checkCanvas](int i) mutable
        {
            auto* cnv = editor->canvases[i];
            bool isLast = i == editor->canvases.size() - 1;
            editor->tabbar.setCurrentTabIndex(i);

            i++;

            if (cnv->patch.isDirty())
            {
                Dialogs::showSaveDialog(editor->getParentComponent(), cnv->patch.getTitle(),
                                        [this, editor, cnv, checkCanvas, i, isLast](int result) mutable
                                        {
                                            if (result == 2)
                                            {
                                                editor->saveProject(
                                                    [this, cnv, editor, checkCanvas, i, isLast]() mutable
                                                    {
                                                        if (isLast)
                                                        {
                                                            JUCEApplication::quit();
                                                        }
                                                        else
                                                        {
                                                            checkCanvas(i);
                                                        }
                                                    });
                                            }
                                            else if (result == 1)
                                            {
                                                if (isLast)
                                                {
                                                    JUCEApplication::quit();
                                                }
                                                else
                                                {
                                                    checkCanvas(i);
                                                }
                                            }
                                            // last option: cancel, where we end the chain
                                        });
            }
            else if (!isLast)
            {
                checkCanvas(i);
            }
            else
            {
                JUCEApplication::quit();
            }
        };

        checkCanvas(0);
    }
}

int PlugDataApp::parseSystemArguments(const String& arguments)
{
    auto args = StringArray::fromTokens(arguments, true);
    size_t argc = args.size();
    const char** argv = new const char*[argc];

    for (int i = 0; i < args.size(); i++)
    {
        argv[i] = args.getReference(i).toRawUTF8();
    }
    
    int retval = parse_startup_arguments(argv, argc);
    
    static t_namelist *sys_openlist;
    static t_namelist *sys_messagelist;

    for (; argc > 0; argc--, argv++) sys_openlist = namelist_append_files(sys_openlist, *argv);

    /* open patches specifies with "-open" args */
    for (auto* nl = sys_openlist; nl; nl = nl->nl_next)
    {
        
        auto toOpen = File(String(nl->nl_string));

        auto* pd = dynamic_cast<PlugDataAudioProcessor*>(mainWindow->getAudioProcessor());
        if (pd && toOpen.existsAsFile())
        {
            pd->loadPatch(toOpen);
        }
    }


    /* send messages specified with "-send" args */
    for (auto* nl = sys_messagelist; nl; nl = nl->nl_next)
    {
        t_binbuf* b = binbuf_new();
        binbuf_text(b, nl->nl_string, strlen(nl->nl_string));
        binbuf_eval(b, nullptr, 0, nullptr);
        binbuf_free(b);
    }

    namelist_free(sys_messagelist);
    sys_messagelist = nullptr;
    
    return retval;
}


JUCE_CREATE_APPLICATION_DEFINE(PlugDataApp);
