
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

class ToolchainInstaller final : public Component
    , public Thread
    , public Timer {

    void timerCallback() override
    {
        repaint();
    }

public:
    explicit ToolchainInstaller(PluginEditor* pluginEditor, Dialog* parentDialog)
        : Thread("Toolchain Install Thread")
        , editor(pluginEditor)
        , dialog(parentDialog)
    {
        addAndMakeVisible(&installButton);

        installButton.onClick = [this] {
            errorMessage = "";
            repaint();

            dialog->setBlockFromClosing(true);

            String latestVersion;
            try {
                auto const compatTable = JSON::parse(URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/COMPATIBILITY").readEntireTextStream());
                if (compatTable.toString().isEmpty())
                    throw 204;
                // Get latest version
                latestVersion = compatTable.getDynamicObject()->getProperty(String(ProjectInfo::versionString).upToFirstOccurrenceOf("-", false, false)).toString();
                if (latestVersion.isEmpty()) {
                    auto const& properties = compatTable.getDynamicObject()->getProperties();
                    latestVersion = properties.getValueAt(properties.size() - 1).toString().upToFirstOccurrenceOf("-", false, false);

                    if (latestVersion.isEmpty()) {
                        throw 418;
                    }
                }
            }
            // Network error, JSON error or empty version string somehow
            catch (int error) {
                if (error == 418) {
                    errorMessage = "Error: Heavy compatibility issue, contact support";
                } else {
                    errorMessage = "Error: Could not download files (possibly no network connection)";
                    installButton.topText = "Try Again";
                }
                repaint();
                return;
            }

            catch (...) {
                errorMessage = "Error: Unknown error, contact support";
                installButton.topText = "Try Again";
                repaint();
                return;
            }

            String downloadLocation = "https://github.com/plugdata-team/plugdata-heavy-toolchain/releases/download/v" + latestVersion + "/";

#if JUCE_MAC
            downloadLocation += "Heavy-MacOS-Universal.tar.xz";
#elif JUCE_WINDOWS
            downloadLocation += "Heavy-Win64.tar.xz";
#elif JUCE_LINUX && !__aarch64__
            downloadLocation += "Heavy-Linux-x64.tar.xz";
#endif
            instream = URL(downloadLocation).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000).withStatusCode(&statusCode));
            startThread();
        };
    }

    ~ToolchainInstaller() override
    {
        stopThread(-1);
    }

    void paint(Graphics& g) override
    {
        auto const colour = PlugDataColours::panelTextColour;
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

        if (installProgress != 0.0f) {
            float const width = getWidth() - 180.0f;
            float const progress = jmap(installProgress, 0.0f, width - 3.0f);

            float constexpr downloadBarBgHeight = 11.0f;
            float constexpr downloadBarHeight = downloadBarBgHeight - 3.0f;

            auto const downloadBarBg = Rectangle<float>(90.0f, 250.0f - downloadBarBgHeight * 0.5, width, downloadBarBgHeight);
            auto const downloadBar = Rectangle<float>(91.5f, 250.0f - downloadBarHeight * 0.5, progress, downloadBarHeight);

            g.setColour(PlugDataColours::panelTextColour);
            g.fillRoundedRectangle(downloadBarBg, Corners::defaultCornerRadius);

            g.setColour(PlugDataColours::panelActiveBackgroundColour);
            g.fillRoundedRectangle(downloadBar, Corners::defaultCornerRadius);
        }

        if (errorMessage.isNotEmpty()) {
            Fonts::drawText(g, errorMessage, Rectangle<int>(30, 300, getWidth() - 60, 20), Colours::red, 15, Justification::centred);
        }

        if (isTimerRunning()) {
            getLookAndFeel().drawSpinningWaitAnimation(g, PlugDataColours::panelTextColour, getWidth() / 2 - 16, getHeight() / 2 + 118, 32, 32);
        }
    }

    void resized() override
    {
        installButton.setBounds(getLocalBounds().withSizeKeepingCentre(450, 50).translated(0, -30));
    }

    void run() override
    {
        MemoryBlock toolchainData;

        if (!instream)
            return; // error!

        int64 const totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;

        MemoryOutputStream mo(toolchainData, false);

        // pre-allocate memory to improve download speed
#if JUCE_MAC
        mo.preallocate(1024 * 1024 * 128);
#else
        mo.preallocate(1024 * 1024 * 256);
#endif

        while (true) {

            // If app or windows gets closed
            if (threadShouldExit())
                return;

            // Download blocks of 1mb at a time
            auto const written = mo.writeFromInputStream(*instream, 1024 * 1024);

            if (written == 0)
                break;

            bytesDownloaded += written;

            float const progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);

            if (threadShouldExit())
                return;

            MessageManager::callAsync([_this = SafePointer(this), progress]() mutable {
                if (!_this)
                    return;
                _this->installProgress = progress;
                _this->repaint();
            });
        }

        startTimer(25);

        auto const toolchainDir = ProjectInfo::appDataDir.getChildFile("Toolchain");

        if (toolchainDir.exists())
            toolchainDir.deleteRecursively();

#if JUCE_LINUX || JUCE_WINDOWS
        int expectedSize = 800 * 1024 * 1024;
#else
        int expectedSize = 500 * 1024 * 1024;
#endif
        auto success = Decompress::extractTarXz((const uint8_t*)toolchainData.getData(), toolchainData.getSize(), toolchainDir.getParentDirectory(), expectedSize);

        if (!success || statusCode >= 400) {
            MessageManager::callAsync([this] {
                installButton.topText = "Try Again";
                errorMessage = "Error: Could not extract downloaded package";
                repaint();
                stopTimer();
            });
            return;
        }

#if JUCE_WINDOWS
        File usbDriverInstaller = toolchainDir.getChildFile("etc").getChildFile("usb_driver").getChildFile("install-filter.exe");
        File driverSpec = toolchainDir.getChildFile("etc").getChildFile("usb_driver").getChildFile("DFU_in_FS_Mode.inf");

        // Since we interact with ComponentPeer, better call it from the message thread
        MessageManager::callAsync([this, usbDriverInstaller, driverSpec]() mutable {
            OSUtils::runAsAdmin(usbDriverInstaller.getFullPathName().toStdString(), ("install --inf=" + driverSpec.getFullPathName()).toStdString(), editor->getPeer());
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

        installProgress = 0.0f;
        stopTimer();

        MessageManager::callAsync([this] {
            dialog->setBlockFromClosing(false);
            toolchainInstalledCallback();
        });
    }

    float installProgress = 0.0f;

    bool needsUpdate = false;
    int statusCode = 0;

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
            auto const colour = PlugDataColours::panelTextColour;
            if (isMouseOver()) {
                g.setColour(PlugDataColours::panelActiveBackgroundColour);
                g.fillRoundedRectangle(Rectangle<float>(1, 1, getWidth() - 2, getHeight() - 2), Corners::largeCornerRadius);
            }

            Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
            Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
            Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Regular, 14);
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
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

    String errorMessage;

    std::unique_ptr<InputStream> instream;

    PluginEditor* editor;
    Dialog* dialog;
};

#pragma clang diagnostic pop
