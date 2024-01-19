/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

struct Spinner : public Component
    , public Timer {

    void startSpinning()
    {
        setVisible(true);
        startTimer(20);
    }

    void stopSpinning()
    {
        setVisible(false);
        stopTimer();
    }

    void timerCallback() override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), 3, 3, getWidth() - 6, getHeight() - 6);
    }
};

// Struct with info about the deken package
struct PackageInfo {
    PackageInfo(String name, String author, String timestamp, String url, String description, String version, StringArray objects)
    {
        this->name = name;
        this->author = author;
        this->timestamp = timestamp;
        this->url = url;
        this->description = description;
        this->version = version;
        this->objects = objects;
        packageId = Base64::toBase64(name + "_" + version + "_" + timestamp + "_" + author);
    }

    // fast compare by ID
    friend bool operator==(PackageInfo const& lhs, PackageInfo const& rhs)
    {
        return lhs.packageId == rhs.packageId;
    }

    String name, author, timestamp, url, description, version, packageId;
    StringArray objects;
};

// Array with package info to store the result of a search action in
using PackageList = Array<PackageInfo>;

struct PackageSorter {
    static void sort(ValueTree& packageState)
    {
        PackageSorter sorter;
        packageState.sort(sorter, nullptr, true);
    }

    static int compareElements(ValueTree const& first, ValueTree const& second)
    {
        return first.getType().toString().compare(second.getType().toString());
    }
};

