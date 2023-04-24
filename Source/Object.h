/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/ModifierKeyListener.h"
#include <JuceHeader.h>

class ObjectBase;
class Iolet;
class ObjectDragState;
class Canvas;
class Connection;
class ObjectBoundsConstrainer;

class Object : public Component
    , public Value::Listener
    , public ChangeListener
    , public Timer
    , private TextEditor::Listener {
public:
    Object(Canvas* parent, String const& name = "", Point<int> position = { 100, 100 });

    Object(void* object, Canvas* parent);

    ~Object();

    void valueChanged(Value& v) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    void timerCallback() override;

    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void resized() override;

    void updateIolets();

    void setType(String const& newType, void* existingObject = nullptr);
    void updateBounds();
    void applyBounds();

    void showEditor();
    void hideEditor();

    Rectangle<int> getSelectableBounds();
    Rectangle<int> getObjectBounds();
    void setObjectBounds(Rectangle<int> bounds);

    ComponentBoundsConstrainer* getConstrainer();

    void openHelpPatch() const;
    void* getPointer() const;

    Array<Connection*> getConnections() const;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void mouseMove(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;

    void updateOverlays(int overlay);

    void textEditorReturnKeyPressed(TextEditor& ed) override;
    void textEditorTextChanged(TextEditor& ed) override;

    bool hitTest(int x, int y) override;

    bool validResizeZone = false;

    Array<Rectangle<float>> getCorners() const;

    int numInputs = 0;
    int numOutputs = 0;

    Value locked;
    Value commandLocked;
    Value presentationMode;
    Value hvccMode = Value(var(false));
    Value paletteDragMode = Value(var(false));

    Canvas* cnv;

    std::unique_ptr<ObjectBase> gui;

    OwnedArray<Iolet> iolets;
    ResizableBorderComponent::Zone resizeZone;

    static inline constexpr int margin = 8;
    static inline constexpr int doubleMargin = margin * 2;
    static inline constexpr int height = 37;

    bool attachedToMouse = false;
    bool isSearchTarget = false;

    static inline const StringArray hvccObjects = { "!=", "%", "&", "&&", "|", "||", "*", "+", "-", "/", "<", "<<", "<=", "==", ">", ">=", ">>", "abs", "atan", "atan2", "b", "bang", "bendin", "bendout", "bng", "change", "clip", "cnv", "cos", "ctlin", "ctlout", "dbtopow", "dbtorms", "declare", "del", "delay", "div", "exp", "f", "float", "floatatom", "ftom", "gatom", "hradio", "hsl", "i", "inlet", "int", "line", "loadbang", "log", "msg", "message", "makenote", "max", "metro", "min", "midiin", "midiout", "midirealtimein", "mod", "moses", "mtof", "nbx", "notein", "noteout", "outlet", "pack", "pgmin", "pgmout", "pipe", "poly", "polytouchin", "polytouchout", "pow", "powtodb", "print", "r", "random", "receive", "rmstodb", "route", "s", "sel", "select", "send", "sin", "spigot", "sqrt", "stripnote", "swap", "symbol", "symbolatom", "t", "table", "tabread", "tabwrite", "tan", "text", "tgl", "timer", "touchin", "touchout", "trigger", "unpack", "until", "vradio", "vsl", "wrap", "*~", "+~", "-~", "/~", "abs~", "adc~", "biquad~", "bp~", "catch~", "clip~", "cos~", "cpole~", "czero_rev~", "czero~", "dac~", "dbtopow~", "dbtorms~", "delread~", "delwrite~", "delread4~", "env~", "exp~", "ftom~", "hip~", "inlet~", "line~", "lop~", "max~", "min~", "mtof~", "noise~", "osc~", "outlet~", "phasor~", "powtodb~", "pow~", "q8_rsqrt~", "q8_sqrt~", "receive~", "rmstodb~", "rpole~", "rsqrt~", "rzero_rev~", "rzero~", "r~", "samphold~", "samplerate~", "send~", "sig~", "snapshot~", "sqrt~", "s~", "tabosc4~", "tabplay~", "tabread4~", "tabread~", "tabwrite~", "throw~", "vcf~", "vd~", "wrap~",

        // Heavylib abstractions:
        // These won't be used for the compatibility testing (it will recognise any abstractions as a canvas)
        // These are only for the suggestions
        "hv.comb~", "hv.compressor~", "hv.compressor2~", "hv.dispatch", "hv.drunk", "hv.envfollow~", "hv.eq~", "hv.exp~", "hv.filter.gain~", "hv.filter~", "hv.flanger~", "hv.flanger2~", "hv.freqshift~", "hv.gt~", "hv.gte~", "hv.log~", "hv.lt~", "hv.lte~", "hv.multiplex~", "hv.neq~", "hv.osc~", "hv.pinknoise~", "hv.pow~", "hv.reverb~", "hv.tanh~", "hv.vline~" };

    Rectangle<int> originalBounds;

    static inline int const minimumSize = 12;

    bool isSelected();

    bool isValidObject = false;

private:
    void initialise();

    void updateTooltips();

    void openNewObjectEditor();

    void setSelected(bool shouldBeSelected);
    bool selectedFlag = false;
    bool selectionStateChanged = false;

    bool createEditorOnMouseDown = false;
    bool wasLockedOnMouseDown = false;
    bool indexShown = false;
    bool isHvccCompatible = true;

    ObjectDragState& ds;

    std::unique_ptr<TextEditor> newObjectEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)
    JUCE_DECLARE_WEAK_REFERENCEABLE(Object)
};
