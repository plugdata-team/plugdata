#pragma once


struct Spinner : public Component, public Timer
{
    int numSpinning = 0;
    
    void startSpinning() {
        numSpinning++;
        setVisible(true);
        startTimer(20);
    }
    
    void stopSpinning() {
        numSpinning--;
        if(!numSpinning) {
            setVisible(false);
            stopTimer();
        }
    }
    
    void timerCallback() override {
        repaint();
    }
    
    void paint(Graphics& g) override {
        getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::textColourId), 3, 3, getWidth() - 6, getHeight() - 6);
    }
};

// Struct with info about the deken package
struct PackageInfo
{
    PackageInfo(String name, String author, String timestamp, String url, String description, String version)
    {
        this->name = name;
        this->author = author;
        this->timestamp = timestamp;
        this->url = url;
        this->description = description;
        this->version = version;
        packageId = Base64::toBase64(name + "_" + version + "_" + timestamp + "_" + author);
    }

    // fast compare by ID
    friend bool operator==(const PackageInfo& lhs, const PackageInfo& rhs)
    {
        return lhs.packageId == rhs.packageId;
    }

    String name, author, timestamp, url, description, version, packageId;
};

// Array with package info to store the result of a search action in
using SearchResult = Array<PackageInfo>;

class Deken : public Component, public ListBoxModel, public ScrollBar::Listener, public ThreadPool, public ValueTree::Listener, public Timer
{
    struct DownloadTask : public URL::DownloadTaskListener
    {
        Deken& deken;
        PackageInfo packageInfo;
        File destination;

        std::unique_ptr<URL::DownloadTask> task;

        DownloadTask(Deken& d, PackageInfo& info, File destFile)
            : deken(d),
              packageInfo(info),
              task(URL(info.url).downloadToFile(destFile, URL::DownloadTaskOptions().withListener(this))){

              };

        // Finish installation process
        void finished(URL::DownloadTask* task, bool success) override
        {
            auto error = [this](String message)
            {
                MessageManager::callAsync(
                    [this, message]() mutable
                    {
                        deken.repaint();

                        deken.showError(message);
                        onFinish(false);

                        auto& d = deken;
                        d.downloads.removeObject(this);
                        d.updateResults(d.input.getText());
                        d.listBox.updateContent();
                    });
            };

            if (!success)
            {
                error("Download failed");
                return;
            }

            auto downloadLocation = task->getTargetLocation();
            // Install
            auto zipfile = ZipFile(downloadLocation);

            auto result = zipfile.uncompressTo(downloadLocation.getParentDirectory());
            downloadLocation.deleteFile();

            if (!result.wasOk())
            {
                error("Extraction failed: " + result.getErrorMessage());
                return;
            }

            String extractedPath = filesystem.getChildFile(packageInfo.name).getFullPathName();

            // Tell deken about the newly installed package
            deken.addPackageToRegister(packageInfo, extractedPath);

            MessageManager::callAsync(
                [this]() mutable
                {
                    onFinish(true);

                    // deken is relative to this, which gets deleted
                    auto& d = deken;
                    d.downloads.removeObject(this);
                    d.updateResults(d.input.getText());
                    d.listBox.updateContent();
                });
        }

        void progress(URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override
        {
            float progress = static_cast<long double>(task->getLengthDownloaded()) / static_cast<long double>(task->getTotalLength());

            MessageManager::callAsync([this, progress]() mutable { onProgress(progress); });
        }

        std::function<void(float)> onProgress;
        std::function<void(bool)> onFinish;
    };

   public:
    Deken() : ThreadPool(1)
    {
        if (!filesystem.exists())
        {
            filesystem.createDirectory();
        }

        if (pkgInfo.existsAsFile())
        {
            try
            {
                auto newTree = ValueTree::fromXml(pkgInfo.loadFileAsString());
                if (newTree.getType() == Identifier("pkg_info"))
                {
                    packageState = newTree;
                }
            }
            catch (...)
            {
                packageState = ValueTree("pkg_info");
            }
        }

        packageState.addListener(this);

        listBox.setModel(this);
        listBox.setRowHeight(32);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setName("sidebar::searcheditor");

        input.onTextChange = [this]() { startTimer(250); };

        clearButton.setName("statusbar:clearsearch");
        clearButton.onClick = [this]()
        {
            input.clear();
            input.giveAwayKeyboardFocus();
            input.repaint();
            updateResults("");
        };

        clearButton.setAlwaysOnTop(true);

        addAndMakeVisible(clearButton);
        addAndMakeVisible(listBox);
        addAndMakeVisible(input);

        listBox.addMouseListener(this, true);

        input.setJustification(Justification::centredLeft);
        input.setBorder({1, 23, 3, 1});

        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

        listBox.getViewport()->getVerticalScrollBar().addListener(this);

        setInterceptsMouseClicks(false, true);
        
        addAndMakeVisible(searchSpinner);
        searchSpinner.setAlwaysOnTop(true);

        updateResults("");
    }

