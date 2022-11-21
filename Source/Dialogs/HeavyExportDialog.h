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

struct ToolchainInstaller : public Component, public Thread, public Timer
{
    struct InstallButton : public Component
    {
        
#if JUCE_WINDOWS
        String downloadSize = "390 MB";
#elif JUCE_MAC
        String downloadSize = "650 MB";
#else
        String downloadSize = "800 MB";
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
                g.drawRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, 4.0f, 0.5f);
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
    
#if JUCE_LINUX
    std::tuple<String, String, String> getDistroID()
    {
        ChildProcess catProcess;
        catProcess.start({"cat", "/etc/os-release"});
        
        auto ret = catProcess.readAllProcessOutput();
        
        auto items = StringArray::fromLines(String(ret));
        
        String name;
        String idLike;
        String version;
        
        for(auto& item : items) {
            if(item.startsWith("ID=")) {
                name = item.fromFirstOccurrenceOf("=", false, false).trim();
            }
            else if(item.startsWith("ID_LIKE=")) {
                idLike = item.fromFirstOccurrenceOf("=", false, false).trim();
            }
            else if(item.startsWith("VERSION_ID=")) {
                version = item.fromFirstOccurrenceOf("=", false, false).trim();
            }
        }
        
        return {name, idLike, version};
    }
#endif
    
    ToolchainInstaller(PlugDataPluginEditor* pluginEditor) : Thread("Toolchain Install Thread"), editor(pluginEditor) {
        addAndMakeVisible(&installButton);
        
        installButton.onClick = [this](){
            errorMessage = "";
            repaint();
            
            // Get latest version
            auto latestVersion = "v" + URL("https://raw.githubusercontent.com/timothyschoen/HeavyDistributable/main/VERSION").readEntireTextStream().trim();
            
            if(latestVersion == "v") {
                errorMessage = "Error: Could not download files (possibly no network connection)";
                installButton.topText = "Try Again";
                repaint();
            }
            
            String downloadLocation = "https://github.com/timothyschoen/HeavyDistributable/releases/download/" + latestVersion + "/";
            
#if JUCE_MAC
            downloadLocation += "Heavy-MacOS-Universal.zip";
#elif JUCE_WINDOWS
            downloadLocation += "Heavy-Win64.zip";
#elif JUCE_LINUX && __aarch64__
            downloadLocation += "Heavy-Linux-arm64.zip";
#else
            
            auto [distroName, distroBackupName, distroVersion] = getDistroID();
            
            if(distroName == "fedora" && distroVersion == "36") {
                downloadLocation += "Heavy-Fedora-36-x64.zip";
            }
            else if(distroName == "fedora" && distroVersion == "35") {
                downloadLocation += "Heavy-Fedora-35-x64.zip";
            }
            else if(distroName == "ubuntu" && distroVersion == "22.04") {
                downloadLocation += "Heavy-Ubuntu-22.04-x64.zip";
            }
            else if(distroBackupName == "ubuntu" || distroName == "ubuntu") {
                downloadLocation += "Heavy-Ubuntu-20.04-x64.zip";
            }
            else if(distroName == "arch" || distroBackupName == "arch") {
                downloadLocation += "Heavy-Arch-x64.zip";
            }
            else if(distroName == "debian" || distroBackupName == "debian") {
                downloadLocation += "Heavy-Debian-x64.zip";
            }
            else if(distroName == "opensuse-leap" || distroBackupName.contains("suse")) {
                downloadLocation += "Heavy-OpenSUSE-Leap-x64.zip";
            }
            else if(distroName == "mageia") {
                downloadLocation += "Heavy-Mageia-x64.zip";
            }
            // If we're not sure, just try the debian one and pray
            else {
                downloadLocation += "Heavy-Debian-x64.zip";
            }
#endif
            instream = URL(downloadLocation).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                                                               .withConnectionTimeoutMs(5000)
                                                               .withStatusCode(&statusCode));
            startThread(3);
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
        MemoryBlock toolchainData;
        
        if(!instream) return; // error!
        
        int64 totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;
        
        MemoryOutputStream mo(toolchainData, true);
        
        while (true) {
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
        
        auto result = zip.uncompressTo(toolchain);
        
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
#if JUCE_MAC || JUCE_LINUX
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/Heavy/Heavy").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/make").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/dfu-util").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/arm-none-eabi-*").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/arm-none-eabi/bin/*").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/libexec/gcc/arm-none-eabi/*/*").toRawUTF8());
#endif
        
#if JUCE_LINUX
        // Add udev rule for the daisy seed
        // This makes sure we can use dfu-util without admin privileges
        // Kinda sucks that we need to sudo this, but there's no other way AFAIK
        system("echo SUBSYSTEM==\"usb\", ATTR{idVendor}==\"0483\", ATTR{idProduct}==\"df11\", MODE=\"0664\", GROUP=\"plugdev\" | pkexec tee /etc/udev/rules.d/50-daisy-stmicro-dfu.rules >/dev/null");
#elif JUCE_WINDOWS
        File usbDriverInstaller = toolchain.getChildFile("etc").getChildFile("usb_driver").getChildFile("install-filter.exe");
        File driverSpec = toolchain.getChildFile("etc").getChildFile("usb_driver").getChildFile("DFU_in_FS_Mode.inf");

