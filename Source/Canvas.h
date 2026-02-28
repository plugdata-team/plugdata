/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

#include "ObjectGrid.h"          // move to impl
#include "Utility/RateReducer.h" // move to impl
#include "Utility/ModifierKeyListener.h"
#include "Components/CheckedTooltip.h"
#include "Pd/MessageListener.h"
#include "Pd/Patch.h"
#include "Constants.h"
#include "Objects/ObjectParameters.h"
#include "NVGSurface.h"
#include "Utility/NVGUtils.h"
#include "Utility/GlobalMouseListener.h"

namespace pd {
class Patch;
}

class SuggestionComponent;
class GraphArea;
class GraphOnParent;
class Iolet;
class Object;
class Connection;
class PluginEditor;
class PluginProcessor;
class ConnectionPathUpdater;
class ConnectionBeingCreated;
class TabComponent;
class BorderResizer;
class CanvasSearchHighlight;
class ObjectsResizer;
class CanvasViewport;

struct ObjectDragState {
    bool wasDragDuplicated : 1 = false;
    bool didStartDragging : 1 = false;
    bool wasSelectedOnMouseDown : 1 = false;
    bool wasResized : 1 = false;
    bool wasDuplicated : 1 = false;
    Point<int> canvasDragStartPosition = { 0, 0 };
    Component::SafePointer<Object> componentBeingDragged;
    Component::SafePointer<Object> objectSnappingInbetween;
    Component::SafePointer<Connection> connectionToSnapInbetween;

    Point<int> duplicateOffset = { 0, 0 };
    Point<int> lastDuplicateOffset = { 0, 0 };
};

