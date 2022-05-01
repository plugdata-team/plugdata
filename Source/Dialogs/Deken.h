
// Struct with info about the deken package
struct PackageInfo
{
    PackageInfo(String name, String author, String timestamp, String url, String description, String version) {
        this->name = name;
        this->author = author;
        this->timestamp = timestamp;
        this->url = url;
        this->description = description;
        this->version = version;
        packageId = Base64::toBase64(name + "_" + version + "_" + timestamp + "_" + author);
    }
    
    // fast compare by ID
    friend bool operator==(const PackageInfo& lhs, const PackageInfo& rhs) {
        return lhs.packageId == rhs.packageId;
    }
    
    
    String name, author, timestamp, url, description, version, packageId;
};

// Array with package info to store the result of a search action in
using SearchResult = Array<PackageInfo>;

class Deken : public Component, public TableListBoxModel, public ScrollBar::Listener, public ThreadPool, public ValueTree::Listener
{
   public:
    Deken() : ThreadPool(1)
    {
        
        if(!filesystem.exists()) {
            filesystem.createDirectory();
        }
        
        if(pkgInfo.existsAsFile())
        {
            try {
                auto newTree = ValueTree::fromXml(pkgInfo.loadFileAsString());
                if(newTree.getType() == Identifier("pkg_info")) {
                    packageState = newTree;
                }
            }
            catch (...) {
                packageState = ValueTree( "pkg_info");
            }
        }
        
        packageState.addListener(this);

        table.setModel(this);
        table.setRowHeight(32);
        table.setOutlineThickness(0);
        table.deselectAllRows();

        table.getHeader().setStretchToFitActive(true);
        table.setHeaderHeight(0);
        table.getHeader().addColumn("Results", 1, 800, 50, 800, TableHeaderComponent::defaultFlags);

        table.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setName("sidebar::searcheditor");

        input.onTextChange = [this]()
        {
            updateResults(input.getText());
        };

        clearButton.setName("statusbar:clearsearch");
        clearButton.onClick = [this]()
        {
            input.clear();
            input.giveAwayKeyboardFocus();
            input.repaint();
        };

        clearButton.setAlwaysOnTop(true);

        addAndMakeVisible(clearButton);
        addAndMakeVisible(table);
        addAndMakeVisible(input);

        table.addMouseListener(this, true);

        input.setJustification(Justification::centredLeft);
        input.setBorder({1, 23, 3, 1});

        table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        table.setColour(TableListBox::backgroundColourId, Colours::transparentBlack);

        table.getViewport()->getVerticalScrollBar().addListener(this);

        setInterceptsMouseClicks(false, true);
    }

