/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

#include "Pd/PdGui.h"
#include "PluginProcessor.h"
#include "Sidebar/Sidebar.h"

class Canvas;

namespace pd
{
class Patch;
}

class Box;


struct GUIComponent : public Component, public ComponentListener, public Value::Listener
{
    GUIComponent(pd::Gui gui, Box* parent, bool newObject);

    ~GUIComponent() override;

    virtual void updateValue();

    virtual void update(){};

    void initialise(bool newObject);

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked);

    void componentMovedOrResized(Component& component, bool moved, bool resized) override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void closeOpenedSubpatchers();

    static GUIComponent* createGui(const String& name, Box* parent, bool newObject);

    virtual void checkBoxBounds(){};

    virtual ObjectParameters defineParameters();

    virtual ObjectParameters getParameters();

    virtual pd::Patch* getPatch();
    virtual Canvas* getCanvas();
    virtual bool noGui();
    virtual bool usesCharWidth();

    void showEditor()
    {
        input.showEditor();
    }

    std::unique_ptr<Label> label;

    void updateLabel();

    pd::Type getType();

    float getValueOriginal() const noexcept;

    void setValueOriginal(float v);

    float getValueScaled() const noexcept;

    void setValueScaled(float v);

    void startEdition() noexcept;

    void stopEdition() noexcept;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void valueChanged(Value& value) override;

    Box* box;
    pd::Gui gui;
    
   protected:
    bool inspectorWasVisible = false;
    bool recursiveResize = false;

    const std::string stringGui = std::string("gui");
    const std::string stringMouse = std::string("mouse");

    static inline constexpr int maxSize = 1000000;

    PlugDataAudioProcessor& processor;

    Label input;
    
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

    const int atomSizes[7] = {0, 8, 10, 12, 16, 24, 36};
};
