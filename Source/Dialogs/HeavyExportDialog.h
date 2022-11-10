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

class ExporterListBox  : public ListBox, private ListBoxModel
{
public:
    ExporterListBox()
    {
        items = {{"C++", false}, {"Daisy", false}, {"Plugin (DPF)", false}};
        setModel (this);
        setOutlineThickness (1);
    }
    
    int getNumRows() override
    {
        return items.size();
    }
    
    StringArray getExports() {
        StringArray exports;
        for(auto& [item, enabled] : items) {
            if(enabled) exports.add(item);
        }
        
        return exports;
    }
    
    
    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                           .withMultipliedAlpha (0.3f));
            
            auto item = items[row];
            bool enabled = item.second;
            
            auto x = getTickX();
            auto tickW = (float) height * 0.75f;
            
            getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                          enabled, true, true, false);
            
            auto textBounds = Rectangle<int>(x + 5, 0, width - x - 5, height);
            const auto textColour = findColour(ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f);
            
            AttributedString attributedString { item.first };
            attributedString.setColour (textColour);
            attributedString.setFont ((float) textBounds.getHeight() * 0.6f);
            attributedString.setJustification (Justification::centredLeft);
            attributedString.setWordWrap (AttributedString::WordWrap::none);
            
            TextLayout textLayout;
            textLayout.createLayout (attributedString,
                                     (float) textBounds.getWidth(),
                                     (float) textBounds.getHeight());
            textLayout.draw (g, textBounds.toFloat());
        }
    }
    
    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        selectRow (row);
        
        if (e.x < getTickX())
            flipEnablement (row);
    }
    
    void listBoxItemDoubleClicked (int row, const MouseEvent&) override
    {
        flipEnablement (row);
    }
    
    void returnKeyPressed (int row) override
    {
        flipEnablement (row);
    }
    
    int getBestHeight (int preferredHeight)
    {
        auto extra = getOutlineThickness() * 2;
        
        return jmax (getRowHeight() * 2 + extra,
                     jmin (getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }
    
    std::function<void(int)> onChange;
    
private:
    
    std::vector<std::pair<String, bool>> items;
    
    void flipEnablement (const int row)
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            items[row].second = !items[row].second;
            repaintRow(row);
            
            int numEnabled = 0;
            for(auto& [name, enabled] : items) numEnabled += enabled;
            
            onChange(numEnabled);
        }
    }
    
    int getTickX() const
    {
        return getRowHeight();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExporterListBox)
};

struct ToolchainInstaller : public Component, public Thread
{
    struct InstallButton : public Component
    {
        
#if JUCE_MAC
        String downloadSize = "40 MB";
#elif JUCE_WINDOWS
        String downloadSize = "20 MB";
#else
        String downloadSize = "15 MB";
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
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            
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
        
        auto* cmd = "cat /etc/os-release";
        
        int bufSize = 2048;
        auto* buff = new char[bufSize];
        
        char* ret = NULL;
        std::string str = "";
        
        int fd[2];
        int oldFd[3];
        pipe(fd);
        
        oldFd[0] = dup(STDIN_FILENO);
        oldFd[1] = dup(STDOUT_FILENO);
        oldFd[2] = dup(STDERR_FILENO);
        
        int pid = fork();
        switch(pid){
            case 0:
                close(fd[0]);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                dup2(fd[1], STDOUT_FILENO);
                dup2(fd[1], STDERR_FILENO);
                system(cmd);
                //execlp((const char*)cmd, cmd,0);
                close (fd[1]);
                exit(0);
                break;
            case -1:
                std::cerr << "getDistroID/fork() error\n" << std::endl;
                exit(1);
            default:
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                
                int rc = 1;
                while (rc > 0){
                    rc = read(fd[0], buff, bufSize);
                    str.append(buff, rc);
                    //memset(buff, 0, bufSize);
                }
                
                ret = new char [strlen((char*)str.c_str())];
                
                strcpy(ret, (char*)str.c_str());
                
                waitpid(pid, NULL, 0);
                close(fd[0]);
        }
        
        dup2(STDIN_FILENO, oldFd[0]);
        dup2(STDOUT_FILENO, oldFd[1]);
        dup2(STDERR_FILENO, oldFd[2]);
        
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
        
#if (JUCE_LINUX || JUCE_WINDOWS) && (!defined(__x86_64__) && !defined(_M_X64))
        installButton.setEnabled(false);
#endif
        
