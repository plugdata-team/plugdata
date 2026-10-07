/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#pragma clang diagnostic push

#include <juce_gui_basics/juce_gui_basics.h>
#include <fstream>
#include "Utility/Decompress.h"
#include "Constants.h"

// Downloads and unpacks the toolchain in the background, and finds out whether it needs an update.
// There's only one, so the export dialog and the quick export toolbar follow the same install instead
// of unpacking over each other, and it carries on when the dialog closes
class ToolchainInstall final : public DeletedAtShutdown {
public:
    struct Listener {
        virtual ~Listener() = default;

        // All on the message thread: any change to the state below, and how an install ended
        virtual void toolchainInstallChanged() { }
        virtual void toolchainInstalled() { }
        virtual void toolchainInstallFailed(String const& error) { ignoreUnused(error); }
    };

    ~ToolchainInstall() override
    {
        shuttingDown = true;
        pool.removeAllJobs(true, -1);
        clearSingletonInstance();
    }

    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

    // Asks GitHub whether there's a newer toolchain for this version of plugdata, once per session
    void checkForUpdate()
    {
        if (updateCheckStarted)
            return;

        updateCheckStarted = true;

        pool.addJob([] {
            String fetchError;
            auto const outdated = isOutdated(fetchCompatibleVersion(fetchError));

            onMessageThread([outdated](ToolchainInstall& install) {
                install.updateAvailable = outdated;
            });
        });
    }

    // Starts downloading, unless that's already happening. On Windows, the window is what the USB driver
    // installer asks for admin rights over
    void install(Component* window)
    {
        if (installing)
            return;

        installing = true;
        unpacking = false;
        progress = 0.0f;
        error.clear();
        driverInstallWindow = window;

        listeners.call(&Listener::toolchainInstallChanged);

        pool.addJob([this] { runInstall(); });
    }

    // Message thread only
    bool installing = false;
    bool unpacking = false;
    float progress = 0.0f;
    String error;
    bool updateAvailable = false;

    JUCE_DECLARE_SINGLETON_INLINE(ToolchainInstall, false)

private:
    // Which toolchain goes with this version of plugdata, or an empty string with the reason in error
    static String fetchCompatibleVersion(String& error)
    {
        auto const compatibilityUrl = URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/COMPATIBILITY");

        int statusCode = 0;
        auto const stream = compatibilityUrl.createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withStatusCode(&statusCode));
        if (!stream) {
            error = "Error: Could not reach GitHub to look up the toolchain version (possibly no network connection)";
            return { };
        }
        if (statusCode >= 400) {
            error = "Error: Could not look up the toolchain version (HTTP " + String(statusCode) + ")";
            return { };
        }

        var compatTable;
        try {
            compatTable = JSON::parse(stream->readEntireStreamAsString());
        } catch (...) {
        }

        auto const* versions = compatTable.getDynamicObject();
        if (!versions) {
            error = "Error: Could not read the toolchain version list from GitHub";
            return { };
        }

        auto const& properties = versions->getProperties();
        auto version = properties[String(ProjectInfo::versionString).upToFirstOccurrenceOf("-", false, false)].toString();

        // Versions the table doesn't know about get the newest toolchain
        if (version.isEmpty() && !properties.isEmpty())
            version = properties.getValueAt(properties.size() - 1).toString().upToFirstOccurrenceOf("-", false, false);

        if (version.isEmpty())
            error = "Error: Heavy compatibility issue, contact support";

        return version;
    }

    // Versions compare as integers with the dots taken out
    static bool isOutdated(String const& compatibleVersion)
    {
        // Don't do this relative to the toolchain dir in ExporterBase, that won't work on Windows
        auto const versionFile = ProjectInfo::appDataDir.getChildFile("Toolchain").getChildFile("VERSION");
        auto const installedVersion = versionFile.loadFileAsString().trim().removeCharacters(".").getIntValue();

        return compatibleVersion.removeCharacters(".").getIntValue() > installedVersion;
    }

    void runInstall()
    {
        String fetchError;
        auto const version = fetchCompatibleVersion(fetchError);
        if (version.isEmpty()) {
            finishInstall(fetchError);
            return;
        }

        String downloadLocation = "https://github.com/plugdata-team/plugdata-heavy-toolchain/releases/download/v" + version + "/";

#if JUCE_MAC
        downloadLocation += "Heavy-MacOS-Universal.tar.xz";
#elif JUCE_WINDOWS
        downloadLocation += "Heavy-Win64.tar.xz";
#elif JUCE_LINUX && !__aarch64__
        downloadLocation += "Heavy-Linux-x64.tar.xz";
#endif

        int statusCode = 0;
        auto const instream = URL(downloadLocation).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000).withStatusCode(&statusCode));

        if (!instream) {
            finishInstall("Error: Could not connect to download the toolchain (possibly no network connection)");
            return;
        }
        if (statusCode >= 400) {
            finishInstall("Error: Toolchain download failed (HTTP " + String(statusCode) + ")");
            return;
        }

        int64 const totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;

        MemoryBlock toolchainData;
        MemoryOutputStream mo(toolchainData, false);

        // pre-allocate memory to improve download speed
