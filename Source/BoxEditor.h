/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "LookAndFeel.h"
#include "MultiComponentDragger.h"
#include <JuceHeader.h>

// Text element in suggestion box
class SuggestionComponent : public TextButton {

    int type = -1;
    Array<Colour> colours = { findColour(ScrollBar::thumbColourId), Colours::yellow };

    Array<String> letters = { "pd", "~" };

public:
    SuggestionComponent()
    {
        setText("");
        setWantsKeyboardFocus(true);
        setConnectedEdges(12);
        setClickingTogglesState(true);
        setRadioGroupId(1001);
        setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
    }

    ~SuggestionComponent()
    {
    }


    void setText(String name)
    {
        setButtonText(name);
        type = name.contains("~") ? 1 : 0;

        repaint();
    }

    void paint(Graphics& g) override
    {
        TextButton::paint(g);

        if (type == -1)
            return;

        g.setColour(colours[type].withAlpha(float(0.8)));
        Rectangle<int> iconbound = getLocalBounds().reduced(4);
        iconbound.setWidth(getHeight() - 8);
        iconbound.translate(3, 0);
        g.fillRect(iconbound);

        g.setColour(Colours::white);
        g.drawFittedText(letters[type], iconbound.reduced(1), Justification::centred, 1);
    }
};

// Box with suggestions for object names
class Box;
class ClickLabel;
class SuggestionBox : public Component, public KeyListener, public TextEditor::InputFilter
{

public:
    bool selecting = false;

    SuggestionBox(Resources& r);

    ~SuggestionBox();

    void createCalloutBox(Box* box, TextEditor* editor);

    void move(int step, int setto = -1);

    TextEditor* openedEditor = nullptr;
    Box* currentBox;

    void resized() override;

    
private:
    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    bool keyPressed(const KeyPress& key, Component* originatingComponent) override;

    String filterNewText(TextEditor& e, const String& newInput) override;

    bool running = false;
    int numOptions = 0;
    int currentidx = 0;
    
    std::unique_ptr<Viewport> port;
    std::unique_ptr<Component> buttonholder;
    OwnedArray<SuggestionComponent> buttons;

    Array<Colour> colours = { MainLook::firstBackground, MainLook::secondBackground };

    Colour bordercolor = Colour(142, 152, 155);
    
    BoxEditorLook editorLook;

    int highlightStart = 0;
    int highlightEnd = 0;

    bool isCompleting = false;
};

// Label that shows the box name and can be double clicked to change the text
class ClickLabel : public Component,
                   public SettableTooltipClient,
                    public Value::Listener,
                   protected TextEditor::Listener
{
public:
    ClickLabel(Box* parent, MultiComponentDragger<Box>& multiDragger);

    ~ClickLabel()
    {
        setLookAndFeel(nullptr);
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    
    TextEditor* createEditorComponent();
    void editorAboutToBeHidden(TextEditor*);
    
    void inputAttemptWhenModal() override;

    void valueChanged (Value& v) override
    {
        if (lastTextValue != textValue.toString())
            setText (textValue.toString(), sendNotification);
    }
    
    Box* box;

    bool isDown = false;
    MultiComponentDragger<Box>& dragger;
    
    
    
    void setText (const String& newText,
                  NotificationType notification);


    String getText (bool returnActiveEditorContents = false) const;

    
    Font getFont() { return font; }
    
    void setEditable(bool editable);
    
// Simplified version of JUCE label class, because we need to modify the behaviour for the suggestions box
    void showEditor();

    /** Hides the editor if it was being shown.

        @param discardCurrentEditorContents     if true, the label's text will be
                                                reset to whatever it was before the editor
                                                was shown; if false, the current contents of the
                                                editor will be used to set the label's text
                                                before it is hidden.
    */
    void hideEditor (bool discardCurrentEditorContents);

    /** Returns true if the editor is currently focused and active. */
    bool isBeingEdited() const noexcept;

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    TextEditor* getCurrentTextEditor() const noexcept;
    
    void setBorderSize (BorderSize<int> newBorder)
    {
        if (border != newBorder)
        {
            border = newBorder;
            repaint();
        }
    }
    
    void textEditorReturnKeyPressed (TextEditor& ed) override;
    
    std::function<void()> onEditorShow, onEditorHide, onTextChange;

protected:

    /** Called when the text editor has just appeared, due to a user click or other focus change. */
    virtual void editorShown (TextEditor*);

    void paint (Graphics&) override;
    void resized() override;
    void mouseDoubleClick (const MouseEvent&) override;
    

    
    //==============================================================================
    Value textValue;
    String lastTextValue;
    Font font { 15.0f };
    Justification justification = Justification::centred;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border { 1, 5, 1, 5 };
    float minimumHorizontalScale = 0;
    TextInputTarget::VirtualKeyboardType keyboardType = TextInputTarget::textKeyboard;
    bool editDoubleClick = false;

    bool updateFromTextEditorContents (TextEditor&);
};
