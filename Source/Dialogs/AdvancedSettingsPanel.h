/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "LookAndFeel.h"

#pragma once

class AdvancedSettingsPanel : public Component
    , public Value::Listener {

public:
    AdvancedSettingsPanel(Component* editor)
        : editor(editor)
    {
        auto* settingsFile = SettingsFile::getInstance();

        if (ProjectInfo::isStandalone) {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            macTitlebarButtons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));
            reloadPatch.referTo(settingsFile->getPropertyAsValue("reload_last_state"));

            macTitlebarButtons.addListener(this);

            useNativeTitlebar.reset(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            useMacTitlebarButtons.reset(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macTitlebarButtons, { "No", "Yes" }));
            reloadLastOpenedPatch.reset(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));

            addAndMakeVisible(*useNativeTitlebar);
            addAndMakeVisible(*useMacTitlebarButtons);
            addAndMakeVisible(*reloadLastOpenedPatch);
        }

        scaleValue = settingsFile->getProperty<float>("global_scale");
        scaleValue.addListener(this);
        globalScale.reset(new PropertiesPanel::EditableComponent<float>("Global scale factor", scaleValue));
        addAndMakeVisible(*globalScale);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        if (ProjectInfo::isStandalone) {
            useNativeTitlebar->setBounds(bounds.removeFromTop(23));
            useMacTitlebarButtons->setBounds(bounds.removeFromTop(23));
            reloadLastOpenedPatch->setBounds(bounds.removeFromTop(23));
        }
        globalScale->setBounds(bounds.removeFromTop(23));
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(macTitlebarButtons)) {
            auto window = Desktop::getInstance().getComponent(0);
            window->sendLookAndFeelChange();

            editor->resized();
        }
        if (v.refersToSameSourceAs(scaleValue)) {
            auto scale = std::clamp(getValue<float>(scaleValue), 0.5f, 2.5f);
            SettingsFile::getInstance()->setGlobalScale(scale);
            scaleValue = scale;
        }
    }
    Component* editor;

    ValueTree settingsTree;

    Value nativeTitlebar;
    Value macTitlebarButtons;
    Value reloadPatch;
    Value scaleValue;

    std::unique_ptr<PropertiesPanel::BoolComponent> useNativeTitlebar;
    std::unique_ptr<PropertiesPanel::BoolComponent> useMacTitlebarButtons;
    std::unique_ptr<PropertiesPanel::BoolComponent> reloadLastOpenedPatch;
    std::unique_ptr<PropertiesPanel::EditableComponent<float>> globalScale;
};
