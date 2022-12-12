/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"
#include "../Utility/WindowsUtils.h"

#if JUCE_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <z_libpd.h>

struct Toolchain
{
#if JUCE_WINDOWS
    inline static File dir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain").getChildFile("usr");
#else
    inline static File dir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain");
#endif
    
    static void deleteTempFileLater(File script) {
        tempFilesToDelete.add(script);
    }
    
    static void deleteTempFiles() {
        for(auto& file : tempFilesToDelete) {
            if(file.existsAsFile()) file.deleteFile();
            if(file.isDirectory()) file.deleteRecursively();
        }
    }
    
private:
    
    inline static Array<File> tempFilesToDelete;
};

static void startShellScript(String scriptText, ChildProcess* processToUse = nullptr) {
    
    File scriptFile = File::createTempFile(".sh");
    Toolchain::deleteTempFileLater(scriptFile);
    
    auto bash = String("#!/bin/bash\n");
    scriptFile.replaceWithText(bash + scriptText, false, false, "\n");
    
#if JUCE_WINDOWS
    auto sh = Toolchain::dir.getChildFile("bin").getChildFile("sh.exe");
    
    if(processToUse) {
        processToUse->start(StringArray{sh.getFullPathName(), "--login", scriptFile.getFullPathName().replaceCharacter('\\', '/')});
    }
    else {
        ChildProcess process;
        process.start(StringArray{sh.getFullPathName(), "--login", scriptFile.getFullPathName().replaceCharacter('\\', '/')});
        process.waitForProcessToFinish(-1);
    }
#else
    scriptFile.setExecutePermission(true);
    
    if(processToUse) {
        processToUse->start(scriptFile.getFullPathName());
    }
    else {
        ChildProcess process;
        process.start(scriptFile.getFullPathName());
        process.waitForProcessToFinish(-1);
    }
#endif
}

struct ToolchainInstaller : public Component, public Thread, public Timer
{
    struct InstallButton : public Component
    {
        
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
        
        std::function<void(void)> onClick = [](){};
        
        InstallButton()
        {
            setInterceptsMouseClicks(true, false);
        }
        
        void paint(Graphics& g)
        {
            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
            
            auto textColour = findColour(PlugDataColour::canvasTextColourId);
            
            g.setColour(textColour);
            
            g.setFont(lnf->iconFont.withHeight(24));
            g.drawText(iconText, 20, 5, 40, 40, Justification::centredLeft);
            
            g.setFont(lnf->defaultFont.withHeight(16));
            g.drawText(topText, 60, 7, getWidth() - 60, 20, Justification::centredLeft);
            
            g.setFont(lnf->thinFont.withHeight(14));
            g.drawText(bottomText, 60, 25, getWidth() - 60, 16, Justification::centredLeft);
            
            if(isMouseOver()) {
                g.drawRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, Constants::smallCornerRadius, 0.5f);
            }
        }
        
        void mouseUp(const MouseEvent& e)
        {
            onClick();
        }
        
        void mouseEnter(const MouseEvent& e)
        {
            repaint();
        }
        