        runAsAdmin(usbDriverInstaller.getFullPathName().toStdString(), ("install --inf=" + driverSpec.getFullPathName()).toStdString(), editor->getPeer()->getNativeHandle());
#endif
        installProgress = 0.0f;
        stopTimer();
        
        MessageManager::callAsync([this](){
            toolchainInstalledCallback();
        });
    }
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    float installProgress = 0.0f;
    
    bool needsUpdate = false;
    int statusCode;
    
    InstallButton installButton;
    std::function<void()> toolchainInstalledCallback;
    
    String errorMessage;
    
    std::unique_ptr<InputStream> instream;
    
    PlugDataPluginEditor* editor;
};

class ExportingView : public Component
{
    TextEditor console;
    
public:
    enum ExportState
    {
        Busy,
        Success,
        Failure,
        NotExporting
    };
    
    TextButton continueButton = TextButton("Continue");
    
    ExportState state = NotExporting;
    
    ExportingView()
    {
        setVisible(false);
        addChildComponent(continueButton);
        addAndMakeVisible(console);
        
        continueButton.onClick = [this](){
            showState(NotExporting);
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
    
    void showState(ExportState newState) {
        state = newState;
        
        MessageManager::callAsync([this](){
            setVisible(state < NotExporting);
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
    
    void paint(Graphics& g) override
    {
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        if(state == Busy)
        {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting...", 0, 25, getWidth(), 40, Justification::centred);
            
            lnf->drawSpinningWaitAnimation(g, findColour(PlugDataColour::canvasTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        }
        else if(state == Success) {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Export successful", 0, 25, getWidth(), 40, Justification::centred);
            
        }
        else if(state == Failure) {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting failed", 0, 25, getWidth(), 40, Justification::centred);
        }
    }
    
    void resized() override {
        console.setBounds(proportionOfWidth (0.1f), 80, proportionOfWidth (0.8f), getHeight() - 172);
        continueButton.setBounds(proportionOfWidth (0.42f), getHeight() - 60, proportionOfWidth (0.12f), 24);
    }
};

struct ExporterSettingsPanel : public Component, public Value::Listener, public Timer, public ChildProcess, public ThreadPool
{
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    TextButton exportButton = TextButton("Export");
    
    Value inputPatchValue;
    Value projectNameValue;
    Value projectCopyrightValue;
    
    inline static File heavyExecutable = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain").getChildFile("bin").getChildFile("Heavy").getChildFile("Heavy");
    
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
    
    ExporterSettingsPanel(PlugDataPluginEditor* pluginEditor, ExportingView* exportView) : ThreadPool(2), exportingView(exportView), editor(pluginEditor)
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
            openedPatchFile.replaceWithText(cnv->patch.getCanvasContent());
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
            startTimer(25);
            exportingView->showState(ExportingView::Busy);
            
            auto result = performExport(patchPath, outPath, projectTitle, projectCopyright, searchPaths);
            
            if(shouldQuit) return;
            
            exportingView->showState(result ? ExportingView::Failure : ExportingView::Success);
            
            MessageManager::callAsync([this](){
                stopTimer();
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
    
    void paintOverChildren(Graphics& g) override {
        auto b = Rectangle<int>(0, 50, getWidth(), getHeight() - 38);
        
        g.setColour(findColour(PlugDataColour::outlineColourId).withAlpha(0.4f));
        //g.drawLine(Line<int>(b.getTopLeft(), b.getTopRight()).toFloat());
        
        for (auto* panel : properties) {
            auto panelBounds = b.removeFromTop(23);
            //g.drawLine(Line<int>(panelBounds.getBottomLeft(), panelBounds.getBottomRight()).toFloat());
        }
        
        //g.drawLine((getWidth() / 2.0f) + 1, 50, (getWidth() / 2.0f) + 1, 50 + properties.size() * 23);
    }
    
private:
    
    static constexpr int maxLength = 32768;
    char processOutput[maxLength];
    
    void timerCallback() override {
        
        if(getNumJobs() < 2) {
            addJob([this](){
                // This blocks, so we need to run it on another thread
                int len = readProcessOutput(processOutput, maxLength);
                
                MessageManager::callAsync([_this = SafePointer(this), len]() mutable {
                    if(!_this) return;
                    _this->exportingView->logToConsole(String(_this->processOutput, len));
                });
            });
        }
        
        exportingView->repaint();
    }
    
    virtual bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) = 0;
    
public:
    
    String createMetadata(String exporter, std::map<String, String> settings) {
        auto metadata = File::createTempFile(".json");
        
        String metaString = "{\n";
        metaString +=  "\"" + exporter + "\": {\n";
        for(auto& [key, value] : settings) {
            metaString += "\"" + key + "\": \"" + value + "\"\n";
        }
        metaString += "}\n";
        metaString += "}";
        
        metadata.replaceWithText(metaString);
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
            args.add("\"" + name + "\"");
        }
        
        args.add("-v");
        
        String searchPathArg = "-p\"[";
        for(auto& path : searchPaths) {
            searchPathArg += path + ", ";
        }
        searchPathArg.trimCharactersAtEnd(", ");
        searchPathArg += "]";
        
        args.add(searchPathArg);
        
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
        addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, {"Seed", "Pod", "Petal", "Patch", "Field"})));
        addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, {"Source code", "Binary", "Flash"})));
        addAndMakeVisible(romOptimisation = properties.add(new PropertiesPanel::ComboComponent("ROM Optimisation", romOptimisationType, {"Optimise for size", "Optimise for speed"})));
        addAndMakeVisible(ramOptimisation = properties.add(new PropertiesPanel::ComboComponent("RAM Optimisation", ramOptimisationType, {"Optimise for size", "Optimise for speed"})));
        
        exportButton.setVisible(false);
        addAndMakeVisible(flashButton);
        
        exportTypeValue.addListener(this);
        
        flashButton.onClick = [this](){
            auto tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
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
        
        auto boards = StringArray{"seed", "pod", "petal", "patch", "field"};
        auto board = boards[target];
        
        args.add("-m" + createMetadata("daisy", {{"board", board}}));
        args.add("-v");
        args.add("-gdaisy");
        
        start(args);
        waitForProcessToFinish(-1);
        
        exportingView->logToConsole("Compiling...");
        
        if(shouldQuit) return 1;
        
        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
        
        int heavyExitCode = getExitCode();
        
        if(compile) {
            
            auto bin = toolchain.getChildFile("bin");
            auto libDaisy = toolchain.getChildFile("lib").getChildFile("libDaisy");
            auto make = bin.getChildFile("make" + exeSuffix);
            auto compiler = bin.getChildFile("arm-none-eabi-gcc" + exeSuffix);
            
            auto outputFile = File(outdir);
            libDaisy.copyDirectoryTo(outputFile.getChildFile("libDaisy"));
            
            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            
            auto workingDir = File::getCurrentWorkingDirectory();
            auto sourceDir = outputFile.getChildFile("daisy").getChildFile("source");
            
            sourceDir.setAsCurrentWorkingDirectory();
            
            sourceDir.getChildFile("build").createDirectory();
            toolchain.getChildFile("lib").getChildFile("heavy-static.a").copyFileTo(sourceDir.getChildFile("build").getChildFile("heavy-static.a"));
            toolchain.getChildFile("etc").getChildFile("daisy_makefile").copyFileTo(sourceDir.getChildFile("Makefile"));
            
            auto gccPath = bin.getFullPathName();
            
            int ramType = static_cast<int>(romOptimisationType.getValue());
            int romType = static_cast<int>(romOptimisationType.getValue());
            
            
            auto linkerDir = toolchain.getChildFile("etc").getChildFile("linkers");
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
            
            auto makefileText = sourceDir.getChildFile("Makefile").loadFileAsString();
            if(linkerFile.existsAsFile()) makefileText = makefileText.replace("# LINKER", "LDSCRIPT = " + linkerFile.getFullPathName());
            if(bootloader) makefileText = makefileText.replace("# BOOTLOADER", "APP_TYPE = BOOT_SRAM");
            sourceDir.getChildFile("Makefile").replaceWithText(makefileText);
            
#if JUCE_WINDOWS
            auto buildScript = sourceDir.getChildFile("build.sh");
            buildScript.replaceWithText(make.getFullPathName().replaceCharacter('\\', '/') + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/') + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/') + " PROJECT_NAME=" + name);
            
            auto sh = toolchain.getChildFile("bin").getChildFile("sh.exe");
            String command = sh.getFullPathName() + " --login " + buildScript.getFullPathName().replaceCharacter('\\', '/');
#else
            String command = make.getFullPathName().replaceCharacter('\\', '/') + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/') + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/') + " PROJECT_NAME=" + name;
#endif
            // Use std::system because on Mac, juce ChildProcess is slow when using Rosetta
            start(command.toRawUTF8());
            waitForProcessToFinish(-1);
            
            // Restore original working directory
            workingDir.setAsCurrentWorkingDirectory();
            
            auto binLocation = outputFile.getChildFile(name + ".bin");
            sourceDir.getChildFile("build").getChildFile("Heavy_" + name + ".bin").moveFileTo(binLocation);
            
            outputFile.getChildFile("daisy").deleteRecursively();
            outputFile.getChildFile("libDaisy").deleteRecursively();
            
            // Delay to get correct exit code
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
            
            auto compileExitCode = getExitCode();
            if(flash && !compileExitCode) {
                
                auto dfuUtil = bin.getChildFile("dfu-util" + exeSuffix);
                
                if(bootloader) {
                    exportingView->logToConsole("Flashing bootloader...");
                    auto bootBin = libDaisy.getChildFile("core").getChildFile("dsy_bootloader_v5_4.bin");
                    auto dfuBootloaderCommand = dfuUtil.getFullPathName() + " -a 0 -s " + internalAddress + ":leave -D " + bootBin.getFullPathName() + " -d ,0483:df11";
                    start(dfuBootloaderCommand.toRawUTF8());
                    waitForProcessToFinish(-1);
                    Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
                }
                
                exportingView->logToConsole("Flashing...");
                
                
                auto dfuCommand = dfuUtil.getFullPathName() + " -a 0 -s " + flashAddress + ":leave -D " + binLocation.getFullPathName() + " -d ,0483:df11";
                
                start(dfuCommand.toRawUTF8());
                waitForProcessToFinish(-1);
                
                // Delay to get correct exit code
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);
                
                auto flashExitCode = getExitCode();
                
                return heavyExitCode && compileExitCode && flashExitCode;
            }
            
            return heavyExitCode && compileExitCode;
        }
        else {
            auto outputFile = File(outdir);
            
            auto libDaisy = toolchain.getChildFile("lib").getChildFile("libDaisy");
            libDaisy.copyDirectoryTo(outputFile.getChildFile("libDaisy"));
            
            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            outputFile.getChildFile("c").deleteRecursively();
            return heavyExitCode;
        }
    }
    
