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
#include "Utility/PatchInfo.h"
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

class PlugDataApp final : public JUCEApplication {

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
    }

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
        auto const tokens = StringArray::fromTokens(commandLine, " ", "\"");
        auto const file = File(tokens[0].unquoted());
        if (file.existsAsFile()) {
            if (file.hasFileExtension("pd")) {
                auto const* pd = dynamic_cast<PluginProcessor*>(pluginHolder->processor.get());
                auto const* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                if (pd && editor && file.existsAsFile()) {
                    auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                    editor->getTabComponent().openPatch(URL(file));
                    SettingsFile::getInstance()->addToRecentlyOpened(file);
                }
            } else if (file.hasFileExtension("plugdata")) {
                auto zip = ZipFile(file);
                auto patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");
                auto const result = zip.uncompressTo(patchesDir, false);
                auto* editor = dynamic_cast<PluginEditor*>(mainWindow->mainComponent->getEditor());
                if (result.wasOk()) {
                    auto const macOSTrash = ProjectInfo::appDataDir.getChildFile("Patches").getChildFile("__MACOSX");
                    if (macOSTrash.isDirectory()) {
                        macOSTrash.deleteRecursively();
                    }

                    auto extractedLocation = patchesDir.getChildFile(zip.getEntry(0)->filename);
                    auto const metaFile = extractedLocation.getChildFile("meta.json");
                    if (!metaFile.existsAsFile()) {
                        PatchInfo info;
                        info.title = file.getFileNameWithoutExtension();
                        info.setInstallTime(Time::currentTimeMillis());
                        auto json = info.json;
                        metaFile.replaceWithText(info.json);
                    } else {
                        auto info = PatchInfo(JSON::fromString(metaFile.loadFileAsString()));
                        info.setInstallTime(Time::currentTimeMillis());
                        auto json = info.json;
                        metaFile.replaceWithText(info.json);
                    }

                    Dialogs::showMultiChoiceDialog(&editor->openedDialog, editor, "Successfully installed " + file.getFileNameWithoutExtension(), [](int) { }, { "Dismiss" }, Icons::Checkmark);
                } else {
                    Dialogs::showMultiChoiceDialog(&editor->openedDialog, editor, "Failed to install " + file.getFileNameWithoutExtension(), [](int) { }, { "Dismiss" });
                }
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
            auto const width = mainWindow->getWidth();
            auto const height = mainWindow->getHeight();

            auto const& displays = Desktop::getInstance().getDisplays();

            if (auto const* props = pluginHolder->settings.get()) {
                constexpr int defaultValue = -100;

                auto const x = props->getIntValue("windowX", defaultValue);
                auto const y = props->getIntValue("windowY", defaultValue);

                if (x != defaultValue && y != defaultValue) {
                    auto const screenLimits = displays.getDisplayForRect({ x, y, width, height })->userArea;

                    return { jlimit(screenLimits.getX(), jmax(screenLimits.getX(), screenLimits.getRight() - width), x), jlimit(screenLimits.getY(), jmax(screenLimits.getY(), screenLimits.getBottom() - height), y), width, height };
                }
            }

            auto const displayArea = displays.getPrimaryDisplay()->userArea;

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

    int parseSystemArguments(String const& arguments) const
    {
        auto const settingsTree = SettingsFile::getInstance()->getValueTree();
        bool const hasReloadStateProperty = settingsTree.hasProperty("reload_last_state");

        // When starting with any sysargs, assume we don't want the last patch to open
        // Prevents a possible crash and generally kinda makes sense
        if (arguments.isEmpty() && hasReloadStateProperty && static_cast<bool>(settingsTree.getProperty("reload_last_state"))) {
            // TODO: probably remove this option, because it's kind of risky, easy to get in a crash loop this way
            // For now we'll just disable it to see if anyone misses it
            // pluginHolder->reloadPluginState();
        }

        auto args = StringArray::fromTokens(arguments, true);
        size_t const argc = args.size();

        auto argv = std::vector<char const*>(argc);

        for (int i = 0; i < args.size(); i++) {
            argv[i] = args.getReference(i).toRawUTF8();
        }

        t_namelist* openlist = nullptr;
        t_namelist* messagelist = nullptr;

        constexpr int retval = 0; // parse_startup_arguments(argv.data(), argc, &openlist, &messagelist);

        StringArray openedPatches;
        // open patches specifies with "-open" args
        for (auto const* nl = openlist; nl; nl = nl->nl_next) {
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
        for (auto const* nl = messagelist; nl; nl = nl->nl_next) {
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
                [] {
                    if (auto const app = JUCEApplicationBase::getInstance())
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
        auto const* mainEditor = dynamic_cast<PluginEditor*>(processor->getActiveEditor());
        auto& openedEditors = editor->pd->openedEditors;

        if (editor == mainEditor) {
            processor->editorBeingDeleted(editor);
        }

        if (openedEditors.size() == 1) {
            editor->getTabComponent().closeAllTabs(true, nullptr, [this, editor, &openedEditors] {
                editor->nvgSurface.detachContext();
                removeFromDesktop();
                openedEditors.removeObject(editor);
            });
        } else {
            editor->getTabComponent().closeAllTabs(false, nullptr, [this, editor, &openedEditors] {
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

void StandalonePluginHolder::setupAudioDevices(bool const enableAudioInput, String const& preferredDefaultDeviceName, AudioDeviceManager::AudioDeviceSetup const* preferredSetupOptions)
{
#if JUCE_IOS
    deviceManager.addAudioCallback(&maxSizeEnforcer);
#else
    deviceManager.addAudioCallback(this);
#endif
    reloadAudioDeviceState(enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
}

void StandalonePluginHolder::shutDownAudioDevices()
{
    saveAudioDeviceState();

#if JUCE_IOS
    deviceManager.removeAudioCallback(&maxSizeEnforcer);
#else
    deviceManager.removeAudioCallback(this);
#endif
}

START_JUCE_APPLICATION(PlugDataApp)