        void mouseExit(const MouseEvent& e)
        {
            repaint();
        }
    };
    
    void timerCallback() override
    {
        repaint();
    }
    
    ToolchainInstaller(PlugDataPluginEditor* pluginEditor) : Thread("Toolchain Install Thread"), editor(pluginEditor) {
        addAndMakeVisible(&installButton);
        
        installButton.onClick = [this](){
            errorMessage = "";
            repaint();
            
            // Get latest version
            auto latestVersion = "v" + URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/VERSION").readEntireTextStream().trim();
            
            if(latestVersion == "v") {
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
            
            instream = URL(downloadLocation).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                                                               .withConnectionTimeoutMs(5000)
                                                               .withStatusCode(&statusCode));
            startThread();
        };
    }
    
    void paint(Graphics& g) override {
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(32));
        if(needsUpdate) {
            g.drawText("Toolchain needs to be updated", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        }
        else {
            g.drawText("Toolchain not found", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        }
        
        
        g.setFont(lnf->thinFont.withHeight(23));
        if(needsUpdate) {
            g.drawText("Update the toolchain to get started", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
        }
        else {
            g.drawText("Install the toolchain to get started", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
        }
        
        if(installProgress != 0.0f)
        {
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
        
        if(errorMessage.isNotEmpty()) {
            g.setFont(Font(15));
            g.setColour(Colours::red);
            g.drawText(errorMessage, Rectangle<float>(90.0f, 300.0f, getWidth() - 90.0f, 20), Justification::centred);
        }
        
        if(isTimerRunning()) {
            lnf->drawSpinningWaitAnimation(g, findColour(PlugDataColour::canvasTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        }
    }
    
    void resized() override
    {
        installButton.setBounds(getLocalBounds().withSizeKeepingCentre(450, 50).translated(15, -30));
    }
    
    void run() override
    {
        
#if JUCE_LINUX
        // Add udev rule for the daisy seed
        // This makes sure we can use dfu-util without admin privileges
        // Kinda sucks that we need to sudo this, but there's no other way AFAIK
        startShellScript("echo \'SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"0483\", ATTRS{idProduct}==\"df11\", MODE=\"0666\", GROUP=\"plugdev\"\' | pkexec tee /etc/udev/rules.d/50-daisy-stmicro-dfu.rules >/dev/null")
#elif JUCE_MAC
        startShellScript("xcode-select --install");
#endif
        
        MemoryBlock toolchainData;
        
        if(!instream) return; // error!
        
        int64 totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;
        
        MemoryOutputStream mo(toolchainData, true);
        
        while (true) {
            
            // If app or windows gets closed
            if (threadShouldExit()) return;
            
            auto written = mo.writeFromInputStream(*instream, 8192);
            
            if (written == 0) break;
            
            bytesDownloaded += written;
            
            float progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);
            
            MessageManager::callAsync([this, progress]() mutable {
                installProgress = progress;
                repaint();
            });
        }
        
        startTimer(25);
        
        MemoryInputStream input(toolchainData, false);
        ZipFile zip(input);
        
        if(Toolchain::dir.exists()) Toolchain::dir.deleteRecursively();
        
        auto result = zip.uncompressTo(Toolchain::dir);
        
        if (!result.wasOk() || (statusCode >= 400)) {
            MessageManager::callAsync([this](){
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
        auto permissionsScript =  String("#!/bin/bash")
        +"\nchmod +x " + tcPath + "/bin/Heavy/Heavy"
        +"\nchmod +x " + tcPath + "/bin/*"
        +"\nchmod +x " + tcPath + "/lib/dpf/utils/generate-ttl.sh"
        +"\nchmod +x " + tcPath + "/arm-none-eabi/bin/*"
        +"\nchmod +x " + tcPath + "/libexec/gcc/arm-none-eabi/*/*"
#if JUCE_LINUX
        +"\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/bin/*"
        +"\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/sysroot/sbin/*"
        +"\nchmod +x " + tcPath + "/x86_64-anywhere-linux-gnu/sysroot/usr/bin/*"
#endif
        ;
        
        startShellScript(permissionsScript);
        
#elif JUCE_WINDOWS
        File usbDriverInstaller = Toolchain::dir.getChildFile("etc").getChildFile("usb_driver").getChildFile("install-filter.exe");
        File driverSpec = Toolchain::dir.getChildFile("etc").getChildFile("usb_driver").getChildFile("DFU_in_FS_Mode.inf");
        
        // Since we interact with ComponentPeer, better call it from the message thread
        MessageManager::callAsync([this, usbDriverInstaller, driverSpec]() mutable {
            runAsAdmin(usbDriverInstaller.getFullPathName().toStdString(), ("install --inf=" + driverSpec.getFullPathName()).toStdString(), editor->getPeer()->getNativeHandle());
        });
#endif
        
        installProgress = 0.0f;
        stopTimer();
        
        MessageManager::callAsync([this](){
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
    
    PlugDataPluginEditor* editor;
};

class ExportingView : public Component, public Thread, public Timer
{
    TextEditor console;
    
    ChildProcess* processToMonitor;
    
public:
    enum ExportState
    {
        Busy,
        WaitingForUserInput,
        Success,
        Failure,
        NotExporting
    };
    
    TextButton continueButton = TextButton("Continue");
    
    ExportState state = NotExporting;
    
    String userInteractionMessage;
    WaitableEvent userInteractionWait;
    TextButton confirmButton = TextButton("Done!");
    
    
    static constexpr int maxLength = 16384;
    char processOutput[maxLength];
    
    ExportingView() : Thread("Console thread")
    {
        setVisible(false);
        addChildComponent(continueButton);
        addChildComponent(confirmButton);
        addAndMakeVisible(console);
        
        continueButton.onClick = [this](){
            showState(NotExporting);
        };
        
        confirmButton.onClick = [this](){
            showState(Busy);
            userInteractionWait.signal();
        };
        
        console.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
        console.setScrollbarsShown(true);
        console.setMultiLine(true);
        console.setReadOnly(true);
        console.setWantsKeyboardFocus(true);
        
        // To ensure custom LnF got assigned...
        MessageManager::callAsync([this](){
            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
            if(!lnf) return;
            console.setFont(lnf->monoFont);
        });
    }
    
    // For the spinning animation
    void timerCallback() override
    {
        repaint();
    }
    
    void run() override
    {
        while(processToMonitor && !threadShouldExit()) {
            int len = processToMonitor->readProcessOutput(processOutput, maxLength);
            if(len) logToConsole(String::fromUTF8(processOutput, len));
            
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 100);
        }
    }
    
    void monitorProcessOutput(ChildProcess* process) {
        startTimer(20);
        processToMonitor = process;
        startThread();
    }
    
    void stopMonitoring() {
        stopThread(-1);
        stopTimer();
    }
    
    void showState(ExportState newState) {
        state = newState;
        
        MessageManager::callAsync([this](){
            setVisible(state < NotExporting);
            confirmButton.setVisible(state == WaitingForUserInput);
            continueButton.setVisible(state >= Success);
            if(state == Busy) console.setText("");
            if(console.isShowing()) {
                console.grabKeyboardFocus();
            }
            
            resized();
            repaint();
        });
    }
    
    void logToConsole(String text) {
        
        if(text.isNotEmpty()) {
            MessageManager::callAsync([_this = SafePointer(this), text](){
                if(!_this) return;
                
                _this->console.setText(_this->console.getText() + text);
                _this->console.moveCaretToEnd();
                _this->console.setScrollToShowCursor(true);
            });
        }
    }
    
    
    // Don't call from message thread!
    void waitForUserInput(String message) {
        MessageManager::callAsync([this, message]() mutable {
            userInteractionMessage = message;
            showState(WaitingForUserInput);
            repaint();
        });
        
        userInteractionWait.wait();
    }
    
    void paint(Graphics& g) override
    {
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Constants::windowCornerRadius);
        
        if(state == Busy)
        {
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting...", 0, 25, getWidth(), 40, Justification::centred);
            
            lnf->drawSpinningWaitAnimation(g, findColour(PlugDataColour::canvasTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        }
        else if(state == Success) {
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Export successful", 0, 25, getWidth(), 40, Justification::centred);
            
        }
        else if(state == Failure) {
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting failed", 0, 25, getWidth(), 40, Justification::centred);
        }
        else if(state == WaitingForUserInput)
        {
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText(userInteractionMessage, 0, 25, getWidth(), 40, Justification::centred);
        }
    }
    
    void resized() override {
        console.setBounds(proportionOfWidth (0.1f), 80, proportionOfWidth (0.8f), getHeight() - 172);
        continueButton.setBounds(proportionOfWidth (0.42f), getHeight() - 60, proportionOfWidth (0.12f), 24);
        confirmButton.setBounds(proportionOfWidth (0.42f), getHeight() - 60, proportionOfWidth (0.12f), 24);
    }
};

struct ExporterSettingsPanel : public Component, public Value::Listener, public ChildProcess, public ThreadPool
{
    TextButton exportButton = TextButton("Export");
    
    Value inputPatchValue;
    Value projectNameValue;
    Value projectCopyrightValue;
    
#if JUCE_WINDOWS
    const inline static String exeSuffix = ".exe";
#else
    const inline static String exeSuffix = "";
#endif
    
    inline static File heavyExecutable = Toolchain::dir.getChildFile("bin").getChildFile("Heavy").getChildFile("Heavy" + exeSuffix);
    
    bool validPatchSelected = false;
    
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
    
    OwnedArray<PropertiesPanel::Property> properties;
    
    File patchFile;
    File openedPatchFile;
    File realPatchFile;
    
    ExportingView* exportingView;
    
    int labelWidth = 180;
    bool shouldQuit = false;
    
    PlugDataPluginEditor* editor;
    
    ExporterSettingsPanel(PlugDataPluginEditor* pluginEditor, ExportingView* exportView) : ThreadPool(1), exportingView(exportView), editor(pluginEditor)
    {
        addAndMakeVisible(exportButton);
        
        auto* patchChooser = new PropertiesPanel::ComboComponent("Patch to export", inputPatchValue, {"Currently opened patch", "Other patch (browse)"});
        properties.add(patchChooser);
        patchChooser->comboBox.setTextWhenNothingSelected("Choose a patch to export...");
        patchChooser->comboBox.setSelectedId(-1);
        
        properties.add(new PropertiesPanel::EditableComponent<String>("Project Name (optional)", projectNameValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Project Copyright (optional)", projectCopyrightValue));
        
        inputPatchValue.addListener(this);
        projectNameValue.addListener(this);
        projectCopyrightValue.addListener(this);
        
        for(auto* panel : properties) {
            
            panel->setColour(ComboBox::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));
            addAndMakeVisible(panel);
        }
        
        if(auto* cnv = editor->getCurrentCanvas())
        {
            openedPatchFile = File::createTempFile(".pd");
            Toolchain::deleteTempFileLater(openedPatchFile);
            openedPatchFile.replaceWithText(cnv->patch.getCanvasContent(), false, false, "\n");
            patchChooser->comboBox.setItemEnabled(1, true);
            patchChooser->comboBox.setSelectedId(1);
            patchFile = openedPatchFile;
            realPatchFile = cnv->patch.getCurrentFile();
            
            if(realPatchFile.existsAsFile()) {
                projectNameValue = realPatchFile.getFileNameWithoutExtension();
            }
        }
        else {
            patchChooser->comboBox.setItemEnabled(1, false);
            patchChooser->comboBox.setSelectedId(0);
            validPatchSelected = false;
        }
        
        exportButton.onClick = [this](){
            
            auto constexpr folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
            
            saveChooser = std::make_unique<FileChooser>("Choose a location...", File::getSpecialLocation(File::userHomeDirectory), "", true);
            
            saveChooser->launchAsync(folderChooserFlags,
                                     [this](FileChooser const& fileChooser) {
                
                auto const file = fileChooser.getResult();
                
                if(file.getParentDirectory().exists()) {
                    startExport(file);
                }
            });
            
        };
    }
    
    ~ExporterSettingsPanel() {
        if(openedPatchFile.existsAsFile()) {
            openedPatchFile.deleteFile();
        }
        
        shouldQuit = true;
        if(isRunning()) kill();
        removeAllJobs(true, -1);
    }
    
    void startExport(File outDir) {
        
        auto patchPath = patchFile.getFullPathName();
        auto outPath = outDir.getFullPathName();
        auto projectTitle = projectNameValue.toString();
        auto projectCopyright = projectCopyrightValue.toString();
        
        
        if(projectTitle.isEmpty()) projectTitle = "Untitled";
        
        // Add file location to search paths
        auto searchPaths = StringArray{patchFile.getParentDirectory().getFullPathName()};
        
        // Get pd's search paths
        char* paths[1024];
        int numItems;
        libpd_get_search_paths(paths, &numItems);
        
        if(realPatchFile.existsAsFile()) {
            searchPaths.add(realPatchFile.getParentDirectory().getFullPathName());
        }
        
        for(int i = 0; i < numItems; i++) {
            searchPaths.add(paths[i]);
        }
        
        // Make sure we don't add the file location twice
        searchPaths.removeDuplicates(false);
        
        addJob([this, patchPath, outPath, projectTitle, projectCopyright, searchPaths]() mutable {
            
            exportingView->monitorProcessOutput(this);
            
            exportingView->showState(ExportingView::Busy);
            
            auto result = performExport(patchPath, outPath, projectTitle, projectCopyright, searchPaths);
            
            if(shouldQuit) return;
            
            exportingView->showState(result ? ExportingView::Failure : ExportingView::Success);
            
            exportingView->stopMonitoring();
            
            MessageManager::callAsync([this](){
                repaint();
            });
        });
    }
    
    void valueChanged(Value& v) override
    {
        if(v.refersToSameSourceAs(inputPatchValue)) {
            int idx = static_cast<int>(v.getValue());
            
            if(idx == 1) {
                patchFile = openedPatchFile;
                validPatchSelected = true;
            }
            else if(idx == 2) {
                // Open file browser
                openChooser = std::make_unique<FileChooser>("Choose file to open", File::getSpecialLocation(File::userHomeDirectory), "*.pd", true);
                
                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser){
                    
                    auto result = fileChooser.getResult();
                    if(result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    }
                    else {
                        inputPatchValue = -1;
                        patchFile = "";
                        validPatchSelected = false;
                    }
                });
            }
        }
        
        exportButton.setEnabled(validPatchSelected);
    }
    
    void resized() override
    {
        auto b = Rectangle<int>(0, 50, getWidth(), getHeight() - 38);
        
        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));
        
        for (auto* panel : properties) {
            auto panelBounds = b.removeFromTop(23);
            panel->setBounds(panelBounds);
            b.removeFromTop(5);
        }
    }
    
private:
    
    virtual bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) = 0;
    
public:
    
    
    String createMetaJson(DynamicObject::Ptr metaJson) {
        auto metadata = File::createTempFile(".json");
        Toolchain::deleteTempFileLater(metadata);
        String metaString = JSON::toString (var (metaJson.get()));
        metadata.replaceWithText(metaString, false, false, "\n");
        return metadata.getFullPathName();
    }
    
};

class CppSettingsPanel : public ExporterSettingsPanel
{
public:
    CppSettingsPanel(PlugDataPluginEditor* editor, ExportingView* exportingView) : ExporterSettingsPanel(editor, exportingView) {
    }
    
    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingView::Busy);
        
        StringArray args = {heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir};
        
        args.add("-n" + name);
        
        if(copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }
        
        args.add("-v");
        
        for(auto& path : searchPaths) {
            args.add("-p" + path);
        }
        
        if(shouldQuit) return 1;
        
        start(args);
        waitForProcessToFinish(-1);
        
        if(shouldQuit) return 1;
        
        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        
        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
        
        return getExitCode();
    }
};

class DaisySettingsPanel : public ExporterSettingsPanel
{
public:
    
    Value targetBoardValue = Value(var(1));
    Value exportTypeValue = Value(var(3));
    Value romOptimisationType = Value(var(2));
    Value ramOptimisationType = Value(var(2));
    
    TextButton flashButton = TextButton("Flash");
    Component* ramOptimisation;
    Component* romOptimisation;
    
    DaisySettingsPanel(PlugDataPluginEditor* editor, ExportingView* exportingView) : ExporterSettingsPanel(editor, exportingView)
    {
        addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, {"Seed", "Pod", "Petal", "Patch", "Path Init", "Field"})));
        addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, {"Source code", "Binary", "Flash"})));
        
        addAndMakeVisible(romOptimisation = properties.add(new PropertiesPanel::ComboComponent("ROM Optimisation", romOptimisationType, {"Optimise for size", "Optimise for speed"})));
        addAndMakeVisible(ramOptimisation = properties.add(new PropertiesPanel::ComboComponent("RAM Optimisation", ramOptimisationType, {"Optimise for size", "Optimise for speed"})));
        
        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);
        
        exportTypeValue.addListener(this);
        
        flashButton.onClick = [this](){
            
            auto tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            Toolchain::deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        };
    }
    
    void resized() override {
        ExporterSettingsPanel::resized();
        flashButton.setBounds(exportButton.getBounds());
    }
    
    void valueChanged(Value& v) override
    {
        ExporterSettingsPanel::valueChanged(v);
        flashButton.setEnabled(validPatchSelected);
        
        bool flash = static_cast<int>(exportTypeValue.getValue()) == 3;
        exportButton.setVisible(!flash);
        flashButton.setVisible(flash);
        
        ramOptimisation->setVisible(flash);
        romOptimisation->setVisible(flash);
    }
    
    
    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        auto target = static_cast<int>(targetBoardValue.getValue()) - 1;
        bool compile = static_cast<int>(exportTypeValue.getValue()) - 1;
        bool flash = static_cast<int>(exportTypeValue.getValue()) == 3;
        
        StringArray args = {heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir};
        
        args.add("-n" + name);
        
        if(copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + name + "\"");
        }
        
        auto boards = StringArray{"seed", "pod", "petal", "patch", "patch_init", "field"};
        auto board = boards[target];
        
        DynamicObject::Ptr metaJson (new DynamicObject());
        var metaDaisy (new DynamicObject());
        metaDaisy.getDynamicObject()->setProperty("board", board);
        metaJson->setProperty("daisy", metaDaisy);
        
        args.add("-m" + createMetaJson(metaJson));
        
        args.add("-v");
        args.add("-gdaisy");
        
        for(auto& path : searchPaths) {
            args.add("-p" + path);
        }
        
        start(args);
        waitForProcessToFinish(-1);
        
        exportingView->logToConsole("Compiling...");
        
        if(shouldQuit) return 1;
        
        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
        
        auto outputFile = File(outdir);
        auto sourceDir = outputFile.getChildFile("daisy").getChildFile("source");
        
        int heavyExitCode = getExitCode();
        
        if(compile) {
            
            auto bin = Toolchain::dir.getChildFile("bin");
            auto libDaisy = Toolchain::dir.getChildFile("lib").getChildFile("libDaisy");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto compiler = bin.getChildFile("arm-none-eabi-gcc" + exeSuffix);
            
            
            libDaisy.copyDirectoryTo(outputFile.getChildFile("libDaisy"));
            
            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            
            auto workingDir = File::getCurrentWorkingDirectory();
            
            sourceDir.setAsCurrentWorkingDirectory();
            
            sourceDir.getChildFile("build").createDirectory();
            Toolchain::dir.getChildFile("lib").getChildFile("heavy-static.a").copyFileTo(sourceDir.getChildFile("build").getChildFile("heavy-static.a"));
            Toolchain::dir.getChildFile("etc").getChildFile("daisy_makefile").copyFileTo(sourceDir.getChildFile("Makefile"));
            
            bool bootloader = setMakefileVariables(sourceDir.getChildFile("Makefile"));
            
            auto gccPath = bin.getFullPathName();
            
#if JUCE_WINDOWS
            auto buildScript = make.getFullPathName().replaceCharacter('\\', '/')
            + " -j4 -f "
            + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/')
            + " GCC_PATH="
            + gccPath.replaceCharacter('\\', '/')
            + " PROJECT_NAME=" + name;
            
            startShellScript(buildScript, this);
#else
            String buildScript = make.getFullPathName()
            + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName()
            + " GCC_PATH=" + gccPath
            + " PROJECT_NAME=" + name;
            
            startShellScript(buildScript, this);
#endif
            
            waitForProcessToFinish(-1);
            
            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();
            
            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
            
            auto compileExitCode = getExitCode();
            if(flash && !compileExitCode) {
                
                auto dfuUtil = bin.getChildFile("dfu-util" + exeSuffix);
                
                if(bootloader) {
                    exportingView->logToConsole("Flashing bootloader...");
                    
                    String bootloaderScript =  "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
                    + "cd " + sourceDir.getFullPathName() + "\n"
                    + make.getFullPathName() + " program-boot"
                    + " GCC_PATH=" + gccPath
                    + " PROJECT_NAME=" + name;
                    
                    startShellScript(bootloaderScript, this);
                    
                    waitForProcessToFinish(-1);
                    Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 600);
                    
                    // We need to enable DFU mode again after flashing the bootloader
                    // This will show DFU mode dialog synchonously
                    //exportingView->waitForUserInput("Please put your Daisy in DFU mode again");
                }
                
                exportingView->logToConsole("Flashing...");
                
                String flashScript = "export PATH=\"" + bin.getFullPathName() + ":$PATH\"\n"
                + "cd " + sourceDir.getFullPathName() + "\n"
                + make.getFullPathName() + " program-dfu"
                + " GCC_PATH=" + gccPath
                + " PROJECT_NAME=" + name;
                
                startShellScript(flashScript, this);
                
                waitForProcessToFinish(-1);
                
                // Delay to get correct exit code
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
                
                auto flashExitCode = getExitCode();
                
                return heavyExitCode && compileExitCode && flashExitCode;
            }
            else
            {
                auto binLocation = outputFile.getChildFile(name + ".bin");
                sourceDir.getChildFile("build").getChildFile("Heavy_" + name + ".bin").moveFileTo(binLocation);
            }
            
            outputFile.getChildFile("daisy").deleteRecursively();
            outputFile.getChildFile("libDaisy").deleteRecursively();
            
            return heavyExitCode && compileExitCode;
        }
        else {
            auto outputFile = File(outdir);
            
            auto libDaisy = Toolchain::dir.getChildFile("lib").getChildFile("libDaisy");
            libDaisy.copyDirectoryTo(outputFile.getChildFile("libDaisy"));
            
            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }
    
    bool setMakefileVariables(File makefile) {
        
        int ramType = static_cast<int>(ramOptimisationType.getValue());
        int romType = static_cast<int>(romOptimisationType.getValue());
        
        auto linkerDir = Toolchain::dir.getChildFile("etc").getChildFile("linkers");
        File linkerFile;
        
        String internalAddress = "0x08000000";
        String qspiAddress = "0x90040000";
        String flashAddress = internalAddress;
        
        bool bootloader = false;
        
        // 1 is size, 2 is speed
        if(romType == 1) {
            if(ramType == 1) {
                linkerFile = linkerDir.getChildFile("sram_linker_sdram.lds");
            }
            else if(ramType == 2) {
                linkerFile = linkerDir.getChildFile("sram_linker.lds");
            }
            
            flashAddress = qspiAddress;
            bootloader = true;
        }
        else if(romType == 2 && ramType == 1) {
            linkerFile = linkerDir.getChildFile("default_linker_sdram.lds");
        }
        // 2-2 is skipped because it's the default
        
        auto makefileText = makefile.loadFileAsString();
        if(linkerFile.existsAsFile()) makefileText = makefileText.replace("# LINKER", "LDSCRIPT = " + linkerFile.getFullPathName());
        if(bootloader) makefileText = makefileText.replace("# BOOTLOADER", "APP_TYPE = BOOT_SRAM");
        
        makefile.replaceWithText(makefileText, false, false, "\n");
        
        return bootloader;
    }
};

