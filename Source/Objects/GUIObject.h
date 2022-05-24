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

namespace pd
{
class Patch;
}

class Box;

struct ObjectBase : public Component
{
    void* ptr;
    Box* box;
    Canvas* cnv;

    ObjectBase(void* obj, Box* parent);

    // Functions to show and hide a text editor
    // Used internally, or to trigger a text editor when creating a new object (comment, message, new text object etc.)
    virtual void showEditor(){};
    virtual void hideEditor(){};

    // Called whenever any GUI object's value changes
    virtual void updateValue() = 0;

    // Gets position from pd and applies it to Box
    virtual void updateBounds() = 0;

    // Push current object bounds into pd
    virtual void applyBounds() = 0;

    // Called whenever a drawable changes
    virtual void updateDrawables(){};

    virtual bool drawOutline()
    {
        return true;
    };

    // Flag to make object visible or hidden inside a GraphOnParent
    virtual bool hideInGraph()
    {
        return false;
    }

    // Flag for valid/invalid objects, to create the red outline on Box
    virtual bool isValid()
    {
        return true;
    }

    virtual void setText(const String&){};

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked)
    {
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    void moveToFront();

    virtual Canvas* getCanvas()
    {
        return nullptr;
    };
    virtual Label* getLabel()
    {
        return nullptr;
    };
    virtual pd::Patch* getPatch()
    {
        return nullptr;
    };

    virtual ObjectParameters getParameters()
    {
        return {};
    };

    void closeOpenedSubpatchers();

    String getText();
};

// Class for non-patchable objects
struct NonPatchable : public ObjectBase
{
    NonPatchable(void* obj, Box* parent);
    ~NonPatchable();

    virtual void updateValue(){};
    virtual void updateBounds(){};
    virtual void applyBounds(){};
};

struct GUIObject : public ObjectBase, public ComponentListener, public Value::Listener
{
    GUIObject(void* obj, Box* parent);

    ~GUIObject() override;

    void updateValue() override;

    virtual void update(){};

    virtual void initialise();

    void componentMovedOrResized(Component& component, bool moved, bool resized) override;

    void paint(Graphics& g) override;

    String getName() const;

    static ObjectBase* createGui(void* ptr, Box* parent);

    virtual void checkBounds(){};

    // Get rid of this mess!!
    virtual ObjectParameters defineParameters();
    ObjectParameters getParameters() override;

    virtual void updateLabel(){};

    virtual float getValue()
    {
        return 0.0f;
    };

    float getValueOriginal() const noexcept;

    void setValueOriginal(float v);

    float getValueScaled() const noexcept;

    void setValueScaled(float v);

    void startEdition() noexcept;

    void stopEdition() noexcept;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void valueChanged(Value& value) override{};

    Label* getLabel() override
    {
        return label.get();
    }

    void setValue(float value) noexcept;

   protected:
    std::unique_ptr<Label> label;

    bool inspectorWasVisible = false;

    static inline constexpr int maxSize = 1000000;

    PlugDataAudioProcessor& processor;

    std::atomic<bool> edited;
    float value = 0;
    Value min = Value(0.0f);
    Value max = Value(0.0f);
    int width = 6;

    Value sendSymbol;
    Value receiveSymbol;

    Value primaryColour;
    Value secondaryColour;
    Value labelColour;

    Value labelX = Value(0.0f);
    Value labelY = Value(0.0f);
    Value labelHeight = Value(18.0f);

    Value labelText;
};
