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


struct DAWAudioSettings : public Component
{

    DAWAudioSettings(AudioProcessor& p) : processor(p) {
        
        addAndMakeVisible(latencySlider);
        latencySlider.setRange (0, 88200, 1);
        latencySlider.setTextValueSuffix (" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        
        latencySlider.onValueChange = [this](){
            processor.setLatencySamples(latencySlider.getValue());
        };
        
        addAndMakeVisible (latencyLabel);
        latencyLabel.setText ("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);
    }
    
    
    void resized() override {
        latencySlider.setBounds(90, 5, getWidth() - 130, 20);
    }
    
    
    void visibilityChanged() override {
        latencySlider.setValue(processor.getLatencySamples());
    }
    
    AudioProcessor& processor;
    Label  latencyLabel;
    Slider latencySlider;
    
};

struct SettingsComponent : public Component
{
        

    SettingsComponent(AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths);
    
    ~SettingsComponent(){
        for(auto& button : toolbarButtons)
            button->setLookAndFeel(nullptr);
    }

    void paint(Graphics& g);
    
    void resized();

    AudioDeviceManager* deviceManager = nullptr;
    std::unique_ptr<Component> audioSetupComp;
    
    std::unique_ptr<Component> libraryPanel;
    

    int toolbarHeight = 50;
    
    ToolbarLook lnf = ToolbarLook(true);
    
    OwnedArray<TextButton> toolbarButtons = {new TextButton(CharPointer_UTF8("\xef\x80\xa8")), new TextButton(CharPointer_UTF8 ("\xef\x80\x82"))};
    
};


struct SettingsDialog : public Component
{
    MainLook mainLook;
    SettingsComponent settingsComponent;
    ComponentDragger dragger;
    
    ComponentBoundsConstrainer constrainer;
    
    SettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths) : settingsComponent(processor, manager, settingsTree, updatePaths) {
        
        setLookAndFeel(&mainLook);
        closeButton.reset(getLookAndFeel().createDocumentWindowButton(4));
        
        setCentrePosition(400, 400);
        setSize(600, 400);
        
        setVisible (false);
        
        addAndMakeVisible(&settingsComponent);
        addAndMakeVisible(closeButton.get());
        
        settingsComponent.addMouseListener(this, false);
        
        closeButton->onClick = [this](){
            setVisible(false);
        };
        
        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);
        
    }
    
    ~SettingsDialog() {
        setLookAndFeel(nullptr);

    }
    
    void mouseDown(const MouseEvent& e) {
        if(e.getPosition().getY() < 30) {
            dragger.startDraggingComponent(this, e);
        }
    }
    
    void mouseDrag(const MouseEvent& e) {
        dragger.dragComponent(this, e, &constrainer);
    }
    
    void resized() {
        closeButton->setBounds(getWidth() - 30, 0, 30, 30);
        settingsComponent.setBounds(getLocalBounds());
    }
    
    void paint(Graphics& g) {
        g.fillAll(MainLook::firstBackground);
    }
    
    void paintOverChildren(Graphics& g) {
        g.setColour(Colours::white);
        g.drawText("Settings", 0, 0, getWidth(), 30, Justification::centred,
                   true);
    }
    
    
    void closeButtonPressed()
    {
        setVisible(false);
    }
    
    std::unique_ptr<Button> closeButton;
};
