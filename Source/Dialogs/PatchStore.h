#pragma once
#include <random>
#include "Utility/PatchInfo.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

class DownloadPool final : public DeletedAtShutdown {
public:
    struct DownloadListener {
        virtual ~DownloadListener() = default;
        virtual void downloadProgressed(hash32 hash, float progress) { }
        virtual void databaseDownloadCompleted(HeapArray<std::pair<PatchInfo, int>> const& patches) { }
        virtual void patchDownloadCompleted(hash32 hash, bool success) { }
        virtual void imageDownloadCompleted(hash32 hash, Image const& downloadedImage) { }
    };

    ~DownloadPool() override
    {
        imagePool.removeAllJobs(true, -1);
        patchPool.removeAllJobs(true, -1);
        clearSingletonInstance();
    }

    void addDownloadListener(DownloadListener* listener)
    {
        listeners.insert(listener);
    }

    void removeDownloadListener(DownloadListener* listener)
    {
        listeners.erase(listener);
    }

    void cancelImageDownloads()
    {
        imagePool.removeAllJobs(true, 500);
    }

    void downloadDatabase()
    {
        imagePool.addJob([this] {
            SmallArray<PatchInfo> patches;

            auto const webstream = std::make_unique<WebInputStream>(URL("https://plugdata.org/store.json"), false);
            webstream->connect(nullptr);

            if (webstream->isError() || webstream->getStatusCode() == 400) {
                // TODO: show error
                return;
            }

            MemoryBlock block;
            webstream->readIntoMemoryBlock(block);
            MemoryInputStream memstream(block, false);

            auto const parsedData = JSON::parse(memstream);
            auto patchData = parsedData["Patches"];
            if (patchData.isArray()) {
                for (int i = 0; i < patchData.size(); ++i) {
                    var const& patchObject = patchData[i];
                    patches.add(PatchInfo(patchObject));
                }
            }

            HeapArray<std::pair<PatchInfo, int>> sortedPatches;
            for (auto& patch : patches) {
                sortedPatches.emplace_back(patch, patch.isPatchInstalled() + 2 * patch.updateAvailable());
            }
            
            // Don't show paid patches on iOS, because then Apple might want a cut of the sale, which we cannot do
#if JUCE_IOS
            sortedPatches.remove_if([](auto const& item){
                return item.first.price != "Free";
            });
#endif

            std::ranges::sort(sortedPatches, [](std::pair<PatchInfo, int> const& first, std::pair<PatchInfo, int> const& second) {
                auto& [patchA, flagsA] = first;
                auto& [patchB, flagsB] = second;

                if (flagsA > flagsB)
                    return true;
                if (flagsA < flagsB)
                    return false;

                return patchA.releaseDate > patchB.releaseDate;
            });

            MessageManager::callAsync([this, sortedPatches] {
                for (auto& listener : listeners) {
                    listener->databaseDownloadCompleted(sortedPatches);
                }
            });
        });
    }

    void downloadImage(hash32 hash, URL location)
    {
        imagePool.addJob([this, hash, location] {
            auto updateImageListeners = [this](hash32 hash, Image& image) {
                MessageManager::callAsync([this, hash, image] {
                    for (auto& listener : listeners) {
                        listener->imageDownloadCompleted(hash, image);
                    }
                });
            };

            static UnorderedMap<hash32, Image> downloadImageCache;
            static CriticalSection cacheMutex;

            {
                ScopedLock lock(cacheMutex);

                if (downloadImageCache.contains(hash)) {
                    if (auto img = downloadImageCache[hash]; img.isValid()) {
                        updateImageListeners(hash, img);
                        return;
                    }
                }
            }

            MemoryBlock block;
            // Load the image data from the URL
            WebInputStream memstream(location, false);
            memstream.readIntoMemoryBlock(block);
            auto image = ImageFileFormat::loadFrom(block.getData(), block.getSize());
            {
                ScopedLock lock(cacheMutex);
                downloadImageCache[hash] = image;
            }
            updateImageListeners(hash, image);
        });
    }