#if JUCE_MAC
        mo.preallocate(1024 * 1024 * 128);
#else
        mo.preallocate(1024 * 1024 * 256);
#endif

        while (true) {
            // If the app gets closed
            if (shuttingDown)
                return;

            // Download blocks of 1mb at a time
            auto const written = mo.writeFromInputStream(*instream, 1024 * 1024);

            if (written == 0)
                break;

            bytesDownloaded += written;

            float const downloaded = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);

            onMessageThread([downloaded](ToolchainInstall& install) {
                install.progress = downloaded;
            });
        }

        // A dropped connection ends the stream early. Bail out before the installed toolchain gets deleted
        if (totalBytes > 0 && bytesDownloaded < totalBytes) {
            finishInstall("Error: Toolchain download was interrupted (" + File::descriptionOfSizeInBytes(bytesDownloaded) + " of " + File::descriptionOfSizeInBytes(totalBytes) + ")");
            return;
        }

        onMessageThread([](ToolchainInstall& install) {
            install.unpacking = true;
        });

        auto const toolchainDir = ProjectInfo::appDataDir.getChildFile("Toolchain");

        if (toolchainDir.exists())
            toolchainDir.deleteRecursively();

#if JUCE_LINUX || JUCE_WINDOWS
        int expectedSize = 800 * 1024 * 1024;
#else
        int expectedSize = 500 * 1024 * 1024;
#endif
        if (!Decompress::extractTarXz(static_cast<uint8_t const*>(mo.getData()), mo.getDataSize(), toolchainDir.getParentDirectory(), expectedSize)) {
            finishInstall("Error: Could not extract downloaded package");
            return;
        }

#if JUCE_WINDOWS
        File usbDriverInstaller = toolchainDir.getChildFile("usr").getChildFile("etc").getChildFile("usb_driver").getChildFile("install-filter.exe");
        File driverSpec = toolchainDir.getChildFile("usr").getChildFile("etc").getChildFile("usb_driver").getChildFile("DFU_in_FS_Mode.inf");

        // Since we interact with ComponentPeer, better call it from the message thread
        MessageManager::callAsync([usbDriverInstaller, driverSpec] {
            auto const* install = getInstanceWithoutCreating();
            auto const* window = install ? install->driverInstallWindow.getComponent() : nullptr;

            // The window that started the install may be gone by now, any other one will do
            auto* peer = window ? window->getPeer() : nullptr;
            if (!peer && ComponentPeer::getNumPeers() > 0)
                peer = ComponentPeer::getPeer(0);

            if (peer)
                OSUtils::runAsAdmin(usbDriverInstaller.getFullPathName().toStdString(), ("install --inf=" + driverSpec.getFullPathName()).toStdString(), peer);
        });
#endif

#if JUCE_LINUX
        // Add udev rule for the daisy seed
        // This makes sure we can use dfu-util without admin privileges
        // Kinda sucks that we need to sudo this, but there's no other way AFAIK

        auto askpassScript = toolchainDir.getChildFile("scripts").getChildFile("askpass.sh");
        auto udevInstallScript = toolchainDir.getChildFile("scripts").getChildFile("install_udev_rule.sh");

        askpassScript.setExecutePermission(true);
        udevInstallScript.setExecutePermission(true);

        if (!File("/etc/udev/rules.d/50-daisy-stmicro-dfu.rules").exists()) {
            std::system(udevInstallScript.getFullPathName().toRawUTF8());
        }

#elif JUCE_MAC
        ChildProcess process;
        process.start("xcode-select --install");
        process.waitForProcessToFinish(-1);
#endif

        finishInstall({ });
    }

    void finishInstall(String const& installError)
    {
        onMessageThread([installError](ToolchainInstall& install) {
            install.installing = false;
            install.unpacking = false;
            install.progress = 0.0f;
            install.error = installError;

            if (installError.isEmpty()) {
                install.updateAvailable = false;
                install.listeners.call(&Listener::toolchainInstalled);
            } else {
                install.listeners.call(&Listener::toolchainInstallFailed, installError);
            }
        });
    }

    // Hands a change from the install thread to the message thread, where the listeners hear about it
    template<typename Callback>
    static void onMessageThread(Callback&& callback)
    {
        MessageManager::callAsync([callback = std::forward<Callback>(callback)] {
            if (auto* install = getInstanceWithoutCreating()) {
                callback(*install);
                install->listeners.call(&Listener::toolchainInstallChanged);
            }
        });
    }

    ListenerList<Listener> listeners;
    Component::SafePointer<Component> driverInstallWindow;
    bool updateCheckStarted = false;

    AtomicValue<bool> shuttingDown = false;
    ThreadPool pool = ThreadPool(1);
};

