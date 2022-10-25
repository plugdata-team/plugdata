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

/*
#if JUCE_LINUX || JUCE_BSD
namespace xlib
{
extern "C" {
    #include <X11/Xlib.h>
    #include <X11/Xatom.h>

    void maximizeWindow(Component* component) {
        
      Window win = (Window)component->getPeer()->getNativeHandle();
      auto display = XOpenDisplay(NULL);

      XEvent ev;
      ev.xclient.window = win;
      ev.xclient.type = ClientMessage;
      ev.xclient.format = 32;
      ev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
      ev.xclient.data.l[0] = 1;
      ev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
      ev.xclient.data.l[2] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
      ev.xclient.data.l[3] = 1;

      XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
      XCloseDisplay(display);
    }
  }
}
#endif */

#include <JuceHeader.h>
#include "PlugDataWindow.h"
#include "../Canvas.h"
#include "../PluginProcessor.h"

extern "C" {
#include <x_libpd_multi.h>
}

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

    return NULL;
}

class PlugDataApp : public JUCEApplication {
    
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
    void anotherInstanceStarted(String const& commandLine) override
    {
        auto tokens = StringArray::fromTokens(commandLine, " ", "\"");
        auto file = File(tokens[0].unquoted());
        if (file.existsAsFile()) {
            auto* pd = dynamic_cast<PlugDataAudioProcessor*>(mainWindow->getAudioProcessor());

            if (pd && file.existsAsFile()) {
                pd->loadPatch(file);
            }
        }
    }

    PlugDataWindow* createWindow(const String& systemArgs)
    {
        return new PlugDataWindow(systemArgs, getApplicationName(), LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), appProperties.getUserSettings(), false, {}, nullptr, {});
    }

    void initialise(String const& arguments) override
    {
        LookAndFeel::getDefaultLookAndFeel().setColour(ResizableWindow::backgroundColourId, Colours::transparentBlack);

        mainWindow.reset(createWindow(arguments));

        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow)
            mainWindow->pluginHolder->savePluginState();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents()) {
            Timer::callAfterDelay(100,
                []() {
                    if (auto app = JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
        } else {
            quit();
        }
    }

    PlugDataWindow* getWindow()
    {
        return mainWindow.get();
    }

protected:
    ApplicationProperties appProperties;
    std::unique_ptr<PlugDataWindow> mainWindow;
};

void PlugDataWindow::closeButtonPressed()
{
#if JUCE_MAC
    if (Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCommandDown()) {
        return;
    }
#endif

    // Show an ask to save dialog for each patch that is dirty
    // Because save dialog uses an asynchronous callback, we can't loop over them (so have to chain them)
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pluginHolder->processor->getActiveEditor())) {
        static std::function<void(int)> checkCanvas;
        checkCanvas = [this, editor](int i) mutable {
            auto* cnv = editor->canvases[i];
            
            if(!cnv)  {
                JUCEApplication::quit();
                return;
            }
            
            bool isLast = i == editor->canvases.size() - 1;
            editor->tabbar.setCurrentTabIndex(i);

            i++;

            if (cnv->patch.isDirty()) {
                Dialogs::showSaveDialog(&editor->openedDialog, editor, cnv->patch.getTitle(),
                    [this, editor, cnv, i, isLast](int result) mutable {
                        if (result == 2) {
                            editor->saveProject(
                                [this, cnv, editor, i, isLast]() mutable {
                                    if (isLast) {
                                        JUCEApplication::quit();
                                    } else {
                                        checkCanvas(i);
                                    }
                                });
                        } else if (result == 1) {
                            if (isLast) {
                                JUCEApplication::quit();
                            } else {
                                checkCanvas(i);
                            }
                        }
                        // last option: cancel, where we end the chain
                    });
            } else if (!isLast) {
                checkCanvas(i);
            } else {
                JUCEApplication::quit();
            }
        };
        

        checkCanvas(0);
    }
}

int PlugDataWindow::parseSystemArguments(String const& arguments)
{
    auto args = StringArray::fromTokens(arguments, true);
    size_t argc = args.size();
    char const** argv = new char const*[argc];

    for (int i = 0; i < args.size(); i++) {
        argv[i] = args.getReference(i).toRawUTF8();
    }

    t_namelist* openlist = nullptr;
    t_namelist* messagelist = nullptr;
    
    int retval = parse_startup_arguments(argv, argc, openlist, messagelist);

    /* open patches specifies with "-open" args */
    for (auto* nl = openlist; nl; nl = nl->nl_next) {

        auto toOpen = File(String(nl->nl_string).unquoted());

        auto* pd = dynamic_cast<PlugDataAudioProcessor*>(getAudioProcessor());
        if (pd && toOpen.existsAsFile()) {
            pd->loadPatch(toOpen);
        }
    }

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

JUCE_CREATE_APPLICATION_DEFINE(PlugDataApp);
