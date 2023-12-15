/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Pd/Instance.h"
#include "Pd/MessageListener.h"
#include "Constants.h"
#include "ObjectParameters.h"
#include "Utility/SynchronousValue.h"

class PluginProcessor;
class Canvas;

namespace pd {
class Patch;
}

class Object;

class ObjectLabel : public Label {

public:
    explicit ObjectLabel()
    {
        setJustificationType(Justification::centredLeft);
        setBorderSize(BorderSize<int>(0, 0, 0, 0));
        setMinimumHorizontalScale(1.f);
        setEditable(false, false);
        setInterceptsMouseClicks(false, false);
    }

private:
};

class ObjectBase : public Component
    , public pd::MessageListener
    , public Value::Listener
    , public SettableTooltipClient {

    struct ObjectSizeListener : public juce::ComponentListener
        , public Value::Listener {

        ObjectSizeListener(Object* obj);

        void componentMovedOrResized(Component& component, bool moved, bool resized) override;

        void valueChanged(Value& v) override;

        Object* object;
    };

    struct PropertyUndoListener : public Value::Listener {
        PropertyUndoListener();

        void valueChanged(Value& v) override;

        uint32 lastChange;
        std::function<void()> onChange = []() {};
    };

public:
    ObjectBase(pd::WeakReference obj, Object* parent);

    ~ObjectBase() override;

    void initialise();

    void paint(Graphics& g) override;

    // Functions to show and hide a text editor
    // Used internally, or to trigger a text editor when creating a new object (comment, message, new text object etc.)
    virtual bool isEditorShown() { return false; }
    virtual void showEditor() { }
    virtual void hideEditor() { }

    bool hitTest(int x, int y) override;

    // Some objects need to show/hide iolets when send/receive symbols are set
    virtual bool hideInlets() { return false; }
    virtual bool hideOutlets() { return false; }

    // Gets position from pd and applies it to Object
    virtual Rectangle<int> getPdBounds() = 0;

    // Gets position from pd and applies it to Object
    virtual Rectangle<int> getSelectableBounds();

    // Push current object bounds into pd
    virtual void setPdBounds(Rectangle<int> newBounds) = 0;

    // Called whenever a drawable changes
    virtual void updateDrawables() { }

    // Called after creation, to initialise parameter listeners
    virtual void update() { }

    virtual void tabChanged() { }

    virtual bool canOpenFromMenu();
    virtual void openFromMenu();

    // Flag to make object visible or hidden inside a GraphOnParent
    virtual bool hideInGraph();

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked);

    // Returns the Pd class name of the object
    String getType() const;

    void moveToFront();
    void moveForward();
    void moveBackward();
    void moveToBack();

    virtual Canvas* getCanvas();
    virtual pd::Patch::Ptr getPatch();

    // Override if you want a part of your object to ignore mouse clicks
    virtual bool canReceiveMouseEvent(int x, int y);

    // Called whenever the object receives a pd message
    virtual void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) {};

    // Close any tabs with opened subpatchers
    void closeOpenedSubpatchers();
    void openSubpatch();

    // Attempt to send "click" message to object. Returns false if the object has no such method
    bool click(Point<int> position, bool shift, bool alt);

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override;

    static ObjectBase* createGui(pd::WeakReference ptr, Object* parent);

    // Override this to return parameters that will be shown in the inspector
    virtual ObjectParameters getParameters();
    virtual bool showParametersWhenSelected();

    void objectMovedOrResized(bool resized);
    virtual void updateSizeProperty() { }

    virtual void updateLabel() { }

    // Implement this if you want to allow toggling an object by dragging over it in run mode
    virtual void toggleObject(Point<int> position) { }
    virtual void untoggleObject() { }

    virtual ObjectLabel* getLabel();

    // Should return current object text if applicable
    // Currently only used to subsitute arguments in tooltips
    // TODO: does that even work?
    virtual String getText();

    // Global flag to find out if any GUI object is currently being interacted with
    static bool isBeingEdited();

    ComponentBoundsConstrainer* getConstrainer();

    ObjectParameters objectParameters;

protected:
    // Set parameter without triggering valueChanged
    void setParameterExcludingListener(Value& parameter, var const& value);
    void setParameterExcludingListener(Value& parameter, var const& value, Value::Listener* otherListener);

    // Call when you start/stop editing a gui object
    void startEdition();
    void stopEdition();

    String getBinbufSymbol(int argIndex);

    // Called whenever one of the inspector parameters changes
    void valueChanged(Value& value) override { }

    // Send a float value to Pd
    void sendFloatValue(float value);

    // Used by various ELSE objects, though sometimes with char*, sometimes with unsigned char*
    template<typename T>
    void colourToHexArray(Colour colour, T* hex)
    {
        hex[0] = colour.getRed();
        hex[1] = colour.getGreen();
        hex[2] = colour.getBlue();
    }

    // Min and max limit a juce::Value
    template<typename T>
    T limitValueMax(Value& v, T max)
    {
        auto clampedValue = std::min<T>(max, getValue<T>(v));
        setParameterExcludingListener(v, clampedValue);
        return clampedValue;
    }

    template<typename T>
    T limitValueMin(Value& v, T min)
    {
        auto clampedValue = std::max<T>(min, getValue<T>(v));
        setParameterExcludingListener(v, clampedValue);
        return clampedValue;
    }

    template<typename T>
    T limitValueRange(Value& v, T min, T max)
    {
        auto clampedValue = std::clamp<T>(getValue<T>(v), min, max);
        setParameterExcludingListener(v, clampedValue);
        return clampedValue;
    }

public:
    pd::WeakReference ptr;
    Object* object;
    Canvas* cnv;
    PluginProcessor* pd;

protected:
    PropertyUndoListener propertyUndoListener;

    std::function<void()> onConstrainerCreate = []() {};

    virtual std::unique_ptr<ComponentBoundsConstrainer> createConstrainer();

    std::unique_ptr<ObjectLabel> label;
    static inline constexpr int maxSize = 1000000;
    static inline std::atomic<bool> edited = false;
    std::unique_ptr<ComponentBoundsConstrainer> constrainer;

    ObjectSizeListener objectSizeListener;
    Value positionParameter = SynchronousValue();

    friend class IEMHelper;
    friend class AtomHelper;
};
