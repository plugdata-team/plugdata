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

    virtual void checkBounds() {};

    // Gets position from pd and applies it to Object
    virtual void updateBounds() = 0;

    // Push current object bounds into pd
    virtual void applyBounds() = 0;

    // Called whenever a drawable changes
    virtual void updateDrawables() {};

    virtual void updateParameters();

    virtual bool canOpenFromMenu();
    virtual void openFromMenu();

    // Flag to make object visible or hidden inside a GraphOnParent
    virtual bool hideInGraph();

    virtual void setText(String const&) {};

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked);

    String getType() const;

    void moveToFront();
    void moveToBack();

    virtual Canvas* getCanvas();
    virtual pd::Patch* getPatch();
    virtual bool canReceiveMouseEvent(int x, int y);

    virtual void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) {};

    void closeOpenedSubpatchers();
    void openSubpatch();

    virtual String getText();

    void receiveMessage(String const& symbol, int argc, t_atom* argv) override;

    static ObjectBase* createGui(void* ptr, Object* parent);

    virtual ObjectParameters getParameters();

    virtual void updateLabel() {};
    virtual void toggleObject(Point<int> position) {};
    virtual void untoggleObject() {};

    virtual ObjectLabel* getLabel();

protected:
    void setParameterExcludingListener(Value& parameter, var value);

    void startEdition();
    void stopEdition();

    void valueChanged(Value& value) override {};

    void sendFloatValue(float value);

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
    static inline bool draggingSlider = false;

    void* ptr;
    Object* object;
    Canvas* cnv;
    PluginProcessor* pd;

protected:
    std::unique_ptr<ObjectLabel> label;
    static inline constexpr int maxSize = 1000000;
    std::atomic<bool> edited;

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