    void downloadPatch(hash32 downloadHash, PatchInfo& info)
    {
        patchPool.addJob([this, downloadHash, info]() mutable {
            MemoryBlock data;

            int statusCode = 0;

            auto const instream = URL(info.download).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000).withStatusCode(&statusCode));
            int64 const totalBytes = instream->getTotalLength();
            int64 bytesDownloaded = 0;

            MemoryOutputStream mo(data, true);

            while (true) {
                auto const written = mo.writeFromInputStream(*instream, 8192);

                if (written == 0)
                    break;

                bytesDownloaded += written;

                float progress = static_cast<long double>(bytesDownloaded) / static_cast<long double>(totalBytes);

                if (cancelledDownloads.contains(downloadHash)) {
                    cancelledDownloads.erase(downloadHash);
                    MessageManager::callAsync([this, downloadHash]() mutable {
                        for (auto& listener : listeners) {
                            listener->patchDownloadCompleted(downloadHash, false);
                        }
                    });
                    return;
                }

                MessageManager::callAsync([this, downloadHash, progress]() mutable {
                    for (auto& listener : listeners) {
                        listener->downloadProgressed(downloadHash, progress);
                    }
                });
            }

            MemoryInputStream input(data, false);
            ZipFile zip(input);

            auto const patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");
            auto result = zip.uncompressTo(patchesDir, true);
            auto const downloadedPatch = patchesDir.getChildFile(zip.getEntry(0)->filename);

            auto const targetLocation = downloadedPatch.getParentDirectory().getChildFile(info.getNameInPatchFolder());
            targetLocation.deleteRecursively(true);

            downloadedPatch.moveFileTo(targetLocation);

            auto const metaFile = targetLocation.getChildFile("meta.json");
            if (!metaFile.existsAsFile()) {
                info.setInstallTime(Time::currentTimeMillis());
                auto json = info.json;
                metaFile.replaceWithText(info.json);
            } else {
                info = PatchInfo(JSON::fromString(metaFile.loadFileAsString()));
                info.setInstallTime(Time::currentTimeMillis());
                auto json = info.json;
                metaFile.replaceWithText(info.json);
            }

            auto const macOSTrash = ProjectInfo::appDataDir.getChildFile("Patches").getChildFile("__MACOSX");
            if (macOSTrash.isDirectory()) {
                macOSTrash.deleteRecursively();
            }

            MessageManager::callAsync([this, downloadHash, result] {
                for (auto& listener : listeners) {
                    listener->patchDownloadCompleted(downloadHash, result.wasOk());
                }
            });
        });
    }

    void cancelDownload(hash32 hash)
    {
        cancelledDownloads.insert(hash);
    }

private:
    UnorderedSet<DownloadListener*> listeners;

    CriticalSection cancelledDownloadsLock;
    UnorderedSet<hash32> cancelledDownloads;

    ThreadPool imagePool = ThreadPool(3);
    ThreadPool patchPool = ThreadPool(2);

public:
    JUCE_DECLARE_SINGLETON(DownloadPool, false);
};

JUCE_IMPLEMENT_SINGLETON(DownloadPool);

class OnlineImage final : public Component
    , public DownloadPool::DownloadListener {
public:
    OnlineImage(bool const roundedTop, bool const roundedBottom)
        : roundTop(roundedTop)
        , roundBottom(roundedBottom)
    {
        spinner.setSize(50, 50);
        spinner.setCentrePosition(getWidth() / 2, getHeight() / 2);
        addAndMakeVisible(spinner);
        setInterceptsMouseClicks(false, false);
        DownloadPool::getInstance()->addDownloadListener(this);
    }

    ~OnlineImage() override
    {
        DownloadPool::getInstance()->removeDownloadListener(this);
    }

    void imageDownloadCompleted(hash32 const hash, Image const& image) override
    {
        if (hash == imageHash) {
            scaledImage = resampleImageToFit(image);
            repaint();
            spinner.stopSpinning();
        }
    }

    void setImageURL(const URL& url)
    {
        imageHash = hash(url.toString(false));
        scaledImage = Image();

        // Lock the thread to safely update the image URL
        imageURL = url;
        DownloadPool::getInstance()->downloadImage(imageHash, url);
        spinner.startSpinning();

        repaint();
    }

    void paint(Graphics& g) override
    {
        // Create a rounded rectangle to use as a mask
        Path roundedRectanglePath;
        roundedRectanglePath.addRoundedRectangle(0, 0, getWidth(), getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, roundBottom, roundBottom);

        if (!scaledImage.isValid()) {
            g.setColour(findColour(PlugDataColour::panelForegroundColourId));
            g.fillPath(roundedRectanglePath);
            return;
        }

        g.saveState();

        g.reduceClipRegion(roundedRectanglePath);

        Rectangle<float> const targetBounds(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()));

        g.setImageResamplingQuality(Graphics::highResamplingQuality);
        g.drawImage(scaledImage, targetBounds, RectanglePlacement::centred | RectanglePlacement::fillDestination);

        g.restoreState();
    }

    Image resampleImageToFit(Image const& downloadedImage) const
    {
        Image result;
#if JUCE_MAC || JUCE_IOS
        if (downloadedImage.isValid()) {
            auto srcWidth = downloadedImage.getWidth();
            auto srcHeight = downloadedImage.getHeight();

            int targetWidth = getWidth();
            int targetHeight = getHeight();

            // Calculate the aspect ratios
            float srcAspect = static_cast<float>(srcWidth) / static_cast<float>(srcHeight);
            float targetAspect = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);

            // Crop the image to match the target aspect ratio
            int cropX = 0, cropY = 0, cropWidth = srcWidth, cropHeight = srcHeight;

            if (srcAspect > targetAspect) {
                // Image is wider than the target, crop width
                cropWidth = static_cast<int>(srcHeight * targetAspect);
                cropX = (srcWidth - cropWidth) / 2; // Center the crop
            } else if (srcAspect < targetAspect) {
                // Image is taller than the target, crop height
                cropHeight = static_cast<int>(srcWidth / targetAspect);
                cropY = (srcHeight - cropHeight) / 2; // Center the crop
            }

            // Resample the image to the new size
            result = downloadedImage.getClippedImage(Rectangle<int>(cropX, cropY, cropWidth, cropHeight));
        }
