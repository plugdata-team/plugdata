/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

extern "C"
{
#include <m_pd.h>
}

#include "Edge.h"
#include "GUIObjects.h"

class Canvas;
class Box : public Component, public Value::Listener, private TextEditor::Listener
{
    bool isOver = false;

   public:
    explicit Box(Canvas* parent, const String& name = "", Point<int> position = {100, 100});

    Box(pd::Object* object, Canvas* parent, const String& name = "", Point<int> position = {100, 100});

    ~Box() override;

    void valueChanged(Value& v) override;

    void paint(Graphics&) override;
    void resized() override;

    void updatePorts();

    std::unique_ptr<pd::Object> pdObject = nullptr;

    int numInputs = 0;
    int numOutputs = 0;
    Value locked;
    Value commandLocked;

    Canvas* cnv;

    std::unique_ptr<GUIComponent> graphics = nullptr;

    OwnedArray<Edge> edges;

    ComponentBoundsConstrainer restrainer;
    ResizableBorderComponent resizer;

    void setType(const String& newType, bool exists = false);

    void showEditor();
    void hideEditor();

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    TextEditor* getCurrentTextEditor() const noexcept;

    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;

    String getText(bool returnActiveEditorContents = false) const;

   private:
    void initialise();
    bool hitTest(int x, int y) override;

    void setText(const String& newText, NotificationType notification);

    void setEditable(bool editable);

    void textEditorReturnKeyPressed(TextEditor& ed) override;

    std::function<void()> onTextChange;

   protected:
    void mouseDoubleClick(const MouseEvent&) override;

    bool hideLabel = false;

    Value textValue;
    String lastTextValue;
    Font font{15.0f};
    Justification justification = Justification::centred;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border{1, 5, 1, 5};
    float minimumHorizontalScale = 0;
    TextInputTarget::VirtualKeyboardType keyboardType = TextInputTarget::textKeyboard;
    bool editDoubleClick = false;

    Colour outline = findColour(Slider::thumbColourId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
};
