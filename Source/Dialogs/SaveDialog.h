/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SaveDialog : public Component {

public:
    SaveDialog(Dialog* parent, String const& filename, std::function<void(int)> callback)
        : savelabel("savelabel", filename.isEmpty() ? "Save changes before closing?" : "Save changes to \"" + filename + "\"\n before closing?")
    {
        cb = callback;
        setSize(265, 280);
        addAndMakeVisible(savelabel);
        addAndMakeVisible(cancel);
        addAndMakeVisible(dontsave);
        addAndMakeVisible(save);
        
        savelabel.setFont(Fonts::getBoldFont().withHeight(15.0f));
        savelabel.setJustificationType(Justification::centred);
        
        cancel.onClick = [parent] {
            MessageManager::callAsync(
                [parent]() {
                    cb(0);
                    parent->closeDialog();
                });
        };
        save.onClick = [parent] {
            MessageManager::callAsync(
                [parent]() {
                    cb(2);
                    parent->closeDialog();
                });
        };
        dontsave.onClick = [parent] {
            MessageManager::callAsync(
                [parent]() {
                    cb(1);
                    parent->closeDialog();
                });
        };

        cancel.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        dontsave.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        save.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        
        cancel.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
        dontsave.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
        save.setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
        
        setOpaque(false);
    }
    
    void paint(Graphics& g) override
    {
        auto contentBounds = getLocalBounds().reduced(16);
        auto logoBounds = contentBounds.removeFromTop(contentBounds.getHeight() / 3.5f).withSizeKeepingCentre(64, 64);
        g.drawImage(logo, logoBounds.toFloat());
    }

    void resized() override
    {
        auto contentBounds = getLocalBounds().reduced(16);
        
        // logo space
        contentBounds.removeFromTop(contentBounds.getHeight() / 3.5f);
        
        contentBounds.removeFromTop(8);
        savelabel.setBounds(contentBounds.removeFromTop(contentBounds.getHeight() / 3));
        contentBounds.removeFromTop(8);
        
        save.setBounds(contentBounds.removeFromTop(26));
        contentBounds.removeFromTop(6);
        dontsave.setBounds(contentBounds.removeFromTop(26));
        contentBounds.removeFromTop(16);
        cancel.setBounds(contentBounds.removeFromTop(26));
    }

    static inline std::function<void(int)> cb = [](int) {};

private:
    Label savelabel;

    Image logo = ImageFileFormat::loadFrom(BinaryData::plugdata_logo_png, BinaryData::plugdata_logo_pngSize);

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");
};