#else
        if (!downloadedImage.isValid())
            return result;

        auto srcWidth = downloadedImage.getWidth();
        auto srcHeight = downloadedImage.getHeight();

        int targetWidth = getWidth() * scale;
        int targetHeight = getHeight() * scale;

        // Calculate the aspect ratios
        float srcAspect = static_cast<float>(srcWidth) / static_cast<float>(srcHeight);
        float targetAspect = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);

        // Crop the image to match the target aspect ratio
        int cropX = 0, cropY = 0, cropWidth = srcWidth, cropHeight = srcHeight;

        if (srcAspect > targetAspect) {
            // Image is wider than the target, crop width
            cropWidth = static_cast<int>(srcHeight * targetAspect);
            cropX = (srcWidth - cropWidth) / 2; // Center the crop
        } else if (srcAspect < targetAspect) {
            // Image is taller than the target, crop height
            cropHeight = static_cast<int>(srcWidth / targetAspect);
            cropY = (srcHeight - cropHeight) / 2; // Center the crop
        }

        int numChannels = 0;
        auto srcData = packImageData(downloadedImage.getClippedImage(Rectangle<int>(cropX, cropY, cropWidth, cropHeight)), numChannels);

        HeapArray<unsigned char> resampledData(targetWidth * targetHeight * numChannels);

        // Perform resampling
        stbir_resize_uint8_linear(
            srcData.data(), cropWidth, cropHeight, 0,           // Source image
            resampledData.data(), targetWidth, targetHeight, 0, // Destination image
            static_cast<stbir_pixel_layout>(numChannels)        // Number of channels
        );

        result = Image(downloadedImage.getFormat(), targetWidth, targetHeight, true);
        Image::BitmapData destData(result, Image::BitmapData::writeOnly);

        for (int y = 0; y < targetHeight; ++y) {
            unsigned char* destRow = destData.getLinePointer(y);
            std::memcpy(destRow, &resampledData[y * targetWidth * numChannels], targetWidth * numChannels);
        }
#endif

        return result;
    }

    static HeapArray<unsigned char> packImageData(Image const& image, int& numChannels)
    {
        if (!image.isValid())
            return {};

        Image::BitmapData const bitmapData(image, Image::BitmapData::readOnly);

        // Determine the number of channels
        switch (image.getFormat()) {
        case Image::PixelFormat::RGB:
            numChannels = 3;
            break;
        case Image::PixelFormat::ARGB:
            numChannels = 4;
            break;
        case Image::PixelFormat::SingleChannel:
            numChannels = 1;
            break;
        default:
            return {};
        }

        // Allocate tightly packed buffer
        int const width = image.getWidth();
        int const height = image.getHeight();
        HeapArray<unsigned char> packedData(width * height * numChannels);

        for (int y = 0; y < height; ++y) {
            unsigned char const* row = bitmapData.getLinePointer(y);

            for (int x = 0; x < width; ++x) {
                int const srcIndex = x * bitmapData.pixelStride;
                int const destIndex = (y * width + x) * numChannels;

                // Pack based on format
                switch (image.getFormat()) {
                case Image::ARGB: {
                    packedData[destIndex + 0] = row[srcIndex + 1]; // Red
                    packedData[destIndex + 1] = row[srcIndex + 2]; // Green
                    packedData[destIndex + 2] = row[srcIndex + 3]; // Blue
                    if (numChannels == 4)
                        packedData[destIndex + 0] = row[srcIndex + 3]; // Alpha
                } break;
                case Image::RGB: {
                    packedData[destIndex + 0] = row[srcIndex + 0]; // Red
                    packedData[destIndex + 1] = row[srcIndex + 1]; // Green
                    packedData[destIndex + 2] = row[srcIndex + 2]; // Blue
                } break;
                case Image::SingleChannel: {
                    packedData[destIndex] = row[srcIndex];
                } break;
                default:
                    break;
                }
            }
        }

        return packedData;
    }

    void resized() override
    {
        spinner.setCentrePosition(getWidth() / 2, getHeight() / 2);
    }

    static void setScreenScale(float const scaleFactor)
    {
        scale = scaleFactor;
    }

