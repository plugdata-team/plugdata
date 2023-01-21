/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"

class Canvas;

namespace pd {
class Patch;
}

class Object;

class ObjectLabel : public Label {
    struct ObjectListener : public juce::ComponentListener {
        void componentMovedOrResized(Component& component, bool moved, bool resized) override;
    };

public:
    ObjectLabel(Component* parent)
        : object(parent)
    {
        object->addComponentListener(&objListener);

        setJustificationType(Justification::centredLeft);
        setBorderSize(BorderSize<int>(0, 0, 0, 0));
        setMinimumHorizontalScale(1.f);
        setEditable(false, false);
        setInterceptsMouseClicks(false, false);
    }

    ~ObjectLabel()
    {
        object->removeComponentListener(&objListener);
    }

private:
    ObjectListener objListener;
    Component* object;
};

class ObjectBase : public Component
    , public pd::MessageListener
    , public Value::Listener
    , public SettableTooltipClient {

public:
    ObjectBase(void* obj, Object* parent);

    virtual ~ObjectBase();

    void paint(Graphics& g) override;

    // Functions to show and hide a text editor
    // Used internally, or to trigger a text editor when creating a new object (comment, message, new text object etc.)
    virtual void showEditor() {};
    virtual void hideEditor() {};

    // TODO: get rid of this!
    virtual void checkBounds() {};

    // Gets position from pd and applies it to Object
    virtual void updateBounds() = 0;

    // Push current object bounds into pd
    virtual void applyBounds() = 0;

    // Called whenever a drawable changes
    virtual void updateDrawables() {};

    // Called after creation, to initialise parameter listeners
    virtual void initialiseParameters();

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
    void moveToBack();

    virtual Canvas* getCanvas();
    virtual pd::Patch* getPatch();
        
    // Override if you want a part of your object to ignore mouse clicks
    virtual bool canReceiveMouseEvent(int x, int y);

    // Called whenever the object receives a pd message
    virtual void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) {};

    // Close any tabs with opened subpatchers
    void closeOpenedSubpatchers();
    void openSubpatch();

    void receiveMessage(String const& symbol, int argc, t_atom* argv) override;

    static ObjectBase* createGui(void* ptr, Object* parent);

    // Override this to return parameters that will be shown in the inspector
    virtual ObjectParameters getParameters();

    virtual void updateLabel() {};
        
    // Implement this if you want to allow toggling an object by dragging over it in run mode
    virtual void toggleObject(Point<int> position) {};
    virtual void untoggleObject() {};

    virtual ObjectLabel* getLabel();
        
    // Should return current object text if applicable
    // Currently only used to subsitute arguments in tooltips
    // TODO: does that even work?
    virtual String getText();
        
    static bool isDraggingSlider();
        
protected:
        
    // Set parameter without triggering valueChanged
    void setParameterExcludingListener(Value& parameter, var value);

    // Call when you start/stop editing a gui object
    void startEdition();
    void stopEdition();

    // Called whenever one of the inspector parameters changes
    void valueChanged(Value& value) override {};

    // Send a float value to Pd
    void sendFloatValue(float value);

    // Min and max limit a juce::Value
    template<typename T>
    T limitValueMax(Value& v, T max)
    {
        auto clampedValue = std::min<T>(max, static_cast<T>(v.getValue()));
        v = clampedValue;
        return clampedValue;
    }

    template<typename T>
    T limitValueMin(Value& v, T min)
    {
        auto clampedValue = std::max<T>(min, static_cast<T>(v.getValue()));
        v = clampedValue;
        return clampedValue;
    }


public:
    void* ptr;
    Object* object;
    Canvas* cnv;
    PluginProcessor* pd;

protected:
    std::unique_ptr<ObjectLabel> label;
    static inline constexpr int maxSize = 1000000;
    std::atomic<bool> edited;
    static inline bool draggingSlider = false;

    friend class IEMHelper;
    friend class AtomHelper;
};

// Class for non-patchable objects
class NonPatchable : public ObjectBase {

public:
    NonPatchable(void* obj, Object* parent);
    ~NonPatchable();

    virtual void updateValue() {};
    virtual void updateBounds() {};
    virtual void applyBounds() {};
};
