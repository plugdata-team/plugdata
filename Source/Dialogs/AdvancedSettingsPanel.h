/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class AdvancedSettingsPanel : public Component {
    
public:
    AdvancedSettingsPanel(ValueTree tree)
        : settingsTree(tree)
    {
        nativeTitlebar.referTo(settingsTree.getPropertyAsValue("NativeWindow", nullptr));
        reloadPatch.referTo(settingsTree.getPropertyAsValue("ReloadLastState", nullptr));

        useNativeTitlebar.reset(new PropertiesPanel::BoolComponent("Use native titlebar", nativeTitlebar, { "No", "Yes" }));

        reloadLastOpenedPatch.reset(new PropertiesPanel::BoolComponent("Reload last opened patch on startup", reloadPatch, { "No", "Yes" }));

        addAndMakeVisible(*useNativeTitlebar);
        addAndMakeVisible(*reloadLastOpenedPatch);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        useNativeTitlebar->setBounds(bounds.removeFromTop(23));
        reloadLastOpenedPatch->setBounds(bounds.removeFromTop(23));
    }

    ValueTree settingsTree;

    Value nativeTitlebar;
    Value reloadPatch;

    std::unique_ptr<PropertiesPanel::BoolComponent> useNativeTitlebar;
    std::unique_ptr<PropertiesPanel::BoolComponent> reloadLastOpenedPatch;
};
