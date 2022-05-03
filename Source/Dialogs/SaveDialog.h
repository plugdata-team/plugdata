struct SaveDialog : public Component
{
    SaveDialog(Component* editor, Dialog* parent, const String& filename) : savelabel("savelabel", "Save Changes to \"" + filename + "\"?")
    {
        setSize(400, 200);
        addAndMakeVisible(savelabel);
        addAndMakeVisible(cancel);
        addAndMakeVisible(dontsave);
        addAndMakeVisible(save);

        cancel.onClick = [this, parent]
        {
            MessageManager::callAsync(
                [this, parent]()
                {
                    cb(0);
                    parent->onClose();
                });
        };
        save.onClick = [this, parent]
        {
            MessageManager::callAsync(
                [this, parent]()
                {
                    cb(2);
                    parent->onClose();
                });
        };
        dontsave.onClick = [this, parent]
        {
            MessageManager::callAsync(
                [this, parent]()
                {
                    cb(1);
                    parent->onClose();
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

    std::function<void(int)> cb;

   private:

    Label savelabel;

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");
};
