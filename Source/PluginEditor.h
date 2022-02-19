/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Dialogs.h"
#include "Sidebar.h"
#include "Statusbar.h"

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
class PlugDataPluginEditor : public AudioProcessorEditor, public Value::Listener
{
   public:
    explicit PlugDataPluginEditor(PlugDataAudioProcessor&);

    ~PlugDataPluginEditor() override;
    void showNewObjectMenu();

    void paint(Graphics&) override;
    void resized() override;

    bool keyPressed(const KeyPress& key) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void mouseMagnify(const MouseEvent& e, float scaleFactor) override;

    void openProject();
    void saveProject(const std::function<void()>& nestedCallback = []() {});
    void saveProjectAs(const std::function<void()>& nestedCallback = []() {});

    
    void addTab(Canvas* cnv, bool deleteWhenClosed = false);

    Canvas* getCurrentCanvas();
    Canvas* getCanvas(int idx);

    void updateValues();

    void updateUndoState();

    void valueChanged(Value& v) override;

    PlugDataAudioProcessor& pd;

    AffineTransform transform;

    TabComponent tabbar;
    OwnedArray<Canvas, CriticalSection> canvases;
    Sidebar sidebar;
    Statusbar statusbar;

   private:
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    static constexpr int toolbarHeight = 40;

    OwnedArray<TextButton> toolbarButtons;

    std::unique_ptr<Component> settingsDialog = nullptr;

    ComponentBoundsConstrainer constrainer;
    ResizableCornerComponent resizer;

    SharedResourcePointer<TooltipWindow> tooltipWindow;

    Component seperators[2];

    enum ToolbarButtonType
    {
        New = 0,
        Open,
        Save,
        SaveAs,
        Undo,
        Redo,
        Add,
        Settings,
        Hide
    };

    TextButton* toolbarButton(ToolbarButtonType type)
    {
        return toolbarButtons[static_cast<int>(type)];
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataPluginEditor)
};