class PackageManager : public Thread
    , public ActionBroadcaster
    , public ValueTree::Listener
    , public DeletedAtShutdown {

public:
    struct DownloadTask : public Thread {
        PackageManager& manager;
        PackageInfo packageInfo;

        std::unique_ptr<InputStream> instream;

        DownloadTask(PackageManager& m, PackageInfo& info)
            : Thread("Download Thread")
            , manager(m)
            , packageInfo(info)
        {
            int statusCode = 0;
            instream = URL(info.url).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                                                           .withConnectionTimeoutMs(10000)
                                                           .withStatusCode(&statusCode));

            if (instream != nullptr && statusCode == 200) {
                startThread();
            } else {
                finish(Result::fail("Failed to start download"));
                return;
            }
        }

        ~DownloadTask() override
        {
            stopThread(-1);
        }

        void run() override
        {
            MemoryBlock dekData;

            int64 totalBytes = instream->getTotalLength();
            int64 bytesDownloaded = 0;

            MemoryOutputStream mo(dekData, true);

            while (true) {
                if (threadShouldExit()) {
                    finish(Result::fail("Download cancelled"));
                    return;
                }

                auto written = mo.writeFromInputStream(*instream, 8192);

                if (written == 0)
                    break;

                bytesDownloaded += written;

                float progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);

                MessageManager::callAsync([this, progress]() mutable {
                    onProgress(progress);
                });
            }

            MemoryInputStream input(dekData, false);
            ZipFile zip(input);

            /* This check produces false positives sometimes, so I've disabled it
             if (zip.getNumEntries() == 0) {
             finish(Result::fail("The downloaded file was not a valid Deken package"));
             return;
             } */

            auto extractedPath = filesystem.getChildFile(packageInfo.name).getFullPathName();
            auto result = zip.uncompressTo(filesystem);

            if (!result.wasOk()) {
                finish(result);
                return;
            }

            // Tell deken about the newly installed package
            manager.addPackageToRegister(packageInfo, extractedPath);

            finish(Result::ok());
        }

        void finish(Result result)
        {
            MessageManager::callAsync(
                [this, result, finishCopy = onFinish]() mutable {
                    waitForThreadToExit(-1);

                    // Self-destruct
                    manager.downloads.removeObject(this);

                    finishCopy(result);
                });
        }

        std::function<void(float)> onProgress;
        std::function<void(Result)> onFinish;
    };

    PackageManager()
        : Thread("Deken thread")
    {
        if (!filesystem.exists()) {
            filesystem.createDirectory();
        }

        if (pkgInfo.existsAsFile()) {
            auto newTree = ValueTree::fromXml(pkgInfo.loadFileAsString());
            if (newTree.isValid() && newTree.getType() == Identifier("pkg_info")) {
                packageState = newTree;
            }
        }

        packageState.addListener(this);
    }

    ~PackageManager()
    {
        if (webstream)
            webstream->cancel();
        downloads.clear();
        stopThread(500);
        clearSingletonInstance();
    }

    void update()
    {
        sendActionMessage("");
        startThread();
    }

    void run() override
    {
        // Continue on pipe errors
#ifndef _MSC_VER
        signal(SIGPIPE, SIG_IGN);
#endif
        allPackages = getAvailablePackages();
        sendActionMessage("");
    }

    PackageList getAvailablePackages()
    {

        // plugdata's deken servers, hosted on GitHub
        // This will pre-parse the deken repo information to a faster and smaller format
        // This saves a lot of work that plugdata would have to do on startup!

        auto triplet = os + "-" + machine + "-" + floatsize;
        auto repoForArchitecture = "https://raw.githubusercontent.com/plugdata-team/plugdata-deken/main/bin/" + triplet + ".bin";

        webstream = std::make_unique<WebInputStream>(URL(repoForArchitecture), false);
        webstream->connect(nullptr);

        if (webstream->isError()) {
            sendActionMessage("Failed to connect to server");
            return {};
        }

        MemoryBlock block;
        webstream->readIntoMemoryBlock(block);

        // Parse tree that was downloaded
        auto tree = ValueTree::readFromData(block.getData(), block.getSize());

        PackageList packages;

        for (auto package : tree) {
            auto name = package.getProperty("Name").toString();

            for (auto version : package) {
                auto author = version.getProperty("Author").toString();
                auto timestamp = version.getProperty("Timestamp").toString();
                auto url = version.getProperty("URL").toString();
                auto description = version.getProperty("Description").toString();
                auto versionNumber = version.getProperty("Version").toString();

                StringArray objects;
                for (auto object : version.getChildWithName("Objects")) {
                    objects.add(object.getProperty("Name").toString());
                }

                packages.add(PackageInfo(name, author, timestamp, url, description, versionNumber, objects));
                break;
            }
        }

        return packages;
    }

    // When a property in our pkginfo changes, save it immediately
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
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
        if (toRemove.isValid()) {
            auto folder = File(toRemove.getProperty("Path").toString());
            folder.deleteRecursively();
            packageState.removeChild(toRemove, nullptr);
        }
    }

    DownloadTask* install(PackageInfo packageInfo)
    {
        // Make sure https is used
        packageInfo.url = packageInfo.url.replaceFirstOccurrenceOf("http://", "https://");
        return downloads.add(new DownloadTask(*this, packageInfo));
    }

    void addPackageToRegister(PackageInfo const& info, String path)
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
        if (packageState.getChildWithProperty("ID", info.packageId).isValid()) {
            packageState.removeChild(packageState.getChildWithProperty("ID", info.packageId), nullptr);
        }
        packageState.appendChild(pkgEntry, nullptr);
    }

    bool packageExists(PackageInfo const& info)
    {
        return packageState.getChildWithProperty("ID", info.packageId).isValid();
    }

    // Checks if the current package is already being downloaded
    DownloadTask* getDownloadForPackage(PackageInfo& info)
    {
        for (auto* download : downloads) {
            if (download->packageInfo == info) {
                return download;
            }
        }

        return nullptr;
    }

    PackageList allPackages;

    static inline File const filesystem = ProjectInfo::appDataDir.getChildFile("Externals");

    // Package info file
    File pkgInfo = filesystem.getChildFile(".pkg_info");

    // Package state tree, keeps track of which packages are installed and saves it to pkgInfo
    ValueTree packageState = ValueTree("pkg_info");

    // Thread for unzipping and installing packages
    OwnedArray<DownloadTask> downloads;

    std::unique_ptr<WebInputStream> webstream;

    static inline String const floatsize = String(PD_FLOATSIZE);
    static inline String const os =
#if JUCE_LINUX
        "Linux"
#elif JUCE_MAC || JUCE_IOS
        "Darwin"
#elif JUCE_WINDOWS
        "Windows"
    // plugdata has no official BSD support and testing, but for completeness:
