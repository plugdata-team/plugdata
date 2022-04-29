struct SaveDialog : public Component
{
    SaveDialog(Component* editor, const String& filename) : savelabel("savelabel", "Save Changes to \"" + filename + "\"?")
    {
        setSize(400, 200);
        addAndMakeVisible(savelabel);
        addAndMakeVisible(cancel);
        addAndMakeVisible(dontsave);
        addAndMakeVisible(save);

        cancel.onClick = [this]
        {
            MessageManager::callAsync(
                [this]()
                {
                    background->setVisible(false);
                    cb(0);
                    delete this;
                });
        };
        save.onClick = [this]
        {
            MessageManager::callAsync(
                [this]()
                {
                    cb(2);
                    delete this;
                });
        };
        dontsave.onClick = [this]
        {
            MessageManager::callAsync(
                [this]()
                {
                    cb(1);
                    delete this;
                });
        };

        background.reset(new BlackoutComponent(editor, this));

        cancel.changeWidthToFitText();
        dontsave.changeWidthToFitText();
        save.changeWidthToFitText();
        setOpaque(false);
    }

    void resized() override
    {
        savelabel.setBounds(20, 25, 200, 30);
        cancel.setBounds(20, 80, 80, 25);
        dontsave.setBounds(200, 80, 80, 25);
        save.setBounds(300, 80, 80, 25);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 5.0f);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 5.0f, 1.0f);
    }

    std::function<void(int)> cb;

   private:
    std::unique_ptr<BlackoutComponent> background;

    Label savelabel;

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");
};
