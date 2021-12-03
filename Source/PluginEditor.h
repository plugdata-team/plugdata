#pragma once

#include "LookAndFeel.h"
#include "Utility/ValueTreeObject.h"
#include "Console.h"


#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

struct TabComponent : public TabbedComponent
{
    
    std::function<void(int)> onTabChange = [](int){};
    
    TabComponent() : TabbedComponent(TabbedButtonBar::TabsAtTop) {
        
    }
    
    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        onTabChange(newCurrentTabIndex);
    }
    
    

    
};

class Canvas;
class PlugDataAudioProcessor;
class PlugDataPluginEditor : public AudioProcessorEditor, public ValueTreeObject, public Timer
{
public:
    //==============================================================================
    PlugDataPluginEditor(PlugDataAudioProcessor&, Console* console);
    ~PlugDataPluginEditor() override;
    
    Component::SafePointer<Console> console;
    
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
        
    void openProject();
    void saveProject();
    
    void addTab(Canvas* cnv);
    
    Canvas* getCurrentCanvas();
    Canvas* getMainCanvas();
    Canvas* getCanvas(int idx);
    
    void timerCallback() override;

    
    void valueTreeChanged() override;
    
    ValueTreeObject* factory (const Identifier&, const ValueTree&) override;
    
    TabComponent& getTabbar()  { return tabbar; };
    
    static inline File homeDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("Pd");
    static inline File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PlugDataAudioProcessor& pd;
    
    const std::string defaultPatch = "#N canvas 827 239 527 327 12;";
    
    
private:
    

    Canvas* mainCanvas = nullptr;
    
    FileChooser saveChooser =  FileChooser("Select a save file", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("Cerite").getChildFile("Saves"), "*.pd");
    FileChooser openChooser = FileChooser("Choose file to open", File::getSpecialLocation( File::SpecialLocationType::userDocumentsDirectory).getChildFile("Cerite").getChildFile("Saves"), "*.pd");
    
    int toolbarHeight = 45;
    int statusbarHeight = 27;
    int sidebarWidth = 300;
    int dragbarWidth = 10;
    
    bool sidebarHidden = false;


    std::array<TextButton, 7> toolbarButtons = {TextButton(CharPointer_UTF8("\xef\x85\x9b")), TextButton(CharPointer_UTF8("\xef\x81\xbb")), TextButton(CharPointer_UTF8("\xef\x80\x99")), TextButton(CharPointer_UTF8("\xef\x83\xa2")), TextButton(CharPointer_UTF8("\xef\x80\x9e")), TextButton(CharPointer_UTF8("\xef\x81\xa7")), TextButton(CharPointer_UTF8("\xef\x81\x94"))};
    
    TextButton& hideButton = toolbarButtons[6];
    
    TextButton startButton = TextButton(CharPointer_UTF8("\xef\x80\x91"));
    
    int dragStartWidth = 0;
    bool draggingSidebar = false;

    ToolbarLook toolbarLook;
    StatusbarLook statusbarLook = StatusbarLook(1.4);
    MainLook mainLook;

    TabComponent tabbar;
    
    OpenGLContext openGLContext;
    
    ComponentBoundsConstrainer restrainer;
    std::unique_ptr<ResizableCornerComponent> resizer;

    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlugDataPluginEditor)
};
