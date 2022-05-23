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

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <winbase.h>
#endif
#ifdef _MSC_VER /* This is only for Microsoft's compiler, not cygwin, e.g. */
#define snprintf _snprintf
#endif

extern "C"
{
    static t_namelist* sys_openlist;
    static t_namelist* sys_messagelist;
}

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

        /* load dynamic libraries specified with "-lib" args */
        t_namelist* nl;
        for (nl = STUFF->st_externlist; nl; nl = nl->nl_next)
            if (!sys_load_lib(0, nl->nl_string)) post("%s: can't load library", nl->nl_string);

        /* open patches specifies with "-open" args */
        for (nl = sys_openlist; nl; nl = nl->nl_next)
        {
            auto toOpen = File(String(nl->nl_string));

            auto* pd = dynamic_cast<PlugDataAudioProcessor*>(mainWindow->getAudioProcessor());
            if (pd && toOpen.existsAsFile())
            {
                pd->loadPatch(toOpen);
            }
        }

        namelist_free(sys_openlist);
        sys_openlist = 0;
        /* send messages specified with "-send" args */
        for (nl = sys_messagelist; nl; nl = nl->nl_next)
        {
            t_binbuf* b = binbuf_new();
            binbuf_text(b, nl->nl_string, strlen(nl->nl_string));
            binbuf_eval(b, 0, 0, 0);
            binbuf_free(b);
        }

        namelist_free(sys_messagelist);
        sys_messagelist = 0;
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
    if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pluginHolder->processor->getActiveEditor()))
    {
        checkCanvas = [this, editor](int i) mutable
        {
            auto* cnv = editor->canvases[i];
            bool isLast = i == editor->canvases.size() - 1;
            editor->tabbar.setCurrentTabIndex(i);

            i++;

            if (cnv->patch.isDirty())
            {
                Dialogs::showSaveDialog(editor, cnv->patch.getTitle(),
                                        [this, editor, cnv, i, isLast](int result) mutable
                                        {
                                            if (result == 2)
                                            {
                                                editor->saveProject(
                                                    [this, cnv, editor, i, isLast]() mutable
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

    t_audiosettings as;
    /* get the current audio parameters.  These are set
    by the preferences mechanism (sys_loadpreferences()) or
    else are the default.  Overwrite them with any results
    of argument parsing, and store them again. */
    sys_get_audio_settings(&as);
    while ((argc > 0) && **argv == '-')
    {
        /* audio flags */
        if (!strcmp(*argv, "-r") && argc > 1 && sscanf(argv[1], "%d", &as.a_srate) >= 1)
        {
            argc -= 2;
            argv += 2;
        }
        /*
        else if (!strcmp(*argv, "-inchannels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchindev,
                    as.a_chindevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-outchannels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchoutdev,
                    as.a_choutdevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-channels"))
        {
            if (argc < 2 ||
                !sys_parsedevlist(&as.a_nchindev,
                    as.a_chindevvec, MAXAUDIOINDEV, argv[1]) ||
                !sys_parsedevlist(&as.a_nchoutdev,
                    as.a_choutdevvec, MAXAUDIOINDEV, argv[1]))
                        goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-soundbuf") || (!strcmp(*argv, "-audiobuf")))
        {
            if (argc < 2)
                goto usage;

            as.a_advance = atoi(argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-callback"))
        {
            as.a_callback = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nocallback"))
        {
            as.a_callback = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-blocksize"))
        {
            as.a_blocksize = atoi(argv[1]);
            argc -= 2; argv += 2;
        }*/
        else if (!strcmp(*argv, "-sleepgrain"))
        {
            if (argc < 2) goto usage;
            sys_sleepgrain = 1000 * atof(argv[1]);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-nodac"))
        {
            as.a_noutdev = as.a_nchoutdev = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-noadc"))
        {
            as.a_nindev = as.a_nchindev = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nosound") || !strcmp(*argv, "-noaudio"))
        {
            as.a_noutdev = as.a_nchoutdev = as.a_nindev = as.a_nchindev = 0;
            argc--;
            argv++;
        }
        /* MIDI flags */
        else if (!strcmp(*argv, "-nomidiin"))
        {
            sys_nmidiin = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nomidiout"))
        {
            sys_nmidiout = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nomidi"))
        {
            sys_nmidiin = sys_nmidiout = 0;
            argc--;
            argv++;
        }
        /* other flags */
        else if (!strcmp(*argv, "-path"))
        {
            if (argc < 2) goto usage;
            STUFF->st_temppath = namelist_append_files(STUFF->st_temppath, argv[1]);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-nostdpath"))
        {
            sys_usestdpath = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-stdpath"))
        {
            sys_usestdpath = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-helppath"))
        {
            if (argc < 2) goto usage;
            STUFF->st_helppath = namelist_append_files(STUFF->st_helppath, argv[1]);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-open"))
        {
            if (argc < 2) goto usage;

            sys_openlist = namelist_append_files(sys_openlist, argv[1]);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-lib"))
        {
            if (argc < 2) goto usage;

            STUFF->st_externlist = namelist_append_files(STUFF->st_externlist, argv[1]);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-verbose"))
        {
            sys_verbose++;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-noverbose"))
        {
            sys_verbose = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-version"))
        {
            // sys_version = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-d") && argc > 1 && sscanf(argv[1], "%d", &sys_debuglevel) >= 1)
        {
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-loadbang"))
        {
            sys_noloadbang = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-noloadbang"))
        {
            sys_noloadbang = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nostderr"))
        {
            sys_printtostderr = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-stderr"))
        {
            sys_printtostderr = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-send"))
        {
            if (argc < 2) goto usage;

            sys_messagelist = namelist_append(sys_messagelist, argv[1], 1);
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-batch"))
        {
            // sys_batch = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nobatch"))
        {
            // sys_batch = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-autopatch"))
        {
            // sys_noautopatch = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-noautopatch"))
        {
            // sys_noautopatch = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-compatibility"))
        {
            float f;
            if (argc < 2) goto usage;

            if (sscanf(argv[1], "%f", &f) < 1) goto usage;
            pd_compatibilitylevel = 0.5 + 100. * f; /* e.g., 2.44 --> 244 */
            argv += 2;
            argc -= 2;
        }
        else if (!strcmp(*argv, "-sleep"))
        {
            // sys_nosleep = 0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-nosleep"))
        {
            // sys_nosleep = 1;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv, "-noprefs")) /* did this earlier */
            argc--, argv++;
        else if (!strcmp(*argv, "-prefsfile") && argc > 1) /* this too */
            argc -= 2, argv += 2;
        else
        {
        usage:
            // sys_printusage();
            return (1);
        }
    }
    /*
    if (sys_batch)
        sys_dontstartgui = 1;
    if (sys_dontstartgui)
        sys_printtostderr = 1; */
#ifdef _WIN32
    if (sys_printtostderr) /* we need to tell Windows to output UTF-8 */
        SetConsoleOutputCP(CP_UTF8);
#endif
    // if (!sys_defaultfont)
    //     sys_defaultfont = DEFAULTFONT;
    for (; argc > 0; argc--, argv++) sys_openlist = namelist_append_files(sys_openlist, *argv);

    sys_set_audio_settings(&as);
    return (0);
}

JUCE_CREATE_APPLICATION_DEFINE(PlugDataApp);
