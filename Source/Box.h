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
#include "Objects/GUIObject.h"
#include "Utility/ObjectGrid.h"

class Canvas;
class Box : public Component, public Value::Listener, public Timer, private TextEditor::Listener
{
public:
    Box (Canvas* parent, const String& name = "", Point<int> position = { 100, 100 });

    Box (void* object, Canvas* parent);

    ~Box();

    void valueChanged (Value& v) override;

    void timerCallback() override;

    void paint (Graphics&) override;
    void paintOverChildren (Graphics&) override;
    void resized() override;

    void updatePorts();

    void setType (const String& newType, void* existingObject = nullptr);
    void updateBounds();

    void showEditor();
    void hideEditor();

    Rectangle<int> getObjectBounds();
    void setObjectBounds (Rectangle<int> bounds);

    void openHelpPatch() const;
    void openSubpatch() const;
    void* getPointer() const;

    Array<Connection*> getConnections() const;

    void mouseEnter (const MouseEvent& e) override;
    void mouseExit (const MouseEvent& e) override;

    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    void textEditorReturnKeyPressed (TextEditor& ed) override;
    void textEditorTextChanged (TextEditor& ed) override;

    bool isOver();

    Array<Rectangle<float>> getCorners() const;

    int numInputs = 0;
    int numOutputs = 0;

    Value locked;
    Value commandLocked;
    Value presentationMode;

    Canvas* cnv;

    std::unique_ptr<ObjectBase> gui = nullptr;

    OwnedArray<Edge> edges;
    ResizableBorderComponent::Zone resizeZone;

    static inline constexpr int margin = 8;
    static inline constexpr int doubleMargin = margin * 2;
    static inline constexpr int height = 37;

    bool selectionChanged = false;
    bool edgeHovered = false;

    Point<int> mouseDownPos;
    bool attachedToMouse = false;

private:
    void initialise();
    bool hitTest (int x, int y) override;

    void openNewObjectEditor();

    Rectangle<int> originalBounds;
    bool createEditorOnMouseDown = false;

    std::unique_ptr<TextEditor> newObjectEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};
