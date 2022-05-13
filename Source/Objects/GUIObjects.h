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

   protected:
    bool inspectorWasVisible = false;
    bool recursiveResize = false;

    const std::string stringGui = std::string("gui");
    const std::string stringMouse = std::string("mouse");

    static inline constexpr int maxSize = 1000000;

    PlugDataAudioProcessor& processor;

    Label input;

    pd::Gui gui;
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

struct _fielddesc
{
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union
    {
        t_float fd_float;    /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1; /* min and max values */
    float fd_v2;
    float fd_screen1; /* min and max screen values */
    float fd_screen2;
    float fd_quantum; /* quantization in value */
};

struct t_curve;
struct DrawableTemplate : public DrawablePath
{
    t_scalar* scalar;
    t_curve* object;
    int baseX, baseY;
    Canvas* canvas;

    Rectangle<int> lastBounds;

    DrawableTemplate(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y);

    void update();

    void updateIfMoved();
};