#if JUCE_WINDOWS
    String exeSuffix = ".exe";
#else
    String exeSuffix = "";
#endif
};

class ExporterPanel  : public Component, private ListBoxModel
{
public:
    
    ListBox listBox;
    
    TextButton addButton = TextButton(Icons::Add);
    
    OwnedArray<ExporterSettingsPanel> views;
    
    std::function<void(int)> onChange;
    
    StringArray items = {"C++", "Daisy"};
    
    ExporterPanel(PlugDataPluginEditor* editor, ExportingView* exportingView)
    {
        addChildComponent(views.add(new CppSettingsPanel(editor, exportingView)));
        addChildComponent(views.add(new DaisySettingsPanel(editor, exportingView)));
        
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
        g.fillRoundedRectangle(listboxBounds.toFloat(), 5.0f);
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
                g.setColour(findColour (PlugDataColour::panelActiveBackgroundColourId));
                g.fillRoundedRectangle(5, 3, width - 10, height - 6, 5.0f);
            }
            
            const auto textColour = findColour(rowIsSelected ? PlugDataColour::panelActiveTextColourId : PlugDataColour::panelTextColourId);
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
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    HeavyExportDialog(Dialog* dialog) : exporterPanel(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent), &exportingView), installer(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent)) {
        
        hasToolchain = toolchain.exists();
        
        // Create integer versions by removing the dots
        // Compare latest version on github to the currently installed version
        auto latestVersion = URL("https://raw.githubusercontent.com/timothyschoen/HeavyDistributable/main/VERSION").readEntireTextStream().trim().removeCharacters(".").getIntValue();
        auto installedVersion = toolchain.getChildFile("VERSION").loadFileAsString().trim().removeCharacters(".").getIntValue();
        
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
    
    void paint(Graphics& g)
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    }
    
    void resized() {
        auto b = getLocalBounds();
        exporterPanel.setBounds(b);
        installer.setBounds(b);
        exportingView.setBounds(b);
    }
};
