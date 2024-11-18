/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include "Utility/Autosave.h"
#include "Utility/CachedTextRender.h"
#include "Utility/NanoVGGraphicsContext.h"
#include "Components/BouncingViewport.h"

class WelcomePanel : public Component
    , public NVGComponent
    , public AsyncUpdater {

    class TopFillAllRect : public Component {
        Colour bgCol;

    public:
        TopFillAllRect() { };

        void setBGColour(Colour const& col)
        {
            bgCol = col;
            repaint();
        }

        void paint(Graphics& g) override
        {
            g.fillAll(bgCol);
        }
    };

    class WelcomePanelTile : public Component {
        WelcomePanel& parent;
        float snapshotScale;
        bool isHovered = false;
        String tileName, tileSubtitle;
        std::unique_ptr<Drawable> snapshot = nullptr;
        NVGImage titleImage, subtitleImage, snapshotImage;

        Image thumbnailImageData;

        int lastWidth = -1;
        int lastHeight = -1;

    public:
        bool isFavourited;
        std::function<void()> onClick = []() { };
        std::function<void(bool)> onFavourite = nullptr;

        WelcomePanelTile(WelcomePanel& welcomePanel, String name, String subtitle, String svgImage, Colour iconColour, float scale, bool favourited, Image const& thumbImage = Image())
            : parent(welcomePanel)
            , snapshotScale(scale)
            , tileName(name)
            , tileSubtitle(subtitle)
            , thumbnailImageData(thumbImage)
            , isFavourited(favourited)
        {
            if (!thumbImage.isValid()) {
                snapshot = Drawable::createFromImageData(svgImage.toRawUTF8(), svgImage.getNumBytesAsUTF8());
                if (snapshot) {
                    snapshot->replaceColour(Colours::black, iconColour);
                }
            }

            resized();
        }
        
        void setSearchQuery(String const& searchQuery)
        {
            setVisible(tileName.containsIgnoreCase(searchQuery));
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(12);

            auto* nvg = dynamic_cast<NanoVGGraphicsContext&>(g.getInternalContext()).getContext();
            parent.drawShadow(nvg, getWidth(), getHeight());

            if (thumbnailImageData.isValid()) {
                if (!snapshotImage.isValid() || lastWidth != bounds.getWidth() || lastHeight != bounds.getHeight()) {
                    lastWidth = bounds.getWidth();
                    lastHeight = bounds.getHeight();

                    snapshotImage = NVGImage(nvg, bounds.getWidth() * 2, (bounds.getHeight() - 32) * 2, [this, bounds](Graphics& g) {
                        g.addTransform(AffineTransform::scale(2.0f));
                        if (thumbnailImageData.isValid()) {
                            auto imageWidth = thumbnailImageData.getWidth();
                            auto imageHeight = thumbnailImageData.getHeight();
                            auto componentWidth = bounds.getWidth();
                            auto componentHeight = bounds.getHeight();

                            float imageAspect = static_cast<float>(imageWidth) / imageHeight;
                            float componentAspect = static_cast<float>(componentWidth) / componentHeight;

                            int drawWidth, drawHeight;
                            int offsetX = 0, offsetY = 0;

                            if (componentAspect < imageAspect) {
                                // Component is wider than the image aspect ratio, scale based on height
                                drawHeight = componentHeight;
                                drawWidth = static_cast<int>(drawHeight * imageAspect);
                            } else {
                                // Component is taller than the image aspect ratio, scale based on width
                                drawWidth = componentWidth;
                                drawHeight = static_cast<int>(drawWidth / imageAspect);
                            }

                            // Calculate offsets to center the image
                            offsetX = (componentWidth - drawWidth) / 2;
                            offsetY = (componentHeight - drawHeight - 32) / 2;

                            g.drawImage(thumbnailImageData, offsetX, offsetY, drawWidth, drawHeight, 0, 0, imageWidth, imageHeight);
                        }
                    });
                }
            } else {
                if (snapshot && !snapshotImage.isValid()) {
                    snapshotImage = NVGImage(nvg, bounds.getWidth() * 2, (bounds.getHeight() - 32) * 2, [this](Graphics& g) {
                        g.addTransform(AffineTransform::scale(2.0f));
                        snapshot->drawAt(g, 0, 0, 1.0f);
                    });
                }
            }

            nvgSave(nvg);
            auto sB = bounds.toFloat().reduced(0.2f);
            nvgRoundedScissor(nvg, sB.getX(), sB.getY(), sB.getWidth(), sB.getHeight(), Corners::largeCornerRadius);

            auto lB = bounds.toFloat().expanded(0.5f);
            // Draw background even for images incase there is a transparent PNG
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), convertColour(findColour(PlugDataColour::canvasBackgroundColourId)), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);
            if (thumbnailImageData.isValid()) {
                // Render the thumbnail image file that is in the root dir of the pd patch
                auto sB = bounds.toFloat().reduced(0.2f);
                snapshotImage.render(nvg, Rectangle<int>(sB.getX(), sB.getY(), sB.getWidth(), sB.getHeight() - 32));
            } else {
                // Otherwise render the generated snapshot
                snapshotImage.render(nvg, bounds.withTrimmedBottom(32));
            }
            nvgRestore(nvg);

            // Draw border around
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), nvgRGBA(0, 0, 0, 0), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);

            auto hoverColour = findColour(PlugDataColour::toolbarHoverColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.5f);

            nvgBeginPath(nvg);
            nvgRoundedRectVarying(nvg, bounds.getX(), bounds.getHeight() - 32, bounds.getWidth(), 44, 0.0f, 0.0f, Corners::largeCornerRadius, Corners::largeCornerRadius);
            nvgFillColor(nvg, convertColour(isHovered ? hoverColour : findColour(PlugDataColour::toolbarBackgroundColourId)));
            nvgFill(nvg);
            nvgStrokeColor(nvg, convertColour(findColour(PlugDataColour::toolbarOutlineColourId)));
            nvgStroke(nvg);

            auto textWidth = bounds.getWidth() - 8;
            if (titleImage.needsUpdate(textWidth * 2, 24 * 2) || subtitleImage.needsUpdate(textWidth * 2, 16 * 2)) {
                titleImage = NVGImage(nvg, textWidth * 2, 24 * 2, [this, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getBoldFont().withHeight(14));
                    g.drawText(tileName, Rectangle<int>(0, 0, textWidth, 24), Justification::centredLeft, true);
                }, NVGImage::AlphaImage);

                subtitleImage = NVGImage(nvg, textWidth * 2, 16 * 2, [this, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getDefaultFont().withHeight(13.5f));
                    g.drawText(tileSubtitle, Rectangle<int>(0, 0, textWidth, 16), Justification::centredLeft, true);
                }, NVGImage::AlphaImage);
            }

            {
                auto textColour = findColour(PlugDataColour::panelTextColourId);
                
                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, 22, bounds.getHeight() - 30);
                titleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 24), convertColour(textColour));
                nvgTranslate(nvg, 0, 20);
                subtitleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 16), convertColour(textColour.withAlpha(0.75f)));
            }

            if (onFavourite) {
                auto favouriteIconBounds = getHeartIconBounds();
                nvgFontFace(nvg, "icon_font-Regular");

                if (isFavourited) {
                    nvgFillColor(nvg, nvgRGBA(250, 50, 40, 200));
                    nvgText(nvg, favouriteIconBounds.getX(), favouriteIconBounds.getY() + 14, Icons::HeartFilled.toRawUTF8(), nullptr);
                } else if (isMouseOver()) {
                    nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                    nvgText(nvg, favouriteIconBounds.getX(), favouriteIconBounds.getY() + 14, Icons::HeartStroked.toRawUTF8(), nullptr);
                }
            }
        }

        Rectangle<int> getHeartIconBounds()
        {
            return Rectangle<int>(20, getHeight() - 80, 16, 16);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isHovered = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isHovered = false;
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;

            if (onFavourite && getHeartIconBounds().contains(e.x, e.y)) {
                isFavourited = !isFavourited;
                onFavourite(isFavourited);
                repaint();
            } else {
                onClick();
            }
        }

        void resized() override
        {
            if (snapshot) {
                auto bounds = getLocalBounds().reduced(12).withTrimmedBottom(44);
                snapshot->setTransformToFit(bounds.withSizeKeepingCentre(bounds.getWidth() * snapshotScale, bounds.getHeight() * snapshotScale).toFloat(), RectanglePlacement::centred);
            }
        }
    };

