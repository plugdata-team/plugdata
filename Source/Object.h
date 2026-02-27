/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/ModifierKeyListener.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"
#include "Utility/NVGUtils.h"
#include "Pd/WeakReference.h"
#include "Iolet.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

struct ObjectDragState;
class ObjectBase;
class Iolet;
class Canvas;
class Connection;

class Object final : public Component
    , public Value::Listener
    , public ChangeListener
    , public Timer
    , public KeyListener
    , public NVGComponent
    , public SettingsFileListener
    , private TextEditor::Listener {
public:
    explicit Object(Canvas* parent, String const& name = "", Point<int> position = { 100, 100 });

    Object(pd::WeakReference object, Canvas* parent);

    ~Object() override;

    void settingsChanged(String const& name, var const& value) override;
    void valueChanged(Value& v) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void timerCallback() override;

    void resized() override;

    void updateIoletGeometry();

    bool keyPressed(KeyPress const& key, Component* component) override;

    void updateIolets();

    void setType(String const& newType, pd::WeakReference existingObject = nullptr);
    void updateBounds();
    void applyBounds();

    void showEditor();
    void hideEditor();
    bool isInitialEditorShown() const;

    String getType(bool withOriginPrefix = true) const;

    Rectangle<int> getSelectableBounds() const;
    Rectangle<int> getObjectBounds() const;
    void setObjectBounds(Rectangle<int> bounds);

    ComponentBoundsConstrainer* getConstrainer() const;

    void openHelpPatch() const;
    t_gobj* getPointer() const;

    SmallArray<Connection*> getConnections() const;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void render(NVGcontext* nvg) override;

    void renderIolets(NVGcontext* nvg);
    void renderLabel(NVGcontext* nvg);

    void mouseMove(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;

    void lookAndFeelChanged() override;

    void textEditorReturnKeyPressed(TextEditor& ed) override;
    void textEditorTextChanged(TextEditor& ed) override;

    bool hitTest(int x, int y) override;

    void triggerOverlayActiveState();

    SmallArray<Rectangle<float>> getCorners() const;

    uint16_t numInputs = 0;
    uint16_t numOutputs = 0;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    CachedValue<bool> hvccMode;
    CachedValue<bool> patchDownwardsOnly;

    Canvas* cnv;
    PluginEditor* editor;

    std::unique_ptr<ObjectBase> gui;

    PooledPtrArray<Iolet, 8, 4> iolets;
    ResizableBorderComponent::Zone resizeZone;

    static constexpr int margin = 6;
    static constexpr int doubleMargin = margin * 2;
    static constexpr int height = 32;
    static constexpr int minimumSize = 9;

    Rectangle<int> originalBounds;

    bool isSelected() const;

    void hideHandles(bool const shouldHide)
    {
        showHandles = !shouldHide;
        repaint();
    }

    // Controls the way object activity propagates upwards inside GOPs.
    enum ObjectActivityPolicy {
        Self,     // Trigger object's own activity only.
        Parent,   // Trigger activity of object itself, and direct parent GOP only.
        Recursive // Trigger activity of object itself, and all parent GOPs recursively.
    };

    ObjectActivityPolicy objectActivityPolicy : 2 = ObjectActivityPolicy::Self;

private:
    void initialise();

    void updateTooltips();

    void openNewObjectEditor();

    void setSelected(bool shouldBeSelected);

    bool selectedFlag : 1 = false;
    bool showHandles : 1 = true;
    bool selectionStateChanged : 1 = false;
    bool drawIoletExpanded : 1 = false;
    bool validResizeZone : 1 = false;
    bool wasLockedOnMouseDown : 1 = false;
    bool isHvccCompatible : 1 = true;
    bool isGemObject : 1 = false;
    bool isObjectMouseActive : 1 = false;

    float activeStateAlpha = 0.0f;

    ObjectDragState& ds;

    RateReducer rateReducer = RateReducer(30);

    std::unique_ptr<TextEditor> newObjectEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)
    JUCE_DECLARE_WEAK_REFERENCEABLE(Object)

    friend class Iolet;
};
