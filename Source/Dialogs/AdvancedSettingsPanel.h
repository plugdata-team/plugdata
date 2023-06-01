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
    explicit AdvancedSettingsPanel(Component* editor)
        : editor(editor)
    {
        auto* settingsFile = SettingsFile::getInstance();

        Array<PropertiesPanel::Property*> otherProperties;

        if (ProjectInfo::isStandalone) {
            nativeTitlebar.referTo(settingsFile->getPropertyAsValue("native_window"));
            macTitlebarButtons.referTo(settingsFile->getPropertyAsValue("macos_buttons"));
            reloadPatch.referTo(settingsFile->getPropertyAsValue("reload_last_state"));

            macTitlebarButtons.addListener(this);

            Array<PropertiesPanel::Property*> windowProperties;

            windowProperties.add(new PropertiesPanel::BoolComponent("Use system titlebar", nativeTitlebar, { "No", "Yes" }));
            windowProperties.add(new PropertiesPanel::BoolComponent("Use macOS style window buttons", macTitlebarButtons, { "No", "Yes" }));

            propertiesPanel.addSection("Window", windowProperties);

            otherProperties.add(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));
        }
        
        showPalettesValue.referTo(settingsFile->getPropertyAsValue("show_palettes"));
        showPalettesValue.addListener(this);
        otherProperties.add(new PropertiesPanel::BoolComponent("Show palette bar", showPalettesValue, { "No", "Yes" }));
        
        autoPatchingValue.referTo(settingsFile->getPropertyAsValue("autoconnect"));
        otherProperties.add(new PropertiesPanel::BoolComponent("Enable auto patching", autoPatchingValue, { "No", "Yes" }));
        
        scaleValue = settingsFile->getProperty<float>("global_scale");
        scaleValue.addListener(this);
        otherProperties.add(new PropertiesPanel::EditableComponent<float>("Global scale factor", scaleValue));
        
        propertiesPanel.addSection("Other", otherProperties);

        addAndMakeVisible(propertiesPanel);
    }

    void resized() override
    {
        propertiesPanel.setBounds(getLocalBounds());
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(macTitlebarButtons)) {
            auto window = Desktop::getInstance().getComponent(0);
            window->sendLookAndFeelChange();

            editor->resized();
        }
        if (v.refersToSameSourceAs(showPalettesValue)) {
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
        
    Value showPalettesValue;
    Value autoPatchingValue;

    PropertiesPanel propertiesPanel;
};
