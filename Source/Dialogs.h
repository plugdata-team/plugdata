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
