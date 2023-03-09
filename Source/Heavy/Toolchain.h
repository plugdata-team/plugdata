/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct Toolchain {
#if JUCE_WINDOWS
    inline static File dir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain").getChildFile("usr");
#else
    inline static File dir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain");
#endif

    static void deleteTempFileLater(File script)
    {
        tempFilesToDelete.add(script);
    }

    static void deleteTempFiles()
    {
        for (auto& file : tempFilesToDelete) {
            if (file.existsAsFile())
                file.deleteFile();
            if (file.isDirectory())
                file.deleteRecursively();
        }
    }

    static void startShellScript(String scriptText, ChildProcess* processToUse = nullptr)
    {

        File scriptFile = File::createTempFile(".sh");
        Toolchain::deleteTempFileLater(scriptFile);

        auto bash = String("#!/bin/bash\n");
        scriptFile.replaceWithText(bash + scriptText, false, false, "\n");

#if JUCE_WINDOWS
        auto sh = Toolchain::dir.getChildFile("bin").getChildFile("sh.exe");

        if (processToUse) {
            processToUse->start(StringArray { sh.getFullPathName(), "--login", scriptFile.getFullPathName().replaceCharacter('\\', '/') });
        } else {
            ChildProcess process;
            process.start(StringArray { sh.getFullPathName(), "--login", scriptFile.getFullPathName().replaceCharacter('\\', '/') });
            process.waitForProcessToFinish(-1);
        }
#else
        scriptFile.setExecutePermission(true);

        if (processToUse) {
            processToUse->start(scriptFile.getFullPathName());
        } else {
            ChildProcess process;
            process.start(scriptFile.getFullPathName());
            process.waitForProcessToFinish(-1);
        }
#endif
    }

private:
    inline static Array<File> tempFilesToDelete;
};