#elif defined __FreeBSD__
        "FreeBSD"
#elif defined __NetBSD__
        "NetBSD"
#elif defined __OpenBSD__
        "OpenBSD"
#else
#    if defined(__GNUC__)
#        warning unknown OS
#    endif
        0
#endif
        ;

    static inline String const machine =
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
        "amd64"
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
        "i386"
#elif defined(__ppc__)
        "ppc"
#elif defined(__aarch64__)
        "arm64"
#elif __ARM_ARCH == 6 || defined(__ARM_ARCH_6__)
        "armv6"
#elif __ARM_ARCH == 7 || defined(__ARM_ARCH_7__)
        "armv7"
#else
#    if defined(__GNUC__)
#        warning unknown architecture
#    endif
        ""
#endif
        ;

    // Create a single package manager that exists even when the dialog is not open
    // This allows more efficient pre-fetching of packages, and also makes it easy to
    // continue downloading when the dialog closes
    // Inherits from deletedAtShutdown to handle cleaning up
    JUCE_DECLARE_SINGLETON(PackageManager, false)
};

JUCE_IMPLEMENT_SINGLETON(PackageManager)

class Deken : public Component
    , public ListBoxModel
    , public ActionListener {

public:
    Deken()
    {
        setInterceptsMouseClicks(false, true);

        listBox.setModel(this);

        input.setTextToShowWhenEmpty("Type to search for objects or libraries", findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f));
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::panelTextColourId));
        input.setJustification(Justification::centredLeft);
        input.setBorder({ 0, 3, 5, 1 });
        input.setEnabled(false);
        input.onTextChange = [this]() {
            filterResults();
            repaint();
        };

        updateSpinner.setAlwaysOnTop(true);

        addAndMakeVisible(listBox.getViewport());
        addChildComponent(input);
        addAndMakeVisible(updateSpinner);

        refreshButton.setTooltip("Refresh packages");
        addAndMakeVisible(refreshButton);
        refreshButton.onClick = [this]() {
            packageManager->startThread();
            packageManager->sendActionMessage("");
        };

        packageManager->addActionListener(this);

        refreshButton.setEnabled(false);
        input.setText("Updating Packages...");
        updateSpinner.startSpinning();

        if (!packageManager->isThreadRunning()) {
            packageManager->startThread();
        }

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this]() {
            auto isSearching = searchButton.getToggleState();
            input.setVisible(isSearching);
            if (isSearching)
                input.grabKeyboardFocus();

            installedButton.setVisible(!isSearching);
            exploreButton.setVisible(!isSearching);
            repaint();
            filterResults();
        };

        addAndMakeVisible(searchButton);
        addAndMakeVisible(installedButton);
        addAndMakeVisible(exploreButton);

        installedButton.setRadioGroupId(hash("deken_toolbar"));
        exploreButton.setRadioGroupId(hash("deken_toolbar"));

        installedButton.setToggleState(true, dontSendNotification);
        installedButton.onClick = [this]() {
            filterResults();
        };
        exploreButton.onClick = [this]() {
            filterResults();
        };

        filterResults();
    }

    ~Deken()
    {
        packageManager->removeActionListener(this);
    }

    // Package update starts
    void actionListenerCallback(String const& message) override
    {

        auto* thread = dynamic_cast<Thread*>(packageManager);
        bool running = thread->isThreadRunning();

        // Handle errors
        if (message.isNotEmpty()) {
            showError(message);
            input.setEnabled(false);
            updateSpinner.stopSpinning();
            return;
        } else {
            showError("");
        }

        if (running) {

            input.setText("Updating packages...");
            input.setEnabled(false);
            refreshButton.setEnabled(false);
            updateSpinner.startSpinning();
        } else {
            // Clear text if it was previously disabled
            // If it wasn't, this is just an update call from the package manager
            if (!input.isEnabled()) {
                input.setText("");
            }

            refreshButton.setEnabled(true);
            input.setEnabled(true);
            updateSpinner.stopSpinning();
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        auto titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);

        if (errorMessage.isNotEmpty()) {
            Fonts::drawText(g, errorMessage, getLocalBounds().removeFromBottom(28).withTrimmedLeft(8).translated(0, 2), Colours::red);
        }

        if (searchResult.isEmpty()) {
            auto message = installedButton.getToggleState() ? "No externals installed" : "Couldn't find any externals";
            Fonts::drawText(g, message, getLocalBounds().withTrimmedTop(40).removeFromTop(32), findColour(PlugDataColour::panelTextColourId), 14, Justification::centred);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 40, getWidth(), 40);
    }

    int getNumRows() override
    {
        return searchResult.size() + packageManager->downloads.size();
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
    }

    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;

        bool isFirst = rowNumber == 0;
        bool isLast = rowNumber == (packageManager->downloads.size() + searchResult.size()) - 1;
        if (isPositiveAndBelow(rowNumber, packageManager->downloads.size())) {
            return new DekenRowComponent(*this, packageManager->downloads[rowNumber]->packageInfo, isFirst, isLast);
        } else if (isPositiveAndBelow(rowNumber - packageManager->downloads.size(), searchResult.size())) {
            return new DekenRowComponent(*this, searchResult.getReference(rowNumber - packageManager->downloads.size()), isFirst, isLast);
        }

        return nullptr;
    }

    void filterResults()
    {
        String query = input.getText();
        bool isSearching = searchButton.getToggleState();

        PackageList newResult;

        searchResult.clear();

        if (installedButton.getToggleState() && !isSearching) {

            // make sure installed packages are sorted alphabetically
            PackageSorter::sort(packageManager->packageState);

            for (auto child : packageManager->packageState) {
                auto name = child.getType().toString();
                auto description = child.getProperty("Description").toString();
                auto timestamp = child.getProperty("Timestamp").toString();
                auto url = child.getProperty("URL").toString();
                auto version = child.getProperty("Version").toString();
                auto author = child.getProperty("Author").toString();
                auto objects = StringArray();

                auto info = PackageInfo(name, author, timestamp, url, description, version, objects);

                searchResult.addIfNotAlreadyThere(info);
            }

            listBox.updateContent();
            return;
        }

        PackageList allPackages = packageManager->allPackages;

        if (isSearching && !query.isEmpty()) {
            // First check for name match
            for (auto const& result : allPackages) {
                if (result.name.contains(query)) {
                    newResult.addIfNotAlreadyThere(result);
                }
            }

            // Then check for description match
            for (auto const& result : allPackages) {
                if (result.description.contains(query)) {
                    newResult.addIfNotAlreadyThere(result);
                }
            }

            // Then check for object match
            for (auto const& result : allPackages) {
                if (result.objects.contains(query)) {
                    newResult.addIfNotAlreadyThere(result);
                }
            }

            // Then check for author match
            for (auto const& result : allPackages) {
                if (result.author.contains(query)) {
                    newResult.addIfNotAlreadyThere(result);
                }
            }

            // Then check for object close match
            for (auto const& result : allPackages) {
                for (auto const& obj : result.objects) {
                    if (obj.contains(query)) {
                        newResult.addIfNotAlreadyThere(result);
                    }
                }
            }
        } else if (!isSearching) {
            newResult = allPackages;
        }

        // Downloads are already always visible, so filter them out here
        newResult.removeIf([this](PackageInfo const& package) {
            for (const auto* download : packageManager->downloads) {
                if (download->packageInfo == package) {
                    return true;
                }
            }
            return false;
        });

        searchResult = newResult;
        listBox.updateContent();
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().removeFromTop(40).withTrimmedLeft(46).reduced(42, 5));

        auto bounds = getLocalBounds().withTrimmedTop(40);

        updateSpinner.setBounds(input.getBounds().removeFromRight(30));

        listBox.setBounds(getLocalBounds().withHeight(listBox.getHeight()));
        listBox.getViewport()->setBounds(bounds);

        refreshButton.setBounds(getLocalBounds().removeFromTop(40).removeFromLeft(40).translated(2, 0).reduced(1));
        searchButton.setBounds(getLocalBounds().removeFromTop(40).removeFromLeft(40).translated(42, 0).reduced(1));

        installedButton.setBounds(getLocalBounds().removeFromTop(40).withSizeKeepingCentre(105, 36).translated(-54, 0));
        exploreButton.setBounds(getLocalBounds().removeFromTop(40).withSizeKeepingCentre(105, 36).translated(54, 0));
    }

    // Show error message in statusbar
    void showError(String const& message)
    {
        errorMessage = message;
        repaint();
    }