    // Group fast typing together
    void timerCallback() override
    {
        updateResults(input.getText());
        stopTimer();
    }

    // Check if busy when deleting settings component
    bool isBusy()
    {
        if (downloads.size()) return true;

        return getNumJobs();
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        PlugDataLook::paintStripes(g, 32, listBox.getHeight() + 24, *this, -1, listBox.getViewport()->getViewPositionY() + 4);

        if (errorMessage.isNotEmpty())
        {
            g.setColour(Colours::red);
            g.drawText(errorMessage, getLocalBounds().removeFromBottom(28).withTrimmedLeft(8).translated(0, 2), Justification::centredLeft);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setFont(getLookAndFeel().getTextButtonFont(clearButton, 30));
        g.setColour(findColour(PlugDataColour::textColourId));

        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);

        if (input.getText().isEmpty())
        {
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.setFont(Font());
            g.drawText("Type to search for objects or libraries", 32, 0, 350, 30, Justification::centredLeft);
        }

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 28, getWidth(), 28);
    }

    int getNumRows() override
    {
        return searchResult.size() + downloads.size();
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
    }

    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;

        if (isPositiveAndBelow(rowNumber, downloads.size()))
        {
            return new DekenRowComponent(*this, downloads[rowNumber]->packageInfo);
        }

        else if (isPositiveAndBelow(rowNumber - downloads.size(), searchResult.size()))
        {
            return new DekenRowComponent(*this, searchResult.getReference(rowNumber - downloads.size()));
        }