        installButton.onClick = [this](){
            
            String downloadLocation = "https://github.com/timothyschoen/HeavyDistributable/releases/download/v0.0.1/";
                        
#if JUCE_MAC
            downloadLocation += "Heavy-MacOS-Universal.zip";
#elif JUCE_WINDOWS
            downloadLocation += "Heavy-Win64.zip";
#else
            
            auto [distroName, distroBackupName, distroVersion] = getDistroID();
            
            std::cout << distroName << std::endl;
            std::cout << distroVersion << std::endl;
            
            if(distroName == "fedora" && distroVersion == "36") {
                downloadLocation += "Heavy-Fedora-36-x64.zip";
            }
            else if(distroName == "fedora" && distroVersion == "35") {
                downloadLocation += "Heavy-Fedora-35-x64.zip";
            }
            else if(distroName == "ubuntu" && distroVersion == "22.04") {
                downloadLocation += "Heavy-Ubuntu-22.04-x64.zip";
            }
            else if(distroBackupName == "ubuntu" || (distroName == "ubuntu" && distroVersion == "20.04")) {
                downloadLocation += "Heavy-Ubuntu-20.04-x64.zip";
            }
            else if(distroName == "arch" || distroBackupName == "arch") {
                downloadLocation += "Heavy-Arch-x64.zip";
            }
            else if(distroName == "debian" || distroBackupName == "debian") {
                downloadLocation += "Heavy-Debian-x64.zip";
            }
            else if(distroName == "opensuse-leap" || distroBackupName == "suse") {
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
        
#if (JUCE_LINUX || JUCE_WINDOWS) && (!defined(__x86_64__) && !defined(_M_X64))
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(32));
        g.drawText("Only x64 Windows and Linux are supported for now", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        
        g.setFont(lnf->thinFont.withHeight(23));
        g.drawText("We're working on it!", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
#else
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(32));
        g.drawText("Toolchain not found", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        
        g.setFont(lnf->thinFont.withHeight(23));
        g.drawText("Install the toolchain to get started", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
#endif
        
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
            return;
        }
        
        
#if JUCE_MAC || JUCE_LINUX
        system(("chmod +x " + toolchain.getFullPathName() + "/Heavy").toRawUTF8());
#endif
        
        installProgress = 0.0f;
        
        MessageManager::callAsync([this](){
            toolchainInstalledCallback();
        });
    }
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    float installProgress = 0.0f;
    
    
    int statusCode;
    
    InstallButton installButton;
    std::function<void()> toolchainInstalledCallback;
    
    std::unique_ptr<InputStream> instream;
};


struct ExporterPanel : public Component, public Value::Listener, public Timer, public ChildProcess, public Thread
{
    ExporterListBox exporters;
    TextButton exportButton = TextButton("Export");
    
    ComboBox patchChooser;
    
    TextEditor outputPathEditor;
    TextButton outputPathBrowseButton = TextButton("Browse");
    
    TextEditor projectNameEditor;
    TextEditor projectCopyrightEditor;
    
    File patchFile;
    File openedPatchFile;
    
    ExporterPanel(PlugDataPluginEditor* editor) : Thread ("Heavy Export Thread"){
        addAndMakeVisible(exportButton);
        addAndMakeVisible(patchChooser);
        addAndMakeVisible(outputPathEditor);
        addAndMakeVisible(outputPathBrowseButton);
        addAndMakeVisible(exporters);
        
        addAndMakeVisible(projectNameEditor);
        addAndMakeVisible(projectCopyrightEditor);
        
        outputPathBrowseButton.setConnectedEdges(12);
        
        patchChooser.addItem("Currently opened patch", 1);
        patchChooser.addItem("Other patch (browse)", 2);
        
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
            patchChooser.setSelectedId(2);
            validPatchSelected = false;
        }
        
        patchChooser.onChange = [this](){
            if(patchChooser.getSelectedId() == 1) {
                patchFile = openedPatchFile;
                validPatchSelected = true;
            }
            else {
                // Open file browser
                openChooser = std::make_unique<FileChooser>("Choose file to open", File::getSpecialLocation(File::userHomeDirectory), "*.pd", true);
                
                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser){
                    
                    auto result = fileChooser.getResult();
                    if(result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    }
                    else {
                        patchFile = "";
                        validPatchSelected = false;
                    }
                }
                                         );
            };
        };
        
        exporters.onChange = [this](int numEnabled) {
            exporterSelected = static_cast<bool>(numEnabled);
        };
        
        outputPathEditor.onTextChange = [this](){
            auto outputDir = File(outputPathEditor.getText());
            targetFolderSelected = outputDir.getParentDirectory().exists() && !outputDir.existsAsFile();
        };
        
        
        exportButton.onClick = [this, editor](){
            auto outDir = File(outputPathEditor.getText());
            outDir.createDirectory();
            startExport(patchFile.getFullPathName(), exporters.getExports(), outDir.getFullPathName(), projectNameEditor.getText(), projectCopyrightEditor.getText(), {});
        };
        
