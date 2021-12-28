/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"


class SaveDialog : public Component
{

    MainLook mainLook;

    Label savelabel = Label("savelabel", "Save Changes?");

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");

    void resized() override;

    void paint(Graphics & g) override;
    

    std::function<void(int)> cb;
    
public:
    
    
    static void show(Component* centre, std::function<void(int)> callback);

    SaveDialog();

    ~SaveDialog();


};


class ArrayDialog : public Component
{
    

    MainLook mainLook;

    Label label = Label("savelabel", "Array Properties");
    
    Label nameLabel = Label("namelabel", "Name:");
    Label sizeLabel = Label("sizelabel", "Size:");


    TextEditor nameEditor;
    TextEditor sizeEditor;
    
    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");

    void resized() override;

    void paint(Graphics & g) override;
    
    std::function<void(int, String, String)> cb;
    
public:
    
    static void show(Component* centre, std::function<void(int, String, String)> callback);
    
    
    
    ArrayDialog();

    ~ArrayDialog();

};



struct SettingsComponent : public Component
{
        

    SettingsComponent(AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths);
    
    ~SettingsComponent(){
        for(auto& button : toolbarButtons)
            button->setLookAndFeel(nullptr);
    }

    void paint(Graphics& g);
        
    void resized();

    AudioDeviceManager* deviceManager = nullptr;
    std::unique_ptr<AudioDeviceSelectorComponent> audioSetupComp;
    
    std::unique_ptr<Component> libraryPanel;
    

    int toolbarHeight = 50;
    
    ToolbarLook lnf = ToolbarLook(true);
    
    OwnedArray<TextButton> toolbarButtons = {new TextButton(CharPointer_UTF8("\xef\x80\xa8")), new TextButton(CharPointer_UTF8 ("\xef\x80\x82"))};
    
};


struct SettingsDialog : public DocumentWindow
{
    MainLook mainLook;
    SettingsComponent settingsComponent;
   
    
    SettingsDialog(AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths) : DocumentWindow("Settings",
                                      Colour(50, 50, 50),
                                      DocumentWindow::allButtons), settingsComponent(manager, settingsTree, updatePaths) {
        
        setUsingNativeTitleBar (true);
        
        setCentrePosition(400, 400);
        setSize(600, 400);
        
        setVisible (false);
        
        setResizable(false, false);
        
        setContentOwned (&settingsComponent, false);

        setLookAndFeel(&mainLook);
    }
    
    ~SettingsDialog() {
        setLookAndFeel(nullptr);

    }
    
    void resized() {
        settingsComponent.setBounds(getLocalBounds());
    
    }
    
    
    void closeButtonPressed()
    {
        setVisible(false);
    }
    
};
