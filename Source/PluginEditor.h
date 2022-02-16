/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Dialogs.h"
#include "Sidebar.h"
#include "LevelMeter.h"
#include "LookAndFeel.h"

#include "Standalone/PlugDataWindow.h"

struct TabComponent : public TabbedComponent
{
    std::function<void(int)> onTabChange = [](int) {};

    TabComponent() : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
    }

    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        onTabChange(newCurrentTabIndex);
    }
};

class Canvas;
class PlugDataAudioProcessor;
class PlugDataPluginEditor : public AudioProcessorEditor, public ChangeBroadcaster, public KeyListener, public ValueTree::Listener, public Timer
{
   public:
    PlugDataPluginEditor(PlugDataAudioProcessor&);
    ~PlugDataPluginEditor() override;

    void showNewObjectMenu();

    void paint(Graphics&) override;
    void resized() override;

    void timerCallback() override;
    bool keyPressed(const KeyPress& key, Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, Component* originatingComponent) override;

    void openProject();
    void saveProject(const std::function<void()>& nestedCallback = []() {});
    void saveProjectAs(const std::function<void()>& nestedCallback = []() {});

    void addTab(Canvas* cnv, bool deleteWhenClosed = false);

    Canvas* getCurrentCanvas();
    Canvas* getCanvas(int idx);

    void updateValues();

    void updateUndoState();

    void zoom(bool zoomingIn);

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    PlugDataAudioProcessor& pd;

    AffineTransform transform;

    TabComponent tabbar;
    OwnedArray<Canvas, CriticalSection> canvases;
    Sidebar sidebar;

    LevelMeter levelmeter;

    TextButton bypassButton = TextButton(Icons::Power);
    TextButton lockButton = TextButton(Icons::Lock);
    TextButton connectionStyleButton = TextButton(Icons::ConnectionStyle);
    TextButton connectionPathfind = TextButton(Icons::Wand);

    TextButton zoomIn = TextButton(Icons::ZoomIn);
    TextButton zoomOut = TextButton(Icons::ZoomOut);
    Label zoomLabel;

   private:
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    static constexpr int toolbarHeight = 40;
    static constexpr int statusbarHeight = 25;
    int sidebarWidth = 275;

    bool sidebarHidden = false;

    std::array<TextButton, 9> toolbarButtons = {TextButton(Icons::New), TextButton(Icons::Open), TextButton(Icons::Save), TextButton(Icons::SaveAs), TextButton(Icons::Undo), TextButton(Icons::Redo), TextButton(Icons::Add), TextButton(Icons::Settings), TextButton(Icons::Hide)};

    TextButton& hideButton = toolbarButtons[8];

    std::unique_ptr<SettingsDialog> settingsDialog = nullptr;

    ComponentBoundsConstrainer restrainer;
    std::unique_ptr<ResizableCornerComponent> resizer;

    SharedResourcePointer<TooltipWindow> tooltipWindow;

    std::unique_ptr<ButtonParameterAttachment> enableAttachment;

    Component seperators[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataPluginEditor)
};
