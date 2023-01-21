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

struct ObjectLabel : public Label {
    struct ObjectListener : public juce::ComponentListener {
        void componentMovedOrResized(Component& component, bool moved, bool resized) override;
    };

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

    ObjectListener objListener;
    Component* object;
};

struct ObjectBase : public Component
    , public pd::MessageListener
    , public Value::Listener
    , public SettableTooltipClient {
    void* ptr;
    Object* object;
    Canvas* cnv;
    PluginProcessor* pd;

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

    virtual bool canOpenFromMenu()
    {
        return zgetfn(static_cast<t_pd*>(ptr), pd->generateSymbol("menu-open")) != nullptr;
    }

    virtual void openFromMenu()
    {
        pd_typedmess(static_cast<t_pd*>(ptr), pd->generateSymbol("menu-open"), 0, nullptr);
    };

    // Flag to make object visible or hidden inside a GraphOnParent
    virtual bool hideInGraph()
    {
        return false;
    }

    virtual void setText(String const&) {};

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked)
    {
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    String getType() const;

    void moveToFront();
    void moveToBack();

    virtual Canvas* getCanvas()
    {
        return nullptr;
    };

    virtual pd::Patch* getPatch()
    {
        return nullptr;
    };

    virtual bool canReceiveMouseEvent(int x, int y)
    {
        return true;
    }

    virtual void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) {};

    void closeOpenedSubpatchers();
    void openSubpatch();

    virtual String getText();

    void receiveMessage(String const& symbol, int argc, t_atom* argv) override
    {
        auto atoms = pd::Atom::fromAtoms(argc, argv);

        MessageManager::callAsync([_this = SafePointer(this), symbol, atoms]() mutable {
            if (!_this)
                return;

            if (symbol == "size" || symbol == "delta" || symbol == "pos" || symbol == "dim" || symbol == "width" || symbol == "height") {
                // TODO: we can't really ensure the object has updated its bounds yet!
                _this->updateBounds();
            } else {
                _this->receiveObjectMessage(symbol, atoms);
            }
        });
    }

    static ObjectBase* createGui(void* ptr, Object* parent);

    virtual ObjectParameters getParameters();

    virtual void updateLabel() {};

    virtual float getValue()
    {
        return 0.0f;
    };

    virtual void toggleObject(Point<int> position) {};
    virtual void untoggleObject() {};

    void setParameterExcludingListener(Value& parameter, var value)
    {
        parameter.removeListener(this);
        parameter.setValue(value);
        parameter.addListener(this);
    }

    void startEdition();
    void stopEdition();

    void valueChanged(Value& value) override {};

    virtual ObjectLabel* getLabel()
    {
        return label.get();
    }

    void setValue(float value);

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

    static inline bool draggingSlider = false;

    std::unique_ptr<ObjectLabel> label;

    static inline constexpr int maxSize = 1000000;

    std::atomic<bool> edited;
    float value = 0;
    Value min = Value(0.0f);
    Value max = Value(0.0f);
};

// Class for non-patchable objects
struct NonPatchable : public ObjectBase {
    NonPatchable(void* obj, Object* parent);
    ~NonPatchable();

    virtual void updateValue() {};
    virtual void updateBounds() {};
    virtual void applyBounds() {};
};
