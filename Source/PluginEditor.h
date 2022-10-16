/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Dialogs/Dialogs.h"
#include "Sidebar/Sidebar.h"
#include "Statusbar.h"

#ifndef PLUGDATA_STANDALONE
#define PLUGDATA_ROUNDED 0
#else
#define PLUGDATA_ROUNDED 1
#endif

enum CommandIDs
{
    NewProject = 1,
    OpenProject,
    SaveProject,
    SaveProjectAs,
    CloseTab,
    Undo,
    Redo,

    Lock,
    ConnectionStyle,
    ConnectionPathfind,
    ZoomIn,
    ZoomOut,
    ZoomNormal,
    Copy,
    Paste,
    Cut,
    Delete,
    Duplicate,
    Encapsulate,
    SelectAll,
    ShowBrowser,
    NewObject,
    NewComment,
    NewBang,
    NewMessage,
    NewToggle,
    NewNumbox,
    NewVerticalSlider,
    NewHorizontalSlider,
    NewVerticalRadio,
    NewHorizontalRadio,
    NewFloatAtom,
    NewSymbolAtom,
    NewListAtom,
    NewArray,
    NewGraphOnParent,
    NewCanvas,
    NewKeyboard,
    NewVUMeterObject,
    NewButton,
    NewNumboxTilde,
    NewOscilloscope,
    NewFunction,
    NumItems
};

struct WelcomePanel : public Component
{
    
    struct WelcomeButton : public Component
    {
        String iconText;
        String topText;
        String bottomText;
        
        std::function<void(void)> onClick = [](){};
        
        WelcomeButton(String icon, String mainText, String subText) :
        iconText(icon),
        topText(mainText),
        bottomText(subText)
        {
            setInterceptsMouseClicks(true, false);
            setAlwaysOnTop(true);
            
        }
        
        void paint(Graphics& g)
        {
            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            
            g.setFont(lnf->iconFont.withHeight(24));
            g.drawText(iconText, 20, 5, 40, 40, Justification::centredLeft);
            
            g.setFont(lnf->defaultFont.withHeight(16));
            g.drawText(topText, 60, 7, getWidth() - 60, 20, Justification::centredLeft);
            
            g.setFont(lnf->thinFont.withHeight(14));
            g.drawText(bottomText, 60, 25, getWidth() - 60, 16, Justification::centredLeft);
            
            if(isMouseOver()) {
                g.drawRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, 4.0f, 0.5f);
            }
        }
        
        void mouseUp(const MouseEvent& e)
        {
            onClick();
        }
        
        void mouseEnter(const MouseEvent& e)
        {
            repaint();
        }
        
        void mouseExit(const MouseEvent& e)
        {
            repaint();
        }
        
    };
    
    WelcomePanel() {
        newButton = std::make_unique<WelcomeButton>(Icons::New, "New Patch", "Create a new empty patch");
        openButton = std::make_unique<WelcomeButton>(Icons::Open, "Open Patch", "Open a saved patch");
        
        addAndMakeVisible(newButton.get());
        addAndMakeVisible(openButton.get());
    }
    
    void resized() override
    {
        newButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, -30));
        openButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, 30));
        
    }
    
    void paint(Graphics& g) override
    {
        auto styles = Font(32).getAvailableStyles();
        
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(32));
        g.drawText("No Patch Open", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        
        g.setFont(lnf->thinFont.withHeight(23));
        g.drawText("Open a file to begin patching", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
    }
    
    std::unique_ptr<WelcomeButton> newButton;
    std::unique_ptr<WelcomeButton> openButton;
};

struct TabComponent : public TabbedComponent
{
    std::function<void(int)> onTabChange = [](int) {};
    std::function<void()> newTab = []() {};
    std::function<void()> openProject = [](){};
    
    TextButton newButton = TextButton(Icons::Add);

    TabComponent() : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
        addAndMakeVisible(newButton);
        newButton.setName("tabbar:newbutton");
        newButton.onClick = [this](){
            newTab();
        };
        
        addAndMakeVisible(welcomePanel);
        
        welcomePanel.newButton->onClick = [this](){
            newTab();
        };
        
