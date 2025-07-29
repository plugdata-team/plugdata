/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include "Utility/Autosave.h"
#include "Utility/CachedTextRender.h"
#include "Utility/NVGGraphicsContext.h"
#include "Components/BouncingViewport.h"
#include "Utility/PatchInfo.h"

class WelcomePanel final : public Component
    , public NVGComponent
    , public AsyncUpdater {
    class ContentComponent final : public Component {
        WelcomePanel& panel;
        Rectangle<int> clearButtonBounds;
        bool isHoveringClearButton = false;

    public:
        explicit ContentComponent(WelcomePanel& panel)
            : panel(panel)
        {
        }

        void paint(Graphics& g) override
        {
            auto* nvg = dynamic_cast<NVGGraphicsContext&>(g.getInternalContext()).getContext();

            if (panel.currentTab == Home && panel.searchQuery.isEmpty()) {
                if (panel.recentlyOpenedTiles.isEmpty()) {
                    nvgFontFace(nvg, "Inter-Bold");
                    nvgFontSize(nvg, 34);
                    nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                    nvgText(nvg, getWidth() / 2, getHeight() / 2 - 80, "Welcome to plugdata", nullptr);
                } else {
                    nvgFontFace(nvg, "Inter-Bold");
                    nvgFontSize(nvg, 14);
                    nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                    nvgText(nvg, 96, 138, "Recently Opened", nullptr);

                    nvgFontFace(nvg, "icon_font-Regular");
                    nvgFontSize(nvg, 14);
                    nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isHoveringClearButton ? 0.6f : 1.0f)));
                    nvgText(nvg, clearButtonBounds.getCentreX(), clearButtonBounds.getCentreY(), Icons::Clear.toRawUTF8(), nullptr);
                }
            }
        }

        void resized() override
        {
            clearButtonBounds = Rectangle<int>(getWidth() - 14, 138, 16, 16);
        }

        void mouseMove(MouseEvent const& e) override
        {
            bool const wasHoveringClearButton = isHoveringClearButton;
            isHoveringClearButton = clearButtonBounds.contains(e.getPosition());
            if (wasHoveringClearButton != isHoveringClearButton)
                repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            if(isHoveringClearButton) {
                isHoveringClearButton = false;
                repaint();
            }
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (clearButtonBounds.contains(e.getPosition())) {
                auto const settingsTree = SettingsFile::getInstance()->getValueTree();
                settingsTree.getChildWithName("RecentlyOpened").removeAllChildren(nullptr);
                panel.triggerAsyncUpdate();
            }
        }
    };

    class MainActionTile final : public Component {
        NVGImage shadowImage;
        bool isHovered = false;

    public:
        std::function<void()> onClick = [] { };
        enum TileType {
            New,
            Open,
            Store
        };
        TileType type;

        explicit MainActionTile(TileType const type)
            : type(type)
        {
        }

        void paint(Graphics& g) override
        {
            auto const bounds = getLocalBounds().reduced(12);
            auto* nvg = dynamic_cast<NVGGraphicsContext&>(g.getInternalContext()).getContext();

            auto const width = getWidth();
            auto const height = getHeight();
            // We only need one shadow image, because all tiles have the same size
            auto const scale = nvgCurrentPixelScale(nvg);
            if (shadowImage.needsUpdate(width * scale, height * scale)) {
                shadowImage = NVGImage(nvg, width * scale, height * scale, [width, height, scale](Graphics& g) {
                    g.addTransform(AffineTransform::scale(scale, scale));
                    Path tilePath;
                    tilePath.addRoundedRectangle(12.5f, 12.5f, width - 25.0f, height - 25.0f, Corners::largeCornerRadius);
                    StackShadow::renderDropShadow(0, g, tilePath, Colours::white.withAlpha(0.12f), 6, { 0, 1 }); }, NVGImage::AlphaImage);
                repaint();
            }

            shadowImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, width, height), nvgRGB(0, 0, 0));

            auto const lB = bounds.toFloat().expanded(0.5f);
            {
                auto const bgCol = !isHovered ? convertColour(findColour(PlugDataColour::panelForegroundColourId)) : convertColour(findColour(PlugDataColour::toolbarBackgroundColourId));

                // Draw border around
                nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), bgCol, convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);
            }

            auto const bgColour = findColour(PlugDataColour::panelForegroundColourId);
            auto const bgCol = convertColour(bgColour);
            auto const newOpenIconCol = convertColour(bgColour.contrasting().withAlpha(0.32f));
            constexpr auto iconSize = 48;
            constexpr auto iconHalf = iconSize * 0.5f;
            auto const circleBounds = Rectangle<int>(lB.getX() + 40 - iconHalf, lB.getCentreY() - iconHalf, iconSize, iconSize);

            // Background circle
            nvgDrawRoundedRect(nvg, circleBounds.getX(), circleBounds.getY(), iconSize, iconSize, newOpenIconCol, newOpenIconCol, iconHalf);
            switch (type) {
            case New: {
                // Draw a cross icon manually
                constexpr auto lineThickness = 4;
                constexpr auto lineRad = lineThickness * 0.5f;
                constexpr auto crossSize = 26;
                constexpr auto halfSize = crossSize * 0.5f;
                // Horizontal line
                nvgDrawRoundedRect(nvg, circleBounds.getCentreX() - halfSize, circleBounds.getCentreY() - lineRad, crossSize, lineThickness, bgCol, bgCol, lineRad);
                // Vertical line
                nvgDrawRoundedRect(nvg, circleBounds.getCentreX() - lineRad, circleBounds.getCentreY() - halfSize, lineThickness, crossSize, bgCol, bgCol, lineRad);

                nvgFontFace(nvg, "Inter-Bold");
                nvgFontSize(nvg, 12);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_LEFT);
                nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                nvgText(nvg, 92, 45, "New Patch", nullptr);

                nvgFontFace(nvg, "Inter-Regular");
                nvgText(nvg, 92, 63, "Create a new empty patch", nullptr);
                break;
            }

            case Open: {
                nvgFontFace(nvg, "icon_font-Regular");
                nvgFillColor(nvg, bgCol);
                nvgFontSize(nvg, 34);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(nvg, circleBounds.getCentreX(), circleBounds.getCentreY() - 4, Icons::Folder.toRawUTF8(), nullptr);

                nvgFontFace(nvg, "Inter-Bold");
                nvgFontSize(nvg, 12);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_LEFT);
                nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                nvgText(nvg, 92, 45, "Open Patch...", nullptr);

                nvgFontFace(nvg, "Inter-Regular");
                nvgText(nvg, 92, 63, "Browse for a patch to open", nullptr);
                break;
            }
            case Store: {
                nvgFontFace(nvg, "icon_font-Regular");
                nvgFillColor(nvg, bgCol);
                nvgFontSize(nvg, 30);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(nvg, circleBounds.getCentreX(), circleBounds.getCentreY() - 4, Icons::Sparkle.toRawUTF8(), nullptr);

                nvgFontFace(nvg, "Inter-Bold");
                nvgFontSize(nvg, 12);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_LEFT);
                nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::panelTextColourId)));
                nvgText(nvg, 92, 45, "Discover...", nullptr);

                nvgFontFace(nvg, "Inter-Regular");
                nvgText(nvg, 92, 63, "Browse online patch store", nullptr);
            }
            default:
                break;
            }
        }

        bool hitTest(int const x, int const y) override
        {
            return getLocalBounds().reduced(12).contains(Point<int>(x, y));
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
            if (!getScreenBounds().reduced(12).contains(e.getScreenPosition()))
                return;

            if (!e.mods.isLeftButtonDown())
                return;

            onClick();
        }
    };

    class WelcomePanelTile final : public Component {
    public:
        bool isFavourited;
        std::function<void()> onClick = [] { };
        std::function<void(bool)> onFavourite = nullptr;
        std::function<void()> onRemove = [] { };

    private:
        WelcomePanel& parent;
        float snapshotScale;
        bool isHovered = false;
        String tileName, tileSubtitle;
        std::unique_ptr<Drawable> snapshot = nullptr;
        NVGImage titleImage, subtitleImage, snapshotImage;

        Image thumbnailImageData;
        int lastWidth = -1;
        int lastHeight = -1;

        String creationTimeDescription = String();
        String modifiedTimeDescription = String();
        String accessedTimeDescription = String();
        String fileSizeDescription = String();

        UnorderedMap<String, File> previousVersions;

        File patchFile;

        enum TileType {
            Patch,
            LibraryPatch
        };

        TileType tileType = Patch;

    public:
        WelcomePanelTile(WelcomePanel& welcomePanel, File const& patchFile, String const& patchAuthor, float const scale, bool const favourited, Image const& thumbImage = Image())
            : isFavourited(favourited)
            , parent(welcomePanel)
            , snapshotScale(scale)
            , thumbnailImageData(thumbImage)
            , patchFile(patchFile)
        {
            tileName = patchFile.getFileNameWithoutExtension();
            tileSubtitle = patchAuthor;
            tileType = LibraryPatch;
            resized();
        }

        WelcomePanelTile(WelcomePanel& welcomePanel, ValueTree subTree, String svgImage, Colour iconColour, float const scale, bool const favourited, Image const& thumbImage = Image())
            : isFavourited(favourited)
            , parent(welcomePanel)
            , snapshotScale(scale)
            , thumbnailImageData(thumbImage)
        {
            patchFile = File(subTree.getProperty("Path").toString());
            tileName = patchFile.getFileNameWithoutExtension();

            auto is24Hour = OSUtils::is24HourTimeFormat();

            auto formatTimeDescription = [is24Hour](Time const& openTime, bool const showDayAndDate = false) {
                auto const now = Time::getCurrentTime();

                // Extract just the date part (YYYY-MM-DD) for comparison
                auto const openDate = openTime.toISO8601(true).substring(0, 10);
                auto const currentDate = now.toISO8601(true).substring(0, 10);
                auto const yesterday = (now - RelativeTime::days(1)).toISO8601(true).substring(0, 10);

                String dateOrDay;
                if (openDate == currentDate) {
                    dateOrDay = "Today";
                } else if (openDate == yesterday) {
                    dateOrDay = "Yesterday";
                }

                String const time = openTime.toString(false, true, false, is24Hour);

                if (showDayAndDate)
                    return (dateOrDay.isNotEmpty() ? dateOrDay + ", " : "") + openTime.toString(true, false) + ", " + time;

                return (dateOrDay.isNotEmpty() ? dateOrDay : openTime.toString(true, false)) + ", " + time;
            };

            auto const accessedInPlugdata = Time(static_cast<int64>(subTree.getProperty("Time")));

            tileSubtitle = formatTimeDescription(accessedInPlugdata);

            auto const fileSize = patchFile.getSize();

            if (fileSize < 1024) {
                fileSizeDescription = String(fileSize) + " Bytes"; // Less than 1 KiB
            } else if (fileSize < 1024 * 1024) {
                fileSizeDescription = String(fileSize / 1024.0, 2) + " KiB"; // Between 1 KiB and 1 MiB
            } else {
                fileSizeDescription = String(fileSize / (1024.0 * 1024.0), 2) + " MiB"; // 1 MiB or more
            }

            creationTimeDescription = formatTimeDescription(patchFile.getCreationTime(), true);
            modifiedTimeDescription = formatTimeDescription(patchFile.getLastModificationTime(), true);
            // Accessed time will show the last time the file was read, which is when the Home panel has been refreshed.
            // We need to show the time accessed from plugdata, which is saved in the settings XML
            // We want to show this again as well as in the subtile, but format it differently (with both Today/Yesterday and date)
            // because the popup menu may occlude the tile + subtitle
            accessedTimeDescription = formatTimeDescription(accessedInPlugdata, true);

            updateGeneratedThumbnailIfNeeded(thumbImage, svgImage);
        }

        WelcomePanelTile(WelcomePanel& welcomePanel, String const& name, String const& subtitle, String const& svgImage, Colour iconColour, float const scale, bool const favourited, Image const& thumbImage = Image())
            : isFavourited(favourited)
            , parent(welcomePanel)
            , snapshotScale(scale)
            , tileName(name)
            , tileSubtitle(subtitle)
            , thumbnailImageData(thumbImage)
        {
            updateGeneratedThumbnailIfNeeded(thumbImage, svgImage);
        }

        void updateGeneratedThumbnailIfNeeded(Image const& thumbnailImage, String const& svgImage)
        {
            if (!thumbnailImage.isValid()) {
                auto const snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);
                snapshot = Drawable::createFromImageData(svgImage.toRawUTF8(), svgImage.getNumBytesAsUTF8());
                if (snapshot) {
                    snapshot->replaceColour(Colours::black, snapshotColour);
                }
            }

            resized();
        }

        void setPreviousVersions(Array<std::pair<int64, File>> versions)
        {
            previousVersions.clear();

            for (auto& [time, file] : versions) {
                auto metaFile = file.getParentDirectory().getChildFile("meta.json");
                if (metaFile.existsAsFile()) {
                    auto const json = JSON::fromString(metaFile.loadFileAsString());
                    if (json.hasProperty("Version")) {
                        previousVersions[json["Version"].toString()] = file;
                    } else {
                        previousVersions["Added " + Time(time).toString(true, false)] = file;
                    }
                }
            }
        }

        void setHovered()
        {
            isHovered = true;
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isRightButtonDown())
                return;

            PopupMenu tileMenu;

            if (tileType == LibraryPatch) {
                tileMenu.addItem(PlatformStrings::getBrowserTip(), [this] {
                    if (patchFile.existsAsFile())
                        patchFile.revealToUser();
                });

                tileMenu.addSeparator();

                auto metaFile = patchFile.getParentDirectory().getChildFile("meta.json");
                if (metaFile.existsAsFile()) {

                    auto const json = JSON::fromString(metaFile.loadFileAsString());
                    auto const patchInfo = PatchInfo(json);

                    PopupMenu patchInfoSubMenu;
                    patchInfoSubMenu.addItem("Title: " + patchInfo.title, false, false, nullptr);
                    patchInfoSubMenu.addItem("Author: " + patchInfo.author, false, false, nullptr);
                    patchInfoSubMenu.addItem("Released: " + patchInfo.releaseDate, false, false, nullptr);
                    patchInfoSubMenu.addItem("About: " + patchInfo.description, false, false, nullptr);
                    if (patchInfo.version.isNotEmpty()) {
                        patchInfoSubMenu.addItem("Version: " + patchInfo.version, false, false, nullptr);
                    }

                    tileMenu.addSubMenu(String(tileName + " info"), patchInfoSubMenu, true);
                } else {
                    tileMenu.addItem("Patch info not provided", false, false, nullptr);
                }

                if (previousVersions.size()) {
                    PopupMenu versionsSubMenu;
                    for (auto& [name, file] : previousVersions) {
                        versionsSubMenu.addItem(name, [this, file] {
                            parent.editor->getTabComponent().openPatch(URL(file));
                        });
                    }
                    tileMenu.addSubMenu("Other versions", versionsSubMenu, true);
                }

                tileMenu.addSeparator();

                // Put this at the bottom, so it's not accidentally clicked on
                tileMenu.addItem("Delete from library...", [this] {
                    Dialogs::showMultiChoiceDialog(&parent.confirmationDialog, parent.getParentComponent(), "Are you sure you want to delete: " + patchFile.getFileNameWithoutExtension(), [this](int const choice) {
                        if (choice == 0) {
                            patchFile.getParentDirectory().deleteRecursively(true);
                            parent.triggerAsyncUpdate();
                        } }, { "Yes", "No" }, Icons::Warning);
                });
            } else {
                if (tileType == Patch) {
                    tileMenu.addItem(PlatformStrings::getBrowserTip(), [this] {
                        if (patchFile.existsAsFile())
                            patchFile.revealToUser();
                    });

                    tileMenu.addSeparator();
                    tileMenu.addItem(isFavourited ? "Remove from favourites" : "Add to favourites", [this] {
                        isFavourited = !isFavourited;
                        onFavourite(isFavourited);
                    });

                    tileMenu.addSeparator();
                    PopupMenu patchInfoSubMenu;
                    patchInfoSubMenu.addItem(String("Size: " + fileSizeDescription), false, false, nullptr);
                    patchInfoSubMenu.addSeparator();
                    patchInfoSubMenu.addItem(String("Created: " + creationTimeDescription), false, false, nullptr);
                    patchInfoSubMenu.addItem(String("Modified: " + modifiedTimeDescription), false, false, nullptr);
                    patchInfoSubMenu.addItem(String("Accessed: " + accessedTimeDescription), false, false, nullptr);
                    tileMenu.addSubMenu(String(tileName + ".pd file info"), patchInfoSubMenu, true);
                }
                tileMenu.addSeparator();

                // Put this  at he bottom, so it's not accidentally clicked on
                tileMenu.addItem("Remove from recently opened", onRemove);
            }

            tileMenu.showMenuAsync(PopupMenu::Options().withTargetComponent(this));
        }

        void setSearchQuery(String const& searchQuery)
        {
            setVisible(tileName.containsIgnoreCase(searchQuery));
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(12);

            auto* nvg = dynamic_cast<NVGGraphicsContext&>(g.getInternalContext()).getContext();
            auto const scale = nvgCurrentPixelScale(nvg);
            
            parent.drawShadow(nvg, getWidth(), getHeight(), scale);

            if (thumbnailImageData.isValid()) {
                if (!snapshotImage.isValid() || lastWidth != bounds.getWidth() || lastHeight != bounds.getHeight()) {
                    lastWidth = bounds.getWidth();
                    lastHeight = bounds.getHeight();

                    snapshotImage = NVGImage(nvg, bounds.getWidth() * scale, (bounds.getHeight() - 32) * scale, [this, bounds, scale](Graphics& g) {
                        g.addTransform(AffineTransform::scale(scale));
                        if (thumbnailImageData.isValid()) {
                            auto const imageWidth = thumbnailImageData.getWidth();
                            auto const imageHeight = thumbnailImageData.getHeight();
                            auto const componentWidth = bounds.getWidth();
                            auto const componentHeight = (bounds.getHeight() - 32);

                            float const imageAspect = static_cast<float>(imageWidth) / imageHeight;
                            float const componentAspect = static_cast<float>(componentWidth) / componentHeight;

                            int drawWidth, drawHeight;
                            int offsetX = 0, offsetY = 0;
                            
                            if(approximatelyEqual(componentAspect, imageAspect))
                            {
                                drawWidth = componentWidth;
                                drawHeight = componentHeight;
                            }
                            else {
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
                                offsetY = (componentHeight - drawHeight) / 2;
                            }

                            g.drawImage(thumbnailImageData, offsetX, offsetY, drawWidth, drawHeight, 0, 0, imageWidth, imageHeight);
                        }
                    });
                }
            } else {
                if (tileType != LibraryPatch && snapshot && !snapshotImage.isValid()) {
                    auto const scale = nvgCurrentPixelScale(nvg);
                    snapshotImage = NVGImage(nvg, bounds.getWidth() * scale, (bounds.getHeight() - 32) * scale, [this, scale](Graphics& g) {
                        g.addTransform(AffineTransform::scale(scale));
                        snapshot->drawAt(g, 0, 0, 1.0f);
                    });
                }
            }

            nvgSave(nvg);
            auto const lB = bounds.toFloat().expanded(0.5f);
            // Draw background even for images incase there is a transparent PNG
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), convertColour(findColour(PlugDataColour::panelForegroundColourId)), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);
            
            nvgRoundedScissor(nvg,lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), Corners::largeCornerRadius);
            if (thumbnailImageData.isValid() || tileType == Patch) {
                nvgTranslate(nvg, 0.5f, 0.0f);  // account for outline
                snapshotImage.render(nvg, bounds.withTrimmedBottom(32));
            } else {
                auto const placeholderIconColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.22f);

                // We draw the plugdata logo if library tiles don't have a thumbnail (patch snapshot is too busy)
                nvgFillColor(nvg, NVGComponent::convertColour(placeholderIconColour));
                nvgFontFace(nvg, "icon_font-Regular");
                nvgFontSize(nvg, 68.0f);
                nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(nvg, bounds.getCentreX(), (bounds.getHeight() - 30) * 0.5f, tileType == LibraryPatch ? Icons::PlugdataIconStandard.toRawUTF8() : Icons::Error.toRawUTF8(), nullptr);
            }

            nvgRestore(nvg);

            // Draw border around
            nvgDrawRoundedRect(nvg, lB.getX(), lB.getY(), lB.getWidth(), lB.getHeight(), nvgRGBA(0, 0, 0, 0), convertColour(findColour(PlugDataColour::toolbarOutlineColourId)), Corners::largeCornerRadius);

            auto const hoverColour = findColour(PlugDataColour::toolbarHoverColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.5f);

            nvgBeginPath(nvg);
            nvgRoundedRectVarying(nvg, bounds.getX(), bounds.getHeight() - 32, bounds.getWidth(), 44, 0.0f, 0.0f, Corners::largeCornerRadius, Corners::largeCornerRadius);
            nvgFillColor(nvg, convertColour(isHovered ? hoverColour : findColour(PlugDataColour::toolbarBackgroundColourId)));
            nvgFill(nvg);
            nvgStrokeColor(nvg, convertColour(findColour(PlugDataColour::toolbarOutlineColourId)));
            nvgStroke(nvg);

            auto textWidth = bounds.getWidth() - 8;
            if (titleImage.needsUpdate(textWidth * scale, 24 * scale) || subtitleImage.needsUpdate(textWidth * scale, 16 * scale)) {
                titleImage = NVGImage(nvg, textWidth * scale, 24 * scale, [this, textWidth, scale](Graphics& g) {
                    g.addTransform(AffineTransform::scale(scale, scale));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getBoldFont().withHeight(14));
                    g.drawText(tileName, Rectangle<int>(0, 0, textWidth, 24), Justification::centredLeft, true); }, NVGImage::AlphaImage);

                subtitleImage = NVGImage(nvg, textWidth * scale, 16 * scale, [this, textWidth, scale](Graphics& g) {
                    g.addTransform(AffineTransform::scale(scale, scale));
                    g.setColour(Colours::white);
                    g.setFont(Fonts::getDefaultFont().withHeight(13.5f));
                    g.drawText(tileSubtitle, Rectangle<int>(0, 0, textWidth, 16), Justification::centredLeft, true); }, NVGImage::AlphaImage);
            }

            {
                auto const textColour = findColour(PlugDataColour::panelTextColourId);

                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, 22, bounds.getHeight() - 30);
                titleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 24), convertColour(textColour));
                nvgTranslate(nvg, 0, 20);
                subtitleImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, bounds.getWidth() - 8, 16), convertColour(textColour.withAlpha(0.75f)));
            }

            if (onFavourite) {
                auto const favouriteIconBounds = getHeartIconBounds();
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

        Rectangle<int> getHeartIconBounds() const
        {
            return Rectangle<int>(26, getHeight() - 84, 16, 16);
        }

        bool hitTest(int const x, int const y) override
        {
            return getLocalBounds().reduced(12).contains(Point<int>(x, y));
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

            // If the cursor is no longer over the tile, don't trigger the tile
            // (Standard behaviour for mouse up on object)
            if (!getLocalBounds().reduced(12).contains(e.getPosition()))
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
                auto const bounds = getLocalBounds().reduced(12).withTrimmedBottom(44);
                snapshot->setTransformToFit(bounds.withSizeKeepingCentre(bounds.getWidth() * snapshotScale, bounds.getHeight() * snapshotScale).toFloat(), RectanglePlacement::centred);
            }
        }
    };

