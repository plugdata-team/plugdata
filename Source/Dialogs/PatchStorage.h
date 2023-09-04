
class OnlineImage : public Component
    , public ThreadPoolJob
    , private AsyncUpdater {
public:
    OnlineImage(bool roundedTop, bool roundedBottom)
        : roundTop(roundedTop)
        , roundBottom(roundedBottom)
        , ThreadPoolJob("Image download job")
    {
        spinner.setSize(50, 50);
        spinner.setCentrePosition(getWidth() / 2, getHeight() / 2);
        addAndMakeVisible(spinner);
        setInterceptsMouseClicks(false, false);
    }

    ~OnlineImage() override
    {
        // Stop the thread
        signalJobShouldExit();
        imageDownloadPool.removeJob(this, true, -1);
    }

    void setImageURL(const URL& url)
    {
        downloadedImage = Image();

        // Lock the thread to safely update the image URL
        const ScopedLock sl(lock);
        imageURL = url;
        imageDownloadPool.addJob(this, false);
        spinner.startSpinning();
        repaint();
    }

    void paint(Graphics& g) override
    {
        // Create a rounded rectangle to use as a mask
        Path roundedRectanglePath;
        roundedRectanglePath.addRoundedRectangle(0, 0, getWidth(), getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, roundBottom, roundBottom);

        if (!downloadedImage.isValid()) {
            g.setColour(findColour(PlugDataColour::panelForegroundColourId));
            g.fillPath(roundedRectanglePath);
            return;
        }

        g.saveState();

        // Set the path as the clip region for the Graphics context
        g.reduceClipRegion(roundedRectanglePath);

        float scaleX = static_cast<float>(getWidth()) / downloadedImage.getWidth();
        float scaleY = static_cast<float>(getHeight()) / downloadedImage.getHeight();
        float scale = jmax(scaleX, scaleY);

        // Calculate the translation values to keep the image centered
        float translateX = (getWidth() - downloadedImage.getWidth() * scale) * 0.5f;
        float translateY = (getHeight() - downloadedImage.getHeight() * scale) * 0.5f;

        // Apply the transformation to the Graphics context
        g.addTransform(AffineTransform::translation(translateX, translateY).scaled(scale));

        // Apply the transformation to the Graphics context
        g.drawImage(downloadedImage, 0, 0, downloadedImage.getWidth(), downloadedImage.getHeight(), 0, 0, downloadedImage.getWidth(), downloadedImage.getHeight());

        g.restoreState();
    }

    void resized() override
    {
        spinner.setCentrePosition(getWidth() / 2, getHeight() / 2);
    }

private:
    bool roundTop, roundBottom;
    URL imageURL;
    Image downloadedImage;
    Spinner spinner;

    CriticalSection lock;

    JobStatus runJob() override
    {
        // Check if there's an image URL to download
        if (imageURL.isWellFormed()) {
            MemoryBlock block;
            // Load the image data from the URL
            auto inputStream = imageURL.readEntireBinaryStream(block);
            MemoryInputStream memstream(block, false);

            downloadedImage = ImageFileFormat::loadFrom(block.getData(), block.getSize());

            // Redraw the component on the message thread
            triggerAsyncUpdate();
        }

        return jobHasFinished;
    }

    void handleAsyncUpdate() override
    {
        // Repaint the component with the downloaded image or the spinner
        repaint();
        spinner.stopSpinning();
    }

    static inline ThreadPool imageDownloadPool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnlineImage)
};

class PatchInfo {
public:
    int id;
    String created_at;
    String title;
    String artwork_url;
    String thumbnail_url;
    String revision;
    int like_count;
    int download_count;
    int view_count;
    int platform_id;
    String author_name;
    String category;
    String platform;
    String description;