private:
    bool roundTop, roundBottom;
    URL imageURL;
    hash32 imageHash;
    Image scaledImage;
    Spinner spinner;

    static inline float scale = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnlineImage)
};

class PatchDisplay final : public Component {
public:
    PatchDisplay(PatchInfo const& patchInfo, std::function<void(PatchInfo const&)> const& clickCallback, int const statusFlag)
        : image(true, false)
        , callback(clickCallback)
        , info(patchInfo)
        , isInstalled(statusFlag >= 1)
        , needsUpdate(statusFlag >= 2)
    {
        image.setImageURL("https://plugdata.org/thumbnails/png/" + patchInfo.thumbnailUrl + ".png");
        addAndMakeVisible(image);
    }

    bool matchesQuery(String const& query) const
    {
        return query.isEmpty() || info.title.containsIgnoreCase(query) || info.author.containsIgnoreCase(query) || info.description.containsIgnoreCase(query);
    }

private:
    void paintOverChildren(Graphics& g) override
    {
        auto const b = getLocalBounds().reduced(6);
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(6);

        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(hash("patch_display"), g, p, Colour(0, 0, 0).withAlpha(0.4f), 7, { 0, 1 });

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

        g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId).withAlpha(0.5f));
        g.fillRect(nameArea);

        auto platformArea = nameArea.removeFromRight(80).reduced(4).toFloat();
        platformArea.removeFromLeft(10);

        auto const textColour = findColour(PlugDataColour::panelTextColourId);
        Fonts::drawText(g, "by " + info.author, nameArea.withTrimmedLeft(10), textColour, 13.5f, Justification::left);

        auto textBounds = b.reduced(10, 4);

        Fonts::drawStyledText(g, info.title, textBounds.removeFromTop(30), textColour, Bold, 15, Justification::left);

        auto layout = TextLayout();
        auto description = AttributedString(info.description);
        description.setFont(Font(14.5f));
        description.setColour(textColour);
        layout.createLayout(description, textBounds.getWidth(), 150);

        layout.draw(g, textBounds.withTrimmedBottom(32).toFloat());

        auto const bottomRow = b.removeFromBottom(32).reduced(11);
        Fonts::drawStyledText(g, info.price, bottomRow, textColour, Semibold, 15, Justification::centredLeft);

        if (needsUpdate) {
            Fonts::drawStyledText(g, "Update available", bottomRow, textColour, Semibold, 15, Justification::centredRight);
        } else if (isInstalled) {
            Fonts::drawStyledText(g, "Installed", bottomRow, textColour, Semibold, 15, Justification::centredRight);
        }
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
    bool isInstalled;
    bool needsUpdate;
};

class PatchContainer final : public Component {
    int const displayWidth = 260;
    int const displayHeight = 315;

    OwnedArray<PatchDisplay> patchDisplays;
    HeapArray<std::pair<PatchInfo, int>> patches;

public:
    std::function<void(PatchInfo const&)> patchClicked;

    void filterPatches(String const& query)
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

    void sortPatches()
    {
        for (auto& [patch, flags] : patches) {
            flags = patch.isPatchInstalled() + 2 * patch.updateAvailable();
        }

        std::ranges::sort(patches, [](std::pair<PatchInfo, int> const& first, std::pair<PatchInfo, int> const& second) {
            auto& [patchA, flagsA] = first;
            auto& [patchB, flagsB] = second;

            if (flagsA > flagsB)
                return true;
            if (flagsA < flagsB)
                return false;

            return patchA.releaseDate > patchB.releaseDate;
        });
        showPatches(patches);
    }