public:
    enum Tab {
        Home,
        Library
    };

    explicit WelcomePanel(PluginEditor* pluginEditor)
        : NVGComponent(this)
        , editor(pluginEditor)
    {
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, false, false);
        contentComponent.setVisible(true);
        viewport.setVisible(true);

        addChildComponent(viewport);

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));

        newPatchTile = std::make_unique<MainActionTile>(MainActionTile::New);
        openPatchTile = std::make_unique<MainActionTile>(MainActionTile::Open);
        storeTile = std::make_unique<MainActionTile>(MainActionTile::Store);

        newPatchTile->onClick = [this] { editor->getTabComponent().newPatch(); };
        openPatchTile->onClick = [this] { editor->getTabComponent().openPatch(); };
        storeTile->onClick = [this] { Dialogs::showStore(editor); };

        triggerAsyncUpdate();
    }

    void drawShadow(NVGcontext* nvg, int width, int height, float scale)
    {
        // We only need one shadow image, because all tiles have the same size
        if (shadowImage.needsUpdate(width * scale, height * scale)) {
            shadowImage = NVGImage(nvg, width * scale, height * scale, [width, height, scale](Graphics& g) {
                g.addTransform(AffineTransform::scale(scale, scale));
                Path tilePath;
                tilePath.addRoundedRectangle(12.5f, 12.5f, width - 25.0f, height - 25.0f, Corners::largeCornerRadius);
                StackShadow::renderDropShadow(0, g, tilePath, Colours::white.withAlpha(0.12f), 6, { 0, 1 }); }, NVGImage::AlphaImage);
            repaint();
        }

        shadowImage.renderAlphaImage(nvg, Rectangle<int>(0, 0, width, height), nvgRGB(0, 0, 0));
    }

    void setSearchQuery(String const& newSearchQuery)
    {
        viewport.setViewPositionProportionately(0.0, 0.0);
        
        searchQuery = newSearchQuery;
        if (newPatchTile)
            newPatchTile->setVisible(searchQuery.isEmpty());
        if (openPatchTile)
            openPatchTile->setVisible(searchQuery.isEmpty());
        if (storeTile)
            storeTile->setVisible(searchQuery.isEmpty());

        auto& tiles = currentTab == Home ? recentlyOpenedTiles : libraryTiles;
        for (auto* tile : tiles) {
            tile->setSearchQuery(searchQuery);
        }
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(24);
        Rectangle<int> rowBounds;

        constexpr int desiredTileWidth = 190;
        constexpr int tileSpacing = 4;

        int const totalWidth = bounds.getWidth();
        // Calculate the number of columns that can fit in the total width
        int const numColumns = std::max(1, totalWidth / (desiredTileWidth + tileSpacing));
        // Adjust the tile width to fit within the available width
        int const actualTileWidth = (totalWidth - (numColumns - 1) * tileSpacing) / numColumns;

        if (searchQuery.isEmpty()) {
            rowBounds = bounds.removeFromTop(100);
            // Position buttons centre if there are no recent items
            if (recentlyOpenedTiles.size() == 0) {
                constexpr int buttonWidth = 300;
                constexpr int totalButtonWidth = buttonWidth * 2;

                // Calculate the starting X position for centering
                int const startX = rowBounds.getX() + (rowBounds.getWidth() - totalButtonWidth) / 2;

                // Position the tiles
                auto const buttonY = getHeight() * 0.5f - 30;
                newPatchTile->setBounds(rowBounds.withX(startX).withWidth(buttonWidth).withY(buttonY));
                openPatchTile->setBounds(rowBounds.withX(startX + buttonWidth + tileSpacing).withWidth(buttonWidth).withY(buttonY));

                auto const firstTileBounds = rowBounds.removeFromLeft(actualTileWidth * 1.5f);
                storeTile->setBounds(firstTileBounds);
            } else {
                auto const firstTileBounds = rowBounds.removeFromLeft(actualTileWidth * 1.5f);
                newPatchTile->setBounds(firstTileBounds);
                storeTile->setBounds(firstTileBounds);
                rowBounds.removeFromLeft(4);
                openPatchTile->setBounds(rowBounds.withWidth(actualTileWidth * 1.5f + 4));
            }
        }

        auto const viewPos = viewport.getViewPosition();
        viewport.setBounds(getLocalBounds());

        auto& tiles = currentTab == Home ? recentlyOpenedTiles : libraryTiles;
        int const numRows = (tiles.size() + numColumns - 1) / numColumns;
        int const totalHeight = numRows * 160 + 200;

        auto tilesBounds = Rectangle<int>(24, searchQuery.isEmpty() ? 146 : 24, totalWidth + 24, totalHeight + 24);

        contentComponent.setBounds(tiles.size() ? tilesBounds : getLocalBounds());

        viewport.setBounce(!tiles.isEmpty());

        // Start positioning the recentlyOpenedTiles
        rowBounds = tilesBounds.removeFromTop(160);
        for (auto* tile : tiles) {
            if (!tile->isVisible())
                continue;
            if (tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = tilesBounds.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }

        for (auto* tile : tiles) {
            if (!tile->isVisible())
                continue;
            if (!tile->isFavourited) {
                if (rowBounds.getWidth() < actualTileWidth) {
                    rowBounds = tilesBounds.removeFromTop(160);
                }
                tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
                rowBounds.removeFromLeft(tileSpacing);
            }
        }
        viewport.setViewPosition(viewPos);
    }

    void setShownTab(WelcomePanel::Tab tab)
    {
        currentTab = tab;
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        contentComponent.removeAllChildren();

        recentlyOpenedTiles.clear();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

        if (currentTab == Home) {
            contentComponent.addAndMakeVisible(*newPatchTile);
            contentComponent.addAndMakeVisible(*openPatchTile);
        } else {
            contentComponent.addAndMakeVisible(*storeTile);
        }

        if (recentlyOpenedTree.isValid()) {
            for (int i = recentlyOpenedTree.getNumChildren() - 1; i >= 0; i--) {
                auto subTree = recentlyOpenedTree.getChild(i);
                auto patchFile = File(subTree.getProperty("Path").toString());
                if (!File(patchFile).existsAsFile() && !subTree.hasProperty("Removable")) {
                    recentlyOpenedTree.removeChild(i, nullptr);
                }
            }

            // Place favourited patches at the top
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {

                auto subTree = recentlyOpenedTree.getChild(i);
                auto patchFile = File(subTree.getProperty("Path").toString());

                auto patchThumbnailBase = File(patchFile.getParentDirectory().getChildFile(patchFile.getFileNameWithoutExtension()).getFullPathName() + "_thumb");

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
                        auto cachedSilhouette = patchSvgCache.find(patchFile.getFullPathName());
                        if (cachedSilhouette != patchSvgCache.end()) {
                            silhoutteSvg = cachedSilhouette->second;
                        } else {
                            silhoutteSvg = OfflineObjectRenderer::patchToSVG(patchFile.loadFileAsString());
                            patchSvgCache[patchFile.getFullPathName()] = silhoutteSvg;
                        }
                    }
                }

                auto* tile = recentlyOpenedTiles.add(new WelcomePanelTile(*this, subTree, silhoutteSvg, snapshotColour, 1.0f, favourited, thumbImage));

                tile->onClick = [this, patchFile]() mutable {
                    if (patchFile.existsAsFile()) {
                        editor->pd->autosave->checkForMoreRecentAutosave(patchFile, editor, [this](File patchFile, File patchPath) {
                            auto* cnv = editor->getTabComponent().openPatch(URL(patchFile));
                            if(cnv)
                            {
                                cnv->patch.setCurrentFile(URL(patchPath));
                            }
                            SettingsFile::getInstance()->addToRecentlyOpened(patchPath);
                        });
                    } else {
                        editor->pd->logError("Patch not found");
                    }
                };
                tile->onFavourite = [this, path = subTree.getProperty("Path")](bool const shouldBeFavourite) mutable {
                    auto const settingsTree = SettingsFile::getInstance()->getValueTree();
                    auto const recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");

                    // Settings file could be reloaded, we can't assume the old recently opened tree is still valid!
                    // So look up the entry by file path
                    auto subTree = recentlyOpenedTree.getChildWithProperty("Path", path);
                    subTree.setProperty("Pinned", shouldBeFavourite, nullptr);
                    resized();
                };
                tile->onRemove = [this, path = subTree.getProperty("Path")] {
                    auto const settingsTree = SettingsFile::getInstance()->getValueTree();
                    auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
                    auto const subTree = recentlyOpenedTree.getChildWithProperty("Path", path);
                    recentlyOpenedTree.removeChild(subTree, nullptr);
                    // Make sure to clear the recent items in the current welcome panel
                    if (editor->welcomePanel)
                        editor->welcomePanel->triggerAsyncUpdate();
                };

                contentComponent.addAndMakeVisible(tile);
            }
        }

        contentComponent.repaint();
        findLibraryPatches();
        
        // Perform search again if we update content while we have search text
        if(searchQuery.isNotEmpty())
        {
            setSearchQuery(searchQuery);
        }
        resized();
    }

    void findLibraryPatches()
    {
        libraryTiles.clear();

        auto addTile = [this](Array<std::pair<int64, File>> patches) {
            auto patchFile = patches[0].second;
            auto const pName = patchFile.getFileNameWithoutExtension();
            auto foundThumbs = patchFile.getParentDirectory().findChildFiles(File::findFiles, true, pName + "_thumb.png;" + pName + "_thumb.jpg;" + pName + "_thumb.jpeg;" + pName + "_thumb.gif");

            std::ranges::sort(patches, [](std::pair<int64, File> const& first, std::pair<int64, File> const& second) {
                return first.first > second.first;
            });

            patches.remove(0);

            constexpr float scale = 1.0f;
            Image thumbImage;
            for (auto& thumb : foundThumbs) {
                FileInputStream fileStream(thumb);
                if (fileStream.openedOk()) {
                    thumbImage = ImageFileFormat::loadFrom(fileStream).convertedToFormat(Image::ARGB);
                    if (thumbImage.isValid())
                        break;
                }
            }
            auto const metaFile = patchFile.getParentDirectory().getChildFile("meta.json");
            String author;
            if (metaFile.existsAsFile()) {
                auto const json = JSON::fromString(metaFile.loadFileAsString());
                author = json["Author"].toString();
            }
            auto* tile = libraryTiles.add(new WelcomePanelTile(*this, patchFile, author, scale, false, thumbImage));
            tile->onClick = [this, patchFile]() mutable {
                if (patchFile.existsAsFile()) {
                    editor->getTabComponent().openPatch(URL(patchFile));
                    SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                } else {
                    editor->pd->logError("Patch not found");
                }
            };
            tile->setPreviousVersions(patches);
            contentComponent.addAndMakeVisible(tile);
        };

        Array<std::tuple<File, hash32, int64>> allPatches;

        auto const patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");
        for (auto& file : OSUtils::iterateDirectory(patchesFolder, false, false)) {
            if (OSUtils::isDirectoryFast(file.getFullPathName())) {
                for (auto& subfile : OSUtils::iterateDirectory(file, false, false)) {
                    if (subfile.hasFileExtension("pd")) {
                        auto const metaFile = subfile.getParentDirectory().getChildFile("meta.json");
                        String author;
                        String title;
                        int64 installTime;
                        if (metaFile.existsAsFile()) {
                            auto const json = JSON::fromString(metaFile.loadFileAsString());
                            author = json["Author"].toString();
                            title = json["Title"].toString();
                            if (json.hasProperty("InstallTime")) {
                                installTime = static_cast<int64>(json["InstallTime"]);
                            } else {
                                installTime = metaFile.getCreationTime().toMilliseconds();
                            }
                        } else {
                            title = subfile.getFileNameWithoutExtension();
                            installTime = 0;
                        }

                        allPatches.add({ subfile, hash(title + author), installTime });
                        break;
                    }
                }
            } else {
                if (file.hasFileExtension("pd")) {
                    allPatches.add({ file, 0, 0 });
                }
            }
        }

        // Combine different versions of the same patch into one tile
        UnorderedMap<hash32, Array<std::pair<int64, File>>> versions;
        for (auto& [file, hash, time] : allPatches) {
            versions[hash].add({ time, file });
        }
        for (auto& [hash, patches] : versions) {
            addTile(patches);
        }
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
            nvgContext = std::make_unique<NVGGraphicsContext>(nvg);

        nvgFillColor(nvg, convertColour(findColour(PlugDataColour::panelBackgroundColourId)));
        nvgFillRect(nvg, 0, 0, getWidth(), getHeight());

        Graphics g(*nvgContext);
        g.reduceClipRegion(editor->nvgSurface.getInvalidArea());
        paintEntireComponent(g, false);

        auto const gradient = nvgLinearGradient(nvg, 0, viewport.getY(), 0, viewport.getY() + 20, convertColour(findColour(PlugDataColour::panelBackgroundColourId)), nvgRGBA(255, 255, 255, 0));

        nvgFillPaint(nvg, gradient);
        nvgFillRect(nvg, viewport.getX() + 8, viewport.getY(), viewport.getWidth() - 16, 20);
    }

    void lookAndFeelChanged() override
    {
        triggerAsyncUpdate();
    }

    std::unique_ptr<MainActionTile> newPatchTile, openPatchTile, storeTile;
    ContentComponent contentComponent = ContentComponent(*this);
    BouncingViewport viewport;

    std::unique_ptr<NVGGraphicsContext> nvgContext = nullptr;

    NVGImage shadowImage;
    OwnedArray<WelcomePanelTile> recentlyOpenedTiles;
    OwnedArray<WelcomePanelTile> libraryTiles;
    PluginEditor* editor;

    String searchQuery;
    Tab currentTab = Home;
    UnorderedMap<String, String> patchSvgCache;

    std::unique_ptr<Dialog> confirmationDialog;

    // To make the library panel update automatically
    class LibraryFSListener final : public FileSystemWatcher::Listener {
        FileSystemWatcher libraryFsWatcher;
        WelcomePanel& panel;

    public:
        explicit LibraryFSListener(WelcomePanel& panel)
            : panel(panel)
        {
            libraryFsWatcher.addFolder(ProjectInfo::appDataDir.getChildFile("Patches"));
            libraryFsWatcher.addListener(this);
        }

        void filesystemChanged() override
        {
            panel.triggerAsyncUpdate();
        }
    };

    LibraryFSListener libraryFsListener = LibraryFSListener(*this);
};