class ToolchainInstaller final : public Component
    , public ToolchainInstall::Listener
    , public Timer {

    void timerCallback() override
    {
        repaint(Rectangle<int>(getWidth() / 2 - 16, getHeight() / 2 + 118, 32, 32));
    }

public:
    explicit ToolchainInstaller(PluginEditor* pluginEditor)
        : editor(pluginEditor)
    {
        addAndMakeVisible(&installButton);

        installButton.onClick = [this] {
            ToolchainInstall::getInstance()->install(editor);
        };

        ToolchainInstall::getInstance()->addListener(this);
        toolchainInstallChanged();
    }

    ~ToolchainInstaller() override
    {
        if (auto* install = ToolchainInstall::getInstanceWithoutCreating())
            install->removeListener(this);
    }

    void toolchainInstallChanged() override
    {
        auto const& install = *ToolchainInstall::getInstance();

        installButton.setEnabled(!install.installing);
        if (install.error.isNotEmpty())
            installButton.topText = "Try Again";

        if (install.unpacking)
            startTimer(25);
        else
            stopTimer();

        repaint();

        NullCheckedInvocation::invoke(onInstallChanged);
    }

    void toolchainInstalled() override
    {
        NullCheckedInvocation::invoke(toolchainInstalledCallback);
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);
        auto const& install = *ToolchainInstall::getInstance();

        auto const colour = colours.panelTextColour;
        if (needsUpdate) {
            Fonts::drawStyledText(g, "Toolchain needs to be updated", 0, getHeight() / 2 - 150, getWidth(), 40, colour, Bold, 32, Justification::horizontallyCentred);
        } else {
            Fonts::drawStyledText(g, "Toolchain not found", 0, getHeight() / 2 - 150, getWidth(), 40, colour, Bold, 32, Justification::horizontallyCentred);
        }

        if (needsUpdate) {
            Fonts::drawStyledText(g, "Update the toolchain to get started", 0, getHeight() / 2 - 120, getWidth(), 40, colour, Regular, 20, Justification::horizontallyCentred);
        } else {
            Fonts::drawStyledText(g, "Install the toolchain to get started", 0, getHeight() / 2 - 120, getWidth(), 40, colour, Regular, 20, Justification::horizontallyCentred);
        }

        if (install.progress != 0.0f) {
            float const width = getWidth() - 180.0f;
            float const progress = jmap(install.progress, 0.0f, width - 3.0f);

            float constexpr downloadBarBgHeight = 11.0f;
            float constexpr downloadBarHeight = downloadBarBgHeight - 3.0f;

            auto const downloadBarBg = Rectangle<float>(90.0f, 250.0f - downloadBarBgHeight * 0.5, width, downloadBarBgHeight);
            auto const downloadBar = Rectangle<float>(91.5f, 250.0f - downloadBarHeight * 0.5, progress, downloadBarHeight);

            g.setColour(colours.panelTextColour);
            g.fillRoundedRectangle(downloadBarBg, Corners::defaultCornerRadius);

            g.setColour(colours.panelActiveBackgroundColour);
            g.fillRoundedRectangle(downloadBar, Corners::defaultCornerRadius);
        }

        if (install.error.isNotEmpty()) {
            Fonts::drawText(g, install.error, Rectangle<int>(30, 300, getWidth() - 60, 20), Colours::red, 15, Justification::centred);
        }

        if (isTimerRunning()) {
            getLookAndFeel().drawSpinningWaitAnimation(g, colours.panelTextColour, getWidth() / 2 - 16, getHeight() / 2 + 118, 32, 32);
        }
    }

    void resized() override
    {
        installButton.setBounds(getLocalBounds().withSizeKeepingCentre(450, 50).translated(0, -30));
    }

    bool needsUpdate = false;

#if JUCE_WINDOWS
    String downloadSize = "1.2 GB";
#elif JUCE_MAC
    String downloadSize = "426 MB";
#else
    String downloadSize = "764 MB";
#endif

    class ToolchainInstallerButton final : public Component {

    public:
        String iconText;
        String topText;
        String bottomText;

        std::function<void()> onClick = [] { };

        ToolchainInstallerButton(String icon, String mainText, String subText)
            : iconText(std::move(icon))
            , topText(std::move(mainText))
            , bottomText(std::move(subText))
        {
            setInterceptsMouseClicks(true, false);
            setAlwaysOnTop(true);
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            auto const colour = colours.panelTextColour.withAlpha(isEnabled() ? 1.0f : 0.5f);
            if (isMouseOver() && isEnabled()) {
                g.setColour(colours.panelActiveBackgroundColour);
                g.fillRoundedRectangle(Rectangle<float>(1, 1, getWidth() - 2, getHeight() - 2), Corners::largeCornerRadius);
            }

            Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
            Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
            Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Regular, 14);
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown() || !isEnabled())
                return;

            onClick();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            repaint();
        }
    };

    ToolchainInstallerButton installButton = ToolchainInstallerButton(Icons::SaveAs, "Download Toolchain", "Download compilation utilities (" + downloadSize + ")");

    std::function<void()> toolchainInstalledCallback;
    std::function<void()> onInstallChanged;

    PluginEditor* editor;
};

#pragma clang diagnostic pop
