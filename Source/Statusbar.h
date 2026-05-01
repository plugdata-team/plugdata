/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LookAndFeel.h"
#include "Utility/SettingsFile.h"
#include "Utility/NVGUtils.h"
#include "Components/Buttons.h"

class PluginProcessor;
class LatencyDisplayButton;
class ZoomLabel;
class StatusbarButtonGroup;

class Statusbar;

class Statusbar final : public Component
    , public NVGComponent
    , public AsyncUpdater {
public:
    Statusbar(PluginProcessor* processor, PluginEditor* e);

    ~Statusbar() override;

    void updateZoomLevel() { triggerAsyncUpdate(); }

    void setEditButtonState(bool locked, bool present = false);

    static constexpr int statusbarHeight = 30;
    float currentZoomLevel = 100.0f;

private:
    void updateCachedRenderingMode();

    void handleAsyncUpdate() override;

    void paint(Graphics& g) override;

    void resized() override;

    PluginProcessor* pd;
    PluginEditor* editor;

    std::unique_ptr<ZoomLabel> zoomSelector;
    std::unique_ptr<StatusbarButtonGroup> gridGroup;
    std::unique_ptr<StatusbarButtonGroup> overlayGroup;
    std::unique_ptr<StatusbarButtonGroup> editModeGroup;

    bool cachedRenderingEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};
