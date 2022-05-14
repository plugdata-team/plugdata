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

#include "Components/ObjectGrid.h"
#include "Edge.h"
#include "Objects/GUIComponent.h"

class Canvas;
class Box : public Component, public Value::Listener, private TextEditor::Listener, public Timer
{
    bool isOver = false;

   public:
    Box(Canvas* parent, const String& name = "", Point<int> position = {100, 100});

    Box(pd::Object* object, Canvas* parent, const String& name = "");

    ~Box();

    void valueChanged(Value& v) override;

    void timerCallback() override;

    void paint(Graphics&) override;
    void resized() override;

    void updatePorts();

    void setType(const String& newType, bool exists = false);
    void updateBounds(bool newObject);

    void showEditor();
    void hideEditor();

    Array<Connection*> getConnections() const;

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    TextEditor* getCurrentTextEditor() const noexcept;

    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void mouseMove(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;

    int getBestTextWidth(const String& text);

    void setEditable(bool editable);
    Array<Rectangle<float>> getCorners() const;

    std::unique_ptr<pd::Object> pdObject = nullptr;

    int numInputs = 0;
    int numOutputs = 0;
    int widthOffset = 0;
    
    Value locked;
    Value commandLocked;
    Value presentationMode;

    ObjectGrid lastObjectGrid;

    Canvas* cnv;

    std::unique_ptr<GUIComponent> graphics = nullptr;

    OwnedArray<Edge> edges;
    ResizableBorderComponent::Zone resizeZone;

    static inline constexpr int margin = 8;
    static inline constexpr int doubleMargin = margin * 2;
    static inline constexpr int height = 37;

    bool selectionChanged = false;
    bool hideLabel = false;
    bool edgeHovered = false;

    String currentText;

    Point<int> mouseDownPos;
    Font font{15.0f};

   private:
    void initialise();
    bool hitTest(int x, int y) override;

    void textEditorReturnKeyPressed(TextEditor& ed) override;
    void textEditorTextChanged(TextEditor& ed) override;

    Rectangle<int> originalBounds;

    Justification justification = Justification::centredLeft;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border{1, 7, 1, 2};
    float minimumHorizontalScale = 1.0f;
    bool editSingleClick = false;
    bool wasResized = false;

    bool attachedToMouse = false;
    bool createEditorOnMouseDown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
};
