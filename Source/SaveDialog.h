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

public:

	SaveDialog();

	~SaveDialog();


};
