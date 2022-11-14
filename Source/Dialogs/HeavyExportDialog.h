/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"

#if JUCE_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <z_libpd.h>

struct ToolchainInstaller : public Component, public Thread
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
    
    ToolchainInstaller() : Thread("Toolchain Install Thread") {
        addAndMakeVisible(&installButton);

        
        installButton.onClick = [this](){
            
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

    }
    
    void resized() override
    {
        installButton.setBounds(getLocalBounds().withSizeKeepingCentre(450, 50).translated(15, -30));
    }
    
    void run() override
    {
        MemoryBlock dekData;
        
        if(!instream) return; // error!
        
        int64 totalBytes = instream->getTotalLength();
        int64 bytesDownloaded = 0;
        
        MemoryOutputStream mo(dekData, true);
        
        while (true) {
            if (threadShouldExit()) {
                //finish(Result::fail("Download cancelled"));
                return;
            }
            
            auto written = mo.writeFromInputStream(*instream, 8192);
            
            if (written == 0)
                break;
            
            bytesDownloaded += written;
            
            float progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);
            
            MessageManager::callAsync([this, progress]() mutable {
                installProgress = progress;
                repaint();
            });
        }
        
        MemoryInputStream input(dekData, false);
        ZipFile zip(input);
        
        auto result = zip.uncompressTo(toolchain);
        
        if (!result.wasOk()) {
            MessageManager::callAsync([this](){
                installButton.topText = "Try Again";
                errorMessage = "Error: Could not extract downloaded package";
                repaint();
            });
            return;
        }
        
        
        // Make sure downloaded files have executable permission on unix
#if JUCE_MAC || JUCE_LINUX
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/Heavy/Heavy").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/make").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/bin/arm-none-eabi-*").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/arm-none-eabi/bin/*").toRawUTF8());
        system(("chmod +x " + toolchain.getFullPathName() + "/libexec/gcc/arm-none-eabi/*/*").toRawUTF8());
