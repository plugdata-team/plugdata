#pragma once

#include "LookAndFeel.h"
#include "Canvas.h"
#include "PlugData.h"
#include "Utility/gin_valuetreeobject.h"

#include "Console.h"
#include "Utility.h"


#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

struct TabComponent : public TabbedComponent
{
    
    std::function<void(int)> on_tab_change = [](int){};
    
    TabComponent() : TabbedComponent(TabbedButtonBar::TabsAtTop) {
        
    }
    
    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        on_tab_change(newCurrentTabIndex);
    }

    
};


class MainComponent : public Component, public ValueTreeObject
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    
    Canvas* canvas = nullptr;
    
    Console console;
    PlugData pd;
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
        
    void open_project();
    void save_project();
    
    void pd_synchonize();
    
    void add_tab(Canvas* cnv);
    
    Canvas* get_current_canvas();

    
    void valueTreeChanged() override;
    
    ValueTreeObject* factory (const Identifier&, const ValueTree&) override;
    
    TabComponent& get_tabbar()  { return tabbar; };
    
    static inline File home_dir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("Cerite");
    static inline File app_dir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
    
private:
    
    
    FileChooser save_chooser =  FileChooser("Select a save file", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("Cerite").getChildFile("Saves"), "*.pd");
    FileChooser open_chooser = FileChooser("Choose file to open", File::getSpecialLocation( File::SpecialLocationType::userDocumentsDirectory).getChildFile("Cerite").getChildFile("Saves"), "*.pd");
    
    //std::unique_ptr<PatchInterface> player;
    
    int toolbar_height = 45;
    int statusbar_height = 27;
    int sidebar_width = 300;
    int dragbar_width = 10;
    
    bool sidebar_hidden = false;


    std::array<TextButton, 7> toolbar_buttons = {TextButton(CharPointer_UTF8("\xef\x85\x9b")), TextButton(CharPointer_UTF8("\xef\x81\xbb")), TextButton(CharPointer_UTF8("\xef\x80\x99")), TextButton(CharPointer_UTF8("\xef\x83\xa2")), TextButton(CharPointer_UTF8("\xef\x80\x9e")), TextButton(CharPointer_UTF8("\xef\x81\xa7")), TextButton(CharPointer_UTF8("\xef\x81\x94"))};
    
    TextButton& hide_button = toolbar_buttons[6];
    
    TextButton start_button = TextButton(CharPointer_UTF8("\xef\x80\x91"));

    //TextButton hide_button = TextButton("K");
    
    int drag_start_width = 0;
    bool dragging_sidebar = false;

    ToolbarLook toolbar_look;
    StatusbarLook statusbar_look = StatusbarLook(1.4);
    MainLook main_look;
    

    
    TabComponent tabbar;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