        welcomePanel.openButton->onClick = [this](){
            openProject();
        };
  
        
        setTabBarDepth(0);
        
        
    }
    
    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        if(getNumTabs() == 0) {
            setTabBarDepth(0);
            welcomePanel.setVisible(true);
        }
        else {
            welcomePanel.setVisible(false);
            setTabBarDepth(28);
        }
        
        onTabChange(newCurrentTabIndex);
    }
    
    void resized() override
    {
        int depth = getTabBarDepth();
        auto content = getLocalBounds();
        
        welcomePanel.setBounds(content);
        newButton.setBounds(0, 0, depth, depth);
        
        auto tabBounds = content.removeFromTop(depth).withTrimmedLeft(depth);
        tabs->setBounds(tabBounds);

        for(int c = 0; c < getNumTabs(); c++) {
            if (auto* comp = getTabContentComponent(c)) {
                comp->setBounds (content);
            }
        }
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());
        
        g.drawLine(Line<float>(getTabBarDepth() - 0.5f, 0, getTabBarDepth() - 0.5f, getTabBarDepth()), 1.0f);

        g.drawLine(0, 0, getWidth(), 0);

    }
    
    WelcomePanel welcomePanel;
};

struct ZoomLabel : public TextButton, public Timer
{
    ZoomLabel() {
        setInterceptsMouseClicks(false, false);
    }

    void setZoomLevel(float value)
    {
        setButtonText(String(value * 100, 1) + "%");
        startTimer(2000);
        
        if(!labelAnimator.isAnimating(this)) {
            labelAnimator.fadeIn(this, 200);
        }
    }
    
    void timerCallback() override
    {
        labelAnimator.fadeOut(this, 200);
    }
    
    ComponentAnimator labelAnimator;
};

struct WelcomeButton;
class Canvas;
class PlugDataAudioProcessor;
class PlugDataPluginEditor : public AudioProcessorEditor, public Value::Listener, public ValueTree::Listener, public ApplicationCommandTarget, public ApplicationCommandManager, public Timer
{
   public:
    explicit PlugDataPluginEditor(PlugDataAudioProcessor&);

    ~PlugDataPluginEditor() override;

    void paint(Graphics& g) override;

    void resized() override;

    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void mouseMagnify(const MouseEvent& e, float scaleFactor) override;

#ifdef PLUGDATA_STANDALONE
    // For dragging parent window
    void mouseDrag(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;

    ComponentDragger windowDragger;
#endif

    void newProject();
    void openProject();
    void saveProject(const std::function<void()>& nestedCallback = []() {});
    void saveProjectAs(const std::function<void()>& nestedCallback = []() {});

    void addTab(Canvas* cnv, bool deleteWhenClosed = false);

    Canvas* getCurrentCanvas();
    Canvas* getCanvas(int idx);
    
    // Pass modifier keys to statusbar
    void modifierKeysChanged(const ModifierKeys& modifiers) override { statusbar.modifierKeysChanged(modifiers); }

    void valueChanged(Value& v) override;

    void updateCommandStatus();

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    void timerCallback() override;

    PlugDataAudioProcessor& pd;

    AffineTransform transform;

    TabComponent tabbar;
    OwnedArray<Canvas, CriticalSection> canvases;
    Sidebar sidebar;
    Statusbar statusbar;

    std::atomic<bool> canUndo = false, canRedo = false;

    std::unique_ptr<Dialog> openedDialog = nullptr;
    
    Value theme;
    Value zoomScale;
    
   private:
    
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

#ifdef PLUGDATA_STANDALONE
    static constexpr int toolbarHeight = 45;
#else
    static constexpr int toolbarHeight = 40;
#endif

    OwnedArray<TextButton> toolbarButtons;

    SharedResourcePointer<TooltipWindow> tooltipWindow;

    TextButton seperators[2];
    
    ZoomLabel zoomLabel;


    enum ToolbarButtonType
    {
        Open = 0,
        Save,
        SaveAs,
        Undo,
        Redo,
        Add,
        Settings,
        Hide,
        Pin
    };

    TextButton* toolbarButton(ToolbarButtonType type)
    {
        return toolbarButtons[static_cast<int>(type)];
    }
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataPluginEditor)
};