class Canvas final : public Component
    , public Value::Listener
    , public SettingsFileListener
    , public LassoSource<WeakReference<Component>>
    , public ModifierKeyListener
    , public pd::MessageListener
    , public AsyncUpdater
    , public NVGComponent
    , public ChangeListener {
public:
    Canvas(PluginEditor* parent, pd::Patch::Ptr patch, Component* parentGraph = nullptr);

    ~Canvas() override;

    PluginEditor* editor;
    PluginProcessor* pd;

    void mouseDown(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    bool hitTest(int x, int y) override;

    void commandKeyChanged(bool isHeld) override;
    void shiftKeyChanged(bool isHeld) override;
    void middleMouseChanged(bool isHeld) override;
    void altKeyChanged(bool isHeld) override;

    void settingsChanged(String const& name, var const& value) override;

    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    void updateFramebuffers(NVGcontext* nvg) override;
    void performRender(NVGcontext* nvg, Rectangle<int> invalidRegion);

    void resized() override;

    void renderAllObjects(NVGcontext* nvg, Rectangle<int> area);
    void renderAllConnections(NVGcontext* nvg, Rectangle<int> area);

    int getOverlays() const;
    void updateOverlays();

    bool shouldShowObjectActivity() const;
    bool shouldShowIndex() const;
    bool shouldShowConnectionDirection() const;
    bool shouldShowConnectionActivity() const;

    void save(std::function<void()> const& nestedCallback = [] { });
    void saveAs(std::function<void()> const& nestedCallback = [] { });

    void synchroniseAllCanvases();
    void synchroniseSplitCanvas();
    void synchronise();
    void performSynchronise();
    void handleAsyncUpdate() override;

    void updateDrawables();

    bool keyPressed(KeyPress const& key) override;
    void valueChanged(Value& v) override;

    void tabChanged();

    void hideAllActiveEditors();

    void copySelection();
    void removeSelection();
    void removeSelectedConnections();
    void dragAndDropPaste(String const& patchString, Point<int> mousePos, int patchWidth, int patchHeight, String const& name = String());
    void pasteSelection();
    void duplicateSelection();

    void encapsulateSelection();
    void triggerizeSelection();
    void cycleSelection();
    void connectSelection();
    void tidySelection();

    void cancelConnectionCreation();

    void alignObjects(Align alignment);

    void undo();
    void redo();

    void jumpToOrigin();
    void restoreViewportState();
    void saveViewportState();

    void zoomToFitAll();

    bool autoscroll(MouseEvent const& e);

    // Multi-dragger functions
    void deselectAll(bool broadcastChange = true);
    void setSelected(Component* component, bool shouldNowBeSelected, bool updateCommandStatus = true, bool broadcastChange = true);

    SelectedItemSet<WeakReference<Component>>& getLassoSelection() override;

    bool checkPanDragMode();
    bool setPanDragMode(bool shouldPan);

    bool isPointOutsidePluginArea(Point<int> point) const;

    void findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area) override;

    void updateSidebarSelection();

    void orderConnections();

    void showSuggestions(Object* object, TextEditor* textEditor);
    void hideSuggestions();

    bool panningModifierDown() const;

    ObjectParameters& getInspectorParameters();

    void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override;

    void activateCanvasSearchHighlight(Point<float> viewPos, Object* obj);
    void removeCanvasSearchHighlight();

    template<typename T>
    SmallArray<T*> getSelectionOfType()
    {
        SmallArray<T*> result;
        for (auto const& obj : selectedComponents) {
            if (auto* objOfType = dynamic_cast<T*>(obj.get())) {
                result.add(objOfType);
            }
        }
        return result;
    }

    std::unique_ptr<CanvasViewport> viewport;

    bool connectingWithDrag : 1 = false;
    bool connectionCancelled : 1 = false;
    SafePointer<Iolet> nearestIolet;

    std::unique_ptr<SuggestionComponent> suggestor;

    pd::Patch::Ptr refCountedPatch;
    pd::Patch& patch;

    Value locked = SynchronousValue();
    Value commandLocked;
    Value presentationMode;

    SmallArray<juce::WeakReference<NVGComponent>> drawables;

    // Needs to be allocated before object and connection so they can deselect themselves in the destructor
    SelectedItemSet<WeakReference<Component>> selectedComponents;
    PooledPtrArray<Object> objects;
    PooledPtrArray<Connection> connections;
    PooledPtrArray<ConnectionBeingCreated> connectionsBeingCreated;

    bool showOrigin : 1 = false;
    bool showBorder : 1 = false;
    bool showConnectionOrder : 1 = false;
    bool connectionsBehind : 1 = true;
    bool showObjectActivity : 1 = false;
    bool showIndex : 1 = false;
    bool showConnectionDirection : 1 = false;
    bool showConnectionActivity : 1 = false;

    bool isZooming : 1 = false;
    bool isGraph : 1 = false;
    bool isDraggingLasso : 1 = false;
    bool needsSearchUpdate : 1 = false;
    bool altDown : 1 = false;
    bool shiftDown : 1 = false;

    Rectangle<int> currentRenderArea;

    Value isGraphChild = SynchronousValue(var(false));
    Value hideNameAndArgs = SynchronousValue(var(false));
    Value xRange = SynchronousValue();
    Value yRange = SynchronousValue();
    Value patchWidth = SynchronousValue();
    Value patchHeight = SynchronousValue();

    Value zoomScale;

    ObjectGrid objectGrid = ObjectGrid(this);

    int lastObjectGridSize = -1;

    NVGImage dotsLargeImage;

    Point<int> const canvasOrigin;

    std::unique_ptr<GraphArea> graphArea;

    SafePointer<Object> lastSelectedObject;         // For auto patching
    SafePointer<Connection> lastSelectedConnection; // For auto patching

    Point<int> pastedPosition;
    Point<int> pastedPadding;

    std::unique_ptr<ConnectionPathUpdater> pathUpdater;
    RateReducer objectRateReducer = RateReducer(90);

    ObjectDragState dragState;

    static constexpr int infiniteCanvasSize = 128000;

    Component objectLayer;
    Component connectionLayer;

    NVGImage resizeHandleImage;
    NVGImage presentationShadowImage;

    NVGcolor canvasBackgroundCol;
    Colour canvasBackgroundColJuce;
    NVGcolor canvasMarkingsCol;
    Colour canvasMarkingsColJuce;

    Colour canvasTextColJuce;
    NVGcolor presentationBackgroundCol;
    NVGcolor presentationWindowOutlineCol;

    NVGcolor lassoCol;
    NVGcolor lassoOutlineCol;

    // objectOutlineColourId
    NVGcolor objectOutlineCol;
    NVGcolor outlineCol;

    NVGcolor graphAreaCol;

    NVGcolor commentTextCol;

    // guiObjectInternalOutlineColour
    Colour guiObjectInternalOutlineColJuce;
    NVGcolor guiObjectInternalOutlineCol;
    NVGcolor guiObjectBackgroundCol;
    Colour guiObjectBackgroundColJuce;

    NVGcolor textObjectBackgroundCol;
    NVGcolor transparentObjectBackgroundCol;

    // objectSelectedOutlineColourId
    NVGcolor selectedOutlineCol;
    NVGcolor indexTextCol;
    NVGcolor ioletLockedCol;

    NVGcolor baseCol;
    NVGcolor dataCol;
    NVGcolor sigCol;
    NVGcolor gemCol;

    NVGcolor dataColBrighter;
    NVGcolor sigColBrighter;
    NVGcolor gemColBrigher;
    NVGcolor baseColBrigher;

private:
    void changeListenerCallback(ChangeBroadcaster* c) override;

    SelectedItemSet<WeakReference<Component>> previousSelectedComponents;

    void lookAndFeelChanged() override;

    void parentHierarchyChanged() override;

    GlobalMouseListener globalMouseListener;

    bool dimensionsAreBeingEdited = false;

    int lastMouseX, lastMouseY;
    LassoComponent<WeakReference<Component>> lasso;

    RateReducer canvasRateReducer = RateReducer(90);

    // Properties that can be shown in the inspector by right-clicking on canvas
    ObjectParameters parameters;

    std::unique_ptr<BorderResizer> canvasBorderResizer;

    std::unique_ptr<ObjectsResizer> objectsDistributeResizer;

    std::unique_ptr<CanvasSearchHighlight> canvasSearchHighlight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Canvas)
};
