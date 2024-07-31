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

    class WelcomePanelTile : public Component {
        WelcomePanel& parent;
        float snapshotScale;
        bool isHovered = false;
        String tileName, tileSubtitle;
        std::unique_ptr<Drawable> snapshot = nullptr;
        NVGImage titleImage, subtitleImage;
        
    public:
        bool isFavourited;
        std::function<void()> onClick = []() {};
        std::function<void(bool)> onFavourite = nullptr;

        WelcomePanelTile(WelcomePanel& welcomePanel, String name, String subtitle, String svgImage, Colour iconColour, float scale, bool favourited)
            : parent(welcomePanel)
            , snapshotScale(scale)
            , tileName(name)
            , tileSubtitle(subtitle)
            , isFavourited(favourited)
        {
            snapshot = Drawable::createFromImageData(svgImage.toRawUTF8(), svgImage.getNumBytesAsUTF8());
            if (snapshot) {
                snapshot->replaceColour(Colours::black, iconColour);
            }

            resized();
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(12);

            auto* nvg = dynamic_cast<NanoVGGraphicsContext&>(g.getInternalContext()).getContext();
            parent.drawShadow(nvg, getWidth(), getHeight());
            
            Path tilePath;
            tilePath.addRoundedRectangle(bounds.getX() + 1, bounds.getY() + 1, bounds.getWidth() - 2, bounds.getHeight() - 2, Corners::largeCornerRadius);
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillPath(tilePath);

            if (snapshot) {
                snapshot->drawAt(g, 0, 0, 1.0f);
            }

            Path textAreaPath;
            textAreaPath.addRoundedRectangle(bounds.getX(), bounds.getHeight() - 32, bounds.getWidth(), 44, Corners::largeCornerRadius, Corners::largeCornerRadius, false, false, true, true);

            auto hoverColour = findColour(PlugDataColour::toolbarHoverColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.5f);
            g.setColour(isHovered ? hoverColour : findColour(PlugDataColour::toolbarBackgroundColourId));
            g.fillPath(textAreaPath);

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.strokePath(tilePath, PathStrokeType(1.0f));
            
            auto textWidth = bounds.getWidth() - 8;
            if (titleImage.needsUpdate(textWidth * 2, 24 * 2) || subtitleImage.needsUpdate(textWidth * 2, 16 * 2)) {
                auto textColour = findColour(PlugDataColour::panelTextColourId);
                titleImage = NVGImage(nvg, textWidth * 2, 24 * 2, [this, textColour, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(textColour);
                    g.setFont(Fonts::getBoldFont().withHeight(14));
                    g.drawText(tileName, Rectangle<int>(0, 0, textWidth, 24), Justification::centredLeft, true);
                });

                subtitleImage = NVGImage(nvg, textWidth * 2, 16 * 2, [this, textColour, textWidth](Graphics& g) {
                    g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                    g.setColour(textColour.withAlpha(0.75f));
                    g.setFont(Fonts::getDefaultFont().withHeight(13.5f));
                    g.drawText(tileSubtitle, Rectangle<int>(0, 0, textWidth, 16), Justification::centredLeft, true);
                });
            }

            {
                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, 22, bounds.getHeight() - 30);
                titleImage.render(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 24));
                nvgTranslate(nvg, 0, 20);
                subtitleImage.render(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 16));
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

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));
        triggerAsyncUpdate();
    }
        
    void drawShadow(NVGcontext* nvg, int width, int height)
    {
        // We only need one shadow image, because all tiles have the same size
        if(shadowImage.needsUpdate(width * 2.0f, height * 2.0f)) {
            shadowImage = NVGImage(nvg, width * 2.0f, height * 2.0f, [width, height](Graphics& g){
                g.addTransform(AffineTransform::scale(2.0f, 2.0f));
                Path tilePath;
                tilePath.addRoundedRectangle(12.5f, 12.5f, width - 25.0f, height - 25.0f, Corners::largeCornerRadius);
                StackShadow::renderDropShadow(g, tilePath, Colour(0, 0, 0).withAlpha(0.08f), 6, { 0, 1 });
            });
        }
        
        shadowImage.render(nvg, Rectangle<int>(width, height));
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(24).withTrimmedTop(36);
        auto rowBounds = bounds.removeFromTop(160);

        int const desiredTileWidth = 190;
        int const tileSpacing = 4;

        int totalWidth = bounds.getWidth();
        // Calculate the number of columns that can fit in the total width
        int numColumns = std::max(1, totalWidth / (desiredTileWidth + tileSpacing));
        // Adjust the tile width to fit within the available width
        int actualTileWidth = (totalWidth - (numColumns - 1) * tileSpacing) / numColumns;

        if (newPatchTile)
            newPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
        rowBounds.removeFromLeft(4);
        if (openPatchTile)
            openPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));

        bounds.removeFromTop(16);

        auto viewPos = recentlyOpenedViewport.getViewPosition();
        recentlyOpenedViewport.setBounds(getLocalBounds().withTrimmedTop(260));

        int numRows = (tiles.size() + numColumns - 1) / numColumns;
        int totalHeight = numRows * 160;

        auto scrollable = Rectangle<int>(24, 6, totalWidth + 24, totalHeight + 24);
        recentlyOpenedComponent.setBounds(scrollable);

        // Start positioning the tiles
        rowBounds = scrollable.removeFromTop(160);
        for (auto* tile : tiles) {
            if (tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = scrollable.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }

        for (auto* tile : tiles) {
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
                auto patchImage = subTree.getProperty("PatchImage").toString();

                auto favourited = subTree.hasProperty("Pinned") && static_cast<bool>(subTree.getProperty("Pinned"));
                auto snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);

                String silhoutteSvg;
                if (patchImage.isEmpty() && patchFile.existsAsFile()) {
                    silhoutteSvg = OfflineObjectRenderer::patchToSVG(patchFile.loadFileAsString());
                } else {
                    MemoryOutputStream ostream;
                    Base64::convertFromBase64(ostream, patchImage);
                    MemoryInputStream istream(ostream.getMemoryBlock());

                    while (!istream.isExhausted()) {
                        int const x = istream.readCompressedInt();
                        int const y = istream.readCompressedInt();
                        int const w = istream.readCompressedInt();
                        int const h = istream.readCompressedInt();
                        float const rad = Corners::objectCornerRadius;

                        silhoutteSvg += String::formatted("<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%.1f\" ry=\"%.1f\" />\n", x, y, w, h, rad, rad);
                    }
                    silhoutteSvg = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n" + silhoutteSvg + "</svg>";
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

                auto* tile = tiles.add(new WelcomePanelTile(*this, patchFile.getFileName(), timeDescription, silhoutteSvg, snapshotColour, 1.0f, favourited));
                tile->onClick = [this, patchFile]() mutable {
                    if(patchFile.existsAsFile()) {
                        editor->autosave->checkForMoreRecentAutosave(patchFile, editor, [this, patchFile]() {
                            editor->getTabComponent().openPatch(URL(patchFile));
                            SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                        });
                    }
                    else {
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

        auto gradient = nvgLinearGradient(nvg, 0, recentlyOpenedViewport.getY(), 0, recentlyOpenedViewport.getY() + 20, convertColour(findColour(PlugDataColour::panelBackgroundColourId)), nvgRGBAf(1, 1, 1, 0));

        nvgFillPaint(nvg, gradient);
        nvgFillRect(nvg, recentlyOpenedViewport.getX() + 8, recentlyOpenedViewport.getY(), recentlyOpenedViewport.getWidth() - 16, 20);

        nvgBeginPath(nvg);
        nvgFillColor(nvg, findNVGColour(PlugDataColour::panelTextColourId));
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(nvg, 30);
        nvgTextLetterSpacing(nvg, -0.275f);
        nvgFontFace(nvg, "Inter-Bold");
        nvgText(nvg, 35, 38, "Welcome to plugdata", nullptr);

        if(recentlyOpenedViewport.isVisible()) {
            nvgBeginPath(nvg);
            nvgFontSize(nvg, 24);
            nvgText(nvg, 35, 244, "Recently Opened", nullptr);
        }
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

    std::unique_ptr<NanoVGGraphicsContext> nvgContext = nullptr;

    NVGImage shadowImage;
    OwnedArray<WelcomePanelTile> tiles;
    PluginEditor* editor;
};
