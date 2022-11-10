/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct TargetPanel : public Component, public ListBoxModel
{
    
    ListBox listBox;
    
    TextButton addButton = TextButton(Icons::Add);
    
    StringArray exporters = {};
    
    TargetPanel() {
        
        addButton.setName("statusbar::add");
        
        addAndMakeVisible(addButton);
        
        addButton.onClick = [this](){
            
            auto addExporter = [this](String name){
                exporters.add(name);
                listBox.updateContent();
            };
            
            PopupMenu m;
            
            m.addItem("C++",    [addExporter](){ addExporter("C++"); });
            m.addItem("Daisy",  [addExporter](){ addExporter("Daisy"); });
            m.addItem("Unity",   [addExporter](){ addExporter("Unity"); });
            m.addItem("Plugin (hvcc/DPF)",  [addExporter](){ addExporter("Plugin (hvcc/DPF)"); });
            m.addItem("Plugin (libpd/JUCE)", [addExporter](){ addExporter("Plugin (hvcc/DPF)"); });
           
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this));
        };
        
        listBox.setModel(this);
        listBox.setRowHeight(32);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();
        listBox.getViewport()->setScrollBarsShown(true, false, false, false);
        listBox.addMouseListener(this, true);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        addAndMakeVisible(listBox);
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
        g.fillRect(getLocalBounds().removeFromRight(5));
    }
    
    int getNumRows() override
    {
        return exporters.size();
    }
    
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        g.drawText(exporters[rowNumber], 5, 5, width - 10, height - 10, Justification::centredLeft);
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        auto buttonBounds = b.removeFromBottom(23);
        
        listBox.setBounds(b);
        addButton.setBounds(buttonBounds.removeFromRight(23));
        
    }
};


struct ToolchainInstaller : public Component, public Thread
{
    struct InstallButton : public Component
    {
        String iconText = Icons::SaveAs;
        String topText = "Download Toolchain";
        String bottomText = "Download compilation utilities (10 MB)";
        
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
    
    ToolchainInstaller() : Thread("Toolchain Install Thread") {
        addAndMakeVisible(&installButton);
        
        installButton.onClick = [this](){
            instream = URL("https://github.com/timothyschoen/HeavyDistributable/releases/download/v0.0.1/Heavy-MacOS-Universal.zip").createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
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
        g.drawText("Toolchain not found", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        
        g.setFont(lnf->thinFont.withHeight(23));
        g.drawText("Install the toolchain to get started", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
        
        
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



struct HeavyExportDialog : public Component
{
    
    bool hasToolchain = false;
    
    ToolchainInstaller installer;
    TargetPanel sidepanel;
    
    inline static File toolchain = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Toolchain");
    
    HeavyExportDialog(Dialog* dialog) {
        hasToolchain = toolchain.exists();
        
        addChildComponent(installer);
        addChildComponent(sidepanel);
        
        installer.toolchainInstalledCallback = [this](){
            hasToolchain = true;
            sidepanel.setVisible(true);
            installer.setVisible(false);
            resized();
        };
        
        if(hasToolchain) {
            sidepanel.setVisible(true);
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
        
        if(hasToolchain) {
            sidepanel.setBounds(b.removeFromLeft(200));
        }
        else {
            installer.setBounds(b);
        }
    }
    
};