struct ToolchainInstaller : public Component
    , public Thread
    , public Timer {
    struct InstallButton : public Component {

#if JUCE_WINDOWS
        String downloadSize = "720 MB";
#elif JUCE_MAC
        String downloadSize = "650 MB";
#else
        String downloadSize = "1.45 GB";
#endif
        String iconText = Icons::SaveAs;
        String topText = "Download Toolchain";
        String bottomText = "Download compilation utilities (" + downloadSize + ")";

        std::function<void(void)> onClick = []() {};

        InstallButton()
        {
            setInterceptsMouseClicks(true, false);
        }

        void paint(Graphics& g)
        {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));

            if (isMouseOver()) {
                g.fillRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, Corners::smallCornerRadius);
            }

            auto colour = findColour(PlugDataColour::panelTextColourId);

            Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24);

            Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);

            Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Thin, 14);
        }

        void mouseUp(MouseEvent const& e)
        {
            onClick();
        }

        void mouseEnter(MouseEvent const& e)
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e)
        {
            repaint();
        }
    };

    void timerCallback() override
    {
        repaint();
    }

    ToolchainInstaller(PluginEditor* pluginEditor)
        : Thread("Toolchain Install Thread")
        , editor(pluginEditor)
    {
        addAndMakeVisible(&installButton);

        installButton.onClick = [this]() {
            errorMessage = "";
            repaint();

            // Get latest version
            auto latestVersion = "v" + URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/VERSION").readEntireTextStream().trim();

            if (latestVersion == "v") {
                errorMessage = "Error: Could not download files (possibly no network connection)";
                installButton.topText = "Try Again";
                repaint();
            }

            String downloadLocation = "https://github.com/plugdata-team/plugdata-heavy-toolchain/releases/download/" + latestVersion + "/";

#if JUCE_MAC
            downloadLocation += "Heavy-MacOS-Universal.zip";
#elif JUCE_WINDOWS
            downloadLocation += "Heavy-Win64.zip";
#elif JUCE_LINUX && !__aarch64__
            downloadLocation += "Heavy-Linux-x64.zip";
#endif

            instream = URL(downloadLocation).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000).withStatusCode(&statusCode));
            startThread();
        };
    }

    ~ToolchainInstaller()
    {
        stopThread(-1);
    }

    void paint(Graphics& g) override
    {
        auto colour = findColour(PlugDataColour::panelTextColourId);
        if (needsUpdate) {
            Fonts::drawStyledText(g, "Toolchain needs to be updated", 0, getHeight() / 2 - 150, getWidth(), 40, colour, Bold, 32, Justification::centred);
        } else {
            Fonts::drawStyledText(g, "Toolchain not found", 0, getHeight() / 2 - 150, getWidth(), 40, colour, Bold, 32, Justification::centred);
        }

        if (needsUpdate) {
            Fonts::drawStyledText(g, "Update the toolchain to get started", 0, getHeight() / 2 - 120, getWidth(), 40, colour, Thin, 23, Justification::centred);
        } else {
            Fonts::drawStyledText(g, "Install the toolchain to get started", 0, getHeight() / 2 - 120, getWidth(), 40, colour, Thin, 23, Justification::centred);
        }

        if (installProgress != 0.0f) {
            float width = getWidth() - 90.0f;
            float right = jmap(installProgress, 90.f, width);

            Path downloadPath;
            downloadPath.addLineSegment({ 90, 300, right, 300 }, 1.0f);

            Path fullPath;
            fullPath.addLineSegment({ 90, 300, width, 300 }, 1.0f);

            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.strokePath(fullPath, PathStrokeType(11.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        }

        if (errorMessage.isNotEmpty()) {
            Fonts::drawText(g, errorMessage, Rectangle<int>(90, 300, getWidth(), 20), Colours::red, 15);
        }

        if (isTimerRunning()) {
            getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        }
    }

    void resized() override
    {
        installButton.setBounds(getLocalBounds().withSizeKeepingCentre(450, 50).translated(15, -30));
    }

    void run() override
    {
        MemoryBlock toolchainData;

        if (!instream)
            return; // error!

        int64 totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;

        MemoryOutputStream mo(toolchainData, true);

        while (true) {

            // If app or windows gets closed
            if (threadShouldExit())
                return;

            auto written = mo.writeFromInputStream(*instream, 8192);

            if (written == 0)
                break;

            bytesDownloaded += written;

            float progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);

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

        MemoryInputStream input(toolchainData, false);
        ZipFile zip(input);

        auto toolchainDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain");

        if (toolchainDir.exists())
            toolchainDir.deleteRecursively();

        auto result = zip.uncompressTo(toolchainDir);

        if (!result.wasOk() || (statusCode >= 400)) {
            MessageManager::callAsync([this]() {
                installButton.topText = "Try Again";
                errorMessage = "Error: Could not extract downloaded package";
                repaint();
                stopTimer();
            });
            return;
        }

        // Make sure downloaded files have executable permission on unix
#if JUCE_MAC || JUCE_LINUX || JUCE_BSD

        auto tcPath = Toolchain::dir.getFullPathName();
        auto permissionsScript = String("#!/bin/bash")
            + "\nchmod +x " + tcPath + "/bin/Heavy/Heavy"
            + "\nchmod +x " + tcPath + "/bin/*"
            + "\nchmod +x " + tcPath + "/lib/dpf/utils/generate-ttl.sh"
            + "\nchmod +x " + tcPath + "/arm-none-eabi/bin/*"
            + "\nchmod +x " + tcPath + "/libexec/gcc/arm-none-eabi/*/*"
#    if JUCE_LINUX
            + "\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/bin/*"
            + "\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/sysroot/sbin/*"
            + "\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/sysroot/usr/bin/*"
#    endif
            ;

        Toolchain::startShellScript(permissionsScript);

#elif JUCE_WINDOWS
        File usbDriverInstaller = Toolchain::dir.getChildFile("etc").getChildFile("usb_driver").getChildFile("install-filter.exe");
        File driverSpec = Toolchain::dir.getChildFile("etc").getChildFile("usb_driver").getChildFile("DFU_in_FS_Mode.inf");

        // Since we interact with ComponentPeer, better call it from the message thread
        MessageManager::callAsync([this, usbDriverInstaller, driverSpec]() mutable {
            OSUtils::runAsAdmin(usbDriverInstaller.getFullPathName().toStdString(), ("install --inf=" + driverSpec.getFullPathName()).toStdString(), editor->getPeer()->getNativeHandle());
        });
#endif

#if JUCE_LINUX
        // Add udev rule for the daisy seed
        // This makes sure we can use dfu-util without admin privileges
        // Kinda sucks that we need to sudo this, but there's no other way AFAIK

        auto askpassScript = Toolchain::dir.getChildFile("scripts").getChildFile("askpass.sh");
        auto udevInstallScript = Toolchain::dir.getChildFile("scripts").getChildFile("install_udev_rule.sh");

        askpassScript.setExecutePermission(true);
        udevInstallScript.setExecutePermission(true);

        if (!File("/etc/udev/rules.d/50-daisy-stmicro-dfu.rules").exists()) {
            std::system(udevInstallScript.getFullPathName().toRawUTF8());
        }

#elif JUCE_MAC
        Toolchain::startShellScript("xcode-select --install");
#endif

        installProgress = 0.0f;
        stopTimer();

        MessageManager::callAsync([this]() {
            toolchainInstalledCallback();
        });
    }

    float installProgress = 0.0f;

    bool needsUpdate = false;
    int statusCode;

    InstallButton installButton;
    std::function<void()> toolchainInstalledCallback;

    String errorMessage;

    std::unique_ptr<InputStream> instream;

    PluginEditor* editor;
};
