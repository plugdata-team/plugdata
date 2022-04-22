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
    Box(Canvas* parent, const String& name = "", Point<int> position = {100, 100});

    Box(pd::Object* object, Canvas* parent, const String& name = "");

    void valueChanged(Value& v) override;

    void paint(Graphics&) override;
    void resized() override;

    void updatePorts();
    Rectangle<int> getEdgeBounds(const int index, const int total, bool isInlet) const;

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

    void setEditable(bool editable);
    Array<Rectangle<float>> getCorners() const;

    std::unique_ptr<pd::Object> pdObject = nullptr;

    int numInputs = 0;
    int numOutputs = 0;
    Value locked;
    Value commandLocked;
    Value presentationMode;

    Canvas* cnv;

    std::unique_ptr<GUIComponent> graphics = nullptr;

    OwnedArray<Edge> edges;
    ResizableBorderComponent::Zone resizeZone;

    static inline constexpr int widthOffset = 32;
    static inline constexpr int margin = 8;
    static inline constexpr int doubleMargin = margin * 2;
    static inline constexpr int height = 37;

    bool selectionChanged = false;
    bool hideLabel = false;
    bool edgeHovered = false;

    String currentText;
    
    Point<int> mouseDownPos;

   private:
    void initialise();
    bool hitTest(int x, int y) override;

    void textEditorReturnKeyPressed(TextEditor& ed) override;

    Rectangle<int> originalBounds;

    Font font{15.0f};
    Justification justification = Justification::centred;
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border{1, 2, 1, 2};
    float minimumHorizontalScale = 0.8f;
    bool editSingleClick = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
};