    PatchInfo(var const& jsonData)
    {
        id = jsonData["id"];
        created_at = jsonData["created_at"].toString();
        title = jsonData["title"].toString();
        artwork_url = jsonData["artwork"]["url"].toString();
        thumbnail_url = jsonData["artwork"]["thumbnail_url"].toString();
        revision = jsonData["revision"].toString();
        like_count = jsonData["like_count"];
        view_count = jsonData["view_count"];
        download_count = jsonData["download_count"];
        author_name = jsonData["author"]["name"].toString();
        category = jsonData["categories"][0]["name"].toString();
        platform = jsonData["platform"]["name"].toString();
        platform_id = jsonData["platform"]["id"];
        description = removeMarkupTags(jsonData["excerpt"].toString());
    }

    static String removeMarkupTags(String const& text)
    {
        String cleanedText;
        int textLength = text.length();
        bool insideTag = false;

        for (int i = 0; i < textLength; ++i) {
            char currentChar = text[i];

            if (currentChar == '<') {
                insideTag = true;
            } else if (currentChar == '>') {
                insideTag = false;
            } else if (!insideTag) {
                cleanedText += currentChar;
            }
        }

        return cleanedText.removeCharacters("\r");
    }
};

class PatchFullDisplay : public Component {
    String author, title, platform, category, revision, license;
    int views, likes, downloads;
    String description, fileURL, fileName, onlineURL, updatedAt;

    const File patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");

public:
    PatchFullDisplay()
        : image(true, true)
    {
        addAndMakeVisible(viewButton);
        addAndMakeVisible(downloadButton);
        addAndMakeVisible(image);

        downloadButton.onClick = [this]() {
            if (fileName.endsWith(".pd")) {
                URL(fileURL + fileName).downloadToFile(patchesDir.getChildFile(fileName), URL::DownloadTaskOptions());
            } else {
            }
        };

        viewButton.onClick = [this]() {
            URL(onlineURL).launchInDefaultBrowser();
        };
    }

    void showPatch(PatchInfo const& patchInfo)
    {
        views = patchInfo.view_count;
        likes = patchInfo.like_count;
        downloads = patchInfo.download_count;
        platform = patchInfo.platform;
        category = patchInfo.category;
        revision = patchInfo.revision;
        title = patchInfo.title;
        author = patchInfo.author_name;

        auto fullInfoUrl = URL("https://patchstorage.com/api/beta/patches/" + String(patchInfo.id));

        MemoryBlock block;
        // Load the image data from the URL
        auto inputStream = fullInfoUrl.readEntireBinaryStream(block);
        MemoryInputStream memstream(block, false);

        var patchObject = JSON::parse(memstream);

        description = PatchInfo::removeMarkupTags(patchObject["content"].toString());
        fileURL = patchObject["files"][0]["url"].toString();
        fileName = patchObject["files"][0]["filename"].toString();
        onlineURL = patchObject["url"].toString();
        updatedAt = patchObject["updated_at"].toString();
        license = patchObject["license"]["slug"].toString();

        if (fileName.endsWith(".pd")) {
            downloadButton.setButtonText("Open");
        } else {
            downloadButton.setButtonText("Download");
        }

        image.setImageURL(patchInfo.artwork_url);
        setVisible(true);
    }

    void drawSectionBackground(Graphics& g, Rectangle<int> b)
    {
        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 6, { 0, 1 });

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        // Drag image outline
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(image.getBounds().toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(12);
        auto topArea = b.removeFromTop(300).withTrimmedLeft(500);

        g.fillAll(findColour(PlugDataColour::panelBackgroundColourId));

        // Drag image shadow
        Path p;
        p.addRoundedRectangle(image.getBounds().reduced(1.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 6, { 0, 1 });

        topArea.removeFromBottom(90); // space for download button
        topArea = topArea.reduced(16);
        drawSectionBackground(g, topArea);

        auto textColour = findColour(PlugDataColour::panelTextColourId);

        StringArray extraInfo = {
            "Author: " + author,
            "Category: " + category,
            "Revision: " + revision,
            "License: " + license,
            "Last Modified: " + getRelativeTimeDescription(updatedAt),
        };

        auto extraInfoArea = topArea.reduced(12);
        g.setColour(textColour);
        g.setFont(Font(14.5));
        for (auto& line : extraInfo) {
            if (!line.fromFirstOccurrenceOf(":", false, false).containsNonWhitespaceChars())
                continue;

            g.drawText(line, extraInfoArea.removeFromTop(25), Justification::left);
        }

        auto iconArea = extraInfoArea.removeFromBottom(32).reduced(2);
        auto iconSectionWidth = iconArea.getWidth() / 2.75f;

        auto likesArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Heart, likesArea.removeFromLeft(32),
            findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(likes), likesArea, Justification::left);

        auto downloadsArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Download, downloadsArea.removeFromLeft(32), findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(downloads), downloadsArea, Justification::left);

