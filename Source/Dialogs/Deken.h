using SearchResult = std::tuple<String, String, String, String, String, String>;
using SearchResults = Array<SearchResult>;

class Deken : public Component, public TableListBoxModel, public ScrollBar::Listener, public ThreadPool, public ValueTree::Listener
{
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
    
    struct DekenRowComponent : public Component, public URL::DownloadTaskListener
    {
        int idx;
        SearchResult searchResult;
        
        TextButton installButton = TextButton(Icons::SaveAs);
        TextButton reinstallButton = TextButton(Icons::Refresh);
        TextButton uninstallButton = TextButton(Icons::Clear);
        
        Deken& deken;
        
        float installProgress = 0.0f;
        std::unique_ptr<URL::DownloadTask> taskProgress = nullptr;
        
        ValueTree pkgInfo;
        
        bool selected;
        
        ThreadPool& installThread;
        
        DekenRowComponent(int index, SearchResult& searchResult, bool isSelected, ThreadPool& installThreadPool, Deken& parent) : searchResult(searchResult), idx(index), selected(isSelected), installThread(installThreadPool), pkgInfo(parent.getPackageState()), deken(parent)
        {
            addChildComponent(installButton);
            addChildComponent(reinstallButton);
            addChildComponent(uninstallButton);
            
            installButton.setName("statusbar:install");
            reinstallButton.setName("statusbar:reinstall");
            uninstallButton.setName("statusbar:uninstall");
            
            uninstallButton.onClick = [this](){
                uninstall();
            };
            
            reinstallButton.onClick = [this](){
                uninstall();
                install();
            };
            
            installButton.onClick = [this](){
                install();
            };
            
            auto info = pkgInfo.getChildWithProperty("ID", getPackageId());
            setInstalled(info.isValid());
        }
        
        String getPackageId() {
            auto& [name, author, timestamp, url, description, version] = searchResult;
            return Base64::toBase64(name + "_" + version + "_" + timestamp + "_" + author);
        }
        
        void uninstall() {
            auto toRemove = pkgInfo.getChildWithProperty("ID", getPackageId());
            if(toRemove.isValid()) {
                auto folder = File(toRemove.getProperty("Path").toString());
                folder.deleteRecursively();
                pkgInfo.removeChild(toRemove, nullptr);
            }
            setInstalled(false);
        }
        
        void install() {
            URL package;
            {
                const MessageManagerLock mmLock;
                auto& [name, author, timestamp, url, description, version] = searchResult;
                package = url;
            }
            
            // Make sure https is used
            if(package.getScheme() == "http") {
                package = URL(package.toString(true).replaceFirstOccurrenceOf("http://", "https://"));
            }
            
            auto destFile = Deken::filesystem.getChildFile(package.getFileName());
            taskProgress = package.downloadToFile(destFile, URL::DownloadTaskOptions().withListener(this));
        }
   
        
        // Finish installation process
        void finished (URL::DownloadTask* task, bool success) override {
            
            if(!success) {
                const MessageManagerLock mmLock;
                repaint();
                
                deken.showError("Download failed");
                return;
            }
            
            // Unzip on thread because we don't know how long it will take
            installThread.addJob([this, task]() mutable {
                
                auto file = task->getTargetLocation();
                auto zipfile = ZipFile(file);
                
                auto result = zipfile.uncompressTo(file.getParentDirectory());
                file.deleteFile();
                
                if(!result.wasOk()) {
                    deken.showError("Extraction failed");
                    return;
                }
                
                String zipPath = zipfile.getEntry(0)->filename;
                MessageManager::callAsync([this, zipPath]() mutable {
                    installProgress = 0;
                    taskProgress.reset(nullptr);
                    
                    auto& [name, author, timestamp, url, description, version] = searchResult;
                    ValueTree pkgEntry = ValueTree(name);
                    pkgEntry.setProperty("ID", getPackageId(), nullptr);
                    pkgEntry.setProperty("Author", author, nullptr);
                    pkgEntry.setProperty("Timestamp", timestamp, nullptr);
                    pkgEntry.setProperty("Description", description, nullptr);
                    pkgEntry.setProperty("Version", version, nullptr);
                    pkgEntry.setProperty("Path", Deken::filesystem.getChildFile(zipPath).getFullPathName(), nullptr);
                    pkgInfo.appendChild(pkgEntry, nullptr);
                    
                    setInstalled(true);
                    repaint();
                });
            });
        }
        
        void progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override {
            MessageManager::callAsync([this, bytesDownloaded, totalLength]() mutable {
                installProgress = static_cast<long double>(taskProgress->getLengthDownloaded()) / static_cast<long double>(taskProgress->getTotalLength());
                repaint();
            });
        }
        
