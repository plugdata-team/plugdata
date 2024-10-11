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
#include "NVGSurface.h"
#include "Utility/CachedTextRender.h"
#include "Object.h"
#include "Canvas.h"

class PluginProcessor;
class Canvas;

namespace pd {
class Patch;
}

class Object;

class ObjectLabel : public Label
    , public NVGComponent {

    hash32 lastTextHash = 0;
    NVGImage image;
    float lastScale = 1.0f;
    bool updateColour = false;
    Colour lastColour;

public:
    explicit ObjectLabel()
        : NVGComponent(this)
    {
        setJustificationType(Justification::centredLeft);
        setBorderSize(BorderSize<int>(0, 0, 0, 0));
        setMinimumHorizontalScale(0.2f);
        setEditable(false, false);
        setInterceptsMouseClicks(false, false);
    }

    virtual void renderLabel(NVGcontext* nvg, float scale)
    {
        auto textHash = hash(getText());
        if (image.needsUpdate(roundToInt(getWidth() * scale), roundToInt(getHeight() * scale)) || updateColour || lastTextHash != textHash || lastScale != scale) {
            updateImage(nvg, scale);
            lastTextHash = textHash;
            lastScale = scale;
            updateColour = false;
        } else {
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, getWidth(), getHeight(), 0, image.getImageId(), 1.0f));
            nvgFillRect(nvg, 0, 0, getWidth(), getHeight());
        }
    }
        
    void colourChanged() override
    {
        lastColour = findColour(Label::textColourId);
        updateColour = true;

        // Flag this component as dirty
        repaint();
    }

    void updateImage(NVGcontext* nvg, float scale)
    {
        // TODO: use single channel image texture
        image.renderJUCEComponent(nvg, *this, scale);
    }

private:
};

class ObjectBase : public Component
    , public pd::MessageListener
    , public SettableTooltipClient
    , public NVGComponent {

    struct ObjectSizeListener : public juce::ComponentListener
        , public Value::Listener {

        ObjectSizeListener(Object* obj);

        void componentMovedOrResized(Component& component, bool moved, bool resized) override;

        void valueChanged(Value& v) override;

        Object* object;
        uint32 lastChange;
    };

    struct PropertyListener : public Value::Listener {
        PropertyListener(ObjectBase* parent);

        void setNoCallback(bool skipCallback);
        
        void valueChanged(Value& v) override;

        Value lastValue;
        uint32 lastChange;
        ObjectBase* parent;
        bool noCallback;
        std::function<void()> onChange = []() {};
    };

public:
    ObjectBase(pd::WeakReference obj, Object* parent);

    ~ObjectBase() override;

    void initialise();

    void paint(Graphics& g) override;

    enum class ObjectType { Default, PdLua, TextDefine, ArrayEditor};

    ObjectType getObjectType() { return objectType; };

    // Functions to show and hide a text editor
    // Used internally, or to trigger a text editor when creating a new object (comment, message, new text object etc.)
    virtual bool isEditorShown() { return false; }
    virtual void showEditor() { }
    virtual void hideEditor() { }

    virtual bool isTransparent() { return false; };

    bool hitTest(int x, int y) override;

    // Some objects need to show/hide iolets when send/receive symbols are set
    virtual bool inletIsSymbol() { return false; }
    virtual bool outletIsSymbol() { return false; }

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

    void render(NVGcontext* nvg) override;

    virtual bool canOpenFromMenu();
    virtual void openFromMenu();

    // Flag to make object visible or hidden inside a GraphOnParent
    virtual bool hideInGraph();
    
    // Override function if you need to update framebuffers outside of the render loop (but with the correct active context)
    virtual void updateFramebuffers() {};

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked);

    // Returns the Pd class name of the object
    String getType() const;

    // Returns the Pd class name of the object with the library prefix in front of it, eg "else"
    String getTypeWithOriginPrefix() const;

    void moveToFront();
    void moveForward();
    void moveBackward();
    void moveToBack();

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

    virtual void objectMovedOrResized(bool resized);
    virtual void updateSizeProperty() { }

    virtual void updateLabel() { }

    // Implement this if you want to allow toggling an object by dragging over it in run mode
    virtual void toggleObject(Point<int> position) { }
    virtual void untoggleObject() { }

    virtual ObjectLabel* getLabel(int idx = 0);

    // Should return current object text if applicable
    // Currently only used to subsitute arguments in tooltips
    // TODO: does that even work?
    virtual String getText();

    virtual bool canEdgeOverrideAspectRatio() { return false; };

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

    void setObjectType(ObjectType type) { objectType = type; };

    String getBinbufSymbol(int argIndex);
        
    virtual void propertyChanged(Value& v) {};
        
    // Send a float value to Pd
    void sendFloatValue(float value);

    // Gets the scale factor we need to use of we want to draw images inside the component
    float getImageScale();

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

    OwnedArray<ObjectLabel> labels;

    ObjectType objectType = ObjectType::Default;

protected:
    PropertyListener propertyListener;

    NVGImage imageRenderer;

    std::function<void()> onConstrainerCreate = []() {};

    virtual std::unique_ptr<ComponentBoundsConstrainer> createConstrainer();

    static inline constexpr int maxSize = 1000000;
    static inline std::atomic<bool> edited = false;
    std::unique_ptr<ComponentBoundsConstrainer> constrainer;

    ObjectSizeListener objectSizeListener;
    Value positionParameter = SynchronousValue();

    friend class IEMHelper;
    friend class AtomHelper;
};