        outputPathBrowseButton.onClick = [this](){
            auto constexpr folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::warnAboutOverwriting;
            
            saveChooser = std::make_unique<FileChooser>("Export directory", File::getSpecialLocation(File::userHomeDirectory), "", true);
            
            saveChooser->launchAsync(folderChooserFlags,
                                     [this](FileChooser const& fileChooser) {
                auto const file = fileChooser.getResult();
                outputPathEditor.setText(file.getFullPathName());
            });
        };
        
    }
    
    ~ExporterPanel() {
        if(openedPatchFile.existsAsFile()) {
            openedPatchFile.deleteFile();
        }
    }
    
    
    void valueChanged(Value& v) override
    {
        bool exportReady = static_cast<bool>(exporterSelected.getValue()) && static_cast<bool>(validPatchSelected.getValue()) && static_cast<bool>(targetFolderSelected.getValue());
        exportButton.setEnabled(exportReady);
    }
    
    void resized() override
    {
        auto b = Rectangle<int>(proportionOfWidth (0.2f), 0, proportionOfWidth (0.6f), getHeight());
        
        exporters.setBounds(b.removeFromBottom(exporters.getBestHeight(100) + 100).translated(0, -60));
        exportButton.setBounds(getLocalBounds().removeFromBottom(50).withSizeKeepingCentre(80, 23));
        
        b.removeFromTop(20);
        
        patchChooser.setBounds(b.removeFromTop(23));
        
        b.removeFromTop(15);
        
        projectNameEditor.setBounds(b.removeFromTop(23).withTrimmedLeft(200));
        
        b.removeFromTop(15);
        
        projectCopyrightEditor.setBounds(b.removeFromTop(23).withTrimmedLeft(200));
        
        b.removeFromTop(15);
        
        auto outputPathBounds = b.removeFromTop(23);
        outputPathEditor.setBounds(outputPathBounds.removeFromLeft(proportionOfWidth (0.5f)));
        outputPathBrowseButton.setBounds(outputPathBounds.withTrimmedLeft(-1));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        if(exportProgress != 0.0f)
        {
            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
            
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(lnf->boldFont.withHeight(32));
            g.drawText("Exporting...", 0, getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
            
            
            float width = getWidth() - 90.0f;
            float right = jmap(exportProgress, 90.f, width);
            
            Path downloadPath;
            downloadPath.addLineSegment({ 90, 300, right, 300 }, 1.0f);
            
            Path fullPath;
            fullPath.addLineSegment({ 90, 300, width, 300 }, 1.0f);
            
            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.strokePath(fullPath, PathStrokeType(11.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
            
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        }
    }
    
    void paint(Graphics& g) override
    {
        auto b = Rectangle<int>(proportionOfWidth (0.2f), 60, 200, getHeight() - 35);
        
        g.setFont(Font(15));
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText("Project Name (optional)", b.removeFromTop(23), Justification::centredLeft);
        
        b.removeFromTop(15);
        
        g.drawText("Project Copyright (optional)", b.removeFromTop(23), Justification::centredLeft);
    }
    
private:
    
    void timerCallback() override {
        exportProgress = std::min(1.0f, exportProgress + 0.001f);
        repaint();
    }
    
    void startExport(String pdPatch, StringArray exporters, String outdir, String name, String copyright, StringArray searchPaths) {
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
        
        
        start(args);
        startThread(3);
    }
    
    void run() override {
        startTimer(10);
        waitForProcessToFinish(-1);
        
        std::cout << readAllProcessOutput() << std::endl;
        MessageManager::callAsync([this](){
            stopTimer();
            exportProgress = 0.0f;
            repaint();
        });
    }
    
    float exportProgress = 0.0f;
    
    inline static File heavyExecutable = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain").getChildFile("Heavy");
    
    
    Value exporterSelected = Value(var(false));
    Value validPatchSelected = Value(var(false));
    Value targetFolderSelected = Value(var(false));
    
    
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
};

struct HeavyExportDialog : public Component
{
    
    bool hasToolchain = false;
    
    ToolchainInstaller installer;
    ExporterPanel exporterPanel;
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    HeavyExportDialog(Dialog* dialog) : exporterPanel(dynamic_cast<PlugDataPluginEditor*>(dialog->parentComponent)) {
        hasToolchain = toolchain.exists();
        
        addChildComponent(installer);
        addChildComponent(exporterPanel);
        
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
        
    }
    
};
