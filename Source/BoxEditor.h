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
    BoxEditorLook editorLook;

    int type = -1;
    Array<Colour> colours = { findColour(ScrollBar::thumbColourId), Colours::yellow };

    Array<String> letters = { "PD", "~" };

public:
    SuggestionComponent()
    {
        setText("");
        setWantsKeyboardFocus(false);
        setConnectedEdges(12);
        setClickingTogglesState(true);
        setRadioGroupId(1001);
        setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));

        setLookAndFeel(&editorLook);
    }

    ~SuggestionComponent()
    {
        setLookAndFeel(nullptr);
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
        Rectangle<int> iconbound = getLocalBounds().reduced(3);
        iconbound.setWidth(getHeight() - 6);
        iconbound.translate(3, 0);
        g.fillRect(iconbound);

        g.setColour(Colours::white);
        g.drawFittedText(letters[type], iconbound.reduced(1), Justification::centred, 1);
    }
};

// Box with suggestions for object names
class Box;
class ClickLabel;
class SuggestionBox : public Component, public KeyListener, public TextEditor::InputFilter {

public:
    bool selecting = false;

    SuggestionBox();

    ~SuggestionBox();

    void createCalloutBox(Box* box, TextEditor* editor);

    void move(int step, int setto = -1);

    TextEditor* openedEditor = nullptr;
    Box* currentBox;

    void resized() override;

private:
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

    int highlightStart = 0;
    int highlightEnd = 0;

    bool isCompleting = false;
};

// Label that shows the box name and can be double clicked to change the text
class ClickLabel : public Label {
public:
    ClickLabel(Box* parent, MultiComponentDragger<Box>& multiDragger);

    ~ClickLabel()
    {
        setLookAndFeel(nullptr);
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    TextEditor* createEditorComponent() override;
    void editorAboutToBeHidden(TextEditor*) override;

    Box* box;

    SuggestionBox suggestor;

    bool isDown = false;
    MultiComponentDragger<Box>& dragger;
};