    void showPatches(HeapArray<std::pair<PatchInfo, int>> const& patchesToShow)
    {
        patches = patchesToShow;

        patchDisplays.clear();

        for (auto& patch : patches) {
            auto* display = patchDisplays.add(new PatchDisplay(patch.first, patchClicked, patch.second));
            addAndMakeVisible(display);
        }

        setSize(getWidth(), patches.size() / (getWidth() / displayWidth) * (displayHeight + 8) + 12);
        resized(); // Even if size if the same, we still want to call resize
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6);

        auto currentRow = bounds.removeFromTop(displayHeight);

        auto const numColumns = currentRow.getWidth() / displayWidth;
        auto const extraSpace = currentRow.getWidth() - numColumns * displayWidth;
        auto const padding = extraSpace / numColumns / 2;

        for (auto* display : patchDisplays) {
            if (!display->isVisible())
                continue;

            if (currentRow.getWidth() < displayWidth) {
                bounds.removeFromTop(8);
                currentRow = bounds.removeFromTop(displayHeight);
            }

            currentRow.removeFromLeft(padding);
            display->setBounds(currentRow.removeFromLeft(displayWidth));
            currentRow.removeFromLeft(padding);
        }
    }

    HeapArray<std::pair<PatchInfo, int>> getPatches()
    {
        return patches;
    }
};

