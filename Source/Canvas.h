/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Box.h"
#include "Pd/PdPatch.h"
#include "Pd/PdStorage.h"
#include "PluginProcessor.h"
#include "Utility/ObjectGrid.h"

class SuggestionComponent;
struct GraphArea;
class Edge;
class PlugDataPluginEditor;
class Canvas : public Component, public Value::Listener, public LassoSource<WeakReference<Component>>
{    
   public:
    
    bool isBeingDeleted = false;
    
    Canvas(PlugDataPluginEditor& parent, pd::Patch& patch, Component* parentGraph = nullptr);

    ~Canvas() override;

    PlugDataPluginEditor& main;
    PlugDataAudioProcessor* pd;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics&) override;

    void resized() override
    {
        repaint();
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;

    void synchronise(bool updatePosition = true);

    bool keyPressed(const KeyPress& key) override;
    
    void copySelection();
    void removeSelection();
    void pasteSelection();
    void duplicateSelection();

    void valueChanged(Value& v) override;
    

    void checkBounds();

    void undo();
    void redo();

    // Multi-dragger functions
    void deselectAll();

    void setSelected(Component* component, bool shouldNowBeSelected);
    bool isSelected(Component* component) const;

    void handleMouseDown(Component* component, const MouseEvent& e);
    void handleMouseUp(Component* component, const MouseEvent& e);
    void handleMouseDrag(const MouseEvent& e);

    SelectedItemSet<WeakReference<Component>>& getLassoSelection() override;

    void removeSelectedComponent(Component* component);
    void findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, const Rectangle<int>& area) override;

    void updateSidebarSelection();

    void showSuggestions(Box* box, TextEditor* editor);
    void hideSuggestions();

    template <typename T>
    Array<T*> getSelectionOfType()
    {
        Array<T*> result;

        for (auto obj : selectedComponents)
        {
            if (auto* objOfType = dynamic_cast<T*>(obj.get()))
            {
                result.add(objOfType);
            }
        }

        return result;
    }

    Viewport* viewport = nullptr;

    OwnedArray<DrawablePath> drawables;

    bool connectingWithDrag = false;
    Array<SafePointer<Edge>> connectingEdges;
    SafePointer<Edge> nearestEdge;

    pd::Patch& patch;

    // Needs to be allocated before box and connection so they can deselect themselves in the destructor
    SelectedItemSet<WeakReference<Component>> selectedComponents;
    
    OwnedArray<Box> boxes;
    OwnedArray<Connection> connections;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    Value gridEnabled = Value(var(true));

    
    bool isGraph = false;
    bool hasParentCanvas = false;
    bool updatingBounds = false;  // used by connection
    bool isDraggingLasso = false;
    
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));

    ObjectGrid grid = ObjectGrid(this);

    Point<int> canvasOrigin = {0, 0};
    Point<int> canvasDragStartPosition = {0, 0};
    Point<int> viewportPositionBeforeMiddleDrag = {0, 0};

    GraphArea* graphArea = nullptr;
    SuggestionComponent* suggestor = nullptr;
    
    bool attachNextObjectToMouse = false;
    
    // Multi-dragger variables
    bool didStartDragging = false;
    const int minimumMovementToStartDrag = 5;
    Box* componentBeingDragged = nullptr;
    
   private:
    
    SafePointer<Box> boxSnappingInbetween;
    SafePointer<Connection> connectionToSnapInbetween;
    SafePointer<TabbedComponent> tabbar;

    LassoComponent<WeakReference<Component>> lasso;
    
    // Static makes sure there can only be one
    PopupMenu popupMenu;

    // Properties that can be shown in the inspector by right-clicking on canvas
    ObjectParameters parameters = {{"Is graph", tBool, cGeneral, &isGraphChild, {"No", "Yes"}}, {"Hide name and arguments", tBool, cGeneral, &hideNameAndArgs, {"No", "Yes"}}};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Canvas)
};
