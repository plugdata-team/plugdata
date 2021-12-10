/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <JuceHeader.h>
#include "Dialogs.h"


SaveDialog::SaveDialog()
{
    setLookAndFeel(&mainLook);
    setSize(400, 200);
    addAndMakeVisible(savelabel);
    addAndMakeVisible(cancel);
    addAndMakeVisible(dontsave);
    addAndMakeVisible(save);
    cancel.onClick = [this]
    {
        cb(0);
        delete this; // yikesss
    };
    save.onClick = [this]
    {
        cb(2);
        delete this; // yikesss
    };
    dontsave.onClick = [this]
    {
        cb(1);
        delete this; // yikesss
    };
    cancel.changeWidthToFitText();
    dontsave.changeWidthToFitText();
    save.changeWidthToFitText();
    setOpaque(false);
    addToDesktop (ComponentPeer::windowHasDropShadow);
}

SaveDialog::~SaveDialog()
{
    removeFromDesktop();
    setLookAndFeel(nullptr);
}

void SaveDialog::resized()
{
    savelabel.setBounds(20, 25, 200, 30);
    cancel.setBounds(20, 80, 80, 25);
    dontsave.setBounds(200, 80, 80, 25);
    save.setBounds(300, 80, 80, 25);
}



void SaveDialog::paint(Graphics & g)
{
    g.setColour(Colour(60, 60, 60));
    g.drawRect(getLocalBounds());
    
    g.setColour(MainLook::secondBackground);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
    
    g.setColour(findColour(ComboBox::outlineColourId));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 1.0f);
}

void SaveDialog::show(Component* centre, std::function<void(int)> callback) {
    
    SaveDialog* dialog = new SaveDialog;
    dialog->cb = callback;
    
    centre->addAndMakeVisible(dialog);
    
    dialog->setBounds((centre->getWidth() / 2.) - 200., 40, 400, 130);
}


ArrayDialog::ArrayDialog() {
    setLookAndFeel(&mainLook);
    setSize(400, 200);
    
    addAndMakeVisible(label);
    addAndMakeVisible(cancel);
    addAndMakeVisible(ok);
    
    cancel.onClick = [this]
    {
        cb(0, "", "");
        delete this;
    };
    ok.onClick = [this]
    {
        cb(1, nameEditor.getText(), sizeEditor.getText());
        delete this;
    };
    
    sizeEditor.setInputRestrictions(10, "0123456789");

    cancel.changeWidthToFitText();
    ok.changeWidthToFitText();
    
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(sizeLabel);
    
    addAndMakeVisible(nameEditor);
    addAndMakeVisible(sizeEditor);

    setOpaque(false);
    addToDesktop (ComponentPeer::windowHasDropShadow);
}

ArrayDialog::~ArrayDialog() {
    setLookAndFeel(nullptr);
}

void ArrayDialog::show(Component* centre, std::function<void(int, String, String)> callback) {
    
    ArrayDialog* dialog = new ArrayDialog;
    dialog->cb = callback;
    
    centre->addAndMakeVisible(dialog);
    
    dialog->setBounds((centre->getWidth() / 2.) - 200., 40, 300, 180);
}

void ArrayDialog::resized()  {
    label.setBounds(20, 7, 200, 30);
    cancel.setBounds(30, getHeight() - 40, 80, 25);
    ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);
    
    nameEditor.setBounds(65, 45, getWidth() - 85, 25);
    sizeEditor.setBounds(65, 85, getWidth() - 85, 25);
    nameLabel.setBounds(8, 45, 52, 25);
    sizeLabel.setBounds(8, 85, 52, 25);
}

void ArrayDialog::paint(Graphics & g)  {
    g.setColour(Colour(60, 60, 60));
    g.drawRect(getLocalBounds());
    
    g.setColour(MainLook::secondBackground);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
    
    g.setColour(findColour(ComboBox::outlineColourId));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 1.0f);
}

