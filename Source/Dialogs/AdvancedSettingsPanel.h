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
        nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
        macOS_Buttons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));
        reloadPatch.referTo(settingsFile->getPropertyAsValue("reload_last_state"));

        leftWindowButtons.addListener(this);
        macOS_Buttons.addListener(this);

        useNativeTitlebar.reset(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
        use_macOS_Buttons.reset(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macOS_Buttons, { "No", "Yes" }));
        reloadLastOpenedPatch.reset(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));

        addAndMakeVisible(*useNativeTitlebar);
        addAndMakeVisible(*use_macOS_Buttons);
        addAndMakeVisible(*reloadLastOpenedPatch);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        useNativeTitlebar->setBounds(bounds.removeFromTop(23));
        use_macOS_Buttons->setBounds(bounds.removeFromTop(23));
        reloadLastOpenedPatch->setBounds(bounds.removeFromTop(23));
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(leftWindowButtons) || v.refersToSameSourceAs(macOS_Buttons)) {
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
    Value macOS_Buttons;
    Value reloadPatch;

    std::unique_ptr<PropertiesPanel::BoolComponent> useNativeTitlebar;
    std::unique_ptr<PropertiesPanel::BoolComponent> use_macOS_Buttons;
    std::unique_ptr<PropertiesPanel::BoolComponent> reloadLastOpenedPatch;
};
