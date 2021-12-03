#include <JuceHeader.h>
#include "Dialogs.h"

// Dialog that asks the user if it wants to save the current project before closing
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
		exitModalState(0);
	};
	save.onClick = [this]
	{
		exitModalState(1);
	};
	dontsave.onClick = [this]
	{
		exitModalState(2);
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
	g.setColour(Colour(20, 20, 20));
	g.fillRect(getLocalBounds());
}

ArrayDialog::ArrayDialog(String* size, String* name) : arrSize(size), arrName(name) {
    setLookAndFeel(&mainLook);
    setSize(400, 200);
    
    addAndMakeVisible(label);
    addAndMakeVisible(cancel);
    addAndMakeVisible(ok);
    
    cancel.onClick = [this]
    {
        exitModalState(0);
    };
    ok.onClick = [this]
    {
        *arrSize = sizeEditor.getText();
        *arrName = nameEditor.getText();
        exitModalState(1);
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

void ArrayDialog::resized()  {
    label.setBounds(20, 5, 200, 30);
    cancel.setBounds(30, getHeight() - 40, 80, 25);
    ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);
    
    nameEditor.setBounds(50, 60, getWidth() - 65, 25);
    sizeEditor.setBounds(50, 100, getWidth() - 65, 25);
    nameLabel.setBounds(5, 60, 45, 25);
    sizeLabel.setBounds(5, 100, 45, 25);
}

void ArrayDialog::paint(Graphics & g)  {
    g.setColour(Colour(60, 60, 60));
    g.drawRect(getLocalBounds());
    g.setColour(Colour(20, 20, 20));
    g.fillRect(getLocalBounds());
}