        auto viewsArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Eye, viewsArea.removeFromLeft(32), findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(views), viewsArea, Justification::left);

        auto contentArea = b.reduced(16, 8);
        drawSectionBackground(g, contentArea);

        contentArea = contentArea.reduced(12);

        Fonts::drawStyledText(g, title, contentArea.removeFromTop(22), textColour, Bold, 15, Justification::left);

        auto layout = TextLayout();
        auto descriptionText = AttributedString(description);
        descriptionText.setFont(Font(14.5f));
        descriptionText.setColour(textColour);
        layout.createLayout(descriptionText, contentArea.getWidth(), contentArea.getHeight());

        layout.draw(g, contentArea.withTrimmedBottom(32).toFloat());
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(12);
        auto topArea = b.removeFromTop(300);
        auto imageArea = topArea.removeFromLeft(500);

        image.setBounds(imageArea.reduced(16));

        auto buttonArea = topArea.removeFromBottom(90).withTrimmedBottom(16).reduced(16, 0);
        downloadButton.setBounds(buttonArea.removeFromTop(30));
        buttonArea.removeFromTop(14);
        viewButton.setBounds(buttonArea);
    }

    OnlineImage image;
    TextButton downloadButton;
    TextButton viewButton = TextButton("View on PatchStorage");
};

class PatchDisplay : public Component {
public:
    PatchDisplay(PatchInfo const& patchInfo, std::function<void(PatchInfo const&)> clickCallback)
        : info(patchInfo)
        , callback(clickCallback)
        , image(true, false)
    {
        image.setImageURL(patchInfo.artwork_url);
        addAndMakeVisible(image);
    }

    bool matchesQuery(String const& query)
    {
        // TODO: search tags!
        return query.isEmpty() || info.title.contains(query) || info.author_name.contains(query) || info.description.contains(query);
    }

private:
    void paintOverChildren(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(6);
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(6);

        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 6, { 0, 1 });

        if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
        } else {
            g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        }

        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);

        b.removeFromTop(171); // space for image
        auto nameArea = b.removeFromTop(24);

        g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
        g.fillRect(nameArea);

        auto platformArea = nameArea.removeFromRight(80).reduced(4).toFloat();
        platformArea.removeFromLeft(10);

        // plugdata patch
        if (info.platform_id == 8440) {
            auto backgroundColour = findColour(PlugDataColour::dataColourId);
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(platformArea, Corners::defaultCornerRadius);
            Fonts::drawText(g, "plugdata", platformArea, backgroundColour.contrasting(), 13.5f, Justification::centred);
        } else {                                       // pd-vanilla patch
            auto backgroundColour = Colour(0, 252, 7); // pd logo colour
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(platformArea, Corners::defaultCornerRadius);
            Fonts::drawText(g, "pure-data", platformArea, backgroundColour.contrasting(), 13.5f, Justification::centred);
        }

        auto textColour = findColour(PlugDataColour::panelTextColourId);
        Fonts::drawText(g, "by " + info.author_name, nameArea.withTrimmedLeft(10), textColour, 13.5f, Justification::left);

        auto textBounds = b.reduced(10);

        Fonts::drawStyledText(g, info.title, textBounds.removeFromTop(22), textColour, Bold, 15, Justification::left);

        auto layout = TextLayout();
        auto description = AttributedString(info.description);
        description.setFont(Font(14.5f));
        description.setColour(textColour);
        layout.createLayout(description, 240, 150);

        layout.draw(g, textBounds.withTrimmedBottom(32).toFloat());

        auto iconArea = b.removeFromBottom(32).reduced(6);
        auto iconSectionWidth = iconArea.getWidth() / 2.75f;

        auto likesArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Heart, likesArea.removeFromLeft(32),
            findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(info.like_count), likesArea, Justification::left);

        auto downloadsArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Download, downloadsArea.removeFromLeft(32), findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(info.download_count), downloadsArea, Justification::left);

        auto viewsArea = iconArea.removeFromLeft(iconSectionWidth);

        Fonts::drawIcon(g, Icons::Eye, viewsArea.removeFromLeft(32), findColour(PlugDataColour::panelTextColourId), 15);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Font(14));
        g.drawText(String(info.view_count), viewsArea, Justification::left);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6);
        image.setBounds(b.removeFromTop(171).withWidth(248));
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        callback(info);
    }

    OnlineImage image;

    std::function<void(PatchInfo const&)> callback;
    PatchInfo info;
};