class DPFSettingsPanel : public ExporterSettingsPanel
{
public:
    Value midiinEnableValue = Value(var(1));
    Value midioutEnableValue = Value(var(1));
    
    Value lv2EnableValue = Value(var(1));
    Value vst2EnableValue = Value(var(1));
    Value vst3EnableValue = Value(var(1));
    Value clapEnableValue = Value(var(1));
    Value jackEnableValue = Value(var(1));
    
    Value exportTypeValue = Value(var(2));
    
    DPFSettingsPanel(PlugDataPluginEditor* editor, ExportingView* exportingView) : ExporterSettingsPanel(editor, exportingView)
    {
        
        addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, {"Source code", "Binary"})));
        
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("Midi Input", midiinEnableValue, {"No","yes"})));
        
        midiinEnableValue.addListener(this);
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("Midi Output", midioutEnableValue, {"No","yes"})));
        midioutEnableValue.addListener(this);
        
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("LV2", lv2EnableValue, {"No","Yes"})));
        lv2EnableValue.addListener(this);
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("VST2", vst2EnableValue, {"No","Yes"})));
        vst2EnableValue.addListener(this);
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("VST3", vst3EnableValue, {"No","Yes"})));
        vst3EnableValue.addListener(this);
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("CLAP", clapEnableValue, {"No","Yes"})));
        clapEnableValue.addListener(this);
        addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("JACK", jackEnableValue, {"No","Yes"})));
        
        jackEnableValue.addListener(this);
    }
    
    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        exportingView->showState(ExportingView::Busy);
        
        StringArray args = {heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir};
        
        args.add("-n" + name);
        
        if(copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add("\"" + copyright + "\"");
        }
        
        int midiin = static_cast<int>(midiinEnableValue.getValue());
        int midiout = static_cast<int>(midioutEnableValue.getValue());
        
        bool lv2 = static_cast<int>(lv2EnableValue.getValue());
        bool vst2 = static_cast<int>(vst2EnableValue.getValue());
        bool vst3 = static_cast<int>(vst3EnableValue.getValue());
        bool clap = static_cast<int>(clapEnableValue.getValue());
        bool jack = static_cast<int>(jackEnableValue.getValue());
        
        StringArray formats;
        
        if(lv2){formats.add("lv2_dsp");}
        if(vst2){formats.add("vst2");}
        if(vst3){formats.add("vst3");}
        if(clap){formats.add("clap");}
        if(jack){formats.add("jack");}
        
        DynamicObject::Ptr metaJson (new DynamicObject());
        
        var metaDPF (new DynamicObject());
        metaDPF.getDynamicObject()->setProperty("project", "true");
        metaDPF.getDynamicObject()->setProperty("description", "Rename Me");
        metaDPF.getDynamicObject()->setProperty("maker", "Wasted Audio");
        metaDPF.getDynamicObject()->setProperty("license", "ISC");
        metaDPF.getDynamicObject()->setProperty("midi_input", midiin);
        metaDPF.getDynamicObject()->setProperty("midi_output", midiout);
        metaDPF.getDynamicObject()->setProperty("plugin_formats", formats);
        
        metaJson->setProperty("dpf", metaDPF);
        
        args.add("-m" + createMetaJson(metaJson));
        
        args.add("-v");
        args.add("-gdpf");
        
        for(auto& path : searchPaths) {
            args.add("-p" + path);
        }
        
        if(shouldQuit) return 1;
        
        start(args);
        waitForProcessToFinish(-1);
        
        if(shouldQuit) return 1;
        
        auto outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        outputFile.getChildFile("c").deleteRecursively();
        
        auto DPF = Toolchain::dir.getChildFile("lib").getChildFile("dpf");
        DPF.copyDirectoryTo(outputFile.getChildFile("dpf"));
        
        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
        
        bool generationExitCode = getExitCode();
        // Check if we need to compile
        if(!generationExitCode && static_cast<int>(exportTypeValue.getValue()) == 2)
        {
            auto workingDir = File::getCurrentWorkingDirectory();
            
            outputFile.setAsCurrentWorkingDirectory();
            
            auto bin = Toolchain::dir.getChildFile("bin");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto makefile = outputFile.getChildFile("Makefile");
            
#if JUCE_MAC
            startShellScript("make -j4 -f " + makefile.getFullPathName(), this);
#elif JUCE_WINDOWS
            auto changedir = String("cd \"$(dirname \"$0\")\"\n");
            auto path = "export PATH=\"$PATH:" + Toolchain::dir.getChildFile("bin").getFullPathName().replaceCharacter('\\', '/') + "\"\n";
            auto cc = "CC=" + Toolchain::dir.getChildFile("bin").getChildFile("gcc.exe").getFullPathName().replaceCharacter('\\', '/') + " ";
            auto cxx = "CXX=" + Toolchain::dir.getChildFile("bin").getChildFile("g++.exe").getFullPathName().replaceCharacter('\\', '/') + " ";
            
            startShellScript(changedir + path + cc + cxx + make.getFullPathName().replaceCharacter('\\', '/') + " -j4 -f " + makefile.getFullPathName().replaceCharacter('\\', '/'), this);
            
            
#else // Linux or BSD
            auto bash = String("#!/bin/bash\n");
            auto changedir = String("cd \"$(dirname \"$0\")\"\n");
            auto prepareEnvironmentScript = Toolchain::dir.getChildFile("scripts").getChildFile("anywhere-setup.sh").getFullPathName() + "\n";
            
            auto buildScript = changedir
            + prepareEnvironmentScript
            + make.getFullPathName()
            + " -j4 -f " + makefile.getFullPathName();
                                    
            // For some reason we need to do this again
            outputFile.getChildFile("dpf").getChildFile("utils").getChildFile("generate-ttl.sh").setExecutePermission(true);
            
            startShellScript(buildScript, this);
#endif
            
            waitForProcessToFinish(-1);
            
            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
            
            workingDir.setAsCurrentWorkingDirectory();
            
            // Copy output
            if(lv2) outputFile.getChildFile("bin").getChildFile(name + ".lv2").copyDirectoryTo(outputFile.getChildFile(name + ".lv2"));
            if(vst3) outputFile.getChildFile("bin").getChildFile(name + ".vst3").copyDirectoryTo(outputFile.getChildFile(name + ".vst3"));
#if JUCE_WINDOWS
            if(vst2) outputFile.getChildFile("bin").getChildFile(name + "-vst.dll").moveFileTo(outputFile.getChildFile(name + "-vst.dll"));
#else
            if(vst2) outputFile.getChildFile("bin").getChildFile(name + ".vst").copyDirectoryTo(outputFile.getChildFile(name + ".vst"));
#endif
            if(clap) outputFile.getChildFile("bin").getChildFile(name + ".clap").moveFileTo(outputFile.getChildFile(name + ".clap"));
            if(jack) outputFile.getChildFile("bin").getChildFile(name).moveFileTo(outputFile.getChildFile(name));
            
            
            bool compilationExitCode = getExitCode();
            
            // Clean up if successful
            if(compilationExitCode) {
                outputFile.getChildFile("dpf").deleteRecursively();
                outputFile.getChildFile("build").deleteRecursively();
                outputFile.getChildFile("plugin").deleteRecursively();
                outputFile.getChildFile("bin").deleteRecursively();
                outputFile.getChildFile("README.md").deleteFile();
                outputFile.getChildFile("Makefile").deleteFile();
            }
            
            return compilationExitCode;
        }
        
        return generationExitCode;
    }
};

