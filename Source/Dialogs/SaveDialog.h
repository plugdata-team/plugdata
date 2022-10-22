struct SaveDialog : public Component {
    SaveDialog(Component* editor, Dialog* parent, String const& filename, std::function<void(int)> callback)
        : savelabel("savelabel", filename.isEmpty() ? "Save Changes?" : "Save Changes to \"" + filename + "\"?"), cb(callback)
    {
        setSize(400, 200);
        addAndMakeVisible(savelabel);
        addAndMakeVisible(cancel);
        addAndMakeVisible(dontsave);
        addAndMakeVisible(save);

        cancel.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    parent->closeDialog();
                    cb(0);
                });
        };
        save.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    parent->closeDialog();
                    cb(2);
                });
        };
        dontsave.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    parent->closeDialog();
                    cb(1);
                });
        };

        cancel.changeWidthToFitText();
        dontsave.changeWidthToFitText();
        save.changeWidthToFitText();
        setOpaque(false);
    }

    void resized() override
    {
        savelabel.setBounds(20, 25, 360, 30);
        cancel.setBounds(20, 80, 80, 25);
        dontsave.setBounds(200, 80, 80, 25);
        save.setBounds(300, 80, 80, 25);
    }

    std::function<void(int)> cb = [](int){};

private:
    Label savelabel;

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");
};