class PatchFullDisplay final : public Component
    , public DownloadPool::DownloadListener {
    PatchInfo currentPatch;
    hash32 patchHash;
    File const patchesDir = ProjectInfo::appDataDir.getChildFile("Patches");
    PatchContainer morePatches;

    class Viewport final : public BouncingViewport {
        void paint(Graphics& g) override
        {
            g.fillAll(findColour(PlugDataColour::panelForegroundColourId));
        }
    };
    Viewport viewport;

    class LinkButton final : public TextButton {
    public:
        enum Type {
            UpdateAvailable,
            AlreadyInstalled,
            Download,
            Store,
            View,
            Cancel
        };
        Type type;

        LinkButton(Type const type)
            : type(type)
        {
            setClickingTogglesState(true);
            setConnectedEdges(12);
        }

        String getIcon() const
        {
            if (type == AlreadyInstalled)
                return isMouseOver() ? Icons::Reset : Icons::Checkmark;
            if (type == Download || type == UpdateAvailable)
                return Icons::Download;
            if (type == Store)
                return Icons::Store;
            if (type == View)
                return Icons::Info;
            if (type == Cancel)
                return {};
            return {};
        }

        String getText() const
        {
            if (type == UpdateAvailable)
                return "Update";
            if (type == AlreadyInstalled)
                return isMouseOver() ? "Reinstall" : "Installed";
            if (type == Download)
                return "Download";
            if (type == Store)
                return "View in store";
            if (type == View)
                return "View online";
            if (type == Cancel)
                return "Cancel";
            return {};
        }

        void setType(Type const newType)
        {
            type = newType;
            repaint();
        }

        void paint(Graphics& g) override
        {
            auto const b = getLocalBounds().reduced(2.0f, 4.0f).toFloat();

            auto const mouseOver = isMouseOver();
            auto fillColour = findColour(PlugDataColour::toolbarActiveColourId);
            auto outlineColour = fillColour;
            auto const greyColour = findColour(PlugDataColour::panelActiveBackgroundColourId);

            if (type == Cancel) {
                fillColour = greyColour;
                outlineColour = greyColour;
            } else if (type == View) {
                fillColour = mouseOver ? greyColour.contrasting(0.6f) : findColour(PlugDataColour::panelBackgroundColourId).withAlpha(0.0f);
                outlineColour = greyColour.contrasting(0.6f);
            }

            auto const textColour = fillColour.contrasting(0.96f);

            if (mouseOver) {
                fillColour = fillColour.brighter(0.4f);
                outlineColour = outlineColour.brighter(0.4f);
            }

            g.setColour(fillColour);
            g.fillRoundedRectangle(b, Corners::defaultCornerRadius);

            g.setColour(outlineColour);
            g.drawRoundedRectangle(b, Corners::defaultCornerRadius, 1.5f);

            auto const boldFont = Fonts::getBoldFont().withHeight(14.0f);
            auto const iconFont = Fonts::getIconFont().withHeight(14.0f);

            // Draw icon and text
            AttributedString attrStr;
            attrStr.setJustification(Justification::centred);
            attrStr.append(getIcon(), iconFont, textColour);
            attrStr.append("  " + getText(), boldFont, textColour);
            attrStr.draw(g, b);
        }
    };

public:
    PatchFullDisplay()
        : image(true, true)
    {
        setVisible(true);
        addAndMakeVisible(viewButton);
        addAndMakeVisible(downloadButton);
        addAndMakeVisible(image);
        addAndMakeVisible(morePatches);

        morePatches.patchClicked = [this](PatchInfo const& patch) {
            showPatch(patch, patches);
        };

        setSize(840, 1084);

        viewport.setScrollBarsShown(true, false);
        viewport.setViewedComponent(this, false);

        downloadButton.onClick = [this] {
            if (downloadProgress == 0) {
                repaint();

                if (currentPatch.isPatchArchive()) {
                    downloadProgress = 1;
                    DownloadPool::getInstance()->downloadPatch(patchHash, currentPatch);
                } else {
                    URL(currentPatch.download).launchInDefaultBrowser();
                }
            } else {
                DownloadPool::getInstance()->cancelDownload(patchHash);
            }
        };

        viewButton.onClick = [this] {
            URL("https://plugdata.org/store-item.html?id=" + currentPatch.title).launchInDefaultBrowser();
        };

        DownloadPool::getInstance()->addDownloadListener(this);
    }

    ~PatchFullDisplay() override
    {
        DownloadPool::getInstance()->removeDownloadListener(this);
    }

    void downloadProgressed(hash32 const downloadHash, float const progress) override
    {
        if (downloadHash == patchHash) {
            downloadButton.setType(LinkButton::Cancel);
            downloadProgress = std::max<int>(progress * 100, 1);
            repaint();
        }
    }

    void patchDownloadCompleted(hash32 const downloadHash, bool const success) override
    {
        if (downloadHash == patchHash) {
            downloadProgress = 0;
            auto* parent = viewport.getParentComponent();
            if (success) {
                Dialogs::showMultiChoiceDialog(&confirmationDialog, parent, "Successfully installed " + currentPatch.title, [](int) { }, { "Dismiss" }, Icons::Checkmark);
            } else {
                Dialogs::showMultiChoiceDialog(&confirmationDialog, parent, "Failed to install " + currentPatch.title, [](int) { }, { "Dismiss" });
            }

            downloadButton.setType(success ? LinkButton::AlreadyInstalled : LinkButton::Download);
        }
    }

    PatchFullDisplay::Viewport& getViewport()
    {
        return viewport;
    }

    void showPatch(PatchInfo const& patchInfo, HeapArray<std::pair<PatchInfo, int>> const& allPatches)
    {
        downloadProgress = 0;
        patchHash = hash(patchInfo.title);
        patches = allPatches;
        currentPatch = patchInfo;

        auto fileName = URL(currentPatch.download).getFileName();

        if (currentPatch.updateAvailable()) {
            downloadButton.setType(LinkButton::UpdateAvailable);
        } else if (currentPatch.isPatchInstalled()) {
            downloadButton.setType(LinkButton::AlreadyInstalled);
        } else if (currentPatch.isPatchArchive()) {
            downloadButton.setType(LinkButton::Download);
        } else {
            downloadButton.setType(LinkButton::Store);
        }

        image.setImageURL("https://plugdata.org/thumbnails/png/" + patchInfo.thumbnailUrl + ".png");
        viewport.setVisible(true);

        morePatches.showPatches(filterPatches(patchInfo, allPatches));
    }

    static HeapArray<std::pair<PatchInfo, int>> filterPatches(PatchInfo const& targetPatch, HeapArray<std::pair<PatchInfo, int>> toFilter)
    {
        std::ranges::shuffle(toFilter, std::random_device());

        HeapArray<std::pair<PatchInfo, int>> result;
        for (auto& [patch, flags] : toFilter) {
            if (result.size() >= 3)
                break;
            if (flags == 0 && targetPatch.author == patch.author && patch.title != targetPatch.title) {
                result.add({ patch, 0 });
            }
        }

        if (result.size() < 3) {
            for (auto& [patch, flags] : toFilter) {
                if (result.size() >= 3)
                    break;
                if (flags == 0 && patch.title != targetPatch.title)
                    result.add({ patch, 0 });
            }
        }

        return result;
    }

    void paintOverChildren(Graphics& g) override
    {
        // Drag image outline
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(image.getBounds().toFloat(), Corners::largeCornerRadius, 1.0f);

        if (downloadProgress != 0.0f) {
            g.setFont(Fonts::getCurrentFont().withHeight(14.0f));
            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f));
            g.drawText("Installing: " + String(downloadProgress) + "%", downloadButton.getBounds().translated(0, 30), Justification::centred);

            auto bounds = downloadButton.getBounds().reduced(2, 4);

            g.saveState();
            Path clipPath;
            clipPath.addRoundedRectangle(bounds, Corners::defaultCornerRadius);
            g.reduceClipRegion(clipPath);

            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillRect(bounds.removeFromBottom(4).withWidth(bounds.getWidth() * (downloadProgress / 100.0f)));
            g.restoreState();
        }
    }

    void paint(Graphics& g) override
    {
        auto const b = getLocalBounds().reduced(12);

        g.fillAll(findColour(PlugDataColour::panelForegroundColourId));

        auto contentArea = b.reduced(20, 6);
        auto const textColour = findColour(PlugDataColour::panelTextColourId);
        g.setColour(textColour);

        g.setFont(Fonts::getBoldFont().withHeight(26));
        g.drawText(currentPatch.title, contentArea.removeFromTop(40), Justification::centredLeft);

        g.setFont(Fonts::getCurrentFont().withHeight(16.5f));
        g.drawText("by " + currentPatch.author, contentArea.removeFromTop(24), Justification::centredLeft);

        contentArea.removeFromTop(8);

        // Separator Line
        g.setColour(textColour.withAlpha(0.25f));
        g.drawHorizontalLine(contentArea.getY(), contentArea.getX(), contentArea.getRight());

        contentArea.removeFromTop(8);

        auto layout = TextLayout();
        AttributedString descriptionText(currentPatch.description);
        descriptionText.setFont(Font(15.5f));
        descriptionText.setColour(textColour);
        layout.createLayout(descriptionText, contentArea.getWidth(), contentArea.getHeight());

        layout.draw(g, contentArea.removeFromTop(30).translated(0, 4).toFloat());

        auto extraInfoBounds = contentArea.removeFromTop(72).reduced(0, 12).translated(0, -4);
        Path p;
        p.addRoundedRectangle(extraInfoBounds, Corners::largeCornerRadius);
        StackShadow::renderDropShadow(hash("patch_extra_info"), g, p, Colour(0, 0, 0).withAlpha(0.1f), 7, { 0, 1 });

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillPath(p); // Adjust the thickness as needed

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(p, PathStrokeType(0.5f)); // Adjust the thickness as needed

        auto const hasSizeInfo = currentPatch.size.isNotEmpty();
        int extraInfoItemWidth = getWidth() / (hasSizeInfo ? 3 : 2);
        auto drawExtraInfo = [this, extraInfoItemWidth, &extraInfoBounds](Graphics& g, String const& icon, String const& label, String const& value) mutable {
            auto infoBounds = extraInfoBounds.removeFromLeft(extraInfoItemWidth).withSizeKeepingCentre(110, 32).translated(-12, 0);

            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.setFont(Fonts::getIconFont().withHeight(15));
            g.drawText(icon, infoBounds.removeFromLeft(24), Justification::centredLeft);

            g.setFont(Fonts::getBoldFont().withHeight(15));
            g.drawText(label, infoBounds.removeFromTop(16), Justification::centredLeft);

            g.setFont(Fonts::getDefaultFont().withHeight(15));
            g.drawText(value, infoBounds, Justification::centredLeft);
        };

        if (hasSizeInfo) {
            drawExtraInfo(g, Icons::Storage, "Size", currentPatch.size);
        }
        drawExtraInfo(g, Icons::Money, "Price", currentPatch.price);
        drawExtraInfo(g, Icons::Time, "Release date", currentPatch.releaseDate);

        auto const imageBounds = contentArea.removeFromTop(500).withSizeKeepingCentre(getWidth(), 500);
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRect(imageBounds);

        contentArea.removeFromTop(12);

        g.setColour(textColour);
        g.setFont(Fonts::getSemiBoldFont().withHeight(16.5f));
        g.drawText("More patches", contentArea.removeFromTop(30), Justification::centredLeft);

        g.setColour(textColour.withAlpha(0.25f));
        g.drawHorizontalLine(contentArea.getY(), contentArea.getX(), contentArea.getRight());
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(20, 6);
        auto buttonBounds = b.removeFromTop(204).removeFromTop(40).translated(-11, 12);

        image.setBounds(b.removeFromTop(500).withSizeKeepingCentre(460, 450));

        b.removeFromTop(40);

        morePatches.setBounds(b.withHeight(morePatches.getHeight()));

        downloadButton.setBounds(buttonBounds.removeFromRight(110));
        buttonBounds.removeFromRight(14);
        viewButton.setBounds(buttonBounds.removeFromRight(110));
    }

    OnlineImage image;
    LinkButton downloadButton = LinkButton(LinkButton::Download);
    LinkButton viewButton = LinkButton(LinkButton::View);

    int downloadProgress = 0;
    std::unique_ptr<Dialog> confirmationDialog;
    HeapArray<std::pair<PatchInfo, int>> patches;
};