        void setInstalled(bool installed) {
            installButton.setVisible(!installed);
            reinstallButton.setVisible(installed);
            uninstallButton.setVisible(installed);
            
            if(installed){

            }
            else {
                auto toRemove = pkgInfo.getChildWithProperty("ID", getPackageId());\
                
                // Delete files
                auto path = File(toRemove.getProperty("Path").toString());
                path.deleteRecursively();
                
                // Remove from pkginfo
                pkgInfo.removeChild(toRemove, nullptr);
            }
        }
        
        void paint(Graphics& g) override
        {
            if (selected)
            {
                g.setColour(findColour(PlugDataColour::highlightColourId));
                g.fillRect(1, 0, getWidth() - 3, getHeight());
            }
            
            g.setColour(selected ? Colours::white : findColour(ComboBox::textColourId));
            
            auto& [name, author, timestamp, url, description, version] = searchResult;

            g.setFont(Font());
            g.drawFittedText(name, 5, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
            
            if(installProgress) {
                
                Path downloadPath;
                
                float right = jmap(installProgress, 230.f, 500.f);
                
                g.setColour(findColour(PlugDataColour::highlightColourId));
                downloadPath.addLineSegment({230, 15, right, 15}, 8.0f);
                
               
                g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
            }
            else {
                g.drawFittedText(version, 90, 0, 150, getHeight(), Justification::centredLeft, 1, 0.8f);
                g.drawFittedText(author, 250, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
                g.drawFittedText(timestamp, 440, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);
                
                
            }
        }
        
        void resized() override
        {
            installButton.setBounds(getWidth() - 40, 1, 26, 30);
            uninstallButton.setBounds(getWidth() - 40, 1, 26, 30);
            reinstallButton.setBounds(getWidth() - 70, 1, 26, 30);
        }
        
        
    };
    
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

        auto* newComponent = new DekenRowComponent(rowNumber, searchResult.getReference(rowNumber), isRowSelected, installThread, *this);
        
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
        addJob([this, query]() mutable {
            
        String type = StringArray({"name", "objects", "libraries"})[searchType];
        
        String url = "https://deken.puredata.info/search?" + type + "=" + query;
        WebInputStream webstream(URL(url), false);

        bool success = webstream.connect(nullptr);
        int timer = 50;
        
        // Try to connect with exponential backoff
        while(!success && timer < 2000) {
            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + timer);
            timer *= 2;
            success = webstream.connect(nullptr);
        }

        auto json = webstream.readString();
        
        var parsedJson;
        if(JSON::parse(json, parsedJson).wasOk()) {
            auto* object = parsedJson["result"]["libraries"].getDynamicObject();
            if(!object) {
                MessageManager::callAsync([this](){
                    table.updateContent();
                });
                return;
            }
            
            for(const auto result : object->getProperties())
            {
                SearchResults results;
                String name = result.name.toString();
                auto* versions = result.value.getDynamicObject();
                for(const auto v : versions->getProperties())
                {
                    for(auto& arch : *v.value.getArray()) {
                        String author = arch["author"];
                        String timestamp = arch["timestamp"];
                        auto* archs = arch["archs"].getArray();
                        String description = arch["description"];
                        String url = arch["url"];
                        String version = arch["version"];
                        
                        String platform = archs->getReference(0).toString();
                        if(platform.startsWith(String(os)) && platform.contains(String(machine)));
                        {
                            results.add({name, author, timestamp, url, description, version});
                        }
                    }
                }
                
                // Sort by timestamp to get latest version
                std::sort(results.begin(), results.end(), [](const auto& result1, const auto& result2){
                    auto& timestamp1 = std::get<2>(result1);
                    auto& timestamp2 = std::get<2>(result2);
                    
                    return timestamp1.compare(timestamp2) > 0;
                    
                });
                
                const MessageManagerLock mmLock;
                searchResult.addIfNotAlreadyThere(results.getReference(0));
            }
        }
            MessageManager::callAsync([this](){
                table.updateContent();
            });
        
        });
    }
    
    ValueTree getPackageState() {
        return packageState;
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
    

    void showError(const String& message) {
        errorMessage = message;
        repaint();
    }
    
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

   private:
    TableListBox table;

    enum SearchType
    {
        Both,
        Library,
        Object
    };
    
    inline static File filesystem = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library").getChildFile("Deken");
    
    
    File pkgInfo = filesystem.getChildFile(".pkg_info");
    ValueTree packageState = ValueTree("pkg_info");
    
    String errorMessage;
    SearchType searchType = Both;
    SearchResults searchResult;
    
    TextEditor input;
    TextButton clearButton = TextButton(Icons::Clear);
    
    ThreadPool installThread = ThreadPool(1);
    
};