class PatchContainer : public Component
    , public AsyncUpdater {
    int const displayWidth = 260;
    int const displayHeight = 360;

    OwnedArray<PatchDisplay> patchDisplays;
    std::mutex patchesMutex;
    Array<PatchInfo> patches;

public:
    std::function<void(PatchInfo const&)> patchClicked;

    void handleAsyncUpdate() override
    {
        patchDisplays.clear();

        for (auto& patch : patches) {
            auto* display = patchDisplays.add(new PatchDisplay(patch, patchClicked));
            addAndMakeVisible(display);
        }

        setSize(getWidth(), (patches.size() / (getWidth() / displayWidth)) * displayHeight);
        resized(); // Even if size if the same, we still want to call resize
    }

    void filterPatches(String query)
    {
        for (auto* patch : patchDisplays) {
            if (patch->matchesQuery(query)) {
                patch->setVisible(true);
            } else {
                patch->setVisible(false);
            }
        }

        resized();
    }

    void showPatches(Array<PatchInfo> const& patchesToShow)
    {
        patchesMutex.lock();
        patches = patchesToShow;
        patchesMutex.unlock();
        triggerAsyncUpdate();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6);

        auto currentRow = bounds.removeFromTop(displayHeight);

        auto numColumns = currentRow.getWidth() / displayWidth;
        auto extraSpace = currentRow.getWidth() - (numColumns * displayWidth);
        auto padding = (extraSpace / numColumns) / 2;

        for (auto* display : patchDisplays) {
            if (!display->isVisible())
                continue;

            if (currentRow.getWidth() < displayWidth) {
                currentRow = bounds.removeFromTop(displayHeight);
            }

            currentRow.removeFromLeft(padding);
            display->setBounds(currentRow.removeFromLeft(displayWidth));
            currentRow.removeFromLeft(padding);
        }
    }
};