struct PatchStore final : public Component
    , public DownloadPool::DownloadListener {

    PatchContainer patchContainer;
    BouncingViewport contentViewport;
    PatchFullDisplay patchFullDisplay;

    MainToolbarButton backButton = MainToolbarButton(Icons::Back);
    MainToolbarButton refreshButton = MainToolbarButton(Icons::Refresh);
    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);

    SearchEditor input;
    Spinner spinner;

    PatchStore()
    {
        contentViewport.setViewedComponent(&patchContainer, false);
        patchContainer.setVisible(true);
        addAndMakeVisible(contentViewport);

        contentViewport.setScrollBarsShown(true, false, true, false);

        spinner.startSpinning();
        DownloadPool::getInstance()->downloadDatabase();

        addChildComponent(patchFullDisplay.getViewport());

        patchContainer.patchClicked = [this](PatchInfo const& patch) {
            patchFullDisplay.showPatch(patch, patchContainer.getPatches());
            backButton.setVisible(true);
            refreshButton.setVisible(false);
            input.setVisible(false);
            input.setText("", sendNotification);
            searchButton.setVisible(false);
        };

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this] {
            if (searchButton.getToggleState()) {
                input.setVisible(true);
                input.grabKeyboardFocus();
                input.setText("");

            } else {
                input.setVisible(false);
            }
        };
        addAndMakeVisible(searchButton);

        addChildComponent(backButton);

        backButton.onClick = [this] {
            patchFullDisplay.getViewport().setVisible(false);
            patchContainer.sortPatches();
            backButton.setVisible(false);
            refreshButton.setVisible(true);
            input.setVisible(false);
            input.setText("", sendNotification);
            searchButton.setVisible(true);
        };

        backButton.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        backButton.setColour(TextButton::buttonOnColourId, Colours::transparentBlack);

        refreshButton.setTooltip("Refresh packages");
        refreshButton.setEnabled(false);
        addAndMakeVisible(refreshButton);
        refreshButton.onClick = [this] {
            DownloadPool::getInstance()->cancelImageDownloads();
            spinner.startSpinning();
            DownloadPool::getInstance()->downloadDatabase();
            refreshButton.setEnabled(false);
        };

        input.setTextToShowWhenEmpty("Type to search for patches", findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f));
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::panelTextColourId));
        input.setBorder({ 1, 3, 5, 1 });
        input.setJustification(Justification::centredLeft);
        input.onTextChange = [this] {
            patchContainer.filterPatches(input.getText());
        };
        input.onFocusLost = [this] {
            if (searchButton.isMouseOver()) {
                return;
            }

            if (input.getText().isEmpty()) {
                searchButton.setToggleState(false, dontSendNotification);
                input.setVisible(false);
            }
        };
        addChildComponent(input);
        addChildComponent(spinner);
        DownloadPool::getInstance()->addDownloadListener(this);
    }

    ~PatchStore() override
    {
        DownloadPool::getInstance()->removeDownloadListener(this);
        DownloadPool::getInstance()->cancelImageDownloads();
    }

    void paint(Graphics& g) override
    {
        OnlineImage::setScreenScale(g.getInternalContext().getPhysicalPixelScaleFactor());

        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        auto const bounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);

        Fonts::drawStyledText(g, "Discover", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 40, getWidth(), 40);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().removeFromTop(40).reduced(48, 5).withTrimmedRight(40));

        auto const b = getLocalBounds().withTrimmedTop(40);

        patchFullDisplay.getViewport().setBounds(b);

        contentViewport.setBounds(b);

        patchContainer.setSize(getWidth(), patchContainer.getHeight());

        backButton.setBounds(2, 0, 40, 40);
        refreshButton.setBounds(2, 0, 40, 40);
        searchButton.setBounds(getWidth() - 82, 0, 40, 40);

        spinner.setSize(50, 50);
        spinner.setCentrePosition(b.getWidth() / 2, b.getHeight() / 2);
    }

    void databaseDownloadCompleted(HeapArray<std::pair<PatchInfo, int>> const& patches) override
    {
        patchContainer.showPatches(patches);
        refreshButton.setEnabled(true);
        spinner.stopSpinning();
    }
};
