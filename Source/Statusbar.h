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
class EditModeButton;
class TouchSelectionHelper;

class Statusbar;

class Statusbar final : public Component
    , public NVGComponent
    , public AsyncUpdater {
public:
    Statusbar(PluginProcessor* processor, PluginEditor* e);

    ~Statusbar() override;

    void updateZoomLevel() { triggerAsyncUpdate(); }

    void setEditButtonState(bool locked, bool present = false);

    // Shows/hides the touch selection helper in the centre of the statusbar
    void showTouchSelectionHelper(bool shouldShow);

    static int getStatusbarHeight();

    float currentZoomLevel = 100.0f;

private:
    void handleAsyncUpdate() override;

    void paint(Graphics& g) override;

    void resized() override;

    PluginProcessor* pd;
    PluginEditor* editor;

    std::unique_ptr<ZoomLabel> zoomSelector;
    std::unique_ptr<StatusbarButtonGroup> gridGroup;
    std::unique_ptr<StatusbarButtonGroup> overlayGroup;
    std::unique_ptr<EditModeButton> editModeGroup;
    std::unique_ptr<TouchSelectionHelper> touchSelectionHelper;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statusbar)
};
