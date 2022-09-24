/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct ArrayDialog : public Component {
    ArrayDialog(Component* editor, Dialog* parent)
    {
        setSize(400, 200);

        addAndMakeVisible(label);
        addAndMakeVisible(cancel);
        addAndMakeVisible(ok);

        cancel.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    cb(0, "", "");
                    parent->closeDialog();
                });
        };

        ok.onClick = [this, parent] {
            // Check if input is valid
            if (nameEditor.isEmpty()) {
                nameEditor.setColour(TextEditor::outlineColourId, Colours::red);
                nameEditor.giveAwayKeyboardFocus();
                nameEditor.repaint();
            }
            if (sizeEditor.getText().getIntValue() < 0) {
                sizeEditor.setColour(TextEditor::outlineColourId, Colours::red);
                sizeEditor.giveAwayKeyboardFocus();
                sizeEditor.repaint();
            }
            if (nameEditor.getText().isNotEmpty() && sizeEditor.getText().getIntValue() >= 0) {
                MessageManager::callAsync(
                    [this, parent]() {
                        cb(1, nameEditor.getText(), sizeEditor.getText());
                        parent->closeDialog();
                    });
            }
        };

        sizeEditor.setInputRestrictions(10, "0123456789");

        cancel.changeWidthToFitText();
        ok.changeWidthToFitText();

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(sizeLabel);

        addAndMakeVisible(nameEditor);
        addAndMakeVisible(sizeEditor);

        nameEditor.setText("array1");
        sizeEditor.setText("100");

        setOpaque(false);
    }

    void resized() override
    {
        label.setBounds(20, 7, 200, 30);
        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

        nameEditor.setBounds(65, 45, getWidth() - 85, 25);
        sizeEditor.setBounds(65, 85, getWidth() - 85, 25);
        nameLabel.setBounds(8, 45, 52, 25);
        sizeLabel.setBounds(8, 85, 52, 25);
    }

    std::function<void(int, String, String)> cb;

private:
    Label label = Label("savelabel", "Array Properties");

    Label nameLabel = Label("namelabel", "Name:");
    Label sizeLabel = Label("sizelabel", "Size:");

    TextEditor nameEditor;
    TextEditor sizeEditor;

    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");
};
