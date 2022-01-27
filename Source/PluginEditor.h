/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Console.h"
#include "Dialogs.h"
#include "Inspector.h"
#include "LevelMeter.h"
#include "LookAndFeel.h"

#include "Standalone/PlugDataWindow.h"

#include <JuceHeader.h>


struct TabComponent : public TabbedComponent {

    std::function<void(int)> onTabChange = [](int) {};

    TabComponent()
        : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
    }

    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        onTabChange(newCurrentTabIndex);
    }
};

class Canvas;
class PlugDataAudioProcessor;
class PlugDataPluginEditor : public AudioProcessorEditor,  public ChangeBroadcaster, public FileOpener, public KeyListener  {
    
public:
    
    SharedResourcePointer<Resources> resources;
    
    ToolbarLook toolbarLook = ToolbarLook(resources.get());
    StatusbarLook statusbarLook = StatusbarLook(resources.get(), 1.35f);
    MainLook mainLook = MainLook(resources.get());
    

    //==============================================================================
    PlugDataPluginEditor(PlugDataAudioProcessor&, Console* console);
    ~PlugDataPluginEditor() override;

    Component::SafePointer<Console> console;
    
    
    void showNewObjectMenu();

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    
    bool keyPressed(const KeyPress& key, Component* originatingComponent) override;


    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void openProject();
    void saveProject();
    void saveProjectAs(std::function<void()> nestedCallback = [](){});
    
    void addTab(Canvas* cnv);
    
    void openFile(String path) override;

    Canvas* getCurrentCanvas();
    Canvas* getMainCanvas();
    Canvas* getCanvas(int idx);

    void updateValues();

    void updateUndoState();

    TabComponent& getTabbar() { return tabbar; };

    PlugDataAudioProcessor& pd;

    const std::string defaultPatch = "#N canvas 827 239 527 327 12;";

    Canvas* mainCanvas = nullptr;
    AffineTransform transform;

    TabComponent tabbar;
    OwnedArray<Canvas, CriticalSection> canvases;
    Inspector inspector;

    LevelMeter levelmeter;

    TextButton bypassButton = TextButton(Icons::Power);
    TextButton lockButton = TextButton(Icons::Lock);
    TextButton connectionStyleButton = TextButton(Icons::ConnectionStyle);
    TextButton connectionPathfind = TextButton(Icons::Wand);

private:
    FileChooser saveChooser = FileChooser("Select a save file", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory), "*.pd");
    FileChooser openChooser = FileChooser("Choose file to open", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory), "*.pd");

    static constexpr int toolbarHeight = 40;
    static constexpr int statusbarHeight = 24;
    static constexpr int dragbarWidth = 10;
    int sidebarWidth = 275;

    bool sidebarHidden = false;

    std::array<TextButton, 11> toolbarButtons = {
        TextButton(Icons::New), TextButton(Icons::Open), TextButton(Icons::Save), TextButton(Icons::SaveAs), TextButton(Icons::Undo), TextButton(Icons::Redo), TextButton(Icons::Add), TextButton(Icons::Settings), TextButton(Icons::Hide),
        TextButton(Icons::Console), TextButton(Icons::Inspector) };

    TextButton& hideButton = toolbarButtons[8];

    std::unique_ptr<SettingsDialog> settingsDialog = nullptr;

    int dragStartWidth = 0;
    bool draggingSidebar = false;

    Array<TextButton> sidebarSelectors = {};


    ComponentBoundsConstrainer restrainer;
    std::unique_ptr<ResizableCornerComponent> resizer;
    
    SharedResourcePointer<TooltipWindow> tooltipWindow;
    
    Component seperators[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataPluginEditor)
};
