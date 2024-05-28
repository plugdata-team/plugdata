/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include "Utility/Autosave.h"
#include "Utility/CachedTextRender.h"

class WelcomePanel : public Component, public NVGComponent {

    class WelcomePanelTile : public Component
    {
        float snapshotScale;
        bool isHovered = false;
        String tileName;
        std::unique_ptr<Drawable> snapshot = nullptr;
        
        CachedTextRender titleRenderer, subtitleRenderer;
    public:
        
        std::function<void()> onClick = [](){};
        
        WelcomePanelTile(String name, String subtitle, String svgImage, Colour iconColour, float scale)
        : snapshotScale(scale), tileName(name)
        {
            snapshot = Drawable::createFromImageData(svgImage.toRawUTF8(), svgImage.getNumBytesAsUTF8());
            if(snapshot) {
                snapshot->replaceColour (Colours::black, iconColour);
            }

            resized();

            auto textColour = findColour(PlugDataColour::panelTextColourId);
            titleRenderer.prepareLayout(name, Fonts::getBoldFont().withHeight(14), textColour, 500, 500);
            subtitleRenderer.prepareLayout(subtitle, Fonts::getCurrentFont().withHeight(13.5f), textColour, 500, 500);
        }
        
        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(12);
            
            Path tilePath;
            tilePath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius);
            
            StackShadow::renderDropShadow(g, tilePath, Colour(0, 0, 0).withAlpha(0.1f), 7, {0, 2});
            g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
            g.fillPath(tilePath);
            
            if(snapshot)  {
                snapshot->drawAt(g, 0, 0, 1.0f);
            }
            
            Path textAreaPath;
            textAreaPath.addRoundedRectangle(bounds.getX(), bounds.getHeight() - 32, bounds.getWidth(), 44, Corners::largeCornerRadius, Corners::largeCornerRadius, false, false, true, true);
        
            auto hoverColour = findColour(PlugDataColour::toolbarHoverColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.5f);
            g.setColour(isHovered ? hoverColour : findColour(PlugDataColour::toolbarBackgroundColourId));
            g.fillPath(textAreaPath);
            
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.strokePath(tilePath, PathStrokeType(1.0f));
            
            auto* nvg = dynamic_cast<NanoVGGraphicsContext&>(g.getInternalContext()).getContext();
            
            nvgSave(nvg);
            nvgTranslate(nvg, 0, bounds.getHeight() - 30);
            titleRenderer.renderText(nvg, Rectangle<int>(bounds.getX() + 10, 0, bounds.getWidth() - 8, 24), 2.0f);
            nvgTranslate(nvg, 0, 20);
            subtitleRenderer.renderText(nvg, Rectangle<int>(bounds.getX() + 10, 0, bounds.getWidth() - 8, 16), 2.0f);
            nvgRestore(nvg);
            
            
            //Fonts::drawStyledText(g, tileName, bounds.getX() + 10, bounds.getHeight() - 30, bounds.getWidth() - 8, 24, findColour(PlugDataColour::panelTextColourId), Semibold, 14);
            /*
            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f));
            g.setFont(Fonts::getCurrentFont().withHeight(13.5f));
            g.drawText(tileSubtitle, , Justification::centredLeft); */
        }
        
        void mouseEnter(const MouseEvent& e) override
        {
            isHovered = true;
            repaint();
        }
        
        void mouseExit(const MouseEvent& e) override
        {
            isHovered = false;
            repaint();
        }
        
        void mouseUp(const MouseEvent& e) override
        {
            onClick();
        }
        
        void resized() override
        {
            if(snapshot) {
                auto bounds = getLocalBounds().reduced(12).withTrimmedBottom(44);
                snapshot->setTransformToFit(bounds.withSizeKeepingCentre(bounds.getWidth() * snapshotScale, bounds.getHeight() * snapshotScale).toFloat(), RectanglePlacement::centred);
            }
        }
    };
    
