/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "LookAndFeel.h"

#pragma once

class AdvancedSettingsPanel : public Component, public Value::Listener {

public:
    AdvancedSettingsPanel()
    {
        auto* settingsFile = SettingsFile::getInstance();
        
        if(ProjectInfo::isStandalone)
        {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            macTitlebarButtons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));
            reloadPatch.referTo(settingsFile->getPropertyAsValue("reload_last_state"));
            
            leftWindowButtons.addListener(this);
            macTitlebarButtons.addListener(this);

            useNativeTitlebar.reset(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            useMacTitlebarButtons.reset(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macTitlebarButtons, { "No", "Yes" }));
            reloadLastOpenedPatch.reset(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));

            addAndMakeVisible(*useNativeTitlebar);
            addAndMakeVisible(*useMacTitlebarButtons);
            addAndMakeVisible(*reloadLastOpenedPatch);
        }
        
        infiniteCanvas.referTo(settingsFile->getPropertyAsValue("infinite_canvas"));
        infiniteCanvas.addListener(this);
        useInfiniteCanvas.reset(new PropertiesPanel::BoolComponent("Use infinite canvas", infiniteCanvas, { "No", "Yes" }));
        addAndMakeVisible(*useInfiniteCanvas);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        if(ProjectInfo::isStandalone)
        {
            useNativeTitlebar->setBounds(bounds.removeFromTop(23));
            useMacTitlebarButtons->setBounds(bounds.removeFromTop(23));
            reloadLastOpenedPatch->setBounds(bounds.removeFromTop(23));
        }
        useInfiniteCanvas->setBounds(bounds.removeFromTop(23));
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(leftWindowButtons) || v.refersToSameSourceAs(macTitlebarButtons)) {
            auto window = Desktop::getInstance().getComponent(0);
            // TODO: BUG: for some reason this doesn't change the lnf of the current settings window
            window->sendLookAndFeelChange();

            // TODO: replace this horrible hack
            Desktop::getInstance().setGlobalScaleFactor(Desktop::getInstance().getGlobalScaleFactor() - 0.01f);
            Desktop::getInstance().setGlobalScaleFactor(Desktop::getInstance().getGlobalScaleFactor() + 0.01f);
        }
    }

    ValueTree settingsTree;

    Value nativeTitlebar;
    Value leftWindowButtons;
    Value macTitlebarButtons;
    Value reloadPatch;
    Value infiniteCanvas;

    std::unique_ptr<PropertiesPanel::BoolComponent> useNativeTitlebar;
    std::unique_ptr<PropertiesPanel::BoolComponent> useMacTitlebarButtons;
    std::unique_ptr<PropertiesPanel::BoolComponent> reloadLastOpenedPatch;
    std::unique_ptr<PropertiesPanel::BoolComponent> useInfiniteCanvas;
};
