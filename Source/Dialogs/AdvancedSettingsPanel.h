/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

struct AdvancedSettingsPanel : public Component {
    AdvancedSettingsPanel(ValueTree tree)
        : settingsTree(tree)
    {
        nativeTitlebar.referTo(settingsTree.getPropertyAsValue("NativeWindow", nullptr));
        reloadPatch.referTo(settingsTree.getPropertyAsValue("ReloadLastState", nullptr));
        dashedSignalConnection.referTo(settingsTree.getPropertyAsValue("DashedSignalConnection", nullptr));

        useNativeTitlebar.reset(new PropertiesPanel::BoolComponent("Use native titlebar", nativeTitlebar, { "No", "Yes" }));

        reloadLastOpenedPatch.reset(new PropertiesPanel::BoolComponent("Reload last opened patch startup", reloadPatch, { "No", "Yes" }));

        useDashedSignalConnection.reset(new PropertiesPanel::BoolComponent("Display signal connections dashed", dashedSignalConnection, { "No", "Yes" }));

        addAndMakeVisible(*useNativeTitlebar);
        addAndMakeVisible(*reloadLastOpenedPatch);
        addAndMakeVisible(*useDashedSignalConnection);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        useNativeTitlebar->setBounds(bounds.removeFromTop(23));
        reloadLastOpenedPatch->setBounds(bounds.removeFromTop(23));
        useDashedSignalConnection->setBounds(bounds.removeFromTop(23));
    }

    ValueTree settingsTree;

    Value nativeTitlebar;
    Value reloadPatch;
    Value dashedSignalConnection;

    std::unique_ptr<PropertiesPanel::BoolComponent> useNativeTitlebar;
    std::unique_ptr<PropertiesPanel::BoolComponent> reloadLastOpenedPatch;
    std::unique_ptr<PropertiesPanel::BoolComponent> useDashedSignalConnection;
};