public:
    WelcomePanel(PluginEditor* pluginEditor) : NVGComponent(this), editor(pluginEditor)
    {
        recentlyOpenedViewport.setViewedComponent(&recentlyOpenedComponent, false);
        recentlyOpenedViewport.setScrollBarsShown(true, false, false, false);
        recentlyOpenedComponent.setVisible(true);
        addAndMakeVisible(recentlyOpenedViewport);
        
        // Update async to make sure silhouettes are available
        MessageManager::callAsync([_this = SafePointer(this)](){
            _this->update();
        });
        
        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(24).withTrimmedTop(36);
        auto rowBounds = bounds.removeFromTop(160);
        
        const int desiredTileWidth = 190;
        const int tileSpacing = 4;

        int totalWidth = bounds.getWidth();
        // Calculate the number of columns that can fit in the total width
        int numColumns = totalWidth / (desiredTileWidth + tileSpacing);
        // Adjust the tile width to fit within the available width
        int actualTileWidth = (totalWidth - (numColumns - 1) * tileSpacing) / numColumns;
        
        if(newPatchTile) newPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
        rowBounds.removeFromLeft(4);
        if(openPatchTile) openPatchTile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
        
        bounds.removeFromTop(16);
        
        recentlyOpenedViewport.setBounds(getLocalBounds().withTrimmedTop(260));

        int numRows = (tiles.size() + numColumns - 1) / numColumns;
        int totalHeight = numRows * 160;

        auto scrollable = Rectangle<int>(24, 6, totalWidth + 24, totalHeight);
        recentlyOpenedComponent.setBounds(scrollable);

        // Start positioning the tiles
        rowBounds = scrollable.removeFromTop(160);
        for (auto* tile : tiles)
        {
            if (rowBounds.getWidth() < actualTileWidth) {
                rowBounds = scrollable.removeFromTop(160);
            }
            tile->setBounds(rowBounds.removeFromLeft(actualTileWidth));
            rowBounds.removeFromLeft(tileSpacing);
        }
    }
    
    void update()
    {
        newPatchTile = std::make_unique<WelcomePanelTile>("New Patch", "Create a new empty patch", newIcon, findColour(PlugDataColour::panelTextColourId), 0.33f);
        openPatchTile = std::make_unique<WelcomePanelTile>("Open Patch", "Browse for a patch to open", openIcon, findColour(PlugDataColour::panelTextColourId), 0.33f);
        
        newPatchTile->onClick = [this]() { editor->newProject(); };
        openPatchTile->onClick = [this](){ editor->openProject(); };
        
        addAndMakeVisible(*newPatchTile);
        addAndMakeVisible(*openPatchTile);
        
        tiles.clear();

        auto settingsTree = SettingsFile::getInstance()->getValueTree();
        auto recentlyOpenedTree = settingsTree.getChildWithName("RecentlyOpened");
        
        auto snapshotColour = LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.3f);
        
        if (recentlyOpenedTree.isValid()) {
            for (int i = 0; i < recentlyOpenedTree.getNumChildren(); i++) {
                auto patchFile = File(recentlyOpenedTree.getChild(i).getProperty("Path").toString());
                auto silhoutteSvg = Autosave::findPatchSilhouette(patchFile);
                
                if(silhoutteSvg.isEmpty() && patchFile.existsAsFile())
                {
                    silhoutteSvg = OfflineObjectRenderer::patchToSVGFast(patchFile.loadFileAsString());
                }
                
                auto openTime = Time(static_cast<int64>(recentlyOpenedTree.getChild(i).getProperty("Time")));
                auto diff = Time::getCurrentTime() - openTime;
                String date;
                if(diff.inDays() == 0) date = "Today";
                else if(diff.inDays() == 1) date = "Yesterday";
                else date = openTime.toString(true, false);
                String time = openTime.toString(false, true, false, true);
                String timeDescription = date + ", " + time;
                
                auto* tile = tiles.add(new WelcomePanelTile(patchFile.getFileName(), timeDescription, silhoutteSvg, snapshotColour, 1.0f));
                tile->onClick = [this, patchFile]() mutable {
                    editor->autosave->checkForMoreRecentAutosave(patchFile, [this, patchFile]() {
                        editor->pd->loadPatch(URL(patchFile), editor);
                        SettingsFile::getInstance()->addToRecentlyOpened(patchFile);
                        editor->pd->titleChanged();
                    });
                };
                recentlyOpenedComponent.addAndMakeVisible(tile);
            }
        }

        resized();
    }

    void show()
    {
        update();
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }
    
    void render(NVGcontext* nvg) override
    {
        if(!nvgContext) nvgContext = std::make_unique<NanoVGGraphicsContext>(nvg);
        
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, getWidth(), getHeight());
        nvgFillColor(nvg, convertColour(findColour(PlugDataColour::panelBackgroundColourId)));
        nvgFill(nvg);
        
        Graphics g(*nvgContext);
        g.reduceClipRegion(editor->nvgSurface.getInvalidArea());
        paintEntireComponent(g, false);
        
        auto gradient = nvgLinearGradient(nvg, 0, recentlyOpenedViewport.getY(), 0, recentlyOpenedViewport.getY() + 20, convertColour(findColour(PlugDataColour::panelBackgroundColourId)), nvgRGBAf(1, 1, 1, 0));
        
        nvgFillPaint(nvg, gradient);
        nvgBeginPath(nvg);
        nvgRect(nvg, recentlyOpenedViewport.getX() + 8, recentlyOpenedViewport.getY(), recentlyOpenedViewport.getWidth() - 16, 20);
        nvgFill(nvg);
        
        nvgBeginPath(nvg);
        nvgFillColor(nvg, convertColour(findColour(PlugDataColour::panelTextColourId)));
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(nvg, 30);
        nvgFontFace(nvg, "Inter-Bold");
        nvgText(nvg, 32, 32, "Welcome to plugdata", nullptr);
        
        nvgBeginPath(nvg);
        nvgFontSize(nvg, 24);
        nvgText(nvg, 32, 240, "Recently Opened", nullptr);
    }

    void lookAndFeelChanged() override
    {
        update();
    }

    static inline const String newIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
    "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\" >\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"-10 0 2058 2048\">\n"
    "   <path fill=\"currentColor\"\n"
    "d=\"M1024 170v512q0 72 50 122t120 50h512v852q0 72 -50 122t-120 50h-1024q-70 0 -120 -50.5t-50 -121.5v-1364q0 -72 50 -122t120 -50h512zM1151 213l512 512h-469q-16 0 -29.5 -12.5t-13.5 -30.5v-469z\" />\n"
    "</svg>\n";
    
    static inline const String openIcon = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
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
    
    OwnedArray<WelcomePanelTile> tiles;
    PluginEditor* editor;
};