public:
    WelcomePanel(PluginEditor* pluginEditor)
        : NVGComponent(this)
        , editor(pluginEditor)
    {
        recentlyOpenedViewport.setViewedComponent(&recentlyOpenedComponent, false);
        recentlyOpenedViewport.setScrollBarsShown(true, false, false, false);
        recentlyOpenedComponent.setVisible(true);
#if JUCE_IOS
        recentlyOpenedViewport.setVisible(OSUtils::isIPad());
#else
        recentlyOpenedViewport.setVisible(true);
#endif

        addChildComponent(recentlyOpenedViewport);

        // A top rectangle component that hides anything behind (we use this instead of scissoring)
        topFillAllRect.setBGColour(findColour(PlugDataColour::panelBackgroundColourId));

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));
        triggerAsyncUpdate();
    }

    void drawShadow(NVGcontext* nvg, int width, int height)
    {
        // We only need one shadow image, because all tiles have the same size
        if (shadowImage.needsUpdate(width * 2.0f, height * 2.0f)) {
            shadowImage = NVGImage(nvg, width * 2.0f, height * 2.0f, [width, height](Graphics& g) {
                g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                Path tilePath;
                tilePath.addRoundedRectangle(12.5f, 12.5f, width - 25.0f, height - 25.0f, Corners::largeCornerRadius);
                StackShadow::renderDropShadow(0, g, tilePath, Colours::white.withAlpha(0.12f), 6, { 0, 1 }); }, NVGImage::AlphaImage);
            repaint();
        }

        shadowImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, width, height), nvgRGB(0, 0, 0));
    }
        
    void setSearchQuery(String const& newSearchQuery)
    {
        searchQuery = newSearchQuery;
        if(newPatchTile) newPatchTile->setVisible(searchQuery.isEmpty());
        if(openPatchTile) openPatchTile->setVisible(searchQuery.isEmpty());
        
        for (auto* tile : tiles) {
            tile->setSearchQuery(searchQuery);
        }
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);
        auto rowBounds = bounds.removeFromTop(160);

        int const desiredTileWidth = 190;
        int const tileSpacing = 4;

        int totalWidth = bounds.getWidth();
        // Calculate the number of columns that can fit in the total width
        int numColumns = std::max(1, totalWidth / (desiredTileWidth + tileSpacing));
        // Adjust the tile width to fit within the available width
        int actualTileWidth = (totalWidth - (numColumns - 1) * tileSpacing) / numColumns;

        if (newPatchTile && newPatchTile->isVisible())
            newPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
        rowBounds.removeFromLeft(4);
        if (openPatchTile && openPatchTile->isVisible())
            openPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));

        auto viewPos = recentlyOpenedViewport.getViewPosition();
        recentlyOpenedViewport.setBounds(getLocalBounds().withTrimmedTop((newPatchTile && newPatchTile->isVisible()) ? 200 : 0));

        // Place a rectangle directly behind the newTile & openTile so to hide any content that draws behind it.
        topFillAllRect.setBounds(0, 0, getWidth(), recentlyOpenedViewport.getY());

        int numRows = (tiles.size() + numColumns - 1) / numColumns;
        int totalHeight = numRows * 160;

        auto scrollable = Rectangle<int>(24, 6, totalWidth + 24, totalHeight + 24);
        recentlyOpenedComponent.setBounds(scrollable);
        
        // Start positioning the tiles
        rowBounds = scrollable.removeFromTop(160);
        for (auto* tile : tiles) {
            if(!tile->isVisible()) continue;
            if (tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = scrollable.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }

        for (auto* tile : tiles) {
            if(!tile->isVisible()) continue;
            if (!tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = scrollable.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }
        recentlyOpenedViewport.setViewPosition(viewPos);
    }

    void handleAsyncUpdate() override
    {
        newPatchTile = std::make_unique<WelcomePanelTile>(*this, "New Patch", "Create a new empty patch", newIcon, findColour(PlugDataColour::panelTextColourId), 0.33f, false);
        openPatchTile = std::make_unique<WelcomePanelTile>(*this, "Open Patch", "Browse for a patch to open", openIcon, findColour(PlugDataColour::panelTextColourId), 0.33f, false);

        newPatchTile->onClick = [this]() { editor->getTabComponent().newPatch(); };
        openPatchTile->onClick = [this]() { editor->getTabComponent().openPatch(); };

        topFillAllRect.setBGColour(findColour(PlugDataColour::panelBackgroundColourId));
        addAndMakeVisible(topFillAllRect);

        addAndMakeVisible(*newPatchTile);
        addAndMakeVisible(*openPatchTile);

        tiles.clear();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

        if (recentlyOpenedTree.isValid()) {
            // Place favourited patches at the top
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {

                auto subTree = recentlyOpenedTree.getChild(i);
                auto patchFile = File(subTree.getProperty("Path").toString());
                auto patchThumbnailBase = File(patchFile.getParentDirectory().getFullPathName() + "\\" + patchFile.getFileNameWithoutExtension() + "_thumb");

                auto favourited = subTree.hasProperty("Pinned") && static_cast<bool>(subTree.getProperty("Pinned"));
                auto snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);

                String silhoutteSvg;
                Image thumbImage;

                StringArray possibleExtensions { ".png", ".jpg", ".jpeg", ".gif" };

                for (auto& ext : possibleExtensions) {
                    auto patchThumbnail = patchThumbnailBase.withFileExtension(ext);
                    if (patchThumbnail.existsAsFile()) {
                        FileInputStream fileStream(patchThumbnail);
                        if (fileStream.openedOk()) {
                            thumbImage = ImageFileFormat::loadFrom(fileStream).convertedToFormat(Image::ARGB);
                            break;
                        }
                    }
                }
                if (thumbImage.isNull()) {
                    if (patchFile.existsAsFile()) {
                        silhoutteSvg = OfflineObjectRenderer::patchToSVG(patchFile.loadFileAsString());
                    }
                }

                auto openTime = Time(static_cast<int64>(subTree.getProperty("Time")));
                auto diff = Time::getCurrentTime() - openTime;
                String date;
                if (diff.inDays() == 0)
                    date = "Today";
                else if (diff.inDays() == 1)
                    date = "Yesterday";
                else
                    date = openTime.toString(true, false);
                String time = openTime.toString(false, true, false, true);
                String timeDescription = date + ", " + time;

                auto* tile = tiles.add(new WelcomePanelTile(*this, patchFile.getFileName(), timeDescription, silhoutteSvg, snapshotColour, 1.0f, favourited, thumbImage));
                tile->onClick = [this, patchFile]() mutable {
                    if (patchFile.existsAsFile()) {
                        editor->pd->autosave->checkForMoreRecentAutosave(patchFile, editor, [this, patchFile]() {
                            editor->getTabComponent().openPatch(URL(patchFile));
                            SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                        });
                    } else {
                        editor->pd->logError("Patch not found");
                    }
                };
                tile->onFavourite = [this, path = subTree.getProperty("Path")](bool shouldBeFavourite) mutable {
                    auto settingsTree = SettingsFile::getInstance()->getValueTree();
                    auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

                    // Settings file could be reloaded, we can't assume the old recently opened tree is still valid!
                    // So look up the entry by file path
                    auto subTree = recentlyOpenedTree.getChildWithProperty("Path", path);
                    subTree.setProperty("Pinned", shouldBeFavourite, nullptr);
                    resized();
                };
                recentlyOpenedComponent.addAndMakeVisible(tile);
            }
        }

        resized();
    }

    void show()
    {
        triggerAsyncUpdate();
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

    void render(NVGcontext* nvg) override
    {
        if (!nvgContext || nvgContext->getContext() != nvg)
            nvgContext = std::make_unique<NanoVGGraphicsContext>(nvg);

        nvgFillColor(nvg, convertColour(findColour(PlugDataColour::panelBackgroundColourId)));
        nvgFillRect(nvg, 0, 0, getWidth(), getHeight());

        Graphics g(*nvgContext);
        g.reduceClipRegion(editor->nvgSurface.getInvalidArea());
        paintEntireComponent(g, false);

        auto gradient = nvgLinearGradient(nvg, 0, recentlyOpenedViewport.getY(), 0, recentlyOpenedViewport.getY() + 20, convertColour(findColour(PlugDataColour::panelBackgroundColourId)), nvgRGBA(255, 255, 255, 0));

        nvgFillPaint(nvg, gradient);
        nvgFillRect(nvg, recentlyOpenedViewport.getX() + 8, recentlyOpenedViewport.getY(), recentlyOpenedViewport.getWidth() - 16, 20);
    }

    void lookAndFeelChanged() override
    {
        triggerAsyncUpdate();
    }

    static inline String const newIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
                                         "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n"
                                         "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"-10 0 2058 2048\">\n"
                                         "   <path fill=\"currentColor\"\n"
                                         "d=\"M1024 170v512q0 72 50 122t120 50h512v852q0 72 -50 122t-120 50h-1024q-70 0 -120 -50.5t-50 -121.5v-1364q0 -72 50 -122t120 -50h512zM1151 213l512 512h-469q-16 0 -29.5 -12.5t-13.5 -30.5v-469z\" />\n"
                                         "</svg>\n";

    static inline String const openIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
                                          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n"
                                          "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"-10 0 2058 2048\">\n"
                                          "   <path fill=\"currentColor\"\n"
                                          "d=\"M1180 555h506q72 0 126 47t64 118v13l2 14v768q0 76 -52 131t-128 60h-12h-1324q-76 0 -131 -51.5t-59 -127.5l-2 -12v-620l530 2l17 -2q51 -4 92 -33l4 -3t6 -5l4 -2zM700 342q59 0 109 32l14 11l181 149l-263 219l-8 4q-10 8 -24 11h-9h-530v-236q0 -76 52 -131\n"
                                          "t128 -59h12h338z\" />\n"
                                          "</svg>\n";

    std::unique_ptr<WelcomePanelTile> newPatchTile;
    std::unique_ptr<WelcomePanelTile> openPatchTile;

    Component recentlyOpenedComponent;
    BouncingViewport recentlyOpenedViewport;

    TopFillAllRect topFillAllRect;

    std::unique_ptr<NanoVGGraphicsContext> nvgContext = nullptr;

    NVGImage shadowImage;
    OwnedArray<WelcomePanelTile> tiles;
    PluginEditor* editor;
        
    String searchQuery;
};