struct PatchStorage : public Component
    , public AsyncUpdater
    , public Thread {
    PatchContainer patchContainer;
    BouncingViewport contentViewport;
    PatchFullDisplay patchFullDisplay;

    TextButton backButton = TextButton(Icons::Back);
    TextButton refreshButton = TextButton(Icons::Refresh);

    TextButton clearButton = TextButton(Icons::ClearText);
    TextEditor input;

    Spinner spinner;

public:
    PatchStorage()
        : Thread("PatchStorage API Thread")
    {
        contentViewport.setViewedComponent(&patchContainer, false);
        patchContainer.setVisible(true);
        addAndMakeVisible(contentViewport);

        contentViewport.setScrollBarsShown(true, false, true, false);

        spinner.startSpinning();
        startThread();

        addChildComponent(patchFullDisplay);

        patchContainer.patchClicked = [this](PatchInfo const& patch) {
            patchFullDisplay.showPatch(patch);
            backButton.setVisible(true);
            refreshButton.setVisible(false);
            input.setVisible(false);
            clearButton.setVisible(false);
        };

        addChildComponent(backButton);

        backButton.onClick = [this]() {
            patchFullDisplay.setVisible(false);
            backButton.setVisible(false);
            refreshButton.setVisible(true);
            input.setVisible(true);
            clearButton.setVisible(true);
        };

        backButton.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        backButton.setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        backButton.getProperties().set("Style", "LargeIcon");

        refreshButton.setTooltip("Refresh packages");
        refreshButton.getProperties().set("Style", "LargeIcon");
        refreshButton.setEnabled(false);
        addAndMakeVisible(refreshButton);
        refreshButton.onClick = [this]() {
            refreshButton.setEnabled(false);
            spinner.startSpinning();
            Thread::launch([this]() {
                fetchPatches();
            });
        };

        input.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::searchBarColourId));
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::panelTextColourId));
        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 3, 1 });
        input.getProperties().set("NoOutline", true);
        input.onTextChange = [this]() {
            patchContainer.filterPatches(input.getText());
        };
        addAndMakeVisible(input);

        clearButton.getProperties().set("Style", "SmallIcon");
        clearButton.setAlwaysOnTop(true);
        clearButton.onClick = [this]() {
            input.clear();
            grabKeyboardFocus(); // steal focus from text editor
            input.repaint();
            patchContainer.filterPatches("");
        };
        addAndMakeVisible(clearButton);
        addChildComponent(spinner);
    }

    ~PatchStorage()
    {
        waitForThreadToExit(-1);
    }

    void run() override
    {
        fetchPatches();
    }

    void paintOverChildren(Graphics& g) override
    {
        Fonts::drawStyledText(g, "Discover", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);

        if (input.isVisible()) {
            Fonts::drawIcon(g, Icons::Search, 0, 40, 30, findColour(PlugDataColour::panelTextColourId), 12);

            if (input.getText().isEmpty()) {
                Fonts::drawFittedText(g, "Type to search for patches", 30, 40, getWidth() - 60, 30, findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f), 1, 0.9f, 14);
            }
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawLine(0, 70, getWidth(), 70);
        }

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, 40, getWidth(), 40);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        auto bounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);
    }

    void resized() override
    {
        auto b = getLocalBounds().withTrimmedTop(40);

        patchFullDisplay.setBounds(b);

        auto inputBounds = b.removeFromTop(28);
        input.setBounds(inputBounds);
        clearButton.setBounds(inputBounds.removeFromRight(32));

        contentViewport.setBounds(b);

        patchContainer.setSize(getWidth(), patchContainer.getHeight());

        backButton.setBounds(2, 0, 40, 40);
        refreshButton.setBounds(2, 0, 40, 40);

        spinner.setSize(50, 50);
        spinner.setCentrePosition(b.getWidth() / 2, b.getHeight() / 2);
    }

    void fetchPatches()
    {
        int page = 1;
        bool failed = false;

        Array<PatchInfo> patches;
        std::unique_ptr<WebInputStream> webstream;

        while (!failed && !threadShouldExit()) {
            webstream = std::make_unique<WebInputStream>(URL("https://patchstorage.com/api/beta/patches?page=" + String(page++) + "&per_page=100&platforms=90,8440"), false);
            webstream->connect(nullptr);

            if (webstream->isError() || webstream->getStatusCode() == 400) {
                failed = true;
                break;
            }

            MemoryBlock block;
            webstream->readIntoMemoryBlock(block);
            MemoryInputStream memstream(block, false);

            auto parsedData = JSON::parse(memstream);
            if (parsedData.isArray()) {
                for (int i = 0; i < parsedData.size(); ++i) {
                    var const& patchObject = parsedData[i];

                    patches.add(PatchInfo(patchObject));
                }
            }
        }

        std::sort(patches.begin(), patches.end(), [](PatchInfo const& first, PatchInfo const& second) {
            if (first.platform_id == 8440 && second.platform_id != 8440)
                return true;

            if (first.platform_id != 8440 && second.platform_id == 8440)
                return false;

            return first.download_count > second.download_count;
        });

        if (threadShouldExit())
            return;

        patchContainer.showPatches(patches);
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        refreshButton.setEnabled(true);
        spinner.stopSpinning();
    }
};