class ExporterPanel  : public Component, private ListBoxModel
{
public:
    
    ListBox listBox;
    
    TextButton addButton = TextButton(Icons::Add);
    
    OwnedArray<ExporterSettingsPanel> views;
    
    std::function<void(int)> onChange;
    
    StringArray items = {"C++", "Daisy", "DPF"};
    
    ExporterPanel(PlugDataPluginEditor* editor, ExportingView* exportingView)
    {
        addChildComponent(views.add(new CppSettingsPanel(editor, exportingView)));
        addChildComponent(views.add(new DaisySettingsPanel(editor, exportingView)));
        addChildComponent(views.add(new DPFSettingsPanel(editor, exportingView)));
        
        addAndMakeVisible(listBox);
        
        listBox.setModel(this);
        listBox.setOutlineThickness(0);
        listBox.selectRow(0);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setRowHeight(28);
    }
    
    void paint(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(200);
        
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRoundedRectangle(listboxBounds.toFloat(), Constants::windowCornerRadius);
        g.fillRect(listboxBounds.removeFromRight(10));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(200);
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(Line<float>{listboxBounds.getTopRight().toFloat(), listboxBounds.getBottomRight().toFloat()});
    }
    
    void selectedRowsChanged (int lastRowSelected) override
    {
        for(auto* view : views)
        {
            // Make sure we remember common values when switching views
            if(view->isVisible()) {
                views[lastRowSelected]->patchFile = view->patchFile;
                views[lastRowSelected]->projectNameValue = view->projectNameValue.getValue();
                views[lastRowSelected]->projectCopyrightValue = view->projectCopyrightValue.getValue();
                views[lastRowSelected]->inputPatchValue = view->inputPatchValue.getValue();
            }
            view->setVisible(false);
        }
        
        views[lastRowSelected]->setVisible(true);
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        listBox.setBounds(b.removeFromLeft(200).reduced(4));
        
        for(auto* view : views)
        {
            view->setBounds(b);
        }
    }
    