#endif
        
        installProgress = 0.0f;
        
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
        
        
        console.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));
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
            console.setText(console.getText() + text);
            console.moveCaretToEnd();
            console.setScrollToShowCursor(true);
        }
    }
    
    void paint(Graphics& g) override
    {
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        if(state == Busy)
        {
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting...", 0, 25, getWidth(), 40, Justification::centred);
            
            lnf->drawSpinningWaitAnimation(g, findColour(PlugDataColour::canvasTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        }
        else if(state == Success) {
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Export successful", 0, 25, getWidth(), 40, Justification::centred);
            
        }
        else if(state == Failure) {
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
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
    
    ComboBox patchChooser;
    
    TextEditor outputPathEditor;
    TextButton outputPathBrowseButton = TextButton("Browse");
    
    TextEditor projectNameEditor;
    TextEditor projectCopyrightEditor;
    
    File patchFile;
    File openedPatchFile;
    
    ExportingView* exportingView;
    
    int labelWidth = 180;
    bool shouldQuit = false;
    
    ExporterSettingsPanel(PlugDataPluginEditor* editor, ExportingView* exportView) : ThreadPool(2), exportingView(exportView)
    {
        addAndMakeVisible(exportButton);
        addAndMakeVisible(patchChooser);
        addAndMakeVisible(outputPathEditor);
        addAndMakeVisible(outputPathBrowseButton);
        addAndMakeVisible(projectNameEditor);
        addAndMakeVisible(projectCopyrightEditor);
        
        outputPathBrowseButton.setConnectedEdges(12);
        
        patchChooser.addItem("Currently opened patch", 1);
        patchChooser.addItem("Other patch (browse)", 2);
        patchChooser.setTextWhenNothingSelected("Choose a patch to export...");
        
        exporterSelected.addListener(this);
        validPatchSelected.addListener(this);
        targetFolderSelected.addListener(this);
        
        if(auto* cnv = editor->getCurrentCanvas())
        {
            // TODO: because we save it to a tempfile, it's important to add the real path (if applicable) to Heavy's search paths!
            openedPatchFile = File::createTempFile(".pd");
            openedPatchFile.replaceWithText(cnv->patch.getCanvasContent());
            patchChooser.setItemEnabled(1, true);
            patchChooser.setSelectedId(1);
            patchFile = openedPatchFile;
        }
        else {
            patchChooser.setItemEnabled(1, false);
            patchChooser.setSelectedId(0);
            validPatchSelected = false;
        }
        
        patchChooser.onChange = [this](){
            if(patchChooser.getSelectedId() == 1) {
                patchFile = openedPatchFile;
                validPatchSelected = true;
            }
            else if(patchChooser.getSelectedId() == 2) {
                // Open file browser
                openChooser = std::make_unique<FileChooser>("Choose file to open", File::getSpecialLocation(File::userHomeDirectory), "*.pd", true);
                
                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser){
                    
                    auto result = fileChooser.getResult();
                    if(result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    }
                    else {
                        patchChooser.setSelectedId(0);
                        patchFile = "";
                        validPatchSelected = false;
                    }
                }
                                         );
            };
        };
        
        outputPathEditor.onTextChange = [this](){
            auto outputDir = File(outputPathEditor.getText());
            targetFolderSelected = outputDir.getParentDirectory().exists() && !outputDir.existsAsFile();
        };
        
        exportButton.onClick = [this, editor](){
            auto outDir = File(outputPathEditor.getText());
            outDir.createDirectory();
            
            auto patchPath = patchFile.getFullPathName();
            auto outPath = outDir.getFullPathName();
            auto projectTitle = projectNameEditor.getText();
            auto projectCopyright = projectCopyrightEditor.getText();
            
            
            // Add file location to search paths
            auto searchPaths = StringArray{patchFile.getParentDirectory().getFullPathName()};
            
            // Get pd's search paths
            char* paths[1024];
            int numItems;
            libpd_get_search_paths(paths, &numItems);
            
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
        };
        
        outputPathBrowseButton.onClick = [this](){
            auto constexpr folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
            
            saveChooser = std::make_unique<FileChooser>("Export location", File::getSpecialLocation(File::userHomeDirectory), "", true);
            
            saveChooser->launchAsync(folderChooserFlags,
                                     [this](FileChooser const& fileChooser) {
                auto const file = fileChooser.getResult();
                outputPathEditor.setText(file.getFullPathName());
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
    
    void valueChanged(Value& v) override
    {
        bool exportReady = static_cast<bool>(validPatchSelected.getValue()) && static_cast<bool>(targetFolderSelected.getValue());
        exportButton.setEnabled(exportReady);
    }
    
    void resized() override
    {
        auto b = Rectangle<int>(proportionOfWidth (0.1f), 30, proportionOfWidth (0.8f), getHeight());
                
        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));
        
        b.removeFromTop(20);
        patchChooser.setBounds(b.removeFromTop(23));
        
        b.removeFromTop(15);
        projectNameEditor.setBounds(b.removeFromTop(23).withTrimmedLeft(labelWidth));
        
        b.removeFromTop(15);
        projectCopyrightEditor.setBounds(b.removeFromTop(23).withTrimmedLeft(labelWidth));
        
        b.removeFromTop(15);
        auto outputPathBounds = b.removeFromTop(23);
        outputPathEditor.setBounds(outputPathBounds.removeFromLeft(proportionOfWidth (0.65f)).withTrimmedLeft(labelWidth));
        outputPathBrowseButton.setBounds(outputPathBounds.withTrimmedLeft(-1));
    }
    
    void paint(Graphics& g) override
    {
        auto b = Rectangle<int>(proportionOfWidth (0.1f), 90, labelWidth, getHeight() - 35);
        
        g.setFont(Font(15));
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText("Project Name (optional)", b.removeFromTop(23), Justification::centredLeft);
        
        b.removeFromTop(15);
        
        g.drawText("Project Copyright (optional)", b.removeFromTop(23), Justification::centredLeft);
        
        b.removeFromTop(15);
        
        g.drawText("Output Path", b.removeFromTop(23), Justification::centredLeft);
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
    
    inline static File heavyExecutable = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain").getChildFile("bin").getChildFile("Heavy").getChildFile("Heavy");
    
    
    Value exporterSelected = Value(var(false));
    Value validPatchSelected = Value(var(false));
    Value targetFolderSelected = Value(var(false));
    
    
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
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
        
        if(name.isNotEmpty()) {
            args.add("-n" + name);
        }
        else {
            args.add("-nUntitled");
        }
        
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
        
        return getExitCode();
    }
    
};

class DaisySettingsPanel : public ExporterSettingsPanel
{
public:
    
    ComboBox targetBoard;
    ComboBox exportType;
    
    DaisySettingsPanel(PlugDataPluginEditor* editor, ExportingView* exportingView) : ExporterSettingsPanel(editor, exportingView)
    {
        
        targetBoard.addItem("Seed", 1);
        targetBoard.addItem("Pod", 2);
        targetBoard.addItem("Petal", 3);
        targetBoard.addItem("Patch", 4);
        targetBoard.addItem("Field", 5);
        
        exportType.addItem("Binary", 1);
        exportType.addItem("Source code", 2);
        
        targetBoard.setSelectedId(1);
        exportType.setSelectedId(1);
        
        addAndMakeVisible(exportType);
        addAndMakeVisible(targetBoard);
    }
    
    bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) override
    {
        auto target = targetBoard.getSelectedId() - 1;
        bool compile = !(exportType.getSelectedId() - 1);
        
        StringArray args = {heavyExecutable.getFullPathName(), pdPatch, "-o" + outdir};
        
        if(name.isNotEmpty()) {
            args.add("-n" + name);
        }
        else {
            args.add("-nUntitled");
        }
        
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
        
        
        if(shouldQuit) return 1;
        
        int heavyExitCode = getExitCode();
        
        if(compile) {
            
            auto bin = toolchain.getChildFile("bin");
            auto libDaisy = toolchain.getChildFile("lib").getChildFile("libDaisy");
            
#if JUCE_WINDOWS
            auto make = bin.getChildFile("make.exe");
            auto compiler = bin.getChildFile("arm-none-eabi-gcc.exe");
#else
            auto make = bin.getChildFile("make");
            auto compiler = bin.getChildFile("arm-none-eabi-gcc");
#endif
            
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
            toolchain.getChildFile("share").getChildFile("daisy_makefile").copyFileTo(sourceDir.getChildFile("Makefile"));
            
            auto gccPath = toolchain.getChildFile("bin").getFullPathName();
            
            auto projectName = projectNameEditor.getText();
            
#if JUCE_WINDOWS
            
            auto sh = toolchain.getChildFile("bin").getChildFile("sh.exe");
            auto windowsBuildScript = sourceDir.getChildFile("build.sh");
            windowsBuildScript.replaceWithText(make.getFullPathName().replaceCharacter('\\', '/') + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName().replaceCharacter('\\', '/') + " GCC_PATH=" + gccPath.replaceCharacter('\\', '/') + " PROJECT_NAME=" + projectName);
            
            String command = sh.getFullPathName() + " --login " + windowsBuildScript.getFullPathName().replaceCharacter('\\', '/');
#else
            String command = make.getFullPathName() + " -j4 -f " + sourceDir.getChildFile("Makefile").getFullPathName() + " GCC_PATH=" + gccPath + " PROJECT_NAME=" + projectName;
#endif
            // Use std::system because on Mac, juce ChildProcess is slow when using Rosetta
            start(command.toRawUTF8());
            waitForProcessToFinish(-1);
            
            workingDir.setAsCurrentWorkingDirectory();
            
            sourceDir.getChildFile("build").getChildFile("Heavy_" + projectName + ".bin").moveFileTo(outputFile.getChildFile(projectName + ".bin"));
            
            outputFile.getChildFile("daisy").deleteRecursively();
            outputFile.getChildFile("libDaisy").deleteRecursively();
            
            return heavyExitCode && getExitCode();
        }
        else {
            auto outputFile = File(outdir);
            outputFile.getChildFile("ir").deleteRecursively();
            outputFile.getChildFile("hv").deleteRecursively();
            return heavyExitCode;
        }
        
       
    }
    
    void resized() override
    {
        ExporterSettingsPanel::resized();
        
        auto b = Rectangle<int>(proportionOfWidth (0.1f), 202, proportionOfWidth (0.8f), getHeight() - 202);
        targetBoard.setBounds(b.removeFromTop(23).withTrimmedLeft(labelWidth));
        
        b.removeFromTop(15);
        
        exportType.setBounds(b.removeFromTop(23).withTrimmedLeft(labelWidth));
    }
    
    void paint(Graphics& g) override
    {
        ExporterSettingsPanel::paint(g);
        
        auto b = Rectangle<int>(proportionOfWidth (0.1f), 202, labelWidth, getHeight() - 202);
        
        g.setFont(Font(15));
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText("Target board", b.removeFromTop(23), Justification::centredLeft);
        
        b.removeFromTop(15);
        g.drawText("Export type", b.removeFromTop(23), Justification::centredLeft);
    }
    
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
        
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
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
        String lastTitle;
        String lastCopyright;
        String lastOutdir;
        File lastPatchFile;
        int comboBoxSelection;
        
        for(int i = 0; i < views.size(); i++)
        {
            if(views[i]->isVisible() && i != lastRowSelected) {
                lastTitle = views[i]->projectNameEditor.getText();
                lastCopyright = views[i]->projectCopyrightEditor.getText();
                lastOutdir = views[i]->outputPathEditor.getText();
                lastPatchFile = views[i]->patchFile;
                comboBoxSelection = views[i]->patchChooser.getSelectedId();
            }
        }
        for(int i = 0; i < views.size(); i++)
        {
            views[i]->projectNameEditor.setText(lastTitle);
            views[i]->projectCopyrightEditor.setText(lastCopyright);
            views[i]->outputPathEditor.setText(lastOutdir);
            views[i]->patchFile = lastPatchFile;
            views[i]->patchChooser.setSelectedId(comboBoxSelection);
            
            views[i]->setVisible(i == lastRowSelected);
        }
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
            
            auto item = items[row];
            
            auto textBounds = Rectangle<int>(15, 0, width - 30, height);
            const auto textColour = findColour(rowIsSelected ? PlugDataColour::panelActiveTextColourId : PlugDataColour::panelTextColourId);
            
            AttributedString attributedString { item };
            attributedString.setColour (textColour);
            attributedString.setFont (15);
            attributedString.setJustification (Justification::centredLeft);
            attributedString.setWordWrap (AttributedString::WordWrap::none);
            
            TextLayout textLayout;
            textLayout.createLayout (attributedString,
                                     (float) textBounds.getWidth(),
                                     (float) textBounds.getHeight());
            textLayout.draw (g, textBounds.toFloat());
        }
    }
    
    int getBestHeight (int preferredHeight)
    {
        auto extra = listBox.getOutlineThickness() * 2;
        
        return jmax (listBox.getRowHeight() * 2 + extra,
                     jmin (listBox.getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }
    
private:
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExporterPanel)
};


struct HeavyExportDialog : public Component
{
    bool hasToolchain = false;
    
    ExportingView exportingView;
    ToolchainInstaller installer;
    ExporterPanel exporterPanel;
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    HeavyExportDialog(Dialog* dialog) : exporterPanel(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent), &exportingView) {
        
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
        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    }
    
    void resized() {
        auto b = getLocalBounds();
        exporterPanel.setBounds(b);
        installer.setBounds(b);
        exportingView.setBounds(b);
        
    }
    
};
