/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/ModifierKeyListener.h"
#include <JuceHeader.h>
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"

#define ACTIVITY_UPDATE_RATE 15

class ObjectBase;
class Iolet;
class ObjectDragState;
class Canvas;
class Connection;
class ObjectBoundsConstrainer;

class Object : public Component
    , public Value::Listener
    , public ChangeListener
    , public MultiTimer
    , private TextEditor::Listener {
public:
    Object(Canvas* parent, String const& name = "", Point<int> position = { 100, 100 });

    Object(void* object, Canvas* parent);

    ~Object() override;

    void valueChanged(Value& v) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void timerCallback(int timerID) override;

    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void resized() override;

    void updateIolets();

    void setType(String const& newType, void* existingObject = nullptr);
    void updateBounds();
    void applyBounds();

    void showEditor();
    void hideEditor();
    bool isInitialEditorShown();

    Rectangle<int> getSelectableBounds();
    Rectangle<int> getObjectBounds();
    void setObjectBounds(Rectangle<int> bounds);

    ComponentBoundsConstrainer* getConstrainer() const;

    void openHelpPatch() const;
    void* getPointer() const;

    Array<Connection*> getConnections() const;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void mouseMove(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;

    void updateOverlays(int overlay);

    void textEditorReturnKeyPressed(TextEditor& ed) override;
    void textEditorTextChanged(TextEditor& ed) override;

    bool hitTest(int x, int y) override;

    void triggerOverlayActiveState();

    bool validResizeZone = false;

    Array<Rectangle<float>> getCorners() const;

    int numInputs = 0;
    int numOutputs = 0;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    Value hvccMode = Value(var(false));

    Canvas* cnv;

    std::unique_ptr<ObjectBase> gui;

    OwnedArray<Iolet> iolets;
    ResizableBorderComponent::Zone resizeZone;

    static inline constexpr int margin = 8;
    static inline constexpr int doubleMargin = margin * 2;
    static inline constexpr int height = 37;

    bool attachedToMouse = false;
    bool isSearchTarget = false;
    static inline Object* consoleTarget = nullptr;

    Rectangle<int> originalBounds;

    static inline int const minimumSize = 12;

    bool isSelected() const;

private:
    void initialise();

    void updateTooltips();

    void openNewObjectEditor();

    bool checkIfHvccCompatible();

    void setSelected(bool shouldBeSelected);
    bool selectedFlag = false;
    bool selectionStateChanged = false;

    bool createEditorOnMouseDown = false;
    bool wasLockedOnMouseDown = false;
    bool indexShown = false;
    bool isHvccCompatible = true;

    bool showActiveState = false;
    float activeStateAlpha = 0.0f;

    bool isObjectMouseActive = false;

    Image activityOverlayImage;

    ObjectDragState& ds;

    RateReducer rateReducer = RateReducer(ACTIVITY_UPDATE_RATE);

    std::unique_ptr<TextEditor> newObjectEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)
    JUCE_DECLARE_WEAK_REFERENCEABLE(Object)
};
