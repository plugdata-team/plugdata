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
#include "Tabbar.h"

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
    

    void modifierKeysChanged(const ModifierKeys& modifiers) override;

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
    
    TextButton* toolbarButton(ToolbarButtonType type)
    {
        return toolbarButtons[static_cast<int>(type)];
    }
    

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

#if PLUGDATA_STANDALONE && JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif
    
    bool isMaximised = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataPluginEditor)
};
