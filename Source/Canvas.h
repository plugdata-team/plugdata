/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Object.h"
#include "Pd/PdPatch.h"
#include "PluginProcessor.h"
#include "ObjectGrid.h"
#include "Utility/RateReducer.h"

class SuggestionComponent;
struct GraphArea;
class Iolet;
class PluginEditor;
class ConnectionPathUpdater;
class ConnectionBeingCreated;
class TabComponent;

class Canvas : public Component
    , public Value::Listener
    , public LassoSource<WeakReference<Component>>
    , public ModifierKeyListener
    , public FocusChangeListener
    , public pd::MessageListener {
public:
    Canvas(PluginEditor* parent, pd::Patch& patch, Component* parentGraph = nullptr);

    ~Canvas() override;

    PluginEditor* editor;
    PluginProcessor* pd;
        
    void recreateViewport();

    void lookAndFeelChanged() override;
    void paint(Graphics& g) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;

    void spaceKeyChanged(bool isHeld) override;
    void middleMouseChanged(bool isHeld) override;

    void synchronise(bool updatePosition = true);

    void updateDrawables();

    bool keyPressed(KeyPress const& key) override;
    void valueChanged(Value& v) override;

    TabComponent* getTabbar();
    int getTabIndex();
    void tabChanged();

    void globalFocusChanged(Component* focusedComponent) override;

    void hideAllActiveEditors();

    void copySelection();
    void removeSelection();
    void pasteSelection();
    void duplicateSelection();

    void encapsulateSelection();

    bool canConnectSelectedObjects();
    bool connectSelectedObjects();

    void cancelConnectionCreation();

    void undo();
    void redo();

    void checkBounds();

    bool autoscroll(MouseEvent const& e);

    // Multi-dragger functions
    void deselectAll();
    void setSelected(Component* component, bool shouldNowBeSelected, bool updateCommandStatus = true);
    bool isSelected(Component* component) const;

    void objectMouseDown(Object* component, MouseEvent const& e);
    void objectMouseUp(Object* component, MouseEvent const& e);
    void objectMouseDrag(MouseEvent const& e);

    SelectedItemSet<WeakReference<Component>>& getLassoSelection() override;

    bool checkPanDragMode();
    bool setPanDragMode(bool shouldPan);

    void removeSelectedComponent(Component* component);
    void findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area) override;

    void updateSidebarSelection();

    void showSuggestions(Object* object, TextEditor* textEditor);
    void hideSuggestions();

    static bool panningModifierDown();

    ObjectParameters& getInspectorParameters();

    void receiveMessage(String const& symbol, int argc, t_atom* argv) override;

    template<typename T>
    Array<T*> getSelectionOfType()
    {
        Array<T*> result;

        for (auto obj : selectedComponents) {
            if (auto* objOfType = dynamic_cast<T*>(obj.get())) {
                result.add(objOfType);
            }
        }

        return result;
    }

    Viewport* viewport = nullptr;

    bool connectingWithDrag = false;
    SafePointer<Iolet> nearestIolet;

    pd::Patch& patch;

    // Needs to be allocated before object and connection so they can deselect themselves in the destructor
    SelectedItemSet<WeakReference<Component>> selectedComponents;

    OwnedArray<Object> objects;
    OwnedArray<Connection> connections;
    OwnedArray<ConnectionBeingCreated> connectionsBeingCreated;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    Value gridEnabled;

    bool isGraph = false;
    bool hasParentCanvas = false;
    bool updatingBounds = false; // used by connection
    bool isDraggingLasso = false;

    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
    Value xRange, yRange;

    ObjectGrid objectGrid = ObjectGrid(this);

    Point<int> canvasOrigin = { 0, 0 };
    Point<int> canvasDragStartPosition = { 0, 0 };
    Point<int> viewportPositionBeforeMiddleDrag = { 0, 0 };

    GraphArea* graphArea = nullptr;
    SuggestionComponent* suggestor = nullptr;

    bool attachNextObjectToMouse = false;
    bool wasDragDuplicated = false;
    bool wasSelectedOnMouseDown = false;
    SafePointer<Object> lastSelectedObject = nullptr; // For auto patching
    SafePointer<Connection> lastSelectedConnection;   // For auto patching

    // Multi-dragger variables
    bool didStartDragging = false;

    int const minimumMovementToStartDrag = 5;
    SafePointer<Object> componentBeingDragged = nullptr;

    Point<int> lastMousePosition;
    Point<int> pastedPosition;
    Point<int> pastedPadding;
    Array<Point<int>> mouseDownObjectPositions; // Stores object positions for alt + drag

    std::unique_ptr<ConnectionPathUpdater> pathUpdater;

private:
    SafePointer<Object> objectSnappingInbetween;
    SafePointer<Connection> connectionToSnapInbetween;

    LassoComponent<WeakReference<Component>> lasso;

    RateReducer canvasRateReducer = RateReducer(90);
    RateReducer objectRateReducer = RateReducer(90);

    // Properties that can be shown in the inspector by right-clicking on canvas
    ObjectParameters parameters = { { "Is graph", tBool, cGeneral, &isGraphChild, { "No", "Yes" } },
        { "Hide name and arguments", tBool, cGeneral, &hideNameAndArgs, { "No", "Yes" } },
        { "X range", tRange, cGeneral, &xRange, {} },
        { "Y range", tRange, cGeneral, &yRange, {} } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Canvas)
};
