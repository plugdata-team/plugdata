/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "ObjectGrid.h"          // move to impl
#include "Utility/RateReducer.h" // move to impl
#include "Utility/ModifierKeyListener.h"
#include "Utility/CheckedTooltip.h"
#include "Pd/MessageListener.h"
#include "Pd/Patch.h"
#include "Constants.h"
#include "Objects/ObjectParameters.h"

namespace pd {
class Patch;
}

class SuggestionComponent;
struct GraphArea;
class Iolet;
class Object;
class Connection;
class PluginEditor;
class PluginProcessor;
class ConnectionPathUpdater;
class ConnectionBeingCreated;
class TabComponent;

struct ObjectDragState {
    bool wasDragDuplicated = false;
    bool didStartDragging = false;
    bool wasSelectedOnMouseDown = false;
    bool wasResized = false;
    Point<int> canvasDragStartPosition = { 0, 0 };
    Component::SafePointer<Object> componentBeingDragged;
    Component::SafePointer<Object> objectSnappingInbetween;
    Component::SafePointer<Connection> connectionToSnapInbetween;
};

class Canvas : public Component
    , public Value::Listener
    , public SettingsFileListener
    , public LassoSource<WeakReference<Component>>
    , public ModifierKeyListener
    , public FocusChangeListener
    , public pd::MessageListener
    , public AsyncUpdater {
public:
    Canvas(PluginEditor* parent, pd::Patch::Ptr patch, Component* parentGraph = nullptr, bool isPalette = false);

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

    void commandKeyChanged(bool isHeld) override;
    void spaceKeyChanged(bool isHeld) override;
    void middleMouseChanged(bool isHeld) override;
    void altKeyChanged(bool isHeld) override;

    void propertyChanged(String name, var value) override;

    void moved() override;
    void resized() override;

    int getOverlays();
    void updateOverlays();

    void synchroniseSplitCanvas();
    void synchronise();
    void performSynchronise();
    void handleAsyncUpdate() override;

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
    void removeSelectedConnections();
    void pasteSelection();
    void duplicateSelection();

    void encapsulateSelection();

    bool canConnectSelectedObjects();
    bool connectSelectedObjects();

    void cancelConnectionCreation();

    void undo();
    void redo();

    void jumpToOrigin();
    void zoomToFitAll();

    bool autoscroll(MouseEvent const& e);

    // Multi-dragger functions
    void deselectAll();
    void setSelected(Component* component, bool shouldNowBeSelected, bool updateCommandStatus = true);

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
    bool connectionCancelled = false;
    SafePointer<Iolet> nearestIolet;

    pd::Patch::Ptr refCountedPatch;
    pd::Patch& patch;

    // Needs to be allocated before object and connection so they can deselect themselves in the destructor
    SelectedItemSet<WeakReference<Component>> selectedComponents;

    OwnedArray<Object> objects;
    OwnedArray<Connection> connections;
    OwnedArray<ConnectionBeingCreated> connectionsBeingCreated;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    Value showDirection;
    Value paletteDragMode;
    Value altMode;

    bool showOrigin = false;
    bool showBorder = false;

    bool isGraph = false;
    bool hasParentCanvas = false;
    bool isDraggingLasso = false;
    bool isPalette;
    bool isShowingMenu = false;

    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
    Value xRange, yRange;
    Value patchWidth, patchHeight;

    Value zoomScale;

    ObjectGrid objectGrid = ObjectGrid(this);

    Point<int> const canvasOrigin;
    Point<int> viewportPositionBeforeMiddleDrag = { 0, 0 };

    std::unique_ptr<GraphArea> graphArea;
    SuggestionComponent* suggestor = nullptr;

    bool attachNextObjectToMouse = false;
    // TODO: Move to drag state!
    SafePointer<Object> lastSelectedObject;         // For auto patching
    SafePointer<Connection> lastSelectedConnection; // For auto patching

    int const minimumMovementToStartDrag = 5;

    Point<int> lastMousePosition;
    Point<int> pastedPosition;
    Point<int> pastedPadding;

    std::unique_ptr<ConnectionPathUpdater> pathUpdater;
    RateReducer objectRateReducer = RateReducer(90);

    ObjectDragState dragState;

    inline static constexpr int infiniteCanvasSize = 128000;

private:
    LassoComponent<WeakReference<Component>> lasso;

    RateReducer canvasRateReducer = RateReducer(90);

    // Properties that can be shown in the inspector by right-clicking on canvas
    ObjectParameters parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Canvas)
};
