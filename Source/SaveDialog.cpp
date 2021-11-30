#include <JuceHeader.h>
#include "SaveDialog.h"

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