    int getNumRows() override
    {
        return items.size();
    }
    
    StringArray getExports() {
        return items;
    }
    
    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected) {
                g.setColour(findColour (PlugDataColour::sidebarActiveBackgroundColourId));
                g.fillRoundedRectangle(5, 3, width - 10, height - 6, Constants::smallCornerRadius);
            }
            
            const auto textColour = findColour(rowIsSelected ? PlugDataColour::sidebarActiveTextColourId : PlugDataColour::sidebarTextColourId);
            g.setColour (textColour);
            g.setFont (15);
            g.drawText(items[row], Rectangle<int>(15, 0, width - 30, height), Justification::centredLeft);
        }
    }
    
    int getBestHeight (int preferredHeight)
    {
        auto extra = listBox.getOutlineThickness() * 2;
        return jmax (listBox.getRowHeight() * 2 + extra,
                     jmin (listBox.getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExporterPanel)
};


struct HeavyExportDialog : public Component
{
    bool hasToolchain = false;
    
    ExportingView exportingView;
    ToolchainInstaller installer;
    ExporterPanel exporterPanel;
    
    HeavyExportDialog(Dialog* dialog) : exporterPanel(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent), &exportingView), installer(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent)) {
        
        hasToolchain = Toolchain::dir.exists();
        
        // Create integer versions by removing the dots
        // Compare latest version on github to the currently installed version
        const auto latestVersion = URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/VERSION").readEntireTextStream().trim().removeCharacters(".").getIntValue();
        
        // Don't do this relative to toolchain variable, that won't work on Windows
        const auto versionFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain").getChildFile("VERSION");
        const auto installedVersion = versionFile.loadFileAsString().trim().removeCharacters(".").getIntValue();
        
        if(hasToolchain && latestVersion > installedVersion) {
            installer.needsUpdate = true;
            hasToolchain = false;
        }
        
        addChildComponent(installer);
        addChildComponent(exporterPanel);
        addChildComponent(exportingView);
        
        exportingView.setAlwaysOnTop(true);
        
        installer.toolchainInstalledCallback = [this](){
            hasToolchain = true;
            exporterPanel.setVisible(true);
            installer.setVisible(false);
        };
        
        if(hasToolchain) {
            exporterPanel.setVisible(true);
        }
        else {
            installer.setVisible(true);
        }
    }
    
    ~HeavyExportDialog() {
        // Clean up temp files
        Toolchain::deleteTempFiles();
    }
    
    void paint(Graphics& g)
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Constants::windowCornerRadius);
    }
    
    void resized() {
        auto b = getLocalBounds();
        exporterPanel.setBounds(b);
        installer.setBounds(b);
        exportingView.setBounds(b);
    }
};