        return nullptr;
    }

    void updateResults(String query)
    {
        // Run on threadpool
        // Web requests shouldn't block the message queue!
        searchSpinner.startSpinning();
        addJob(
            [this, query]() mutable
            {
                SearchResult newResult;

                // Add as job to ensure synchronous order
                if (query.isEmpty())
                {
                    for (auto child : packageState)
                    {
                        auto name = child.getType().toString();
                        auto description = child.getProperty("Description").toString();
                        auto timestamp = child.getProperty("Timestamp").toString();
                        auto url = child.getProperty("URL").toString();
                        auto version = child.getProperty("Version").toString();
                        auto author = child.getProperty("Author").toString();

                        bool exists = false;
                        for (auto* download : downloads)
                        {
                            if (download->packageInfo == PackageInfo(name, author, timestamp, url, description, version))
                            {
                                exists = true;
                                break;
                            }
                        }

                        auto info = PackageInfo(name, author, timestamp, url, description, version);

                        if (!getDownloadForPackage(info))
                        {
                            newResult.addIfNotAlreadyThere(info);
                        }
                    }

                    MessageManager::callAsync(
                        [this, newResult]() mutable
                        {
                            searchResult = newResult;
                            listBox.updateContent();
                            searchSpinner.stopSpinning();
                        });
                    return;
                }

                // Set to name for now: there are not that many deken libraries to justify the other options
                String type = StringArray({"name", "objects", "libraries"})[0];

                // Create link for deken search request
                auto url = URL("https://deken.puredata.info/search?" + type + "=" + query);

                // Open webstream
                WebInputStream webstream(url, false);

                bool success = webstream.connect(nullptr);
                int timer = 50;

                // Try to connect with exponential backoff
                while (!success && timer < 2000)
                {
                    Time::waitForMillisecondCounter(Time::getMillisecondCounter() + timer);
                    timer *= 2;
                    success = webstream.connect(nullptr);
                }
                
                if(!success) {
                    MessageManager::callAsync(
                        [this]()
                        {
                            searchResult.clear();
                            listBox.updateContent();
                            searchSpinner.stopSpinning();
                            showError("Failed to connect to server");
                        });
                }
                

                // Read JSON result from search query
                auto json = webstream.readString();

                // Parse outer JSON layer
                var parsedJson;

#undef JUCE_DEBUG
                // Disable assertions:
                // Sometimes the server passes an invalid result causing an assertion
                // But the invalid result is no problem!
                if (JSON::parse(json, parsedJson).wasOk())
                {
#define JUCE_DEBUG 1
                    // Read json
                    auto* object = parsedJson["result"]["libraries"].getDynamicObject();

                    // Invalid result, update table and return
                    if (!object)
                    {
                        MessageManager::callAsync(
                            [this]()
                            {
                                searchResult.clear();
                                listBox.updateContent();
                                searchSpinner.stopSpinning();
                            });

                        return;
                    }

                    // Valid result, go through the options
                    for (const auto result : object->getProperties())
                    {
                        SearchResult results;
                        String name = result.name.toString();
                        


                        // Loop through the different versions
                        auto* versions = result.value.getDynamicObject();
                        for (const auto v : versions->getProperties())
                        {
                            // Loop through architectures
                            for (auto& arch : *v.value.getArray())
                            {

                                
                                
                                auto* archs = arch["archs"].getArray();
                                // Look for matching platform
                                String platform = archs->getReference(0).toString();
                                if (checkArchitecture(platform))
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

                        if (!results.isEmpty())
                        {
                            // Sort by alphabetically by timestamp to get latest version
                            // The timestamp format is yyyy:mm::dd hh::mm::ss so this should work
                            std::sort(results.begin(), results.end(), [](const auto& result1, const auto& result2) { return result1.timestamp.compare(result2.timestamp) > 0; });

                            auto info = results.getReference(0);

                            // check if already being downloaded
                            if (!getDownloadForPackage(info))
                            {
                                newResult.addIfNotAlreadyThere(info);
                            }
                        }
                    }
                }
                // Update content from message thread
                MessageManager::callAsync(
                    [this, newResult]() mutable
                    {
                        searchResult = newResult;
                        listBox.updateContent();
                        searchSpinner.stopSpinning();
                    });
            });
    }

    // Return the package state document
    ValueTree getPackageState()
    {
        return packageState;
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds().withTrimmedBottom(30);
        auto inputBounds = tableBounds.removeFromTop(28);

        input.setBounds(inputBounds);

        clearButton.setBounds(inputBounds.removeFromRight(30));
        searchSpinner.setBounds(inputBounds.removeFromRight(30));

        tableBounds.removeFromLeft(Sidebar::dragbarWidth);
        listBox.setBounds(tableBounds);
    }

    // Show error message in statusbar
    void showError(const String& message)
    {
        errorMessage = message;
        repaint();
    }

    // When a property in our pkginfo changes, save it immediately
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override
    {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override
    {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }

    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        pkgInfo.replaceWithText(packageState.toXmlString());
    }

    void uninstall(PackageInfo& packageInfo)
    {
        auto toRemove = packageState.getChildWithProperty("ID", packageInfo.packageId);
        if (toRemove.isValid())
        {
            auto folder = File(toRemove.getProperty("Path").toString());
            folder.deleteRecursively();
            packageState.removeChild(toRemove, nullptr);
        }
    }

    DownloadTask* install(PackageInfo packageInfo)
    {
        // Make sure https is used
        packageInfo.url = packageInfo.url.replaceFirstOccurrenceOf("http://", "https://");
        auto filename = packageInfo.url.fromLastOccurrenceOf("/", false, false);
        auto destFile = Deken::filesystem.getChildFile(filename);

        // Download file and return unique ptr to task object
        return downloads.add(new DownloadTask(*this, packageInfo, destFile));
    }

    void addPackageToRegister(const PackageInfo& info, String path)
    {
        ValueTree pkgEntry = ValueTree(info.name);
        pkgEntry.setProperty("ID", info.packageId, nullptr);
        pkgEntry.setProperty("Author", info.author, nullptr);
        pkgEntry.setProperty("Timestamp", info.timestamp, nullptr);
        pkgEntry.setProperty("Description", info.description, nullptr);
        pkgEntry.setProperty("Version", info.version, nullptr);
        pkgEntry.setProperty("Path", path, nullptr);
        pkgEntry.setProperty("URL", info.url, nullptr);

        // Prevent duplicate entries
        if (packageState.getChildWithProperty("ID", info.packageId).isValid())
        {
            packageState.removeChild(packageState.getChildWithProperty("ID", info.packageId), nullptr);
        }
        packageState.appendChild(pkgEntry, nullptr);
    }

    bool packageExists(const PackageInfo& info)
    {
        return packageState.getChildWithProperty("ID", info.packageId).isValid();
    }

    // Checks if the current package is already being downloaded
    DownloadTask* getDownloadForPackage(PackageInfo& info)
    {
        for (auto* download : downloads)
        {
            if (download->packageInfo == info)
            {
                return download;
            }
        }

        return nullptr;
    }

   private:
    // List component to list packages
    ListBox listBox;

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
    
    Spinner searchSpinner;

    // Thread for unzipping and installing packages
    OwnedArray<DownloadTask> downloads;

    // Component representing a search result
    // It holds package info about the package it represents
    // and can
    struct DekenRowComponent : public Component
    {
        Deken& deken;
        PackageInfo packageInfo;

        TextButton installButton = TextButton(Icons::SaveAs);
        TextButton reinstallButton = TextButton(Icons::Refresh);
        TextButton uninstallButton = TextButton(Icons::Clear);

        float installProgress;
        ValueTree packageState;

        DekenRowComponent(Deken& parent, PackageInfo& info) : deken(parent), packageInfo(info), packageState(deken.getPackageState())
        {
            addChildComponent(installButton);
            addChildComponent(reinstallButton);
            addChildComponent(uninstallButton);

            installButton.setName("statusbar:install");
            reinstallButton.setName("statusbar:reinstall");
            uninstallButton.setName("statusbar:uninstall");

            uninstallButton.onClick = [this]()
            {
                setInstalled(false);
                deken.uninstall(packageInfo);
                deken.updateResults(deken.input.getText());
            };

            reinstallButton.onClick = [this]()
            {
                auto* downloadTask = deken.install(packageInfo);
                attachToDownload(downloadTask);
            };

            installButton.onClick = [this]()
            {
                auto* downloadTask = deken.install(packageInfo);
                attachToDownload(downloadTask);
            };

            // Check if package is already installed
            setInstalled(deken.packageExists(packageInfo));

            // Check if already in progress
            if (auto* task = deken.getDownloadForPackage(packageInfo))
            {
                attachToDownload(task);
            }
        }

        void attachToDownload(DownloadTask* task)
        {
            task->onProgress = [this](float progress)
            {
                installProgress = progress;
                repaint();
            };
            task->onFinish = [this](bool result)
            {
                setInstalled(result);
                deken.listBox.updateContent();
            };

            installButton.setVisible(false);
            reinstallButton.setVisible(false);
            uninstallButton.setVisible(false);
        }

        // Enables or disables buttons based on package state
        void setInstalled(bool installed)
        {
            installButton.setVisible(!installed);
            reinstallButton.setVisible(installed);
            uninstallButton.setVisible(installed);
            installProgress = 0.0f;

            repaint();
        }

        void paint(Graphics& g) override
        {
            g.setColour(findColour(ComboBox::textColourId));

            g.setFont(Font());
            g.drawFittedText(packageInfo.name, 5, 0, 200, getHeight(), Justification::centredLeft, 1, 0.8f);

            // draw progressbar
            if (deken.getDownloadForPackage(packageInfo))
            {
                float width = getWidth() - 90.0f;
                float right = jmap(installProgress, 90.f, width);

                Path downloadPath;
                downloadPath.addLineSegment({90, 15, right, 15}, 1.0f);

                Path fullPath;
                fullPath.addLineSegment({90, 15, width, 15}, 1.0f);

                g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
                g.strokePath(fullPath, PathStrokeType(11.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

                g.setColour(findColour(PlugDataColour::highlightColourId));
                g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
            }
            else
            {
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
    
    bool checkArchitecture(String platform)
    {
        // Check OS
        if(platform.upToFirstOccurrenceOf("-", false, false) != os) return false;
        platform = platform.fromFirstOccurrenceOf("-", false, false);
        
        // Check floatsize
        if(platform.fromLastOccurrenceOf("-", false, false) != floatsize) return false;
        platform = platform.upToLastOccurrenceOf("-", false, false);

        if(machine.contains(platform)) return true;
        
        return false;
    }
        
    String floatsize = String(PD_FLOATSIZE);
    String os =
#if JUCE_LINUX
        "Linux"
#elif JUCE_MAC
        "Darwin"
#elif JUCE_WINDOWS
        "Windows"
// PlugData has no official BSD support and testing, but for completeness:
#elif defined __FreeBSD__
        "FreeBSD"
#elif defined __NetBSD__
        "NetBSD"
#elif defined __OpenBSD__
        "OpenBSD"
#else
#if defined(__GNUC__)
#warning unknown OS
#endif
        0
#endif
        ;

    StringArray machine =
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
    {"amd64", "x86_64"}
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
    {"i386", "i686", "i586"}
#elif defined(__ppc__)
    {"ppc", "PowerPC"}
#elif defined(__aarch64__)
    {"arm64"}
#elif __ARM_ARCH == 6 || defined(__ARM_ARCH_6__)
    {"armv6", "armv6l", "arm"}
        "armv" stringify(__ARM_ARCH)
#elif __ARM_ARCH == 7 || defined(__ARM_ARCH_7__)
    {"armv7l", "armv7", "armv6l", "armv6", "arm"}
#else
#if defined(__GNUC__)
#warning unknown architecture
#endif
    {}
#endif
        ;
};
