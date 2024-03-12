/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Utility/NVGComponent.h"

#pragma once

class Connection;
class Object;
class Canvas;
struct NVGcontext;

class Iolet : public Component
    , public SettableTooltipClient
    , public Value::Listener
    , public NVGComponent {
public:
    Object* object;

    Iolet(Object* parent, bool isInlet);

    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    bool hitTest(int x, int y) override;
        
    void render(NVGcontext* nvg) override;

    void valueChanged(Value& v) override;

    static Iolet* findNearestIolet(Canvas* cnv, Point<int> position, bool inlet, Object* boxToExclude = nullptr);

    void createConnection();

    void setHidden(bool hidden);

    Array<Connection*> getConnections();

    Rectangle<int> getCanvasBounds();

    int ioletIdx;
    std::atomic<bool> isInlet;
    std::atomic<bool> isSignal;
    std::atomic<bool> isGemState;

    std::atomic<bool> isTargeted = false;

    Canvas* cnv;

private:
    void startConnection();

    std::atomic<int> mouseIsDown;
    bool const insideGraph;
    bool hideIolet = false;

    Value locked;
    Value commandLocked;
    Value presentationMode;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Iolet)
    JUCE_DECLARE_WEAK_REFERENCEABLE(Iolet)
};