    void mouseDoubleClick(const MouseEvent& e) override
    {
        int row = table.getSelectedRow();

        if (isPositiveAndBelow(row, searchResult.size()))
        {
            
        }
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        PlugDataLook::paintStripes(g, 32, table.getHeight() + 32, *this, -1, table.getViewport()->getViewPositionY() + 4);

        
        if(errorMessage.isNotEmpty()) {
            g.setColour(Colours::red);
            g.drawText(errorMessage, getLocalBounds().removeFromBottom(30).withTrimmedLeft(5), Justification::centredLeft);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setFont(getLookAndFeel().getTextButtonFont(clearButton, 30));
        g.setColour(findColour(PlugDataColour::textColourId));

        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);
        
        if(input.getText().isEmpty()) {
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.setFont(Font());
            g.drawText("Type to search object names or library names", 32, 0, 350, 30, Justification::centredLeft);
        }
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 28, getWidth(), 28);
    }

    // Overloaded from TableListBoxModel
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
    }
    void paintRowBackground (Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override
    {
    }

    int getNumRows() override
    {
        return searchResult.size();
    }

    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        
        if(!isPositiveAndBelow(rowNumber, searchResult.size())) return nullptr;

        auto* newComponent = new DekenRowComponent(*this, searchResult.getReference(rowNumber));
        
        return newComponent;
    }

    void updateResults(String query)
    {
        searchResult.clear();
        
        if(query.isEmpty())  {
            table.updateContent();
            return;
        }
        
        removeAllJobs(true, 10);
        
        // Run on threadpool
        // Web requests shouldn't block the message queue!
        addJob([this, query]() mutable {
            
        // Set to name for now: there are not that many deken libraries to justify the other options
        String type = StringArray({"name", "objects", "libraries"})[0];
        
        // Create link for deken search request
        auto url = URL("https://deken.puredata.info/search?" + type + "=" + query);
        
        // Open webstream
        WebInputStream webstream(url, false);

        bool success = webstream.connect(nullptr);
        int timer = 50;
        
        // Try to connect with exponential backoff
        while(!success && timer < 2000) {
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + timer);
            timer *= 2;
            success = webstream.connect(nullptr);
        }

        // Read JSON result from search query
        auto json = webstream.readString();
        
        // Parse outer JSON layer
        var parsedJson;
        if(JSON::parse(json, parsedJson).wasOk()) {
            // Read json
            auto* object = parsedJson["result"]["libraries"].getDynamicObject();
            
            // Invalid result, update table and return
            if(!object) {
                MessageManager::callAsync([this](){
                    table.updateContent();
                });
                return;
            }
            
            // Valid result, go through the options
            for(const auto result : object->getProperties())
            {
                SearchResult results;
                String name = result.name.toString();
                
                // Loop through the different versions
                auto* versions = result.value.getDynamicObject();
                for(const auto v : versions->getProperties())
                {
                    // Loop through architectures
                    for(auto& arch : *v.value.getArray()) {
                        auto* archs = arch["archs"].getArray();
                        // Look for matching platform
                        String platform = archs->getReference(0).toString();
                        if(platform.startsWith(String(os)) && platform.contains(String(machine)));
                        {
                            // Extract info
                            String author = arch["author"];
                            String timestamp = arch["timestamp"];
                            
                            String description = arch["description"];
                            String url = arch["url"];
                            String version = arch["version"];
                            
                            // Add valid option
                            results.add({name, author, timestamp, url, description, version});
                        }
                    }
                }
                
                // Sort by alphabetically by timestamp to get latest version
                // The timestamp format is yyyy:mm::dd hh::mm::ss so this should work
                std::sort(results.begin(), results.end(), [](const auto& result1, const auto& result2) {
                    return result1.timestamp.compare(result2.timestamp) > 0;
                });
                
                // Lock message manager and add search result
                const MessageManagerLock mmLock;
                searchResult.addIfNotAlreadyThere(results.getReference(0));
            }
        }
            // Update content from message thread
            MessageManager::callAsync([this](){
                table.updateContent();
            });
        
        });
    }
    
    // Return the package state document
    ValueTree getPackageState() {
        return packageState;
    }
    
    // Returns thread used for unzipping and installing packages
    ThreadPool& getInstallPool() {
        return installThread;
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds().withTrimmedBottom(30);
        auto inputBounds = tableBounds.removeFromTop(28);

        input.setBounds(inputBounds);

        clearButton.setBounds(inputBounds.removeFromRight(30));

        tableBounds.removeFromLeft(Sidebar::dragbarWidth);
        table.setBounds(tableBounds);
    }
    

    // Show error message in statusbar
    void showError(const String& message) {
        errorMessage = message;
        repaint();
    }
    
    // When a property in our pkginfo changes, save it immediately
    void valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }
     
    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override
    {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }
     
    void valueTreeChildRemoved (ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }
    
    void uninstall(PackageInfo& packageInfo) {
        auto toRemove = packageState.getChildWithProperty("ID", packageInfo.packageId);
        if(toRemove.isValid()) {
            auto folder = File(toRemove.getProperty("Path").toString());
            folder.deleteRecursively();
            packageState.removeChild(toRemove, nullptr);
        }
    }
    
    std::unique_ptr<URL::DownloadTask> download(PackageInfo& packageInfo, URL::DownloadTask::Listener* listener) {
        URL package;
        {
            const MessageManagerLock mmLock;
            package = packageInfo.url;
        }
        
        // Make sure https is used
        if(package.getScheme() == "http") {
            package = URL(package.toString(true).replaceFirstOccurrenceOf("http://", "https://"));
        }
        
        auto destFile = Deken::filesystem.getChildFile(package.getFileName());
        
        // Download file and return unique ptr to task object
        return package.downloadToFile(destFile, URL::DownloadTaskOptions().withListener(listener));
    }
    
    void install(PackageInfo& packageInfo, File downloadLocation) {
        // Unzip on thread because we don't know how long it will take
        installThread.addJob([this, downloadLocation, packageInfo]() mutable {
            
            auto zipfile = ZipFile(downloadLocation);
            
            auto result = zipfile.uncompressTo(downloadLocation.getParentDirectory());
            downloadLocation.deleteFile();
            
            if(!result.wasOk()) {
                showError("Extraction failed");
                return;
            }
            
            String zipPath = zipfile.getEntry(0)->filename;
            MessageManager::callAsync([this, zipPath, packageInfo]() mutable {
                ValueTree pkgEntry = ValueTree(packageInfo.name);
                pkgEntry.setProperty("ID", packageInfo.packageId, nullptr);
                pkgEntry.setProperty("Author", packageInfo.author, nullptr);
                pkgEntry.setProperty("Timestamp", packageInfo.timestamp, nullptr);
                pkgEntry.setProperty("Description", packageInfo.description, nullptr);
                pkgEntry.setProperty("Version", packageInfo.version, nullptr);
                pkgEntry.setProperty("Path", Deken::filesystem.getChildFile(zipPath).getFullPathName(), nullptr);
                packageState.appendChild(pkgEntry, nullptr);
            });
        });
    }
    
    bool packageExists(const PackageInfo& info)
    {
        return packageState.getChildWithProperty("ID", info.packageId).isValid();
    }


   private:
    
    // List component to list packages
    TableListBox table;
    
    inline static File filesystem = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library").getChildFile("Deken");
    
    // Package info file
    File pkgInfo = filesystem.getChildFile(".pkg_info");
    
    // Package state tree, keeps track of which packages are installed and saves it to pkgInfo
    ValueTree packageState = ValueTree("pkg_info");
    
    // Last error message
    String errorMessage;
    
    // Current search result
    SearchResult searchResult;
    
    TextEditor input;
    TextButton clearButton = TextButton(Icons::Clear);
    
    
    // Thread for unzipping and installing packages
    ThreadPool installThread = ThreadPool(1);

    // Component representing a search result
    // It holds package info about the package it represents
    // and can
    struct DekenRowComponent : public Component, public URL::DownloadTaskListener
    {
        Deken& deken;
        PackageInfo packageInfo;
        
        TextButton installButton = TextButton(Icons::SaveAs);
        TextButton reinstallButton = TextButton(Icons::Refresh);
        TextButton uninstallButton = TextButton(Icons::Clear);
        
        std::atomic<float> installProgress = 0.0f;
        std::unique_ptr<URL::DownloadTask> taskProgress = nullptr;
        
        ValueTree packageState;
        ThreadPool& installThread;
        
        DekenRowComponent(Deken& parent, PackageInfo& info) : deken(parent), packageInfo(info), installThread(deken.getInstallPool()), packageState(deken.getPackageState())
        {
            addChildComponent(installButton);
            addChildComponent(reinstallButton);
            addChildComponent(uninstallButton);
            
            installButton.setName("statusbar:install");
            reinstallButton.setName("statusbar:reinstall");
            uninstallButton.setName("statusbar:uninstall");
            
            uninstallButton.onClick = [this](){
                deken.uninstall(packageInfo);
                setInstalled(false);
            };
            
            reinstallButton.onClick = [this](){
                deken.uninstall(packageInfo);
                taskProgress = deken.download(packageInfo, this);
            };
            
            installButton.onClick = [this](){
                taskProgress = deken.download(packageInfo, this);
            };
            
            // Check if package is already installed
            setInstalled(deken.packageExists(packageInfo));
        }
        
        // Finish installation process
        void finished (URL::DownloadTask* task, bool success) override {
            
            if(!success) {
                const MessageManagerLock mmLock;
                repaint();
                
                deken.showError("Download failed");
                return;
            }
            
            // Tell deken to install the package we downloaded
            deken.install(packageInfo, task->getTargetLocation());
            
            MessageManager::callAsync([this](){
                installProgress = 0;
                taskProgress.reset(nullptr);
                setInstalled(true);
            });
        }
        
        // Returns download progress
        void progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override {
            MessageManager::callAsync([this, bytesDownloaded, totalLength]() mutable {
                // scale to 0-1
                installProgress = static_cast<long double>(taskProgress->getLengthDownloaded()) / static_cast<long double>(taskProgress->getTotalLength());
                repaint();
            });
        }
        
        // Enables or disables buttons based on package state
        void setInstalled(bool installed) {
            installButton.setVisible(!installed);
            reinstallButton.setVisible(installed);
            uninstallButton.setVisible(installed);
            repaint();
        }
        
        void paint(Graphics& g) override
        {
            g.setColour(findColour(ComboBox::textColourId));

            g.setFont(Font());
            g.drawFittedText(packageInfo.name, 5, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
            
            // draw progressbar
            if(installProgress) {
                
                Path downloadPath;
                
                float right = jmap(installProgress.load(), 230.f, 500.f);
                downloadPath.addLineSegment({230, 15, right, 15}, 8.0f);
                
                g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
                g.strokePath(downloadPath, PathStrokeType(9.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
                
                g.setColour(findColour(PlugDataColour::highlightColourId));
                g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
            }
            else {
                g.drawFittedText(packageInfo.version, 90, 0, 150, getHeight(), Justification::centredLeft, 1, 0.8f);
                g.drawFittedText(packageInfo.author, 250, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
                g.drawFittedText(packageInfo.timestamp, 440, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
                
                
            }
        }
        
        void resized() override
        {
            installButton.setBounds(getWidth() - 40, 1, 26, 30);
            uninstallButton.setBounds(getWidth() - 40, 1, 26, 30);
            reinstallButton.setBounds(getWidth() - 70, 1, 26, 30);
        }
        
        
    };
    
    
    String os =
#if defined __linux__
        "Linux"
#elif defined __APPLE__
        "Darwin"
#elif defined __FreeBSD__
        "FreeBSD"
#elif defined __NetBSD__
        "NetBSD"
#elif defined __OpenBSD__
        "OpenBSD"
#elif defined _WIN32
        "Windows"
#else
# if defined(__GNUC__)
#  warning unknown OS
# endif
        0
#endif
        ;
    
    String machine =
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
        "amd64"
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
        "i386"
#elif defined(__ppc__)
        "ppc"
#elif defined(__aarch64__)
        "arm64"
#elif defined (__ARM_ARCH)
        "armv" stringify(__ARM_ARCH)
# if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#  if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        "b"
#  endif
# endif
#else
# if defined(__GNUC__)
#  warning unknown architecture
# endif
        0
#endif
        ;

};