private:
    struct DekenListBox : public Component {
        DekenListBox()
        {
            viewport.setViewedComponent(this, false);

            viewport.setScrollBarsShown(true, false, false, false);

            listBox.setRowHeight(66);
            listBox.setOutlineThickness(0);
            listBox.deselectAllRows();
            listBox.getViewport()->setScrollBarsShown(true, false, false, false);
            listBox.addMouseListener(this, true);
            listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
            listBox.getViewport()->setScrollBarsShown(false, false, false, false);

            setVisible(true);
            addAndMakeVisible(listBox);
        }

        Viewport* getViewport()
        {
            return &viewport;
        }

        void updateContent()
        {
            listBox.updateContent();

            auto* model = listBox.getListBoxModel();
            auto height = model ? model->getNumRows() * listBox.getRowHeight() : viewport.getParentComponent()->getHeight();
            listBox.setBounds(getLocalBounds().reduced(20, 18).withHeight(height));
            setSize(getWidth(), height + 26);
        }

        void setModel(ListBoxModel* model)
        {
            listBox.setModel(model);
        }

        void paint(Graphics& g) override
        {
            auto* model = listBox.getListBoxModel();
            if (!model || !model->getNumRows())
                return;

            auto bounds = getLocalBounds();
            auto margin = 30;

            auto shadowY = 20;
            auto shadowX = bounds.getX() + margin;
            auto shadowWidth = bounds.getWidth() - (margin * 2);
            auto shadowHeight = (model->getNumRows() * listBox.getRowHeight()) - 5;

            Path shadowPath;
            shadowPath.addRoundedRectangle(shadowX, shadowY, shadowWidth, shadowHeight, Corners::largeCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.4f), 7, { 0, 1 });
        }

        ListBox listBox;
        BouncingViewport viewport;
    };

    // List component to list packages
    DekenListBox listBox;

    // Last error message
    String errorMessage;

    // Current search result
    PackageList searchResult;

    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);
    MainToolbarButton refreshButton = MainToolbarButton(Icons::Refresh);

    SettingsToolbarButton installedButton = SettingsToolbarButton(Icons::Checkmark, "Installed");
    SettingsToolbarButton exploreButton = SettingsToolbarButton(Icons::Sparkle, "Explore");

    PackageManager* packageManager = PackageManager::getInstance();

    SearchEditor input;

    Spinner updateSpinner;

    // Component representing a search result
    // It holds package info about the package it represents
    // and can
    struct DekenRowComponent : public Component {
        Deken& deken;
        PackageInfo packageInfo;

        TextButton installButton = TextButton("Install");
        TextButton reinstallButton = TextButton(Icons::Refresh);
        TextButton uninstallButton = TextButton("Uninstall");
        TextButton addToPathButton = TextButton("Add to path");

        float installProgress;
        ValueTree& packageState;

        bool isFirst, isLast;

        DekenRowComponent(Deken& parent, PackageInfo& info, bool first, bool last)
            : deken(parent)
            , packageInfo(info)
            , packageState(deken.packageManager->packageState)
            , isFirst(first)
            , isLast(last)
        {
            addChildComponent(installButton);
            addChildComponent(uninstallButton);
            addChildComponent(addToPathButton);

            auto backgroundColour = findColour(PlugDataColour::panelForegroundColourId);
            installButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
            installButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
            installButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            uninstallButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
            uninstallButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
            uninstallButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addToPathButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
            addToPathButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
            addToPathButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            installButton.setTooltip("Install package");
            uninstallButton.setTooltip("Uninstall package");
            addToPathButton.setTooltip("Add to search path");

            uninstallButton.onClick = [this]() {
                setInstalled(false);
                deken.packageManager->uninstall(packageInfo);
                deken.filterResults();
            };

            installButton.onClick = [this]() {
                auto* downloadTask = deken.packageManager->install(packageInfo);
                attachToDownload(downloadTask);
            };

            addToPathButton.onClick = [this]() {
                auto state = packageState.getChildWithProperty("ID", packageInfo.packageId);
                state.setProperty("AddToPath", var(addToPathButton.getToggleState()), nullptr);
            };

            addToPathButton.setClickingTogglesState(true);
            auto state = packageState.getChildWithProperty("ID", packageInfo.packageId);
            if (state.hasProperty("AddToPath")) {
                addToPathButton.setToggleState(static_cast<bool>(state.getProperty("AddToPath")), dontSendNotification);
            }

            // Check if package is already installed
            setInstalled(deken.packageManager->packageExists(packageInfo));

            // Check if already in progress
            if (auto* task = deken.packageManager->getDownloadForPackage(packageInfo)) {
                attachToDownload(task);
            }
        }

        void attachToDownload(PackageManager::DownloadTask* task)
        {
            task->onProgress = [_this = SafePointer(this)](float progress) {
                if (!_this)
                    return;
                _this->installProgress = progress;
                _this->repaint();
            };

            task->onFinish = [_this = SafePointer(this)](Result result) {
                if (!_this)
                    return;

                if (result.wasOk()) {
                    _this->setInstalled(result);
                    _this->deken.filterResults();
                } else {
                    _this->deken.showError(result.getErrorMessage());
                    _this->deken.filterResults();
                }
            };

            installButton.setVisible(false);
            uninstallButton.setVisible(false);
            addToPathButton.setVisible(false);
        }

        // Enables or disables buttons based on package state
        void setInstalled(bool installed)
        {
            installButton.setVisible(!installed);
            uninstallButton.setVisible(installed);
            addToPathButton.setVisible(installed);
            installProgress = 0.0f;

            repaint();
        }

        void paint(Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced(8.0f, 0.0f).withTrimmedBottom(-1.0f);

            Path p;
            p.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), isLast ? b.getHeight() - 2.0f : b.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, isFirst, isFirst, isLast, isLast);

            g.setColour(findColour(PlugDataColour::panelForegroundColourId));
            g.fillPath(p);

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.strokePath(p, PathStrokeType(1.0f));

            Fonts::drawStyledText(g, packageInfo.name, 64, 8, 200, 25, findColour(ComboBox::textColourId), Semibold, 15);
            Fonts::drawIcon(g, Icons::Externals, Rectangle<int>(16, 14, 38, 38), findColour(ComboBox::textColourId));

            Fonts::drawFittedText(g, "Uploaded " + getRelativeTimeDescription(packageInfo.timestamp) + " by " + packageInfo.author, getWidth() - 418, 6, 400, 25, findColour(PlugDataColour::panelTextColourId), 1, 0.8f, 13.5f, Justification::centredRight);

            // draw progressbar
            if (deken.packageManager->getDownloadForPackage(packageInfo)) {
                float width = getWidth() - 26.0f;
                float right = jmap(installProgress, 70.0f, width);

                Path downloadPath;
                downloadPath.addLineSegment({ 70, 42, right, 42 }, 1.0f);

                Path fullPath;
                fullPath.addLineSegment({ 70, 42, width, 42 }, 1.0f);

                g.setColour(findColour(PlugDataColour::panelTextColourId));
                g.strokePath(fullPath, PathStrokeType(11.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                g.strokePath(downloadPath, PathStrokeType(8.0f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
            } else {
                Fonts::drawFittedText(g, packageInfo.version, 64, 31, 400, 25, findColour(PlugDataColour::panelTextColourId), 1, 0.8f, 15);
                // Fonts::drawFittedText(g, packageInfo.timestamp, 435, 0, 200, getHeight(), findColour(PlugDataColour::panelTextColourId));
            }
        }

        void resized() override
        {
            installButton.setBounds(getWidth() - 98, 35, 80, 22);
            uninstallButton.setBounds(getWidth() - 98, 35, 80, 22);
            addToPathButton.setBounds(getWidth() - 192, 35, 80, 22);
        }
    };
};
